
import datetime
import sys
import idautils

""" Generate ShowEQ's zones.h from eggame.exe and Resources/ZoneNames.txt

    NOTE: Requires IDA and python3

    To use this script:
        1. Open eqgame.exe in IDA, and let IDA complete its analysis.
        2. Select View->Recent Scripts
        3. In the Recent Scripts tab, press Insert, and browse to the location
            of this script.  Select this script and click "Open".
        4. An entry for this script now appears in the list of scripts.
            Double-click it to load it.
        5. (optional) right click in the output window and select "Clear".  This
            will reduce the extra stuff you have to delete when you copy/paste
        6. At the idapython command line, run

            dump_zones('/path/to/ZoneNames.txt')

            Note: ZoneNames.txt is found in your EQ client directory, under the
            Resources subdirectory.  If you want you can copy it to somewhere
            convenient (e.g., /tmp), then pass that path to dump_zones.
        7. Create a new file (e.g. zones.h.new)
        8. Right-click in the output window, select "Select All", then "Copy".
        9. Paste into the file you created in step 7
       10. There will be a couple of lines (or more if you skipped step 5) at
            the top of the file (before the start of the block comment containing
            the copyright/license info) that need to be deleted. Basically,
            delete everything before the line consisting of "/*"
       11. Save the file
       12. As a sanity check, diff with zones.h to see what changed.  This is
            important in case the way the code works changed, or there's a bug
            in the script - we don't want to commit an empty or otherwise bad
            zones file.
       13. If everything looks good, rename your new file to zones.h and replace
            the existing zones.h.
       14. Do a test compile to make sure everything is good.
       15. Commit changes.
"""

def find_string_refs_to(haystack, needle):
    refs = set()
    for s in haystack:
        if str(s) == needle:
            for x in idautils.XrefsTo(s.ea):
                refs.add(x.to)
    return refs

def find_string_refs_from(haystack, needle):
    refs = set()
    for s in haystack:
        if str(s) == needle:
            for x in idautils.XrefsTo(s.ea):
                refs.add(x.frm)
    return refs


def get_worlddata_start(ida_strings=None):

    if not ida_strings:
        strings = idautils.Strings(True)
    else:
        strings = ida_strings

    key_string = 'EQWorldData::EQWorldData'

    eas = find_string_refs_from(strings, key_string)

    if len(eas) == 1:
        ea = eas.pop()
        return ea
    else:
        return None


# NOTE we could grab the addresses of EQWorldData::EQWorldData and Unknown Zone
# and use those for addr boundaries when looking for zone string loads
# (if there are too many conflicting loads in amongst the ones we want)

# NOTE also, if we have to, we could determine exact reg numbers based on known
# zone names, with the assumption that the compiler will reuse the same registers
# throughout the function
def get_next_zone_loaded(cur_ea, end_ea, rnum=None):
    ea = cur_ea

    while ea < end_ea:
        mnem = idc.print_insn_mnem(ea)
        if mnem == "lea":
            if idc.get_operand_type(ea, 0) != o_reg:
                ea = idc.next_head(ea)
                continue
            if rnum is not None and idc.get_operand_value(ea, 0) != rnum:
                ea = idc.next_head(ea)
                continue
            if idc.get_operand_type(ea, 1) != o_mem:
                ea = idc.next_head(ea)
                continue
            string = idc.get_strlit_contents(idc.get_operand_value(ea, 1))
            if not string:
                ea = idc.next_head(ea)
                continue
            return ea, idc.get_strlit_contents(idc.get_operand_value(ea, 1)).decode('ascii')
        ea = idc.next_head(ea)

    return ea, None



def get_next_file_index(cur_ea, end_ea, rnum=None):
    ea = cur_ea

    offset = 0
    backtrack = False

    while ea < end_ea:
        mnem = idc.print_insn_mnem(ea)
        if mnem == "lea":
            if idc.get_operand_type(ea, 0) != o_reg:
                ea = idc.next_head(ea)
                continue
            if rnum is not None and idc.get_operand_value(ea, 0) != rnum:
                ea = idc.next_head(ea)
                continue
            if idc.get_operand_type(ea, 1) != o_displ:
                ea = idc.next_head(ea)
                continue
            backtrack = True
            break
        elif mnem == "mov":
            if idc.get_operand_type(ea, 0) != o_reg:
                ea = idc.next_head(ea)
                continue
            if rnum is not None and idc.get_operand_value(ea, 0) != rnum:
                ea = idc.next_head(ea)
                continue
            if idc.get_operand_type(ea, 1) != o_imm:
                ea = idc.next_head(ea)
                continue
            return ea, idc.get_operand_value(ea, 1)
        ea = idc.next_head(ea)

    if backtrack:
            prev = idc.prev_head(ea)
            mnem = idc.print_insn_mnem(prev)
            if mnem == "xor" and \
               idc.get_operand_type(prev, 0) == o_reg and \
               idc.get_operand_type(prev,1) == o_reg:
                return ea, idc.get_operand_value(ea, 1)

            if mnem != "mov" or \
               idc.get_operand_type(prev, 0) != o_reg or\
               idc.get_operand_type(prev, 1) != o_imm:
                    sys.stderr.write("Backtrack failed at " + hex(prev) + '\n')
                    return ea, -1

            offset = idc.get_operand_value(prev, 1)

            return ea, idc.get_operand_value(ea, 1) + offset

    return ea, -1

def find_qeynos_load(cur_ea, end_ea):
    ea = cur_ea

    while ea < end_ea:
        ea, shortzone = get_next_zone_loaded(ea, end_ea)
        if shortzone != 'qeynos':
            ea = idc.next_head(ea)
            continue

        rnum = idc.get_operand_value(ea, 0)

        return ea, shortzone, rnum

def find_qeynos_index(cur_ea, end_ea):
    ea = cur_ea

    while ea < end_ea:
        ea, idx = get_next_file_index(ea, end_ea)
        if idx != 1:
            ea = idc.next_head(ea)
            continue

        rnum = idc.get_operand_value(ea, 0)

        return ea, idx, rnum




# if you know the address where "EQWorldData::EQWorldData" is loaded, you can
# pass it as the start_ea
def dump_zones(zonenames_path=None, ida_strings=None, start_ea=None):

    if not zonenames_path:
        sys.stderr.write("Path to ZoneNames.txt is required\n")
        return

    if not ida_strings:
        strings = idautils.Strings(True)
    else:
        strings = ida_strings

    if not start_ea:
        start_ea = get_worlddata_start(strings)
    if not start_ea:
        sys.stderr.write("Could not find unique start ea.  Found: " + eas + '\n')
        return

    wd_func = ida_funcs.get_func(start_ea)
    func_end = wd_func.end_ea

    cur_ea, shortzone, zone_regnum = find_qeynos_load(idc.next_head(start_ea), func_end)
    if not cur_ea:
        sys.stderr.write("Could not find first zone load\n")
        return

    cur_ea, idx, idx_regnum = find_qeynos_index(idc.next_head(start_ea), func_end) 
    if not cur_ea:
        sys.stderr.write("Could not find first load index\n")
        return

    zonenames = {}
    zones = {}

    with open(zonenames_path, 'r') as f:
        for line in f.readlines():
            index, zonename, _, _ = line.split('^')
            if index == "ZONE":
                continue
            zonenames[int(index)] = zonename.strip()

    #print(idx, shortzone, zonenames[idx])
    zones[idx] = (idx, shortzone, zonenames[idx])

    while cur_ea < func_end:
        cur_ea, shortzone = get_next_zone_loaded(cur_ea, func_end, zone_regnum)
        if not shortzone:
            break

        cur_ea = idc.next_head(cur_ea)
        cur_ea, idx = get_next_file_index(cur_ea, func_end, idx_regnum)
        if idx < 0:
            break;

        if idx not in zonenames:
            cur_ea = idc.next_head(cur_ea)
            #sys.stderr.write("Did not find index {} for zone {} in ZoneNames.txt\n".format(idx, shortzone))
            zones[idx] = (idx, shortzone, shortzone)
        else:
            #print(idx, shortzone, zonenames[idx])
            zones[idx] = (idx, shortzone, zonenames[idx])

        cur_ea = idc.next_head(cur_ea)


    keys = sorted(zones.keys())
    last_idx = keys[-1]
    current_year=datetime.datetime.now().year
    print(f"""/*
 *  zones.h
 *  Copyright 2003-{current_year} by the respective ShowEQ Developers
 *
 *  This file is part of ShowEQ.
 *  http://www.sourceforge.net/projects/seq
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Maps zoneId numbers to strings, used in util.cpp */""")

    for i in range(0, last_idx+1):
        if i in zones:
            idx, shortzone, longzone = zones[i]
            print('{{ "{}", "{}" }}, // {}'.format(shortzone, longzone, idx))
        else:
            print('{{ NULL, NULL }}, // {}'.format(i))


