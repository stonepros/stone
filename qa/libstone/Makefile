CFLAGS = -Wall -Wextra -D_GNU_SOURCE -lstonefs -L../../build/lib

TARGETS = trivial_libstone

.c:
	$(CC) $(CFLAGS) $@.c -o $@

all:	$(TARGETS)

clean:
	rm $(TARGETS)
