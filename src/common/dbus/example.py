#! /usr/bin/python

# HexChat
# Copyright (C) 1998-2010 Peter Zelezny.
# Copyright (C) 2009-2013 Berke Viktor.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
#

import dbus

bus = dbus.SessionBus()
proxy = bus.get_object('org.pchat.service', '/org/pchat/Remote')
remote = dbus.Interface(proxy, 'org.pchat.connection')
path = remote.Connect ("example.py",
		       "Python example",
		       "Example of a D-Bus client written in python",
		       "1.0")
proxy = bus.get_object('org.pchat.service', path)
xchat = dbus.Interface(proxy, 'org.pchat.plugin')

channels = xchat.ListGet ("channels")
while xchat.ListNext (channels):
	name = xchat.ListStr (channels, "channel")
	print("------- " + name + " -------")
	xchat.SetContext (xchat.ListInt (channels, "context"))
	xchat.EmitPrint ("Channel Message", ["John", "Hi there", "@"])
	users = xchat.ListGet ("users")
	while xchat.ListNext (users):
		print("Nick: " + xchat.ListStr (users, "nick"))
	xchat.ListFree (users)
xchat.ListFree (channels)

print(xchat.Strip ("\00312Blue\003 \002Bold!\002", -1, 1|2))

