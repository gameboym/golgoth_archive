# golgoth_archive make

CFLAGS=-O -Wall
CC=gcc
OBJS=golgoth_archive.o
EXE=garc

# output execute
$(EXE): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

# compile c source code
%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

#clean
clean:
	@rm *.o *.exe
