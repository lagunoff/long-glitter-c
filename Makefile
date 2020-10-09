out/long-glitter: src/*.c src/*.h
	bin/gcc $(filter-out src/dev.c src/sexp.c, $(wildcard src/*.c)) -o $@

out/long-glitter-release: src/*.c src/*.h
	bin/gcc src/*.c -o $@ -ffunction-sections -fdata-sections -O3
	strip -s -R .comment -R .gnu.version $@ --strip-unneeded

DEFAULT: out/long-glitter
