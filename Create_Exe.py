#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  parse.py
#
#  Copyright 2017 Floris <floris@ndn-icarus-simulator>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPsubprocessE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Bsubprocesston,
#  MA 02110-1301, USA.
#
#

import subprocess

print "Pulling from GIT"
subprocess.call(["git", "pull"])

print "Compiling"
subprocess.call("./waf")
print "Building executable"
subprocess.call(["./ErmineLightTrial.x86_64", "build/scratch/wsn-iot-v1/wsn-iot-v1", "--output", "static_exe.run"])
print "Done!"