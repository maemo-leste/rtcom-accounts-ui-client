/*
 * client.c
 *
 * Copyright (C) 2022 Ivaylo Dimitrov <ivo.g.dimitrov.75@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>
#include <gdk/gdkx.h>

#include "client.h"

#include "dbus-glib-marshal-aui-instance.h"
#include "dbus-glib-marshal-aui-service.h"

struct _AuicClientPrivate
{
  DBusGConnection *connection;
  DBusGProxy *service;
  DBusGProxy *instance;
  DBusGProxyCall *service_call;
  DBusGProxyCall *instance_call;
  XID parent_xid;
  gboolean flags1 : 1;
  gboolean open : 1;
};

typedef struct _AuicClientPrivate AuicClientPrivate;

#define PRIVATE(auic) \
  ((AuicClientPrivate *)auic_client_get_instance_private((AuicClient *)(auic)))

G_DEFINE_TYPE_WITH_PRIVATE(
  AuicClient,
  auic_client,
  G_TYPE_OBJECT
)

enum
{
  PROP_PARENT_XID = 1,
};

enum
{
  STATUS_UPDATE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
on_instance_proxy_destroy(DBusGProxy *proxy, AuicClient *auic);
static void
on_service_proxy_destroy(DBusGProxy *proxy, AuicClient *auic);

static void
release_instance_proxy(AuicClient *auic)
{
  AuicClientPrivate *priv = PRIVATE(auic);

  if (priv->instance)
  {
    if (priv->instance_call)
    {
      dbus_g_proxy_cancel_call(priv->instance, priv->instance_call);
      priv->instance_call = NULL;
    }

    g_signal_handlers_disconnect_matched(
      priv->instance, G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
      on_instance_proxy_destroy, auic);
    g_object_unref(priv->instance);
    priv->instance = NULL;
  }
}

static void
on_ui_closed(DBusGProxy *proxy, const char *path, AuicClient *auic)
{
  AuicClientPrivate *priv = PRIVATE(auic);
  DBusGProxy *instance = priv->instance;

  if (!path || (instance && !g_strcmp0(dbus_g_proxy_get_path(instance), path)))
  {
    g_object_ref(auic);

    if (priv->open)
    {
      priv->open = FALSE;
      g_signal_emit(auic, signals[STATUS_UPDATE], 0);
    }

    release_instance_proxy(auic);
    g_object_unref(auic);
  }
}

static void
on_instance_proxy_destroy(DBusGProxy *proxy, AuicClient *auic)
{
  on_ui_closed(proxy, NULL, auic);
}

static void
release_service_proxy(AuicClient *auic)
{
  AuicClientPrivate *priv = PRIVATE(auic);

  if (priv->service)
  {
    if (priv->service_call)
    {
      dbus_g_proxy_cancel_call(priv->service, priv->service_call);
      priv->service = priv->service;
      priv->service_call = NULL;
    }

    g_signal_handlers_disconnect_matched(
      priv->service, G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
      on_service_proxy_destroy, auic);
    g_object_unref(priv->service);
    priv->service = NULL;
  }
}

static void
on_service_proxy_destroy(DBusGProxy *proxy, AuicClient *auic)
{
  release_service_proxy(auic);
}

static void
auic_client_dispose(GObject *object)
{
  AuicClient *auic = AUIC_CLIENT(object);
  AuicClientPrivate *priv = PRIVATE(auic);

  release_instance_proxy(auic);
  release_service_proxy(auic);

  if (priv->connection)
  {
    dbus_g_connection_unref(priv->connection);
    priv->connection = NULL;
  }

  G_OBJECT_CLASS(auic_client_parent_class)->dispose(object);
}

static void
auic_client_set_property(GObject *object, guint property_id,
                         const GValue *value, GParamSpec *pspec)
{
  AuicClientPrivate *priv = PRIVATE(object);

  switch (property_id)
  {
    case PROP_PARENT_XID:
    {
      g_assert(priv->parent_xid == 0);
      priv->parent_xid = g_value_get_uint(value);
      break;
    }
    default:
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
    }
  }
}

static void
auic_client_class_init(AuicClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = auic_client_dispose;
  object_class->set_property = auic_client_set_property;

  g_object_class_install_property(
    object_class, PROP_PARENT_XID,
    g_param_spec_uint(
      "parent-xid",
      "parent-xid",
      "parent-xid",
      0, G_MAXUINT32, 0,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

  signals[STATUS_UPDATE] =
    g_signal_new("status-update", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);
}

static void
auic_client_init(AuicClient *auic)
{
  AuicClientPrivate *priv = PRIVATE(auic);
  GError *error = NULL;

  priv->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

  if (priv->connection)
  {
    priv->service = g_object_new(
        DBUS_TYPE_G_PROXY,
        "name", "com.nokia.AccountsUI",
        "path", "/com/nokia/AccountsUI",
        "interface", "com.nokia.Accounts",
        "connection", priv->connection,
        NULL);

    if (priv->service)
    {
      g_signal_connect(priv->service, "destroy",
                       G_CALLBACK(on_service_proxy_destroy), auic);
      dbus_g_proxy_add_signal(priv->service,
                              "UiClosed", DBUS_TYPE_G_OBJECT_PATH,
                              DBUS_TYPE_INVALID);
      dbus_g_proxy_connect_signal(priv->service, "UiClosed",
                                  G_CALLBACK(on_ui_closed), auic, NULL);
    }
  }
  else if (error)
    g_error_free(error);
}

AuicClient *
auic_client_new(GtkWindow *parent)
{
  if (parent)
  {
    XID parent_xid;
    g_return_val_if_fail(gtk_widget_get_realized(GTK_WIDGET(parent)), NULL);

    parent_xid =
        gdk_x11_drawable_get_xid(GDK_DRAWABLE(GTK_WIDGET(parent)->window));

    return g_object_new(
             AUIC_TYPE_CLIENT,
             "parent-xid", parent_xid,
             NULL);
  }

  return g_object_new(AUIC_TYPE_CLIENT, 0);
}

gboolean
auic_client_is_ui_open(AuicClient *client)
{
  g_return_val_if_fail(AUIC_IS_CLIENT(client), FALSE);

  return PRIVATE(client)->open;
}

static void
on_property_changed(DBusGProxy *proxy, GHashTable *props, AuicClient *auic)
{
  AuicClientPrivate *priv = PRIVATE(auic);
  GValue *v = g_hash_table_lookup(props, "com.nokia.Accounts.UI.ParentXid");

  if (v && G_VALUE_HOLDS_UINT(v) && (g_value_get_uint(v) != priv->parent_xid))
  {
    release_instance_proxy(auic);
    priv->open = FALSE;
    g_signal_emit(auic, signals[STATUS_UPDATE], 0);
  }
}

static void
instance_close_cb(DBusGProxy *proxy, GError *error, gpointer user_data)
{
  AuicClient *auic = user_data;

  PRIVATE(auic)->instance_call = NULL;

  if (error)
  {
    g_warning("%s returned error: %s", __FUNCTION__, error->message);
    g_error_free(error);
  }
}

void
auic_client_close(AuicClient *client)
{
  AuicClientPrivate *priv;

  g_return_if_fail(AUIC_IS_CLIENT(client));

  priv = PRIVATE(client);

  if (priv->service_call)
    priv->flags1 = TRUE;
  else
  {
    com_nokia_Accounts_UI_close_async(
      priv->instance, instance_close_cb, client);
  }
}

static void
create_instance(AuicClient *auic, char *ui)
{
  AuicClientPrivate *priv = PRIVATE(auic);

  g_return_if_fail( priv->instance == NULL);

  priv->instance = dbus_g_proxy_new_for_name_owner(priv->connection,
                                                   "com.nokia.AccountsUI",
                                                   ui,
                                                   "com.nokia.Accounts.UI",
                                                   NULL);

  if (priv->instance)
  {
    if (priv->flags1)
      auic_client_close(auic);
    else
    {
      g_signal_connect(priv->instance, "destroy",
                       G_CALLBACK(on_instance_proxy_destroy), auic);
      dbus_g_proxy_add_signal(priv->instance, "PropertyChanged",
                              dbus_g_type_get_map(
                                "GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
                              G_TYPE_INVALID);
      dbus_g_proxy_connect_signal(priv->instance, "PropertyChanged",
                                  G_CALLBACK(on_property_changed), auic, NULL);
      priv->open = TRUE;
    }
  }
  else
    g_warning("%s: couldn't create proxy for %s", __FUNCTION__, ui);
}

static void
open_accounts_cb(DBusGProxy *proxy, char *ui, GHashTable *OUT_UiProperties,
                 GError *error, gpointer user_data)
{
  AuicClient *auic = user_data;

  PRIVATE(auic)->service_call = NULL;

  if (error)
  {
    g_warning("%s returned error: %s", __FUNCTION__, error->message);
    g_error_free(error);
  }
  else
    create_instance(auic, ui);

  g_signal_emit(auic, signals[STATUS_UPDATE], 0);
}

void
auic_client_open_accounts_list(AuicClient *client)
{
  AuicClientPrivate *priv = PRIVATE(client);

  g_return_if_fail(AUIC_IS_CLIENT(client));

  if (priv->service)
  {
    if (priv->service_call)
    {
      g_warning("%s: no more than a pending call allowed", __FUNCTION__);
      g_signal_emit(client, signals[STATUS_UPDATE], 0);
      return;
    }
    else if (!auic_client_is_ui_open(client))
    {
      priv->service_call = com_nokia_Accounts_open_accounts_list_async(
          priv->service, priv->parent_xid, open_accounts_cb, client);
    }
  }

  if (!priv->service || !priv->service_call)
    g_signal_emit(client, signals[STATUS_UPDATE], 0);
}

void
auic_client_open_edit_account (AuicClient *client, const gchar *service_name,
                               const gchar *user_name)
{
  AuicClientPrivate *priv = PRIVATE(client);

  g_return_if_fail(AUIC_IS_CLIENT(client));

  if (priv->service)
  {
    if (priv->service_call)
    {
      g_warning("%s: no more than a pending call allowed", __FUNCTION__);
      g_signal_emit(client, signals[STATUS_UPDATE], 0);
      return;
    }
    else if (!auic_client_is_ui_open(client))
    {
      gchar *account_name = g_strdup_printf("%s/%s", service_name, user_name);

      priv->service_call = com_nokia_Accounts_edit_account_async(
          priv->service, priv->parent_xid, account_name, "close",
          open_accounts_cb, client);
      g_free(account_name);
    }
  }

  if (!priv->service || !priv->service_call)
    g_signal_emit(client, signals[STATUS_UPDATE], 0);
}

void
auic_client_open_new_account (AuicClient *client, const gchar *service_name)
{
  AuicClientPrivate *priv = PRIVATE(client);

  g_return_if_fail(AUIC_IS_CLIENT(client));

  if (priv->service)
  {
    if (priv->service_call)
    {
      g_warning("%s: no more than a pending call allowed", __FUNCTION__);
      g_signal_emit(client, signals[STATUS_UPDATE], 0);
      return;
    }
    else if (!auic_client_is_ui_open(client))
    {
      priv->service_call = com_nokia_Accounts_new_account_async(
          priv->service, priv->parent_xid, service_name, "close",
          open_accounts_cb, client);
    }
  }

  if (!priv->service || !priv->service_call)
    g_signal_emit(client, signals[STATUS_UPDATE], 0);
}
