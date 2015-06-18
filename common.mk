BUILD_ROOT           := $(shell pwd)

PROTOBUF_LDFLAGS     := $(shell pkg-config --libs protobuf) -L$(HOME)/protobuf-install/lib
PROTOBUF_CFLAGS      := $(shell pkg-config --cflags protobuf) -I$(HOME)/protobuf-install/include
ZMQ_LDFLAGS          := $(shell pkg-config --libs libzmq)  -L$(HOME)/libzmq-install/lib
ZMQ_CFLAGS           := $(shell pkg-config --cflags libzmq) -I$(HOME)/libzmq-install/include
SODIUM_LDFLAGS       := $(shell pkg-config --libs libsodium) -lsodium -L$(HOME)/libsodium-install/lib 
SODIUM_CFLAGS        := $(shell pkg-config --cflags libsodium) -I$(HOME)/libsodium-install/include

PROTOBUF_LDFLAGS  := $(shell pkg-config --libs protobuf)

# FIXME on Windows
FIX_CXX_11_BUG =
LINUX_LDFLAGS =
ifeq ($(shell uname), Linux)
FIX_CXX_11_BUG  = -Wl,--no-as-needed
LINUX_LDFLAGS   = -pthread
SODIUM_LDFLAGS  += -lrt
endif

MAC_CFLAGS :=
ifeq ($(shell uname), Darwin)
MAC_CFLAGS := -DCOMMON_MAC_BUILD
endif

BASIC_COMMON_INCLUDES := -I$(BUILD_ROOT)/. -I$(BUILD_ROOT)/deps_/proto -I$(BUILD_ROOT)/cppzmq -I$(BUILD_ROOT)/lz4/lib -I$(BUILD_ROOT)/deps_/fsm/src

ifeq ($(RELEASE), 1)
$(info DOING RELEASE BUILD)
CXXFLAGS += -Wall -O3 -DNDEBUG -DRELEASE -std=c++11 -fPIC $(FIX_CXX_11_BUG) $(LINUX_LDFLAGS) $(MAC_CFLAGS) $(PROTOBUF_CFLAGS) $(ZMQ_CFLAGS) $(SODIUM_CFLAGS) $(BASIC_COMMON_INCLUDES)
LDFLAGS += $(FIX_CXX_11_BUG) $(LINUX_LDFLAGS) $(PROTOBUF_LDFLAGS) $(ZMQ_LDFLAGS) $(SODIUM_LDFLAGS)
CFLAGS += -Wall -O3 -DNDEBUG -DRELEASE -fPIC
else
$(info DOING DEBUG BUILD)
CXXFLAGS += -Wall -g3 -std=c++11 -fPIC $(FIX_CXX_11_BUG) $(LINUX_LDFLAGS) $(MAC_CFLAGS) $(PROTOBUF_CFLAGS) $(ZMQ_CFLAGS) $(SODIUM_CFLAGS) $(BASIC_COMMON_INCLUDES)
LDFLAGS += -g3 $(FIX_CXX_11_BUG) $(LINUX_LDFLAGS) $(PROTOBUF_LDFLAGS) $(ZMQ_LDFLAGS) $(SODIUM_LDFLAGS)
CFLAGS += -Wall -g3 -fPIC
endif

$(info $$CFLAGS is [${CFLAGS}])
$(info $$CXXFLAGS is [${CXXFLAGS}])
$(info $$LDFLAGS is [${LDFLAGS}])

UTIL_SRCS            := $(wildcard util/*.cc)
LOGGER_SRCS          := $(wildcard logger/*.cc)
CONNECTOR_SRCS       := $(wildcard connector/*.cc)
ENGINE_SRCS          := $(wildcard engine/*.cc)
DATASRC_SRCS         := $(wildcard datasrc/*.cc)
FAULT_SRCS           := $(wildcard fault/*.cc)
LZ4_SRCS             := lz4/lib/lz4.c lz4/lib/lz4hc.c lz4/lib/lz4frame.c lz4/lib/xxhash.c
MURMUR3_SRCS         := murmur3/murmur3.c
TEST_SRCS_WILDCARD   := $(wildcard test/*_test.cc) test/gtest_main.cc
TEST_EXCLUDES        := test/gtest_main.cc 
TEST_SRCS            := $(filter-out $(TEST_EXCLUDES),$(TEST_SRCS_WILDCARD))

UTIL_OBJECTS       := $(patsubst %.cc,%.o,$(UTIL_SRCS))
LOGGER_OBJECTS     := $(patsubst %.cc,%.o,$(LOGGER_SRCS))
CONNECTOR_OBJECTS  := $(patsubst %.cc,%.o,$(CONNECTOR_SRCS))
ENGINE_OBJECTS     := $(patsubst %.cc,%.o,$(ENGINE_SRCS))
DATASRC_OBJECTS    := $(patsubst %.cc,%.o,$(DATASRC_SRCS))
FAULT_OBJECTS      := $(patsubst %.cc,%.o,$(FAULT_SRCS))
LZ4_OBJECTS        := $(patsubst %.c,%.o,$(LZ4_SRCS))
MURMUR3_OBJECTS    := $(patsubst %.c,%.o,$(MURMUR3_SRCS))
TEST_OBJECTS       := $(patsubst %.cc,%.o,$(TEST_SRCS))

PROTO_LIB  := deps_/proto/libproto.a
FSM_LIB    := deps_/fsm/libfsm.a
COMMON_LIB := libcommon.a

all: $(COMMON_LIB)

$(COMMON_LIB): $(PROTO_LIB) $(FSM_LIB) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(ENGINE_OBJECTS) $(DATASRC_OBJECTS) $(FAULT_OBJECTS) $(LZ4_OBJECTS) $(MURMUR3_OBJECTS) $(UTIL_OBJECTS)
	ar rcsv $(COMMON_LIB) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(ENGINE_OBJECTS) $(DATASRC_OBJECTS) $(FAULT_OBJECTS) $(LZ4_OBJECTS) $(MURMUR3_OBJECTS) $(PROTO_LIB) $(UTIL_OBJECTS)

$(PROTO_LIB):
	@echo "building proto project in ./desp_/proto"
	cd ./deps_/proto; make -f proto.mk all
	@echo "proto project built"

$(FSM_LIB):
	@echo "building fsm project in ./deps_/fsm"
	cd ./deps_/fsm; make -f fsm.mk all
	@echo "fsm project built"

clean: 
	rm -f $(PROTO_LIB) $(LOGGER_OBJECTS) $(UTIL_OBJECTS) $(CONNECTOR_OBJECTS) $(ENGINE_OBJECTS) $(DATASRC_OBJECTS) $(FAULT_OBJECTS) $(LZ4_OBJECTS) $(MURMUR3_OBJECTS) $(TEST_OBJECTS)
	rm -f *.a *.o *.pb.cc *.pb.h *.pb.desc 
	cd ./deps_/proto; make -f proto.mk clean
	cd ./deps_/fsm; make -f fsm.mk clean
	@echo "checking for suspicious files"
	@find . -type f -name "*.so"
	@find . -type f -name "*.a"
	@find . -type f -name "*.o"
	@find . -type f -name "*.desc"
	@find . -type f -name "*.pb.desc"
	@find . -type f -name "*.pb.h"
	@find . -type f -name "*.pb.cc"
	@find . -type f -name "*.pb.h"
