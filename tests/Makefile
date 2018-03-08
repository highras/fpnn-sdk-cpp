EXES_CLIENT = asyncStressClient
EXES_CLIENT2 = singleClientConcurrentTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../src/core -I../src/base -I../src/proto -I../src/proto/msgpack
LIBS += -L../src/core -L../src/base -L../src/proto

OBJS_CLIENT = asyncStressClient.o
OBJS_CLIENT2 = singleClientConcurrentTest.o

all: $(EXES_CLIENT) $(EXES_CLIENT2)

clean:
	$(RM) *.o $(EXES_CLIENT) $(EXES_CLIENT2)


include ../src/def.mk
