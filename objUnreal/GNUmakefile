# makefile for the Unreal object library
# - built using the g++ libraries

include ../GNUmaster

PROGRAM = libUnreal.a

CXX_SRCS = ParseUnreal.cpp ParseUnrealSkl.cpp

OBJS = $(CXX_SRCS:.cpp=.o)

$(PROGRAM) : $(OBJS)
	$(AR) $(PROGRAM) $(OBJS)
	$(CP) $(PROGRAM) $(LIBUNREAL)

depend:
	$(AR) -MM $(CXX_SRCS)

clean:
	-$(RM) *.o *.a
	-$(RM) $(LIBUNREAL)

###########################

