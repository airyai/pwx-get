# Æ½Îßãù <airyai@gmail.com>
# There is only a simple Makefile, with no configure script or lib check.
#
ODIR = win32\bin
OUTPUT = $(ODIR)\pwxget.exe
LIBS = -lboost_system -lboost_filesystem -lboost_thread -lcurldll
SRCS = filebuffer.cpp sheetctl.cpp webclient.cpp webctl.cpp
INCLUDE_PATH = -IC:\Libraries\boost_1_48_0 -IC:\Libraries\curl\curl-7.24.0-devel-mingw32\include
LIB_PATH = -LC:\Libraries\curl\curl-7.24.0-devel-mingw32\lib -LC:\Libraries\boost_1_48_0\stage\shared\lib

all: pwxget

pwxget: pwxget.cpp $(SRCS)
	g++ -o $(OUTPUT)  pwxget.cpp $(SRCS) $(LIBS) $(INCLUDE_PATH) $(LIB_PATH)
	copy C:\Libraries\curl\curl-7.24.0-devel-mingw32\bin\*.dll $(ODIR)\ /Y
	copy C:\Libraries\boost_1_48_0\stage\shared\lib\libboost_system.dll $(ODIR)\ /Y
	copy C:\Libraries\boost_1_48_0\stage\shared\lib\libboost_filesystem.dll $(ODIR)\ /Y
	copy C:\Libraries\boost_1_48_0\stage\shared\lib\libboost_thread.dll $(ODIR)\ /Y
	copy C:\Compilers\mingw\bin\libgcc_s_dw2-1.dll $(ODIR)\ /Y
	copy C:\Compilers\mingw\bin\mingwm10.dll $(ODIR)\ /Y

clean:
	del $(ODIR) /Q /S
	rmdir $(ODIR)

