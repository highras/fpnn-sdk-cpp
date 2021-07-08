EXES_STRESS = embedAsyncStressClient
EXES_ASYNC_ONEWAY_TEST = embedAsyncOnewayTest
EXES_DUPLEX_CLIENT = embedDuplexClientTest
EXES_PERIOD_TEST = embedPeriodClientTest
EXES_TIMEOUT_TEST = embedTimeoutTest
EXES_STABITLTY_TEST = embedSingleClientConcurrentTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../../src -I../../src/core -I../../src/base -I../../src/proto -I../../src/proto/msgpack
LIBS += -L../../src/core -L../../src/base -L../../src/proto

all: $(EXES_STRESS) $(EXES_ASYNC_ONEWAY_TEST) $(EXES_DUPLEX_CLIENT) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST) $(EXES_STABITLTY_TEST)

clean:
	$(RM) *.o $(EXES_STRESS) $(EXES_ASYNC_ONEWAY_TEST) $(EXES_DUPLEX_CLIENT) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST)  $(EXES_STABITLTY_TEST)


include ../../src/def.mk
