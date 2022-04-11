/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of rtcom-accounts-ui
 *
 * Copyright (C) 2009 Nokia Corporation. All rights reserved.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@nokia.com>
 *
 * This software, including documentation, is protected by copyright controlled
 * by Nokia Corporation. All rights are reserved.
 * Copying, including reproducing, storing, adapting or translating, any or all
 * of this material requires the prior written consent of Nokia Corporation.
 * This material also contains confidential information which may not be
 * disclosed to others without the prior written consent of Nokia.
 */

#ifndef _AUIC_CLIENT_H_
#define _AUIC_CLIENT_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define AUIC_TYPE_CLIENT             (auic_client_get_type ())
#define AUIC_CLIENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), AUIC_TYPE_CLIENT, AuicClient))
#define AUIC_CLIENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), AUIC_TYPE_CLIENT, AuicClientClass))
#define AUIC_IS_CLIENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AUIC_TYPE_CLIENT))
#define AUIC_IS_CLIENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), AUIC_TYPE_CLIENT))
#define AUIC_CLIENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), AUIC_TYPE_CLIENT, AuicClientClass))

typedef struct _AuicClientClass AuicClientClass;
typedef struct _AuicClient AuicClient;
typedef struct _AuicClientPrivate AuicClientPrivate;

struct _AuicClientClass
{
    GObjectClass parent_class;
};

struct _AuicClient
{
    GObject parent_instance;
};

GType auic_client_get_type (void) G_GNUC_CONST;

AuicClient *auic_client_new (GtkWindow *window);

void auic_client_open_accounts_list (AuicClient *client);
void auic_client_open_new_account (AuicClient *client,
                                   const gchar *service_name);
void auic_client_open_edit_account (AuicClient *client,
                                    const gchar *service_name,
                                    const gchar *user_name);
void auic_client_set_visible (AuicClient *client, gboolean visible);

gboolean auic_client_is_ui_open (AuicClient *client);

void auic_client_close (AuicClient *client);

G_END_DECLS

#endif /* _AUIC_CLIENT_H_ */
