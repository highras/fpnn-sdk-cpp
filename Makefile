dirs = src tools

all:
	for x in $(dirs); do (cd $$x; make -j4) || exit 1; done

clean:
	for x in $(dirs); do (cd $$x; make clean) || exit 1; done
	make clean -C tools
	make clean -C tests
	make clean -C examples
