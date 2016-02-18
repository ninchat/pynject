PKGCONFIG	:= pkg-config
PYTHON_NAME	:= python

CPPFLAGS	+= -pthread -DPIC -DNDEBUG $(shell $(PKGCONFIG) --cflags $(PYTHON_NAME))
CFLAGS		+= -std=c99 -fPIC -O2 -Wextra
LDFLAGS		+= -pthread

libpynject.so: pynject.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ pynject.c $(LIBS)

clean:
	rm -f libpynject.so *.pyc

.PHONY: clean
