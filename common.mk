BUILD_ROOT           := $(shell pwd)

PROTOBUF_LDFLAGS     := $(shell pkg-config --libs protobuf)
PROTOBUF_CFLAGS      := $(shell pkg-config --cflags protobuf)
ZMQ_LDFLAGS          := $(shell pkg-config --libs libzmq)
ZMQ_CFLAGS           := $(shell pkg-config --cflags libzmq)

# FIXME integrate libtool better ...
GTEST_PATH           := $(BUILD_ROOT)/gtest
GTEST_CONFIG_STATUS  := $(GTEST_PATH)/config.status
GTEST_LIBDIR         := $(GTEST_PATH)/lib/
GTEST_INCLUDE        := $(GTEST_PATH)/include
GTEST_LIBS           := -lgtest 
GTEST_LDFLAGS        := -L$(GTEST_LIBDIR) -L$(GTEST_LIBDIR)/.libs $(GTEST_LIBS)
GTEST_CFLAGS         := -I$(GTEST_INCLUDE)

# FIXME on Windows
FIX_CXX_11_BUG =
LINUX_LDFLAGS =  
ifeq ($(shell uname), Linux)
FIX_CXX_11_BUG  = -Wl,--no-as-needed
LINUX_LDFLAGS   = -pthread
endif

CXXFLAGS += -Wall -g3 -std=c++11 -fPIC $(FIX_CXX_11_BUG) $(LINUX_LDFLAGS) $(PROTOBUF_CFLAGS) $(ZMQ_CFLAGS) $(GTEST_CFLAGS) -I$(BUILD_ROOT)/. -I$(BUILD_ROOT)/proto -I$(BUILD_ROOT)/cppzmq
LDFLAGS += -g3 $(FIX_CXX_11_BUG) $(LINUX_LDFLAGS) $(PROTOBUF_LDFLAGS) $(ZMQ_LDFLAGS) $(GTEST_LDFLAGS)
CFLAGS += -Wall -g3

$(info $$CFLAGS is [${CFLAGS}])
$(info $$CXXFLAGS is [${CXXFLAGS}])
$(info $$LDFLAGS is [${LDFLAGS}])

UTIL_SRCS            := $(wildcard util/*.cc)
LOGGER_SRCS          := $(wildcard logger/*.cc)
CONNECTOR_SRCS       := $(wildcard connector/*.cc)
TEST_SRCS_WILDCARD   := $(wildcard test/*.cc)
TEST_EXCLUDES        := test/netinfo.cc test/gtest_main.cc
TEST_SRCS            := $(filter-out $(TEST_EXCLUDES),$(TEST_SRCS_WILDCARD))

UTIL_OBJECTS       := $(patsubst %.cc,%.o,$(UTIL_SRCS))
LOGGER_OBJECTS     := $(patsubst %.cc,%.o,$(LOGGER_SRCS))
CONNECTOR_OBJECTS  := $(patsubst %.cc,%.o,$(CONNECTOR_SRCS))
TEST_OBJECTS       := $(patsubst %.cc,%.o,$(TEST_SRCS))

PROTO_LIB := proto/libproto.a
COMMON_LIB := libcommon.a

all: $(COMMON_LIB) gtest-test 

gtest-test: gtest-pkg-build-all test/gtest_main.o test/netinfo.o $(UTIL_OBJECTS) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(TEST_OBJECTS) $(PROTO_LIB)
	g++ -o test/gtest_main test/gtest_main.o $(UTIL_OBJECTS) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(TEST_OBJECTS) $(PROTO_LIB) $(LDFLAGS) 
	g++ -o test/netinfo test/netinfo.o $(UTIL_OBJECTS) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(TEST_OBJECTS) $(PROTO_LIB) $(LDFLAGS) 

$(COMMON_LIB): $(PROTO_LIB) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(UTIL_OBJECTS)
	ar rcsv $(COMMON_LIB) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(PROTO_LIB) $(UTIL_OBJECTS)

$(PROTO_LIB):
	@echo "building proto project in ./proto" 
	cd ./proto; make -f proto.mk all
	@echo "proto project built"

gtest-pkg-build-all: gtest-pkg-configure gtest-pkg-lib

gtest-pkg-configure: $(GTEST_CONFIG_STATUS)

$(GTEST_CONFIG_STATUS):
	@echo "doing configure in gtest in " $(GTEST_PATH) 
	cd $(GTEST_PATH) ; ./configure
	@echo "configure done in gtest"

# NOTE: assumption: the libdir will be created during build
gtest-pkg-lib: $(GTEST_LIBDIR)

$(GTEST_LIBDIR):
	@echo "building the gtest package"
	@make -C $(GTEST_PATH)
	@echo "building finished in gtest package"

gtest-pkg-clean:
	@echo "cleaning the gtest package"
	@make -C $(GTEST_PATH) clean
	@rm -Rf $(GTEST_LIBDIR)
	@echo "cleaning finished in gtest package"

clean: gtest-pkg-clean
	rm -f $(PROTO_LIB) $(LOGGER_OBJECTS) $(UTIL_OBJECTS) $(CONNECTOR_OBJECTS) $(TEST_OBJECTS)
	rm -f *.a *.o *.pb.cc *.pb.h *.pb.desc test/*.o test/gtest_main test/netinfo
	cd ./proto; make -f proto.mk clean
	@echo "checking for suspicious files"
	@find . -type f -name "*.so"
	@find . -type f -name "*.a"
	@find . -type f -name "*.o"
	@find . -type f -name "*.desc"
	@find . -type f -name "*.pb.desc"
	@find . -type f -name "*.pb.h"
	@find . -type f -name "*.pb.cc"
	@find . -type f -name "*.pb.h"

