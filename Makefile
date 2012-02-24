# 平芜泫 <airyai@gmail.com>
# There is only a simple Makefile, with no configure script or lib check.
#
OUTPUT=pwxget
LIBS=-lboost_system -lboost_filesystem -lboost_thread -lcurl
SRCS = filebuffer.cpp sheetctl.cpp webclient.cpp webctl.cpp

all: pwxget

pwxget: pwxget.cpp $(SRCS)
	g++ -o $(OUTPUT)  pwxget.cpp $(SRCS) $(LIBS)

install: pwxget
	cp pwxget /usr/local/bin/pwxget

uninstall: pwxget
	rm /usr/local/bin/pwxget

clean:
	rm -f $(OUTPUT)

