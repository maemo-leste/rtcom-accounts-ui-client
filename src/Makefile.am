lib_LTLIBRARIES = librtcom-accounts-ui-client.la
librtcom_accounts_ui_client_la_CFLAGS = $(CLIENT_CFLAGS)
librtcom_accounts_ui_client_la_LDFLAGS = -Wl,--as-needed		\
		$(CLIENT_LIBS) -Wl,--no-undefined
librtcom_accounts_ui_client_la_SOURCES = client.c

librtcom_accounts_ui_client_includedir = $(includedir)/@PACKAGE_NAME@
librtcom_accounts_ui_client_include_HEADERS = client.h

BUILT_SOURCES =								\
		dbus-glib-marshal-aui-service.h				\
		dbus-glib-marshal-aui-instance.h

dbus-glib-marshal-aui-service.h: $(top_srcdir)/xml/aui-service.xml
	$(DBUS_BINDING_TOOL) --prefix=aui_service			\
		--mode=glib-client $< > xgen-$(@F)			\
	&& ( cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@ )			\
	&& rm -f xgen-$(@F)

dbus-glib-marshal-aui-instance.h: $(top_srcdir)/xml/aui-instance.xml
	$(DBUS_BINDING_TOOL) --prefix=aui_instance			\
		--mode=glib-client $< > xgen-$(@F)			\
	&& ( cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@ )			\
	&& rm -f xgen-$(@F)

CLEANFILES = $(BUILT_SOURCES)

MAINTAINERCLEANFILES = Makefile.in
