TOPDIR ?= ../..
#CROSS_COMPILE ?=

include ../../config.mk

TESTUNIT := test_input_manager
TESTUNIT_OBJS := main.o                                                        \
          $(TOPDIR)/utils/assert.o                                             \
          $(TOPDIR)/input/input_manager.o

.PHONY : all clean

all: $(TESTUNIT)

$(TESTUNIT): $(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(TESTUNIT_OBJS)
