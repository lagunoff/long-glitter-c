out/long-glitter: src/*.c src/*.h
	bin/gcc src/*.c -o $@ -Ofast

out/long-glitter-release: src/*.c src/*.h
	bin/gcc src/*.c -o $@ -ffunction-sections -fdata-sections -O3
	strip -s -R .comment -R .gnu.version $@ --strip-unneeded

DEFAULT: out/long-glitter
