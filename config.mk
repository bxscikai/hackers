UISYSTEM=$(shell uname)

ifeq ($(UISYSTEM),Darwin)
  UIINCDIR = -I/opt/local/include
  UILIBS = -L/opt/local/lib 
else
  UINCDIR = 
  UILIBS = -lSDL
endif

CFLAGS := -g 

MODULE := $(shell basename $CURDIR)

DAGAMELIBHDRS := types.h net.h dummy.h
DAGAMELIBFILE := libdagame.a
DAGAMELIBARCHIVE := ../lib/$(DAGAMELIBFILE)
DAGAMELIB := -L../lib -ldagame


src  = $(wildcard *.c)
objs = $(patsubst %.c,%.o,$(src))

ifeq ($(MODULE),lib)
  DAGAMELIBINCS:=$(DAGAMELIBHDRS)
else
  DAGAMELIBINCS:=$(addprefix ../lib/,$(DAGAMELIBHDRS))
endif


all: $(targets)
.PHONY: all

$(objs) : $(src) $(DAGAMELIBINCS)

clean:
	rm $(objs) $(targets)

