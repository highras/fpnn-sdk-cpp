OBJS_C = 

OBJS_CXX = ClientEngine.o TCPClientIOWorker.o Config.o ConnectionMap.o IOBuffer.o ClientInterface.o TCPClient.o  \
			Encryptor.o EncryptedStreamReceiver.o EncryptedPackageReceiver.o UnencryptedReceiver.o \
			micro-ecc/uECC.o KeyExchange.o PEM_DER_SAX.o IQuestProcessor.o \
			UDPCongestionControl.o UDPClientIOWorker.o UDPClient.o \
			UDP.v2/UDPCommon.v2.o UDP.v2/UDPAssembler.v2.o UDP.v2/UDPParser.v2.o UDP.v2/UDPIOBuffer.v2.o \
			UDP.v2/UDPUnconformedMap.v2.o

# Static 
LIBFPNN_A = libfpnn.a

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I../base -I../proto -I../proto/msgpack -I../proto/rapidjson

all: $(LIBFPNN_A)

clean:
	find . -name \*.lo -o -name \*.o | xargs rm -f
	find . -name \*.la -o -name \*.a | xargs rm -f 
	find . -name \*.so | xargs rm -f
	find . -name .libs -a -type d | xargs rm -rf
	rm -rf $(LIBFPNN_A) 

include ../def.mk
