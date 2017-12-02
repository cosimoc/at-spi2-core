#!/usr/bin/python

import sys

from gi.repository import Gio, GLib

class IdleStateM (object):
        def __init__(self, bus, loop):
                self._bus = bus
                self._loop = loop
                self._func = self.setup

        def idle_handler(self):
                self._func = self._func()
                if self._func == None:
                        self._func = self.teardown
                return True

        def setup(self):
                self.obj = Gio.DBusProxy.new_sync(self._bus, Gio.DBusProxyFlags.NONE,
                                                  None, "org.a11y.atspi.Registry",
                                                  "/org/a11y/atspi/accessible/root",
                                                  "org.a11y.atspi.Accessible", None)
                return self.register_apps

        def register_apps(self):
                #self.itf.registerApplication(":R456", ignore_reply=True)
                #self.itf.registerApplication(":R123", ignore_reply=True)
                return self.print_applications

        def print_applications(self):
                apps = self.obj.GetChildren()
                print "{} applications found".format(len(apps))
                print apps
                return self.teardown

        def teardown(self):
                self._loop.quit()

def main(argv):
        session_bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
        proxy = Gio.DBusProxy.new_sync(session_bus, Gio.DBusProxyFlags.NONE,
                                       None, "org.a11y.Bus", "/org/a11y/bus",
                                       "org.a11y.Bus", None)

        address = proxy.GetAddress()
        bus = Gio.DBusConnection.new_for_address_sync(address,
                                                      Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT |
                                                      Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION,
                                                      None, None)
        loop = GLib.MainLoop()
        stateM = IdleStateM(bus, loop)
        GLib.idle_add(stateM.idle_handler)
        loop.run()

if __name__=="__main__":
        sys.exit(main(sys.argv))
