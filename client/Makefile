# Copyright (C) 2011 by Jonathan Appavoo, Boston University
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

targets = client

include ../client/config.mk

client_objs = $(objs)
libs = $(DAGAMELIB) -lpthread

# client: $(client_objs) $(DAGAMELIBARCHIVE)
# 	gcc $(CFLAGS) -o $@ $(client_objs) $(libs) ${UILIBS}

ttymodse.o: ttymodes.c
	gcc $(CFLAGS) ttymodes.c -c

tty.o: tty.c tty.h ttymodes.h
	gcc $(CFLAGS) tty.c -c

uistandalone.o: uistandalone.c uistandalone.h
	gcc ${CFLAGS} uistandalone.c -c

client.o: client.c uistandalone.h
	gcc $(CFLAGS) client.c -c 

client: uistandalone.o client.o tty.o ttymodes.o
	gcc $(CFLAGS) uistandalone.o client.o tty.o ttymodes.o -o client $(libs) $(UILIBS)
