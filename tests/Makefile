EXES_STRESS = asyncStressClient
EXES_ASYNC_ONEWAY_TEST = clientAsyncOnewayTest
EXES_DUPLEX_CLIENT = duplexClientTest
EXES_PERIOD_TEST = periodClientTest
EXES_TIMEOUT_TEST = timeoutTest
EXES_STABITLTY_TEST = singleClientConcurrentTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../src/core -I../src/base -I../src/proto -I../src/proto/msgpack
LIBS += -L../src/core -L../src/base -L../src/proto

all: $(EXES_STRESS) $(EXES_ASYNC_ONEWAY_TEST) $(EXES_DUPLEX_CLIENT) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST) $(EXES_STABITLTY_TEST)

clean:
	$(RM) *.o $(EXES_STRESS) $(EXES_ASYNC_ONEWAY_TEST) $(EXES_DUPLEX_CLIENT) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST)  $(EXES_STABITLTY_TEST)
	-$(RM) -rf *.dSYM
	make clean -C embedModeTests

include ../src/def.mk
