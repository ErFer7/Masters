# EPOS Application Makefile

include ../../makedefs

SRCS := $(shell find . -name "*.cc")
OBJS := $(SRCS:.cc=.o)

all: install

$(APPLICATION): $(OBJS) $(LIB)/*
	$(ALD) $(ALDFLAGS) -o $@ $(OBJS)

%.o: %.cc
	$(ACC) $(ACCFLAGS) -c -o $@ $<

install: $(APPLICATION)
	$(INSTALL) $(APPLICATION) $(IMG)

clean:
	$(CLEAN) $(OBJS) $(APPLICATION)
