OBJS_C = hex.o md5.o rijndael.o sha1.o sha256.o base64.o

OBJS_CXX = Endian.o FpnnError.o TaskThreadPool.o StringUtil.o TimeUtil.o httpcode.o \
		   FPLog.o FileSystemUtil.o NetworkUtility.o bit.o hashint.o jenkins.o obpool.o MidGenerator.o FPJson.o FPJsonParser.o

# Static 
LIBFPNN_A = libfpbase.a

CFLAGS +=
CXXFLAGS += 
CPPFLAGS +=

all: $(LIBFPNN_A)

clean:
	find . -name \*.lo -o -name \*.o | xargs rm -f
	find . -name \*.la -o -name \*.a | xargs rm -f 
	find . -name \*.so | xargs rm -f
	find . -name .libs -a -type d | xargs rm -rf
	rm -f $(LIBFPNN_A)

include ../def.mk
