# makefile for the Lightwave object library
# - built using the g++ libraries

include ../GNUmaster

PROGRAM = libLW.a

CXX_SRCS = ParseLWO.cpp ParseLWO2.cpp ParseLWOcommon.cpp

OBJS = $(CXX_SRCS:.cpp=.o)

$(PROGRAM) : $(OBJS)
	$(AR) $(PROGRAM) $(OBJS)
	$(CP) $(PROGRAM) $(LIBLIGHTWAVE)

depend:
	$(AR) -MM $(CXX_SRCS)

clean:
	-$(RM) *.o *.a
	-$(RM) $(LIBLIGHTWAVE) 

###########################

ParseLWOcommon.o: $(INCLUDE_DIR)/ParseLWOcommon.h
ParseLWO.o: $(INCLUDE_DIR)/ParseLWO.h $(INCLUDE_DIR)/ParseLWOcommon.h
ParseLWO2.o: $(INCLUDE_DIR)/ParseLWO2.h $(INCLUDE_DIR)/ParseLWOcommon.h
