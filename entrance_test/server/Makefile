CXX=g++
RM=rm -f
CPPFLAGS=-g
LDFLAGS=-g
LDLIBS=-lboost_system -lpthread

SRCS=connection.cpp connection_manager.cpp main.cpp reply.cpp request_handler.cpp request_parser.cpp server.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: async_server

async_server: $(OBJS)
	$(CXX) $(LDFLAGS) -o async_server $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
