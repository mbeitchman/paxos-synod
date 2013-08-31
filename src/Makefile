# Since the programs in this directories are examples for the user, this
# make file should be as ordinary as possible.  It should not rely heavily
# on included make files or configuration parameters.  It should not use
# libtool.  Also, we don't try to build or rebuild the libraries on which
# these programs depend.


ifeq ($(SRCDIR),)
SRCDIR = $(CURDIR)/xmlrpc-c-1.16.40
BLDDIR = $(SRCDIR)
endif
SUBDIR=examples/cpp

include $(BLDDIR)/config.mk

default: all

CXXFLAGS = $(CFLAGS_PERSONAL) $(CADD)
LDFLAGS += $(LADD)

# If this were a real application, working from an installed copy of
# Xmlrpc-c, XMLRPC_C_CONFIG would just be 'xmlrpc-c-config'.  It would be
# found in the user's PATH.
XMLRPC_C_CONFIG = $(BLDDIR)/xmlrpc-c-config.test

SERVERPROGS_ABYSS = \
  paxos_replica \

CLIENTPROGS = \
  paxos_client \


# Build up PROGS:
PROGS = 

ifeq ($(ENABLE_ABYSS_SERVER),yes)
  PROGS += $(SERVERPROGS_ABYSS)
endif

ifeq ($(MUST_BUILD_CLIENT),yes)
  PROGS += $(CLIENTPROGS)
endif

INCLUDES = -I. $(shell $(XMLRPC_C_CONFIG) c++2 client abyss-server --cflags)

LIBS_SERVER_ABYSS = \
  $(shell $(XMLRPC_C_CONFIG) c++2 abyss-server --libs)

LIBS_CLIENT = \
  $(shell $(XMLRPC_C_CONFIG) c++2 client --libs)

LIBS_BASE = \
  $(shell $(XMLRPC_C_CONFIG) c++2 --libs)

all: $(PROGS)

$(SERVERPROGS_ABYSS):%:%.o replicacommon.o
	$(CXXLD) -o $@ $^ $(LIBS_SERVER_ABYSS) $(LDFLAGS) $(LIBS_CLIENT) -lpthread

$(CLIENTPROGS):%:%.o
	$(CXXLD) -o $@ $^ $(LIBS_CLIENT) $(LDFLAGS)

OBJECTS = $(patsubst %,%.o,$(patsubst %.cgi,%_cgi,$(PROGS))) replicacommon.o

$(OBJECTS):%.o:%.cpp
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $<

.PHONY: clean
clean:
	rm -f $(PROGS) *.o

.PHONY: distclean
distclean: clean

.PHONY: dep depend
dep depend:
# We don't do dependencies in this directory, because it's supposed to be
# an example of what a program outside this package would do, so we can't
# go weaving it into the rest of the package.  Ergo, a developer must
# carefully clean and remake examples as he updates other parts of the tree.
