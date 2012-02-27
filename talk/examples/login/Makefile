SRCFILES := \
    xmppauth.cc \
	xmpppump.cc \
	xmppsocket.cc \
	xmppthread.cc \
	talkclient.cc \
	login_main.cc

TARGET := login

LIBS := \
    $(LIBPATH)/libbase.a \
	$(LIBPATH)/libp2p.a \
	$(LIBPATH)/libxmllite.a \
	$(LIBPATH)/libxmpp.a

OBJFILES := $(SRCFILES:.cc=.o)

$(TARGET): $(OBJFILES)
	@echo "linking $@ ..."
	@$(CXX) -o $@ $^ $(LINKFLAGS) $(LIBS) $(LIBFLAGS) -lssl -lcrypto -lexpat -ldl -lrt -lpthread

%.o : %.cc
	@echo "compile CXX $< ..."
	@$(CXX) -c $(CPPDEFINES) $(CCFLAGS) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	-rm -f $(OBJFILES)
	-rm -f $(TARGET)
