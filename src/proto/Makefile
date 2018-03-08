OBJS_C = 

OBJS_CXX = FPMessage.o FPReader.o FPWriter.o JSONConvert.o

# Static 
LIBFPNN_A = libfpproto.a

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I../base -I./msgpack -I./rapidjson 

all: $(LIBFPNN_A)

clean:
	find . -name \*.lo -o -name \*.o | xargs rm -f
	find . -name \*.la -o -name \*.a | xargs rm -f 
	find . -name \*.so | xargs rm -f
	find . -name .libs -a -type d | xargs rm -rf
	rm -rf $(LIBFPNN_A) 

include ../def.mk
