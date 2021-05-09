# makefile for the Quake object library
# - built using the g++ libraries

include ../GNUmaster

PROGRAM = libQuake.a

CXX_SRCS = ParseMD2.cpp ParseMD3.cpp ParseMD4.cpp

OBJS = $(CXX_SRCS:.cpp=.o)

$(PROGRAM) : $(OBJS)
	$(AR) $(PROGRAM) $(OBJS)
	$(CP) $(PROGRAM) $(LIBQUAKE)

depend:
	$(AR) -MM $(CXX_SRCS)

clean:
	-$(RM) *.o *.a
	-$(RM) $(LIBQUAKE) 

###########################

