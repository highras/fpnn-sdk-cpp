EXES_CMD = cmd
EXES_CLIENT = fss

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../src/core -I../src/base -I../src/proto -I../src/proto/msgpack
LIBS += -L../src/core -L../src/base -L../src/proto

OBJS_CMD = cmd.o
OBJS_CLIENT = fss.o linenoise.o

all: $(EXES_CMD) $(EXES_CLIENT)

clean:
	$(RM) *.o $(EXES_CMD) $(EXES_CLIENT)
	-$(RM) -rf *.dSYM


include ../src/def.mk
