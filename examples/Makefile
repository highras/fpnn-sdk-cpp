###############################################
#                                             #
#   !!! Please run sh release.sh first. !!!   #
#                                             #
###############################################

EXES_EVENT_EXAMPLE = connectionEvents
EXES_ENCRYPT_EXAMPLE = encryptedCommunication

EXES_TWO_WAY_DUPLEX = twoWayDuplex
EXES_ONE_WAY_DUPLEX = oneWayDuplex
EXES_ASYNC_DUPLEX = asyncAnswerInDuplex
EXES_AHEAD_DUPLEX = aheadAnswerInDuplex

FPNN_INCLUDE_DIR = ../release/include/fpnn
FPNN_LIB_DIR = ../release/lib

CFLAGS +=
CXXFLAGS += -std=c++11
CPPFLAGS += -Wall -Werror
CPPFLAGS += -I$(FPNN_INCLUDE_DIR) -I$(FPNN_INCLUDE_DIR)/core -I$(FPNN_INCLUDE_DIR)/base -I$(FPNN_INCLUDE_DIR)/proto -I$(FPNN_INCLUDE_DIR)/proto/msgpack
LIBS += -L$(FPNN_LIB_DIR) -lfpnn -lstdc++ -lpthread -lz


all: $(EXES_EVENT_EXAMPLE) $(EXES_ENCRYPT_EXAMPLE) $(EXES_TWO_WAY_DUPLEX) $(EXES_ONE_WAY_DUPLEX) $(EXES_ASYNC_DUPLEX) $(EXES_AHEAD_DUPLEX)

clean:
	$(RM) *.o $(EXES_EVENT_EXAMPLE) $(EXES_ENCRYPT_EXAMPLE) $(EXES_TWO_WAY_DUPLEX) $(EXES_ONE_WAY_DUPLEX) $(EXES_ASYNC_DUPLEX) $(EXES_AHEAD_DUPLEX)
	-$(RM) -rf *.dSYM

# exmples list

$(EXES_EVENT_EXAMPLE):
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $@.cpp $(LIBS)

$(EXES_ENCRYPT_EXAMPLE):
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $@.cpp $(LIBS)

$(EXES_TWO_WAY_DUPLEX):
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $@.cpp $(LIBS)

$(EXES_ONE_WAY_DUPLEX):
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $@.cpp $(LIBS)

$(EXES_ASYNC_DUPLEX):
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $@.cpp $(LIBS)

$(EXES_AHEAD_DUPLEX):
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $@.cpp $(LIBS)
