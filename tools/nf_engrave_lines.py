#!/usr/bin/python

"""
    engrave-lines.py G-Code Engraving Generator for command-line usage
    (C) ArcEye <2012>  <arceye at mgware dot co dot uk>
    syntax  ---   see helpfile below
    
    Allows the generation of multiple lines of engraved text in one go
    Will take each string arguement, apply X and Y offset generating code until last line done
    
  
    based upon code from engrave-11.py
    Copyright (C) <2008>  <Lawrence Glaister> <ve7it at shaw dot ca>
                     based on work by John Thornton  -- GUI framwork from arcbuddy.py
                     Ben Lipkowitz  (fenn)-- cxf2cnc.py v0.5 font parsing code

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    Rev v2 21.06.2012 ArcEye
"""
# change this if you want to use another font
#fontfile = "/usr/share/qcad/fonts/romanc.cxf"
fontfile = "fonts/normal.cxf"

#from Tkinter import *
from math import *
import os
import re
import sys
import string
import getopt

Preamble = "G17 G21 G40 G90 G64 P0.003 F%f"
Postamble = "M2"

stringlist = []

#=======================================================================
class Character:
    def __init__(self, key):
        self.key = key
        self.stroke_list = []

    def __repr__(self):
        return "%s" % (self.stroke_list)

    def get_xmax(self):
        try: return max([s.xmax for s in self.stroke_list[:]])
        except ValueError: return 0

    def get_ymax(self):
        try: return max([s.ymax for s in self.stroke_list[:]])
        except ValueError: return 0



#=======================================================================
class Line:

    def __init__(self, coords):
        self.xstart, self.ystart, self.xend, self.yend = coords
        self.xmax = max(self.xstart, self.xend)
        self.ymax = max(self.ystart, self.yend)

    def __repr__(self):
        return "Line([%s, %s, %s, %s])" % (self.xstart, self.ystart, self.xend, self.yend)




#=======================================================================
# This routine parses the .cxf font file and builds a font dictionary of
# line segment strokes required to cut each character.
# Arcs (only used in some fonts) are converted to a number of line
# segemnts based on the angular length of the arc. Since the idea of
# this font description is to make it support independant x and y scaling,
# we can not use native arcs in the gcode.
#=======================================================================
def parse(file):
    font = {}
    key = None
    num_cmds = 0
    line_num = 0
    for text in file:
        #format for a typical letter (lowercase r):
        ##comment, with a blank line after it
        #
        #[r] 3
        #L 0,0,0,6
        #L 0,6,2,6
        #A 2,5,1,0,90
        #
        line_num += 1
        end_char = re.match('^$', text) #blank line
        if end_char and key: #save the character to our dictionary
            font[key] = Character(key)
            font[key].stroke_list = stroke_list
            font[key].xmax = xmax
            if (num_cmds != cmds_read):
                print "(warning: discrepancy in number of commands %s, line %s, %s != %s )" % (fontfile, line_num, num_cmds, cmds_read)

        new_cmd = re.match('^\[(.*)\]\s(\d+)', text)
        if new_cmd: #new character
            key = new_cmd.group(1)
            num_cmds = int(new_cmd.group(2)) #for debug
            cmds_read = 0
            stroke_list = []
            xmax, ymax = 0, 0

        line_cmd = re.match('^L (.*)', text)
        if line_cmd:
            cmds_read += 1
            coords = line_cmd.group(1)
            coords = [float(n) for n in coords.split(',')]
            stroke_list += [Line(coords)]
            xmax = max(xmax, coords[0], coords[2])

        arc_cmd = re.match('^A (.*)', text)
        if arc_cmd:
            cmds_read += 1
            coords = arc_cmd.group(1)
            coords = [float(n) for n in coords.split(',')]
            xcenter, ycenter, radius, start_angle, end_angle = coords
            # since font defn has arcs as ccw, we need some font foo
            if ( end_angle < start_angle ):
                start_angle -= 360.0
            # approximate arc with line seg every 20 degrees
            segs = int((end_angle - start_angle) / 20) + 1
            angleincr = (end_angle - start_angle)/segs
            xstart = cos(start_angle * pi/180) * radius + xcenter
            ystart = sin(start_angle * pi/180) * radius + ycenter
            angle = start_angle
            for i in range(segs):
                angle += angleincr
                xend = cos(angle * pi/180) * radius + xcenter
                yend = sin(angle * pi/180) * radius + ycenter
                coords = [xstart,ystart,xend,yend]
                stroke_list += [Line(coords)]
                xmax = max(xmax, coords[0], coords[2])
                ymax = max(ymax, coords[1], coords[3])
                xstart = xend
                ystart = yend
    return font


#=======================================================================

def __init__(key):
    key = key
    stroke_list = []

def __repr__():
    return "%s" % (stroke_list)

def get_xmax():
    try: return max([s.xmax for s in stroke_list[:]])
    except ValueError: return 0

def get_ymax():
    try: return max([s.ymax for s in stroke_list[:]])
    except ValueError: return 0



#=======================================================================


def __init__( coords):
    xstart, ystart, xend, yend = coords
    xmax = max(xstart, xend)
    ymax = max(ystart, yend)

def __repr__():
    return "Line([%s, %s, %s, %s])" % (xstart, ystart, xend, yend)


#=======================================================================
def sanitize(string):
    retval = ''
    good=' ~!@#$%^&*_+=-{}[]|\:;"<>,./?'
    for char in string:
        if char.isalnum() or good.find(char) != -1:
            retval += char
        else: retval += ( ' 0x%02X ' %ord(char))
    return retval

#=======================================================================
# routine takes an x and a y in raw internal format
# x and y scales are applied and then x,y pt is rotated by angle
# Returns new x,y tuple
def Rotn(x,y,xscale,yscale,angle):
    Deg2Rad = 2.0 * pi / 360.0
    xx = x * xscale
    yy = y * yscale
    rad = sqrt(xx * xx + yy * yy)
    theta = atan2(yy,xx)
    newx=rad * cos(theta + angle*Deg2Rad)
    newy=rad * sin(theta + angle*Deg2Rad)
    return newx,newy



#=======================================================================

def code(String, visit, last, options):
    str1 = ""
    #erase old gcode as needed
    gcode = []
    
    file = open(fontfile)
  
    oldx = oldy = -99990.0      
    
                 
    if visit != 0:
        # all we need is new X and Y for subsequent lines
        gcode.append("(===================================================================)")
        gcode.append('( Engraving: "%s" )' %(String) )
        gcode.append('( Line %d )' %(visit))

        str1 = '#1002 = %.4f  ( X Start )' %(options.xstart)        
        if options.xlineoffset :
            if options.xindentlist.find(str(visit)) != -1 :
                str1 = '#1002 = %.4f  ( X Start )' %(options.xstart + options.xlineoffset)
            
        gcode.append(str1)
        gcode.append('#1003 = %.4f  ( Y Start )' %(options.ystart - (options.ylineoffset * visit)))
        gcode.append("(===================================================================)")
        
    else:
        gcode.append('( Line %d )' %(visit))
        gcode.append('( Code generated by engrave-lines.py )')
        gcode.append('( args:  %s )' % ' '.join(sys.argv)) 
        gcode.append('( by ArcEye 2012, based on work by <Lawrence Glaister>)')
        gcode.append('( Engraving: "%s")' %(String) )
        gcode.append('( Fontfile: %s )' %(fontfile))
        # write out subroutine for rotation logic just once at head
        gcode.append("(===================================================================)")
        gcode.append("(Subroutine to handle x,y rotation about 0,0)")
        gcode.append("(input x,y get scaled, rotated then offset )")
        gcode.append("( [#1 = 0 or 1 for a G0 or G1 type of move], [#2=x], [#3=y])")
        gcode.append("o9000 sub")
        gcode.append("  #28 = [#2 * #1004]  ( scaled x )")
        gcode.append("  #29 = [#3 * #1005]  ( scaled y )")
        gcode.append("  #30 = [SQRT[#28 * #28 + #29 * #29 ]]   ( dist from 0 to x,y )")
        gcode.append("  #31 = [ATAN[#29]/[#28]]                ( direction to  x,y )")
        gcode.append("  #32 = [#30 * cos[#31 + #1006]]     ( rotated x )")
        gcode.append("  #33 = [#30 * sin[#31 + #1006]]     ( rotated y )")
        gcode.append("  o9010 if [#1 LT 0.5]" )
        gcode.append("    G00 X[#32+#1002] Y[#33+#1003]")
        gcode.append("  o9010 else")
        gcode.append("    G01 X[#32+#1002] Y[#33+#1003]")
        gcode.append("  o9010 endif")
        gcode.append("o9000 endsub")
        gcode.append("(===================================================================)")
    
        gcode.append("#1000 = %.4f" %(options.safe_z))
        gcode.append('#1001 = %.4f  ( Engraving depth Z )' %(options.engrave_z))
        
        str1 = '#1002 = %.4f  ( X Start )' %(options.xstart)        
        if options.xlineoffset :
            if options.xindentlist.find(str(visit)) != -1 :
                str1 = '#1002 = %.4f  ( X Start )' %(options.xstart + options.xlineoffset)
        gcode.append(str1)
        gcode.append('#1003 = %.4f  ( Y Start )' %(options.ystart))
        gcode.append('#1004 = %.4f  ( X Scale )' %(options.xscale))
        gcode.append('#1005 = %.4f  ( Y Scale )' %(options.yscale))
        gcode.append('#1006 = %.4f  ( options.angle )' %(options.angle))
        gcode.append(options.preamble % options.feedrate)
        
    gcode.append( 'G0 Z#1000')

    font = parse(file)          # build stroke lists from font file
    file.close()

    font_line_height = max(font[key].get_ymax() for key in font)
    font_word_space =  max(font[key].get_xmax() for key in font) * (options.wspacep/100.0)
    font_char_space = font_word_space * (options.cspacep /100.0)

    xoffset = 0                 # distance along raw string in font units

    # calc a plot scale so we can show about first 15 chars of string
    # in the preview window
    PlotScale = 15 * font['A'].get_xmax() * options.xscale / 150

    for char in String:
        if char == ' ':
            xoffset += font_word_space
            continue
        try:
            gcode.append("(character '%s')" % sanitize(char))

            first_stroke = True
            for stroke in font[char].stroke_list:
#               gcode.append("(%f,%f to %f,%f)" %(stroke.xstart,stroke.ystart,stroke.xend,stroke.yend ))
                dx = oldx - stroke.xstart
                dy = oldy - stroke.ystart
                dist = sqrt(dx*dx + dy*dy)

                x1 = stroke.xstart + xoffset
                y1 = stroke.ystart
                if options.mirror == 1:
                    x1 = -x1
                if options.flip == 1:
                    y1 = -y1

                # check and see if we need to move to a new discontinuous start point
                if (dist > 0.001) or first_stroke:
                    first_stroke = False
                    #lift engraver, rapid to start of stroke, drop tool
                    gcode.append("G0 Z#1000")
                    gcode.append('o9000 call [0] [%.4f] [%.4f]' %(x1,y1))
                    gcode.append("G1 Z#1001")

                x2 = stroke.xend + xoffset
                y2 = stroke.yend
                if options.mirror == 1:
                    x2 = -x2
                if options.flip == 1:
                    y2 = -y2
                gcode.append('o9000 call [1] [%.4f] [%.4f]' %(x2,y2))
                oldx, oldy = stroke.xend, stroke.yend

            # move over for next character
            char_width = font[char].get_xmax()
            xoffset += font_char_space + char_width

        except KeyError:
           gcode.append("(warning: character '0x%02X' not found in font defn)" % ord(char))

        gcode.append("")       # blank line after every char block

    gcode.append( 'G0 Z#1000')     # final engraver up

    # finish up with icing
    if last:
        gcode.append(options.postamble)
  
    for line in gcode:
            sys.stdout.write(line+'\n')

################################################################################################################

def help_message():
    print '''engrave-lines.py G-Code Engraving Generator for command-line usage
            (C) ArcEye <2012> 
            based upon code from engrave-11.py
            Copyright (C) <2008>  <Lawrence Glaister> <ve7it at shaw dot ca>'''
            
    print '''engrave-lines.py -X -x -i -Y -y -S -s -Z -D -C -W -M -F -P -p -0 -1 -2 -3 ..............
       Options: 
       -h   Display this help message
       -X   Start X value                       Defaults to 0
       -x   X offset between lines              Defaults to 0
       -i   X indent line list                  String of lines to indent in single quotes
       -Y   Start Y value                       Defaults to 0
       -y   Y offset between lines              Defaults to 0
       -S   X Scale                             Defaults to 1
       -s   Y Scale                             Defaults to 1       
       -Z   Safe Z for moves                    Defaults to 2mm
       -D   Z depth for engraving               Defaults to 0.1mm
       -C   Charactor Space %                   Defaults to 25%
       -W   Word Space %                        Defaults to 100%
       -M   options.mirror                              Defaults to 0 (No)
       -F   options.flip                                Defaults to 0 (No)
       -V   options.feedrate                            Defaults to 50 (mm/minute)
       -P   options.preamble g code                     Defaults to "G17 G21 G40 G90 G64 P0.003 F50"
       -p   options.postamble g code                    Defaults to "M2"
       -0   Line0 string follow this
       -1   Line1 string follow this
       -2   Line2 string follow this        
       -3   Line3 string follow this
       -4   Line4 string follow this
       -5   Line5 string follow this
       -6   Line6 string follow this
       -7   Line7 string follow this                                
       -8   Line8 string follow this
       -9   Line9 string follow this
      Example
      engrave-lines.py -X7.5 -x5 -i'123' -Y12.75 -y5.25 -S0.4 -s0.5 -Z2 -D0.1 -0'Line0' -1'Line1' -2'Line2' -3'Line3' > test.ngc
    '''
    sys.exit(0)

#===============================================================================================================

def main():
    import optparse

    example = "Example:  %s --xstart=7.5 --xlineoffset=5  --ystart=12.75 --ylineoffset=5.25 --xscale=0.4 --yscale=0.5 --safez=2 --depth=0.1 --xindentlist='1 2 3' 'Line0' 'Line1' 'Line2' 'Line3' > gcode.ngc" % sys.argv[0]

    parser = optparse.OptionParser(epilog=example)

    parser.add_option("--metric", dest="metric", help="Use metric units.", default=True)
    parser.add_option("--font", dest="font", help="Font file (CXF) to use.")
    parser.add_option("--xstart", dest="xstart", type='float', help="Start X Value.", default=0.0)
    parser.add_option("--xscale", dest="xscale", type='float', help="X scale.", default=1.0)
    parser.add_option("--xlineoffset", dest="xlineoffset", type='float', help="X offset between lines.", default=0.0)
    parser.add_option("--xindentlist", dest="xindentlist", help="X offset between lines.", default='')
    parser.add_option("--ystart", dest="ystart", type='float', help="Start Y Value.", default=0.0)
    parser.add_option("--yscale", dest="yscale", type='float', help="Y scale.", default=1.0)
    parser.add_option("--ylineoffset", dest="ylineoffset", type='float', help="Y offset between lines.", default=0.0)
    parser.add_option("--safez", dest="safe_z", type='float', help="Safe depth.", default=2)
    parser.add_option("-d", "--depth", dest="engrave_z", type='float', help="Engraving depth.", default=0.1)
    parser.add_option("--cspacep", dest="cspacep", type='float', help="Character space percent.", default=25)
    parser.add_option("--wspacep", dest="wspacep", type='float', help="Word space percent.", default=100)
    parser.add_option("--preamble", dest="preamble", help="Gcode preamble.", default=Preamble)
    parser.add_option("--postamble", dest="postamble", help="Gcode postamble.", default=Postamble)
    parser.add_option("--angle", dest="angle", type='float', help="Orientation about the Z axis.", default=0)
    parser.add_option("--mirror", dest="mirror", default=False, help="options.mirror about the Y axis.")
    parser.add_option("--flip", dest="flip", default=False, help="options.mirror about the X axis.")
    parser.add_option("--debug", dest="debug", default=False, help="Enable debug output.")
    parser.add_option("-f", "--feedrate", dest="feedrate", type='float', help="Cutting feedrate in units per minute.", default=50)

    (options, stringlist) = parser.parse_args()

    if not options.font:
        print 'Must specify a font.'

    for index, item in enumerate(stringlist):
        code(item,index, index == (len(stringlist) - 1), options)
   
            
#===============================================================================================
            
if __name__ == "__main__":
    main()

#===============================================================================================END
