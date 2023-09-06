#!/usr/bin/env python

# usage: gen-lookup.py [-h] <filename> <start> <end> <default> <decl>

# Make work with Python v2.7 and v3.
from __future__ import print_function

import sys, re, os
import argparse

s_me = os.path.basename (sys.argv[0])

formatter = argparse.RawDescriptionHelpFormatter
parser = argparse.ArgumentParser (formatter_class=formatter,
                                  description="""
A simple line-by-line parser that generates lookup tables for arrays
of Glyph's.  Each line of the Glyph's array must be of the form

    { code, width, name } [,] [comment]

where 'code' is some integer like '123' or '0x45'.  The output is
a lookup table printed to stdout.
""")

# Add 5 positional arguments.

# A file name like "unifont.c"
parser.add_argument ("filename", metavar="<filename>", help="""Name of a
 C file that contains an array for which a lookup table is to be generated.""")

# Specify the start of the array for which we are going to produce
# a lookup table.  For example "Glyph glyph" or "Glyph glyph[]".
parser.add_argument ("start", metavar="<start>", help="""A string that matches
 (part of) the start of the array definition like 'Glyph glyph[]'.""")

# Specify the end of the array, like "};".
parser.add_argument ("end", metavar="<end>", help="""A string that matches
 the end of the array definition like '};'.""")

# Name of a glyph to use as a default.
# This refers to the 3rd field of a glyph like 'QUESTION_MARK'.
parser.add_argument ("default", help="""A string that specifies a default
 glyph to use when there are less than 256 entries in the array. This refers
 to the 3rd field of a glyph like 'QUESTION_MARK'.""", metavar="<default>")

# String that provides the declaration for the generated lookup table
# like const __flash uint8_t lookup[256]".
parser.add_argument ("decl", metavar="<decl>", help="""The declarator for
 the lookup table that will be generated, as a string like
 'const __flash uint8_t lookup[256]'.""")

args = parser.parse_args()

########################################################################

# { word, word, word } [, ...]
pat_line = re.compile (r'\s*{\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*}(.*)')

# Diagnostic to stderr with loc = ( filename, line-number, line-text ).
def eprint (loc, txt):

    if loc is None:
        print ("\nerror: %s" % txt, file=sys.stderr)
    elif loc[2] is None:
        print ("\nerror: %s: %s" % (loc[0], txt), file=sys.stderr)
    else:
        print ("\n%s:%d: error: %s" % (loc[0], loc[1], txt), file=sys.stderr)
        print ("%s:%d: this is the line: %s" % (loc[0], loc[1], loc[2].strip()),
               file=sys.stderr)


class Glyph:
    def __init__ (self, id, s_code, s_width, s_glyph, loc):
        """ Pop a new Glyph object.  s_code, s_width and s_glyph are the
            three elements found in a line of a Glyph[] array, all read as
            strings.  id is the index in the array, deduced by counting lines.
            loc = ( filename, line-number, line-text ) narrows down the
            location where the current entry was found in the C source file.
        """
        self.id = id
        self.s_code, self.s_width, self.s_glyph = s_code, s_width, s_glyph
        self.loc = ( loc[0], loc[1], loc[2] )

        # s_code must resemble an integer like 123 or "0x12".
        self.code = code = int (s_code, 0)

        # Code, but as an ASCII character value value if applicable, or None.
        self.c_code = None
        if code >= 0x20 and code <= 0x7f and not chr(code) in "'\\`":
            self.c_code = chr(code)

        max_code = 0xff;
        if code > max_code or code < 0:
            args = ( s_code, max_code, max_code )
            eprint (loc, "code = %s exceeds %d = 0x%x" % args),
            sys.exit (-1)


# Can we ignore s? Like 1-line C/C++ comments empty strings.
def is_void (s, loc):
    s = s.strip()
    if s == "":
        # Ignore empty lines / parts.
        return True
    elif s.startswith ("//"):
        # Ignore C99 / C++ comments.
        return True
    elif s.startswith ("/*"):
        # Try ignore C comments.
        if not s.endswith ("*/"):
            eprint (loc, "multi-line comments are not supported by " + s_me)
            sys.exit (1)
        # Ignore C comments.
        return True
    return False


# Decode one line (of the input file) into a Glyph object, or fail.
def decode_line (loc, id):
    m = pat_line.match (loc[2])
    if not m:
        eprint (loc, "line not recognized")
        sys.exit(-1)

    # Check code after { code, width, glyph-name }.
    # This might be a ",", which might be followed by a 1-line comment.
    s_tail = m.group(4).strip()
    if s_tail.startswith(","): s_tail = s_tail[1:]
    if not is_void (s_tail, loc):
        eprint (loc, "line not recognized")
        sys.exit(-1)

    return Glyph (id, m.group(1), m.group(2), m.group(3), loc)


# Run over all lines of c_file_name and try find an array definition
# that starts with s_start and ends with s_end.  If found, parse all lines
# between s_start and s_end to Glyph objects, and return them.
def decode_glyphs (c_file_name, s_start, s_end):
    # Return value maps codes to glyphs of that code.
    glyphs = 256 * [None]

    # Whether we are parsing a part between s_start and s_end.
    started = False

    # Line count for diagnostics
    n_lines = 0
    loc0 = None

    # Index if the glyph in the array.
    id = 0

    with open (c_file_name, "r") as c_file:
        for line in c_file.readlines():
            n_lines += 1
            if not started and s_start in line:
                # Found the start / declarator of the array definition.
                started = True
                loc0 = (c_file_name, n_lines, line)
            elif started:
                # Somewhere between declarator and the end of the array
                # definition.  This must be a line of the form
                # { wode, word, word } [,] [tail]
                # where tail is empty or a 1-line comment.

                # Location info for diagnostics.
                loc = (c_file_name, n_lines, line)

                if line.strip().startswith (s_end):
                    # Found the end of the array definition.
                    global loc_end
                    loc_end = n_lines
                    return loc0, glyphs
                elif is_void (line, loc):
                    # Ignore empty lines and 1-line comments.
                    pass
                else:
                    # Anything that's not a 1-line comment or the end of the
                    # array must be a glyph defined in *ONE* line.
                    g = decode_line (loc, id)
                    glyphs[g.code] = g
                    id += 1

    # Dropping out of the file...
    if started:
        # ... without having found s_end, but s_start was present.
        eprint (loc0, "<start> pattern `%s` not followed by <end>"
                + " pattern '%s'" % (s_start, s_end))
    else:
        # ... without having found s_start.
        eprint (no_loc, "<start> pattern '%s' not found" % s_start)
    sys.exit (-1)


def print_lookup_table (loc, glyphs, s_start, s_decl, s_default):
    n_defaults = 0
    g_default = None

    # Determine whether we need a default glyph; and if so, find it.
    for i in range (0,256):
        if glyphs[i] is None:
            n_defaults += 1
            if g_default:
                # Already found the default glyph.
                continue

            # Need the default elememt, go find it.
            for i in range (0,256):
                if (glyphs[i] and glyphs[i].s_glyph == s_default):
                    g_default = glyphs[i]
                    break
            else:
                eprint (loc, "default element '%s' not found" % s_default)
                exit (-1)

    print ("// A lookup table that maps Glyph.code to the index of the glyph")
    print ("// in array '%s' from %s:%d." % (s_start, loc[0], loc[1]))
    print ("// 256 entries, %d glyphs, %d defaults."
           % (256 - n_defaults, n_defaults))
    print ("%s" % (s_decl + " =\n{"))

    s_line = "    [0x%02x] = 0x%02x, // @%d, %s%s"

    # Print the 256 lines that comprise the entries of s_decl.
    for code in range (0,256):
        g = glyphs[code]
        if g_default and code == g_default.code:
            print ("    // == DEFAULT ==")
        if g is None:
            print (s_line % (code, g_default.id, g_default.id,
                             "default = ", g_default.s_glyph))
        else:
            c_code = "" if g.c_code is None else (" = '%s'" % g.c_code)
            print (s_line % (code, g.id, g.id, g.s_glyph, c_code))
    print ("};")

###########################################################################

# To diagnose errors that are caused by problems with filename,
# but without having actual line to point to.
no_loc = (args.filename, 0, None)

if args.start.strip() == "":
    eprint (None, "empty <start> pattern")
    exit (-1)

if args.end.strip() == "":
    eprint (None, "empty <end> pattern")
    exit (-1)

if args.default.strip() == "":
    eprint (None, "empty <default> pattern")
    exit (-1)


loc, glyphs = decode_glyphs (args.filename, args.start, args.end)

print ("// Auto-generated file, don't change by hand.")
print ("// Generated from: %s, lines %d -- %d."
       % (args.filename, loc[1], loc_end))
print ("// Generated with: %s '%s' '%s' '%s' '%s' '%s'"
       % (s_me, args.filename, args.start, args.end, args.default, args.decl))
print ("")

print_lookup_table (loc, glyphs, args.start, args.decl, args.default)
