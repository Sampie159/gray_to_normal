.PHONY: all install uninstall clean

PREFIX ?= /usr/local
CXXFLAGS ?= -O3
CXXSTD ?= -std=gnu++26

all: gtn

gtn: gray_to_normal.cpp stb_image.h stb_image_write.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CXXSTD) $< $(LDFLAGS) $(LDLIBS) -o $@

stb_image.h:
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h
	
stb_image_write.h:
	wget https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h

install:
	install -Dm755 gtn $(DESTDIR)$(PREFIX)/bin/gtn

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/gtn

clean: rm -f gtn
