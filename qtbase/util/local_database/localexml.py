#############################################################################
##
## Copyright (C) 2017 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################
"""Shared serialization-scanning code for QLocaleXML format.

The Locale class is written by cldr2qlocalexml.py and read by qlocalexml2cpp.py
"""
from xml.sax.saxutils import escape

import xpathlite

# Tools used by Locale:
def camel(seq):
    yield seq.next()
    for word in seq:
        yield word.capitalize()

def camelCase(words):
    return ''.join(camel(iter(words)))

def ordStr(c):
    if len(c) == 1:
        return str(ord(c))
    raise xpathlite.Error('Unable to handle value "%s"' % addEscapes(c))

# Fix for a problem with QLocale returning a character instead of
# strings for QLocale::exponential() and others. So we fallback to
# default values in these cases.
def fixOrdStr(c, d):
    return str(ord(c if len(c) == 1 else d))

def startCount(c, text): # strspn
    """First index in text where it doesn't have a character in c"""
    assert text and text[0] in c
    try:
        return (j for j, d in enumerate(text) if d not in c).next()
    except StopIteration:
        return len(text)

def convertFormat(format):
    """Convert date/time format-specier from CLDR to Qt

    Match up (as best we can) the differences between:
    * https://www.unicode.org/reports/tr35/tr35-dates.html#Date_Field_Symbol_Table
    * QDateTimeParser::parseFormat() and QLocalePrivate::dateTimeToString()
    """
    result = ""
    i = 0
    while i < len(format):
        if format[i] == "'":
            result += "'"
            i += 1
            while i < len(format) and format[i] != "'":
                result += format[i]
                i += 1
            if i < len(format):
                result += "'"
                i += 1
        else:
            s = format[i:]
            if s.startswith('E'): # week-day
                n = startCount('E', s)
                if n < 3:
                    result += 'ddd'
                elif n == 4:
                    result += 'dddd'
                else: # 5: narrow, 6 short; but should be name, not number :-(
                    result += 'd' if n < 6 else 'dd'
                i += n
            elif s[0] in 'ab': # am/pm
                # 'b' should distinguish noon/midnight, too :-(
                result += "AP"
                i += startCount('ab', s)
            elif s.startswith('S'): # fractions of seconds: count('S') == number of decimals to show
                result += 'z'
                i += startCount('S', s)
            elif s.startswith('V'): # long time zone specifiers (and a deprecated short ID)
                result += 't'
                i += startCount('V', s)
            elif s[0] in 'zv': # zone
                # Should use full name, e.g. "Central European Time", if 'zzzz' :-(
                # 'v' should get generic non-location format, e.g. PT for "Pacific Time", no DST indicator
                result += "t"
                i += startCount('zv', s)
            else:
                result += format[i]
                i += 1

    return result

class Locale:
    # Tool used during class body (see del below), not method:
    def propsMonthDay(lengths=('long', 'short', 'narrow'), scale=('months', 'days')):
        for L in lengths:
            for S in scale:
                yield camelCase((L, S))
                yield camelCase(('standalone', L, S))

    # Expected to be numbers, read with int():
    __asint = ("decimal", "group", "zero",
               "list", "percent", "minus", "plus", "exp",
               "currencyDigits", "currencyRounding")
    # Single character; use the code-point number for each:
    __asord = ("quotationStart", "quotationEnd",
               "alternateQuotationStart", "alternateQuotationEnd")
    # Convert day-name to Qt day-of-week number:
    __asdow = ("firstDayOfWeek", "weekendStart", "weekendEnd")
    # Convert from CLDR format-strings to QDateTimeParser ones:
    __asfmt = ("longDateFormat", "shortDateFormat", "longTimeFormat", "shortTimeFormat")
    # Just use the raw text:
    __astxt = ("language", "languageEndonym", "script", "country", "countryEndonym",
               "listPatternPartStart", "listPatternPartMiddle",
               "listPatternPartEnd", "listPatternPartTwo", "am", "pm",
               'byte_unit', 'byte_si_quantified', 'byte_iec_quantified',
               "currencyIsoCode", "currencySymbol", "currencyDisplayName",
               "currencyFormat", "currencyNegativeFormat"
               ) + tuple(propsMonthDay())
    del propsMonthDay

    # Day-of-Week numbering used by Qt:
    __qDoW = {"mon": 1, "tue": 2, "wed": 3, "thu": 4, "fri": 5, "sat": 6, "sun": 7}

    @classmethod
    def fromXmlData(cls, lookup):
        """Constructor from the contents of XML elements.

        Single parameter, lookup, is called with the names of XML
        elements that should contain the relevant data, within a CLDR
        locale element (within a localeList element); these names are
        used for the attributes of the object constructed.  Attribute
        values are obtained by suitably digesting the returned element
        texts.\n"""
        data = {}
        for k in cls.__asint:
            data['listDelim' if k == 'list' else k] = int(lookup(k))

        for k in cls.__asord:
            value = lookup(k)
            assert len(value) == 1, \
                (k, value, 'value should be exactly one character')
            data[k] = ord(value)

        for k in cls.__asdow:
            data[k] = cls.__qDoW[lookup(k)]

        for k in cls.__asfmt:
            data[k] = convertFormat(lookup(k))

        for k in cls.__astxt:
            data[k] = lookup(k)

        return cls(data)

    def toXml(self, indent='        ', tab='    '):
        print indent + '<locale>'
        inner = indent + tab
        get = lambda k: getattr(self, k)
        for key in ('language', 'script', 'country'):
            print inner + "<%s>" % key + get(key) + "</%s>" % key
            print inner + "<%scode>" % key + get(key + '_code') + "</%scode>" % key

        for key in ('decimal', 'group', 'zero'):
            print inner + "<%s>" % key + ordStr(get(key)) + "</%s>" % key
        for key, std in (('list', ';'), ('percent', '%'),
                         ('minus', '-'), ('plus', '+'), ('exp', 'e')):
            print inner + "<%s>" % key + fixOrdStr(get(key), std) + "</%s>" % key

        for key in ('language_endonym', 'country_endonym',
                    'quotationStart', 'quotationEnd',
                    'alternateQuotationStart', 'alternateQuotationEnd',
                    'listPatternPartStart', 'listPatternPartMiddle',
                    'listPatternPartEnd', 'listPatternPartTwo',
                    'byte_unit', 'byte_si_quantified', 'byte_iec_quantified',
                    'am', 'pm', 'firstDayOfWeek',
                    'weekendStart', 'weekendEnd',
                    'longDateFormat', 'shortDateFormat',
                    'longTimeFormat', 'shortTimeFormat',
                    'standaloneLongMonths', 'standaloneShortMonths',
                    'standaloneNarrowMonths',
                    'longMonths', 'shortMonths', 'narrowMonths',
                    'longDays', 'shortDays', 'narrowDays',
                    'standaloneLongDays', 'standaloneShortDays', 'standaloneNarrowDays',
                    'currencyIsoCode', 'currencySymbol', 'currencyDisplayName',
                    'currencyFormat', 'currencyNegativeFormat'):
            ent = camelCase(key.split('_')) if key.endswith('_endonym') else key
            print inner + "<%s>%s</%s>" % (ent, escape(get(key)).encode('utf-8'), ent)

        for key in ('currencyDigits', 'currencyRounding'):
            print inner + "<%s>%d</%s>" % (key, get(key), key)

        print indent + "</locale>"

    def __init__(self, data=None, **kw):
        if data: self.__dict__.update(data)
        if kw: self.__dict__.update(kw)

    @classmethod
    def C(cls,
          # Empty entries at end to ensure final separator when join()ed:
          months = ('January', 'February', 'March', 'April', 'May', 'June', 'July',
                    'August', 'September', 'October', 'November', 'December', ''),
          days = ('Sunday', 'Monday', 'Tuesday', 'Wednesday',
                  'Thursday', 'Friday', 'Saturday', ''),
          quantifiers=('k', 'M', 'G', 'T', 'P', 'E')):
        """Returns an object representing the C locale."""
        return cls(language='C', language_code='0', language_endonym='',
                   script='AnyScript', script_code='0',
                   country='AnyCountry', country_code='0', country_endonym='',
                   decimal='.', group=',', list=';', percent='%',
                   zero='0', minus='-', plus='+', exp='e',
                   quotationStart='"', quotationEnd='"',
                   alternateQuotationStart='\'', alternateQuotationEnd='\'',
                   listPatternPartStart='%1, %2',
                   listPatternPartMiddle='%1, %2',
                   listPatternPartEnd='%1, %2',
                   listPatternPartTwo='%1, %2',
                   byte_unit='bytes',
                   byte_si_quantified=';'.join(q + 'B' for q in quantifiers),
                   byte_iec_quantified=';'.join(q.upper() + 'iB' for q in quantifiers),
                   am='AM', pm='PM', firstDayOfWeek='mon',
                   weekendStart='sat', weekendEnd='sun',
                   longDateFormat='EEEE, d MMMM yyyy', shortDateFormat='d MMM yyyy',
                   longTimeFormat='HH:mm:ss z', shortTimeFormat='HH:mm:ss',
                   longMonths=';'.join(months),
                   shortMonths=';'.join(m[:3] for m in months),
                   narrowMonths='1;2;3;4;5;6;7;8;9;10;11;12;',
                   standaloneLongMonths=';'.join(months),
                   standaloneShortMonths=';'.join(m[:3] for m in months),
                   standaloneNarrowMonths=';'.join(m[:1] for m in months),
                   longDays=';'.join(days),
                   shortDays=';'.join(d[:3] for d in days),
                   narrowDays='7;1;2;3;4;5;6;',
                   standaloneLongDays=';'.join(days),
                   standaloneShortDays=';'.join(d[:3] for d in days),
                   standaloneNarrowDays=';'.join(d[:1] for d in days),
                   currencyIsoCode='', currencySymbol='',
                   currencyDisplayName=';' * 7,
                   currencyDigits=2, currencyRounding=1,
                   currencyFormat='%1%2', currencyNegativeFormat='')
