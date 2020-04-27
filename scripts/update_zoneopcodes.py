#!/usr/bin/env python3

#  update_zoneopcodes.py
#  Copyright 2020, cn187 <cn187@users.sourceforge.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


'''
This module updates the zoneopcode.xml file from a plain list of opcode/opname
pairs.  It's a simple find/replace, rather than a full XML parse, so each
opening opcode tag, with attributes, may not span more than one line.

It can be run from the command line, using either files or stdin/stdout
for input and output.


usage: update_zoneopcodes.py [-h] [-i INPUT] [-o OUTPUT] opcode_file

Update zoneopcodes.xml from plain list of opcodes/names using a simple
find/replace.

positional arguments:
  opcode_file           The file containing the list of opcode/opname pairs

optional arguments:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        Filename/path of zoneopcodes.xml to read. If none
                        provided, defaults to reading from stdin
  -o OUTPUT, --output OUTPUT
                        Filename/path of zoneopcodes.xml file to write. If
                        none provided, defaults to writing to stdout


Sample usage:

update_zoneopcodes.py opcodes.txt -i conf/zoneopcodes.xml -o conf/zoneopcodes.new

or

update_zoneopcodes.py opcodes.txt <conf/zoneopcodes.xml >conf/zoneopcodes.new


The format of the opcodes.txt file is simple opcode/opname pairs, one per line:

    # Comments are allowed if prefixed with #
    ffff    OP_Foo
    eeee    OP_Bar

'''


import re
from datetime import datetime, date
import sys
import argparse


def update_zoneopcodes_filter(opcodes, infile, outfile):

    line_re = re.compile(r'{}{}{}{}'.format(
        '[ \t]*<opcode[ \t]+',
        'id="([A-Fa-f0-9]{4})"[ \t]+',
        'name="([A-Za-z0-9_]+)"[ \t]+',
        'updated="([01][0-9]/[0-3][0-9]/[0-9][0-9])"[ \t]*>[ \t]*\n'))


    new_date = datetime.now().strftime("%m/%d/%y")
    #new_date = date(2020, 4, 15).strftime("%m/%d/%y")  # For testing purposes

    updated = 0

    for line in infile.readlines():

        match = line_re.fullmatch(line)
        if not match:
            outfile.write(line)
            continue

        opcode = match.group(1)
        opname = match.group(2)
        update = match.group(3)

        if opname not in opcodes:
            outfile.write(line)
            continue

        opcode_start = match.start(1)
        opcode_end = match.end(1)

        opname_start = match.start(2)
        opname_end = match.end(2)

        date_start = match.start(3)
        date_end = match.end(3)

        new_line = line[:opcode_start] + opcodes[opname]
        new_line += line[opcode_end:date_start] + new_date + line[date_end:]

        outfile.write(new_line)

        updated += 1

    return (updated, len(opcodes))


def update_zoneopcodes_file(opcodes, xmlin, xmlout):
    with open(xmlin, 'r') as infile:
        with open(xmlout, 'w+') as outfile:
            update_zoneopcodes_filter(opcodes, infile, outfile)

def parse_opcode_file(opcode_file):

    with open(opcode_file, 'r') as opf:
        opcodes = {}
        for line in opf.readlines():
            # handle comments
            line = line[:line.find('#')] + "\n"
            parts = line.split()
            if parts:
                o, n = parts
                opcodes[n] = o

    return opcodes


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description='Update zoneopcodes.xml from plain list of opcodes/names \
            using a simple find/replace.',
        epilog='NOTE: Since this does not fully parse the XML, the opening \
            tag for each opcode (including the "id", "name", and "updated" \
            attributes) must not span more than one line.')


    parser.add_argument('opcode_file', help='The file containing the list \
                        of opcode/opname pairs')
    parser.add_argument('-i', '--input', type=str,
                        help='Filename/path of zoneopcodes.xml to read.  If \
                        none provided, defaults to reading from stdin')
    parser.add_argument('-o', '--output', type=str,
                        help='Filename/path of zoneopcodes.xml file to write. \
                        If none provided, defaults to writing to stdout')

    args = parser.parse_args()

    if args.input:
        infile = open(args.input, 'r')
    else:
        infile = sys.stdin

    if args.output:
        outfile = open(args.output, 'w+')
    else:
        outfile = sys.stdout


    opcodes = parse_opcode_file(args.opcode_file)

    update_zoneopcodes_filter(opcodes, infile, outfile)

    infile.close()
    outfile.close()
