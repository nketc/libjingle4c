bin_PROGRAMS = main_test

AM_CPPFLAGS = -I$(top_srcdir) -DPOSIX
AM_LDFLAGS = $(top_builddir)/.libs/libjingle.a \
             -lstdc++ -lssl -lcrypto -lxml2 -ldl -lrt -lexpat -lpthread
AM_CXXFLAGS = $(AM_CPPFLAGS)

main_test_SOURCES = main_test.c
