EXTRA_DIST = \
	Accessibility.xml \
	Accessible.xml \
	Action.xml \
	Application.xml \
	Cache.xml \
	Collection.xml \
	Component.xml \
	DeviceEventController.xml \
	DeviceEventListener.xml \
	Document.xml \
	EditableText.xml \
	Event.xml \
	Hyperlink.xml \
	Hypertext.xml \
	Image.xml \
	Registry.xml \
	Selection.xml \
	Socket.xml \
	Table.xml \
	Text.xml \
	Value.xml

noinst_LTLIBRARIES = liba11yatspi-dbus.la

a11y-atspi-accessible.h: Accessible.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Accessible. \
	--generate-c-code $(srcdir)/a11y-atspi-accessible \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Accessible" "org.gtk.GDBus.C.Name" Accessible \
	$(srcdir)/Accessible.xml
a11y-atspi-accessible.c: a11y-atspi-accessible.h
	@: # generated as a side-effect

a11y-atspi-action.h: Action.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Action. \
	--generate-c-code $(srcdir)/a11y-atspi-action \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Action" "org.gtk.GDBus.C.Name" Action \
	$(srcdir)/Action.xml
a11y-atspi-action.c: a11y-atspi-action.h
	@: # generated as a side-effect

a11y-atspi-application.h: Application.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Application. \
	--generate-c-code $(srcdir)/a11y-atspi-application \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Application" "org.gtk.GDBus.C.Name" Application \
	$(srcdir)/Application.xml
a11y-atspi-application.c: a11y-atspi-application.h
	@: # generated as a side-effect

a11y-atspi-cache.h: Cache.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Cache. \
	--generate-c-code $(srcdir)/a11y-atspi-cache \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Cache" "org.gtk.GDBus.C.Name" Cache \
	$(srcdir)/Cache.xml
a11y-atspi-cache.c: a11y-atspi-cache.h
	@: # generated as a side-effect

a11y-atspi-collection.h: Collection.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Collection. \
	--generate-c-code $(srcdir)/a11y-atspi-collection \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Collection" "org.gtk.GDBus.C.Name" Collection \
	$(srcdir)/Collection.xml
a11y-atspi-collection.c: a11y-atspi-collection.h
	@: # generated as a side-effect

a11y-atspi-component.h: Component.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Component. \
	--generate-c-code $(srcdir)/a11y-atspi-component \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Component" "org.gtk.GDBus.C.Name" Component \
	$(srcdir)/Component.xml
a11y-atspi-component.c: a11y-atspi-component.h
	@: # generated as a side-effect

a11y-atspi-device-event-listener.h: DeviceEventListener.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.DeviceEventListener. \
	--generate-c-code $(srcdir)/a11y-atspi-device-event-listener \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.DeviceEventListener" "org.gtk.GDBus.C.Name" DeviceEventListener \
	$(srcdir)/DeviceEventListener.xml
a11y-atspi-device-event-listener.c: a11y-atspi-device-event-listener.h
	@: # generated as a side-effect

a11y-atspi-device-event-controller.h: DeviceEventController.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.DeviceEventController. \
	--generate-c-code $(srcdir)/a11y-atspi-device-event-controller \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.DeviceEventController" "org.gtk.GDBus.C.Name" DeviceEventController \
	$(srcdir)/DeviceEventController.xml
a11y-atspi-device-event-controller.c: a11y-atspi-device-event-controller.h
	@: # generated as a side-effect

a11y-atspi-document.h: Document.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Document. \
	--generate-c-code $(srcdir)/a11y-atspi-document \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Document" "org.gtk.GDBus.C.Name" Document \
	$(srcdir)/Document.xml
a11y-atspi-document.c: a11y-atspi-document.h
	@: # generated as a side-effect

a11y-atspi-editable-text.h: EditableText.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.EditableText. \
	--generate-c-code $(srcdir)/a11y-atspi-editable-text \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.EditableText" "org.gtk.GDBus.C.Name" EditableText \
	$(srcdir)/EditableText.xml
a11y-atspi-editable-text.c: a11y-atspi-editable-text.h
	@: # generated as a side-effect

a11y-atspi-hyperlink.h: Hyperlink.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Hyperlink. \
	--generate-c-code $(srcdir)/a11y-atspi-hyperlink \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Hyperlink" "org.gtk.GDBus.C.Name" Hyperlink \
	$(srcdir)/Hyperlink.xml
a11y-atspi-hyperlink.c: a11y-atspi-hyperlink.h
	@: # generated as a side-effect

a11y-atspi-hypertext.h: Hypertext.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Hypertext. \
	--generate-c-code $(srcdir)/a11y-atspi-hypertext \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Hypertext" "org.gtk.GDBus.C.Name" Hypertext \
	$(srcdir)/Hypertext.xml
a11y-atspi-hypertext.c: a11y-atspi-hypertext.h
	@: # generated as a side-effect

a11y-atspi-image.h: Image.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Image. \
	--generate-c-code $(srcdir)/a11y-atspi-image \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Image" "org.gtk.GDBus.C.Name" Image \
	$(srcdir)/Image.xml
a11y-atspi-image.c: a11y-atspi-image.h
	@: # generated as a side-effect

a11y-atspi-registry.h: Registry.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Registry. \
	--generate-c-code $(srcdir)/a11y-atspi-registry \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Registry" "org.gtk.GDBus.C.Name" Registry \
	$(srcdir)/Registry.xml
a11y-atspi-registry.c: a11y-atspi-registry.h
	@: # generated as a side-effect

a11y-atspi-selection.h: Selection.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Selection. \
	--generate-c-code $(srcdir)/a11y-atspi-selection \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Selection" "org.gtk.GDBus.C.Name" Selection \
	$(srcdir)/Selection.xml
a11y-atspi-selection.c: a11y-atspi-selection.h
	@: # generated as a side-effect

a11y-atspi-socket.h: Socket.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Socket. \
	--generate-c-code $(srcdir)/a11y-atspi-socket \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Socket" "org.gtk.GDBus.C.Name" Socket \
	$(srcdir)/Socket.xml
a11y-atspi-socket.c: a11y-atspi-socket.h
	@: # generated as a side-effect

a11y-atspi-table.h: Table.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Table. \
	--generate-c-code $(srcdir)/a11y-atspi-table \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Table" "org.gtk.GDBus.C.Name" Table \
	$(srcdir)/Table.xml
a11y-atspi-table.c: a11y-atspi-table.h
	@: # generated as a side-effect

a11y-atspi-table-cell.h: TableCell.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.TableCell. \
	--generate-c-code $(srcdir)/a11y-atspi-table-cell \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.TableCell" "org.gtk.GDBus.C.Name" TableCell \
	$(srcdir)/TableCell.xml
a11y-atspi-table-cell.c: a11y-atspi-table-cell.h
	@: # generated as a side-effect

a11y-atspi-text.h: Text.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Text. \
	--generate-c-code $(srcdir)/a11y-atspi-text \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Text" "org.gtk.GDBus.C.Name" Text \
	$(srcdir)/Text.xml
a11y-atspi-text.c: a11y-atspi-text.h
	@: # generated as a side-effect

a11y-atspi-value.h: Value.xml
	$(AM_V_GEN) gdbus-codegen --interface-prefix org.a11y.atspi.Value. \
	--generate-c-code $(srcdir)/a11y-atspi-value \
	--c-namespace A11yAtspi \
	--annotate "org.a11y.atspi.Value" "org.gtk.GDBus.C.Name" Value \
	$(srcdir)/Value.xml
a11y-atspi-value.c: a11y-atspi-value.h
	@: # generated as a side-effect

BUILT_SOURCES = \
	a11y-atspi-accessible.c \
	a11y-atspi-accessible.h \
	a11y-atspi-action.c \
	a11y-atspi-action.h \
	a11y-atspi-application.c \
	a11y-atspi-application.h \
	a11y-atspi-cache.c \
	a11y-atspi-cache.h \
	a11y-atspi-collection.c \
	a11y-atspi-collection.h \
	a11y-atspi-component.c \
	a11y-atspi-component.h \
	a11y-atspi-device-event-listener.c \
	a11y-atspi-device-event-listener.h \
	a11y-atspi-device-event-controller.c \
	a11y-atspi-device-event-controller.h \
	a11y-atspi-document.c \
	a11y-atspi-document.h \
	a11y-atspi-editable-text.c \
	a11y-atspi-editable-text.h \
	a11y-atspi-hyperlink.c \
	a11y-atspi-hyperlink.h \
	a11y-atspi-hypertext.c \
	a11y-atspi-hypertext.h \
	a11y-atspi-image.c \
	a11y-atspi-image.h \
	a11y-atspi-registry.c \
	a11y-atspi-registry.h \
	a11y-atspi-selection.c \
	a11y-atspi-selection.h \
	a11y-atspi-socket.c \
	a11y-atspi-socket.h \
	a11y-atspi-table.c \
	a11y-atspi-table.h \
	a11y-atspi-table-cell.c \
	a11y-atspi-table-cell.h \
	a11y-atspi-text.c \
	a11y-atspi-text.h \
	a11y-atspi-value.c \
	a11y-atspi-value.h \
	$(NULL)

liba11yatspi_dbus_la_SOURCES = \
	$(BUILT_SOURCES) \
	$(NULL)

liba11yatspi_dbus_la_LIBADD = \
	$(GIO_UNIX_LIBS) \
	$(NULL)

liba11yatspi_dbus_la_CFLAGS = \
	$(GIO_UNIX_CFLAGS) \
	$(NULL)

#BUILT_SOURCES = spec.xml
#CLEANFILES += spec.xml

XML_SPEC = $(filter %.xml,$(EXTRA_DIST))

spec.xml: $(XML_SPEC)
	xsltproc --xinclude $(top_srcdir)/tools/identity.xsl Accessibility.xml >spec.xml


-include $(top_srcdir)/git.mk
