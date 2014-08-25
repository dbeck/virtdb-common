BUILD_ROOT           := $(shell pwd)
PROTO_PATH           := $(BUILD_ROOT)/
PROTO_FILE_NAMES     := common.proto meta_data.proto db_config.proto data.proto svc_config.proto diag.proto
PROTO_FILES          := $(patsubst %.proto,$(PROTO_PATH)/%.proto,$(PROTO_FILE_NAMES)) 
PROTO_SOURCES        := $(patsubst %.proto,%.pb.cc,$(PROTO_FILES))
PROTO_OBJECTS        := $(patsubst %.pb.cc,%.pb.o,$(PROTO_SOURCES))
PROTO_DESCS          := $(patsubst %.pb.cc,%.pb.desc,$(PROTO_SOURCES))
PROTOBUF_LDFLAGS     := $(shell pkg-config --libs protobuf)
PROTOBUF_CFLAGS      := $(shell pkg-config --cflags protobuf)

ZMQ_LDFLAGS          := $(shell pkg-config --libs libzmq)
ZMQ_CFLAGS           := $(shell pkg-config --cflags libzmq)

# FIXME integrate libtool better ...
GTEST_PATH           := $(BUILD_ROOT)/gtest
GTEST_CONFIG_STATUS  := $(GTEST_PATH)/config.status
GTEST_LIBDIR         := $(GTEST_PATH)/lib/.libs/
GTEST_INCLUDE        := $(GTEST_PATH)/include
GTEST_LIBS           := $(GTEST_LIBDIR)/libgtest.a
GTEST_LDFLAGS        := $(GTEST_LIBS) -L$(GTEST_LIBDIR)
GTEST_CFLAGS         := -I$(GTEST_INCLUDE)

# FIXME on Windows
FIX_CXX_11_BUG =
LINUX_LDFLAGS =  
ifeq ($(shell uname), Linux)
FIX_CXX_11_BUG  = -Wl,--no-as-needed
LINUX_LDFLAGS   = -pthread
endif

CXXFLAGS += -std=c++11 -fPIC $(FIX_CXX_11_BUG) $(LINUX_LDFLAGS) $(PROTOBUF_CFLAGS) $(ZMQ_CFLAGS) $(GTEST_CFLAGS) -I$(BUILD_ROOT)/.
LDFLAGS += $(FIX_CXX_11_BUG) $(LINUX_LDFLAGS) $(PROTOBUF_LDFLAGS) $(ZMQ_LDFLAGS) $(GTEST_LDFLAGS)

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

PROTO_LIB := libproto.a

all: proto-static-lib gtest-test $(PROTO_DESCS) 

gtest-test: gtest-pkg-build-all test/gtest_main.o test/netinfo.o $(UTIL_OBJECTS) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(TEST_OBJECTS) $(PROTO_LIB)
	g++ -o test/gtest_main test/gtest_main.o $(UTIL_OBJECTS) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(TEST_OBJECTS) $(PROTO_LIB) $(LDFLAGS) 
	g++ -o test/netinfo test/netinfo.o $(UTIL_OBJECTS) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(TEST_OBJECTS) $(PROTO_LIB) $(LDFLAGS) 

proto-static-lib: $(PROTO_OBJECTS) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(UTIL_OBJECTS)
	ar rcs $(PROTO_LIB) $(LOGGER_OBJECTS) $(CONNECTOR_OBJECTS) $(PROTO_OBJECTS) $(UTIL_OBJECTS)

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
	@echo "cleaning finished in gtest package"

%.pb.o: %.pb.cc %.pb.h

%.pb.cc %.pb.h: %.proto
	protoc -I $(PROTO_PATH)/ --cpp_out=$(PROTO_PATH)/ $<

%.pb.desc: %.proto
	protoc -I $(PROTO_PATH)/ --descriptor_set_out=$@ --include_imports $<

$(PROTO_PATH)/data.pb.cc: $(PROTO_PATH)/common.proto $(PROTO_PATH)/meta_data.proto

$(PROTO_PATH)/meta_data.pb.cc: $(PROTO_PATH)/common.proto

$(PROTO_PATH)/db_config.pb.cc: $(PROTO_PATH)/common.proto $(PROTO_PATH)/meta_data.proto 

$(PROTO_PATH)/svc_config.pb.cc: $(PROTO_PATH)/meta_data.pb.h

clean: gtest-pkg-clean
	rm -f $(PROTO_OBEJCTS) $(LOGGER_OBJECTS) $(UTIL_OBJECTS) *.a *.o *.pb.cc *.pb.h *.pb.desc

