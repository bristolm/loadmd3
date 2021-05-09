# makefile for the FileChecking tool
# - built using the g++ libraries

include ../GNUmaster

PROGRAM = FileInfo.exe

CXX_SRCS = CheckMD3.cpp

OBJS = $(CXX_SRCS:.cpp=.o)

$(PROGRAM) : $(OBJS)
ifdef DJGPP
	$(CP) ../objQuake/*.o .
	$(CP) ../objLW/*.o .
	$(CP) ../objUnreal/*.o .
	$(CXX) $(LDFLAGS) \*.o -o $(PROGRAM) 
else
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBQUAKE) $(LIBLIGHTWAVE) $(LIBUNREAL) -o $(PROGRAM)
endif

depend:
	$(CXX) -MM $(CXX_SRCS)

clean:
	$(RM) *.o *.exe

###########################
