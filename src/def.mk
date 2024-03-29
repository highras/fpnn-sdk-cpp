# define make macros here
OPTIMIZE=-O2
#LINKARGS=-ltcmalloc

CFLAGS +=

CXXFLAGS += -std=c++11
#CPPFLAGS += -Wextra -Wno-unused-parameter -Wno-implicit-fallthrough
CPPFLAGS += -g -Wall -Werror -fPIC $(OPTIMIZE)

LIBS += $(OPTIMIZE) -rdynamic -lstdc++ -lfpnn -lfpproto -lfpbase -lpthread -lz $(LINKARGS)
#LIBS += $(OPTIMIZE) -rdynamic -static-libstdc++ -lfpnn -lfpproto -lfpbase -lpthread $(LINKARGS)

#for apps
$(EXES_CLIENT): $(OBJS_CLIENT)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_CLIENT2): $(OBJS_CLIENT2)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_CLIENT3): $(OBJS_CLIENT3)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_SERVER): $(OBJS_SERVER)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_SERVER2): $(OBJS_SERVER2)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_SERVER3): $(OBJS_SERVER3)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_TEST): $(OBJS_TEST)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_TEST2): $(OBJS_TEST2)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_TEST3): $(OBJS_TEST3)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_TEST4): $(OBJS_TEST4)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(EXES_TEST5): $(OBJS_TEST5)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

#for libs
#for libs
$(LIBFPNN_A): $(OBJS_C) $(OBJS_CXX)
	$(AR) -rcs $@ $(OBJS_C) $(OBJS_CXX)

$(LIBFPNN_S_SO): $(OBJS_C)
	$(CC) -shared -o $@ -Wl,-soname,$(LIBFPNN_S_SO) $(OBJS_C)

$(LIBFPNN_X_SO): $(OBJS_CXX)
	$(CC) -shared -o $@ -Wl,-soname,$(LIBFPNN_X_SO) $(OBJS_CXX)

#for rules
.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS)

.cpp:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)
