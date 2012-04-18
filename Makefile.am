
AM_CPPFLAGS=
bin_PROGRAMS=
BUILT_SOURCES=

SUBDIRS = libevent-2.0.16-stable

AM_CPPFLAGS += $(LIBXML2_CFLAGS)
AM_CPPFLAGS += -Ilibevent-2.0.16-stable/include

%.pb.h %.pb.cc: %.proto
	protoc -I. --cpp_out=. $<

bin_PROGRAMS += auto

BUILT_SOURCES += person.pb.h person.pb.cc
BUILT_SOURCES += wozconfig.pb.h wozconfig.pb.cc
BUILT_SOURCES += wozstatus.pb.h wozstatus.pb.cc

auto_SOURCES = main.cpp
auto_SOURCES += person.pb.cc
auto_SOURCES += wozconfig.pb.cc
auto_SOURCES += wozstatus.pb.cc
auto_SOURCES += auto_xml_test.cpp

auto_LDFLAGS=
auto_LDFLAGS += `mysql_config --libs`

auto_LDADD=
auto_LDADD += libevent-2.0.16-stable/.libs/libevent.a
auto_LDADD += $(LIBCURL_LIBS)
auto_LDADD += $(LIBXML2_LIBS)
auto_LDADD += $(PROTOBUF_LIBS)
auto_LDADD += -lgtest
auto_LDADD += -lboost_filesystem
auto_LDADD += -lboost_regex
auto_LDADD += -lboost_system

CLEANFILES = $(BUILT_SOURCES)
