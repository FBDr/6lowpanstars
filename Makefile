# Makefile wrapper for waf

all:
	./waf

# free free to change this part to suit your requirements
configure:
	./waf configure --enable-examples --with-brite=../BRITE

build:
	./waf build

install:
	./waf install

clean:
	./waf

distclean:
	./waf distclean
