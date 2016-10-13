################################################################################
##
## Filename:	mkdatev.pl
##
## Project:	XuLA2 board
##
## Purpose:	This file creates a file containing a `define DATESTAMP
##		which can be used to tell when the build took place.
##
##
## Creator:	Dan Gisselquist, Ph.D.
##		Gisselquist Technology, LLC
##
################################################################################
##
## Copyright (C) 2016, Gisselquist Technology, LLC
##
## This program is free software (firmware): you can redistribute it and/or
## modify it under the terms of  the GNU General Public License as published
## by the Free Software Foundation, either version 3 of the License, or (at
## your option) any later version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
## for more details.
##
## License:	GPL, v3, as defined and found on www.gnu.org,
##		http:##www.gnu.org/licenses/gpl.html
##
##
################################################################################
##
##
#!/usr/bin/perl

$now = time;
($sc,$mn,$nhr,$ndy,$nmo,$nyr,$nwday,$nyday,$nisdst) = localtime($now);
$nyr = $nyr+1900; $nmo = $nmo+1;

print "`define DATESTAMP 32\'h";
printf("%04d%02d%02d\n", $nyr, $nmo, $ndy);

