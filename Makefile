
CC=bin/gcc
SRCS=$(wildcard src/*.c)
CFLAGS=-O0 -g

dist/%.o: src/%.c
	unbuffer $(CC) $(CFLAGS) -c -o $@ $(filter %.c,$^) -MM -MF "$(patsubst %.o,%.d,$@)" -MT "$@"  2>&1 | tac
	unbuffer $(CC) $(CFLAGS) -c -o $@ $(filter %.c,$^) 2>&1 | tac

-include dist/*.d

dist/long-glitter: $(patsubst src/%.c,dist/%.o,$(SRCS))
	unbuffer $(CC) $(CFLAGS) $^ -o $@  2>&1 | tac

dist/long-glitter-release: src/*.c src/*.h
	$(CC) $(filter-out src/dev.c src/sexp.c, $(wildcard src/*.c)) -o $@ -ffunction-sections -fdata-sections -O3
	strip -R .comment -R .gnu.version $@ --strip-unneeded

dist/assets:
	ln -s $(realpath assets) dist/assets

.ALL: dist/assets dist/long-glitter

.DEFAULT: .ALL
