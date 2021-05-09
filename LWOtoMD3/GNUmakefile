# makefile for the LWOtoMD3 tool
# - built using the g++ libraries

include ../GNUmaster

PROGRAM = LWOtoMD3.exe

CXX_SRCS = LWOtoMD3.cpp LoadCfgFile.cpp LtoM_obj.cpp

OBJS = $(CXX_SRCS:.cpp=.o)

$(PROGRAM) : $(OBJS)
ifdef DJGPP
	$(CP) ../objQuake/*.o .
	$(CP) ../objLW/*.o .
	$(RM) ParseMD4.o
	$(CXX) $(LDFLAGS) \*.o -o $(PROGRAM) 
else
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBQUAKE) $(LIBLIGHTWAVE) -o $(PROGRAM)
endif

depend:
	$(CXX) -MM $(CXX_SRCS)

clean:
	$(RM) *.o *.exe

###########################

LWOtoMD3.o : LWOtoMD3.h
LoadCfgFile.o : LWOtoMD3.h
LtoM_obj.o : LWOtoMD3.h


