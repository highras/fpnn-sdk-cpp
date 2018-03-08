dirs = base proto core

# LIBFPNN_A = libfpnn.a
# LIB_OBJS = base/*.o proto/*.o core/*.o

all:
	for x in $(dirs); do (cd $$x; make -j4) || exit 1; done
#	$(AR) -rcs $(LIBFPNN_A) $(LIB_OBJS)

clean:
	for x in $(dirs); do (cd $$x; make clean) || exit 1; done
#	rm -rf $(LIBFPNN_A)
