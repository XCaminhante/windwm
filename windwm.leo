<?xml version="1.0" encoding="utf-8"?>
<!-- Created by Leo: https://leo-editor.github.io/leo-editor/leo_toc.html -->
<leo_file xmlns:leo="http://leo-editor.github.io/leo-editor/namespaces/leo-python-editor/1.1" >
<leo_header file_format="2"/>
<globals/>
<preferences/>
<find_panel_settings/>
<vnodes>
<v t="caminhante.20230226160006.1"><vh>@settings</vh>
<v t="caminhante.20200618180759.1"><vh>@command terminal-here @key=Alt-T</vh></v>
<v t="caminhante.20230226160006.2"><vh>@data abbreviations</vh></v>
<v t="caminhante.20230226160006.3"><vh>@outline-data tree-abbreviations</vh></v>
<v t="caminhante.20230226160006.4"><vh>nodeActions</vh>
<v t="caminhante.20230421001433.1"><vh>@bin *</vh></v>
<v t="caminhante.20200604232050.1"><vh>@cmd *</vh></v>
<v t="caminhante.20211012181130.1"><vh>@cp *</vh></v>
<v t="caminhante.20211113172701.1"><vh>@files Makefile</vh></v>
<v t="caminhante.20210501142945.2"><vh>@mkdir *</vh></v>
<v t="caminhante.20211012180812.1"><vh>@mv *</vh></v>
<v t="caminhante.20210905232208.1"><vh>@rm *</vh></v>
<v t="caminhante.20210501142945.4"><vh>@rmdir /*/</vh></v>
<v t="caminhante.20200618203115.1"><vh>@run|@run .* [X]</vh></v>
<v t="caminhante.20230410165201.1"><vh>\@edit * [X]</vh></v>
<v t="caminhante.20230623143806.1"><vh>\@file * [X]</vh></v>
<v t="caminhante.20230623181912.1"><vh>/*/</vh></v>
<v t="caminhante.20231002175916.1"><vh>\@path * [X]</vh></v>
<v t="caminhante.20230623144919.1"><vh>*</vh></v>
</v>
</v>
<v t="caminhante.20240209171937.2"><vh>Wind Window Manager</vh>
<v t="caminhante.20240209172024.1"><vh>@path .</vh>
<v t="caminhante.20240209172034.25"><vh>.fslckout</vh></v>
<v t="caminhante.20240209172034.24"><vh>@clean Makefile</vh></v>
<v t="caminhante.20240209172034.12"><vh>@edit list.h</vh></v>
<v t="caminhante.20240209172034.23"><vh>@edit NEWS</vh></v>
<v t="caminhante.20240209172034.22"><vh>@edit README</vh></v>
<v t="caminhante.20240209172034.9"><vh>@file test.c</vh></v>
<v t="caminhante.20240209172034.8"><vh>@edit wind.1</vh></v>
<v t="caminhante.20240208143459.33"><vh>@file button.c</vh></v>
<v t="caminhante.20240208143459.31"><vh>@file client.c</vh></v>
<v t="caminhante.20240208144745.1"><vh>@file defines.h</vh></v>
<v t="caminhante.20240209172034.16"><vh>@file dragger.c</vh></v>
<v t="caminhante.20240208143459.20"><vh>@file ewmh.c</vh></v>
<v t="caminhante.20240208143459.18"><vh>@file frame.c</vh></v>
<v t="caminhante.20240208143459.15"><vh>@file lib.c</vh></v>
<v t="caminhante.20240209172034.11"><vh>@file mwm.c</vh></v>
<v t="caminhante.20240208143459.7"><vh>@file root.c</vh></v>
<v t="caminhante.20240209172034.7"><vh>@file wind.h</vh></v>
<v t="caminhante.20240208143459.12"><vh>@file windwm.c</vh></v>
<v t="caminhante.20240208143459.3"><vh>@file x11font.c</vh></v>
<v t="caminhante.20240208143459.1"><vh>@file xftfont.c</vh></v>
<v t="caminhante.20240209172034.18"><vh>deleven.xbm</vh></v>
<v t="caminhante.20240209172034.17"><vh>delodd.xbm</vh></v>
<v t="caminhante.20240209172034.5"><vh>windwm.fossil</vh></v>
<v t="caminhante.20240209172034.4"><vh>windwm.leo</vh></v>
<v t="caminhante.20240209172034.3"><vh>windwm.pdf</vh></v>
</v>
</v>
</vnodes>
<tnodes>
<t tx="caminhante.20200604232050.1">import subprocess

def execute (cmd):
    return subprocess.run(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

if c.isChanged(): c.save()

path = c.getNodePath(c.p)
g.os.chdir(path)

command = ''.join(c.p.h.split('@cmd ')[1:])
command = "cat &lt;&lt;'EOFEOFEOFEOFEOFEOF' | bash\n"+command+'\nEOFEOFEOFEOFEOFEOF'
g.es(c.p.h)
c.p.b += execute(command)

c.save()
</t>
<t tx="caminhante.20200618180759.1">@language python
import subprocess

def execute (cmd):
    return subprocess.Popen(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

if c.isChanged(): c.save()
path = c.getNodePath(c.p)

execute('x-terminal-emulator --working-directory="'+path+'"')
</t>
<t tx="caminhante.20200618203115.1">@language python
import subprocess

# def getpath (p):
    # dict = c.scanAllDirectives(p)
    # d = dict.get("path")
    # if p.isAnyAtFileNode():
        # filename = p.anyAtFileNodeName()
        # filename = g.os_path_join(d,filename)
        # if filename:
            # d = g.os_path_dirname(filename)
    # if d is None:
        # return ""
    # else:
        # return g.os_path_normpath(d)

def execute (cmd):
    return subprocess.run(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

path = c.getNodePath(c.p)
# g.os.chdir(path)

w = c.frame.body.wrapper
s1,s2 = w.getSelectionRange()

if s1 == s2:
    code = c.p.b
else:
    linebr = c.p.b.find('\n',s2)
    if linebr == -1: linebr = len(c.p.b)
    code = c.p.b[s1:s2]

g.es(c.p.h)

command = "cat &lt;&lt;'EOFEOFEOFEOFEOFEOF' | bash\ncd '"+path+"';\n"+code+'\nEOFEOFEOFEOFEOFEOF'

if c.isChanged(): c.save()

cpc = c.p.copy()
novoNo = cpc.insertAsLastChild()
novoNo.h = c.p.h + ' ['+code+']'
novoNo.b = execute(command)

c.save()
w.setSelectionRange(s1,s2)
c.redraw(c.p)
</t>
<t tx="caminhante.20210501142945.2">import subprocess

def execute (cmd):
    return subprocess.run(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

path = c.getNodePath(c.p)
g.os.chdir(path)

path = ''.join(c.p.h.split('@mkdir ')[1:])
cmd = "mkdir -p -- '" + path + "'"
g.es(c.p.h)
execute(cmd)
c.p.h = '/'+path+'/'
c.p.b = '@path ' + path
</t>
<t tx="caminhante.20210501142945.4">import subprocess
import shutil

def execute (cmd):
    return subprocess.run(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

ps = c.p.parents()
for par in ps:
    break
path = c.getNodePath(par)
g.os.chdir(path)

path = ''.join(c.p.h.split('@rmdir /')[1:]).split('/')[0]
shutil.rmtree(path)
c.p.b = 'deleted'
c.p.deleteAllChildren()
c.redraw(c.p)
</t>
<t tx="caminhante.20210905232208.1">import subprocess

def execute (cmd):
    return subprocess.run(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

path = c.getNodePath(c.p)
g.os.chdir(path)

filename = ''.join(c.p.h.split('@rm ')[1:])
cmd = 'rm -vf -- "' + filename + '"'
g.es(c.p.h)
g.es(execute(cmd))
c.p.deleteAllChildren()
c.redraw(c.p)
</t>
<t tx="caminhante.20211012180812.1">import subprocess

def execute (cmd):
    return subprocess.run(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

path = c.getNodePath(c.p)
g.os.chdir(path)

if 'file://' in c.p.h:
  path = ''.join(c.p.h.split('@mv file://')[1:])
else:
  path = ''.join(c.p.h.split('@mv ')[1:])

cmd = "mv -v -- '" + path + "' ./"
g.es(c.p.h)
g.es(execute(cmd))
</t>
<t tx="caminhante.20211012181130.1">import subprocess

def execute (cmd):
    return subprocess.run(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

path = c.getNodePath(c.p)
g.os.chdir(path)

if 'file://' in c.p.h:
  path = ''.join(c.p.h.split('@cp file://')[1:])
else:
  path = ''.join(c.p.h.split('@cp ')[1:])

cmd = "cp -v -- '" + path + "' ./"
g.es(c.p.h)
g.es(execute(cmd))
</t>
<t tx="caminhante.20211113172701.1">@language python
import subprocess
import re

def execute (cmd):
    return subprocess.run(cmd,shell=True,universal_newlines=True,stderr=subprocess.STDOUT,stdout=subprocess.PIPE).stdout

if c.isChanged(): c.save()

path = c.getNodePath(c.p)
g.os.chdir(path)

cpc = c.p.copy()
novoNo = cpc.insertAfter()
novoNo.h = 'make return'
novoNo.b = execute('make')
c.redraw(c.p)
</t>
<t tx="caminhante.20230226160006.1">@tabwidth -2
</t>
<t tx="caminhante.20230226160006.2"></t>
<t tx="caminhante.20230226160006.3"></t>
<t tx="caminhante.20230226160006.4">@language python
</t>
<t tx="caminhante.20230410165201.1">path = ''.join(c.p.h.split('@edit ')[1:])
c.p.h = path
c.p.b = ''
</t>
<t tx="caminhante.20230421001433.1">import subprocess

def execute (cmd):
  return subprocess.run(
    cmd,shell=True,
    universal_newlines=True,
    stderr=subprocess.STDOUT,
    stdout=subprocess.PIPE).stdout

path = c.getNodePath(c.p)
g.os.chdir(path)

command = ''.join(c.p.h.split('@bin ')[1:])
command = "xxd -- '"+command+"'"

g.es(c.p.h)
c.p.b = execute(command)

c.save()
</t>
<t tx="caminhante.20230623143806.1">path = ''.join(c.p.h.split('@file ')[1:])
c.p.h = path
c.p.b = ''
c.p.deleteAllChildren()
c.redraw(c.p)
</t>
<t tx="caminhante.20230623144919.1">path = c.getNodePath(c.p)
g.os.chdir(path)

filename = c.p.h

at = c.atFileCommands
c.p.deleteAllChildren()
c.p.h = "@file " + filename
c.recreateGnxDict()
at.readFileAtPosition(c.p)

c.p.setDirty()
c.redraw(c.p)
c.p.contract()
</t>
<t tx="caminhante.20230623181912.1">c.p.deleteAllChildren()
c.p.setDirty()
c.redraw(c.p)
</t>
<t tx="caminhante.20231002175916.1">c.p.deleteAllChildren()
c.p.setDirty()
c.redraw(c.p)
</t>
<t tx="caminhante.20240209171937.2"></t>
<t tx="caminhante.20240209172024.1"></t>
<t tx="caminhante.20240209172034.17"></t>
<t tx="caminhante.20240209172034.18"></t>
<t tx="caminhante.20240209172034.24">@tabwidth 2
CFLAGS := -Wall -Wextra -Werror -Wfatal-errors \
-Wno-unused-function -Wno-ignored-qualifiers \
-Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing \
--std=gnu2x -O0 -g3

PROGFLAGS := $(shell pkg-config x11 --cflags --libs) \
	$(shell pkg-config xft --cflags --libs) \
	$(shell pkg-config freetype2 --cflags --libs)

all: windwm
	DISPLAY=:1 ./windwm -c xterm :1

.PHONY: test
test:
	gcc windwm.c $(CFLAGS) $(PROGFLAGS) -c -o /dev/null

windwm: windwm.c Makefile *.c *.h
	gcc windwm.c $(CFLAGS) $(PROGFLAGS) -o "$@"
</t>
<t tx="caminhante.20240209172034.25"></t>
<t tx="caminhante.20240209172034.3"></t>
<t tx="caminhante.20240209172034.4"></t>
<t tx="caminhante.20240209172034.5"></t>
</tnodes>
</leo_file>
