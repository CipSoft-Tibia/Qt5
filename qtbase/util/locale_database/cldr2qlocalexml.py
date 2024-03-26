#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Convert CLDR data to QLocaleXML

The CLDR data can be downloaded as a zip-file from CLDR_, which has a
sub-directory for each version; you need the ``core.zip`` file for
your version of choice (typically the latest), which you should then
unpack. Alternatively, you can clone the git repo from github_, which
has a tag for each release and a maint/maint-$ver branch for each
major version. Either way, the CLDR top-level directory should have a
subdirectory called common/ which contains (among other things)
subdirectories main/ and supplemental/.

This script has had updates to cope up to v44.1; for later versions,
we may need adaptations. Pass the path of the CLDR top-level directory
to this script as its first command-line argument. Pass the name of
the file in which to write output as the second argument; either omit
it or use '-' to select the standard output. This file is the input
needed by ``./qlocalexml2cpp.py``

When you update the CLDR data, be sure to also update
src/corelib/text/qt_attribution.json's entry for unicode-cldr. Check
this script's output for unknown language, territory or script messages;
if any can be resolved, use their entry in common/main/en.xml to
append new entries to enumdata.py's lists and update documentation in
src/corelib/text/qlocale.qdoc, adding the new entries in alphabetic
order.

While updating the locale data, check also for updates to MS-Win's
time zone names; see cldr2qtimezone.py for details.

All the scripts mentioned support --help to tell you how to use them.

.. _CLDR: https://unicode.org/Public/cldr/
.. _github: https://github.com/unicode-org/cldr
"""

from pathlib import Path
import sys
import argparse

from cldr import CldrReader
from qlocalexml import QLocaleXmlWriter


def main(out, err):
    all_calendars = ['gregorian', 'persian', 'islamic']  # 'hebrew'

    parser = argparse.ArgumentParser(
        description='Generate QLocaleXML from CLDR data.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('cldr_path', help='path to the root of the CLDR tree')
    parser.add_argument('out_file', help='output XML file name',
                        nargs='?', metavar='out-file.xml')
    parser.add_argument('--calendars', help='select calendars to emit data for',
                        nargs='+', metavar='CALENDAR',
                        choices=all_calendars, default=all_calendars)

    args = parser.parse_args()

    root = Path(args.cldr_path)
    root_xml_path = 'common/main/root.xml'

    if not root.joinpath(root_xml_path).exists():
        parser.error('First argument is the root of the CLDR tree: '
                     f'found no {root_xml_path} under {root}')

    xml = args.out_file
    if not xml or xml == '-':
        emit = out
    elif not xml.endswith('.xml'):
        parser.error(f'Please use a .xml extension on your output file name, not {xml}')
    else:
        try:
            emit = open(xml, 'w')
        except IOError as e:
            parser.error(f'Failed to open "{xml}" to write output to it')

    # TODO - command line options to tune choice of grumble and whitter:
    reader = CldrReader(root, err.write, err.write)
    writer = QLocaleXmlWriter(emit.write)

    writer.version(reader.root.cldrVersion)
    writer.enumData()
    writer.likelySubTags(reader.likelySubTags())
    writer.locales(reader.readLocales(args.calendars), args.calendars)

    writer.close(err.write)
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.stdout, sys.stderr))
