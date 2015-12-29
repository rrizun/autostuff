
AM_CPPFLAGS=
AM_LDFLAGS=
bin_PROGRAMS=
BUILT_SOURCES=

AM_CPPFLAGS += $(LIBXML2_CFLAGS)

%.pb.h %.pb.cc: %.proto
	protoc -I. --cpp_out=. $<

# auto

bin_PROGRAMS += auto

BUILT_SOURCES += person.pb.h person.pb.cc
BUILT_SOURCES += wozconfig.pb.h wozconfig.pb.cc
BUILT_SOURCES += wozstatus.pb.h wozstatus.pb.cc

auto_SOURCES = main.cpp
auto_SOURCES += person.pb.cc
auto_SOURCES += wozconfig.pb.cc
auto_SOURCES += wozstatus.pb.cc
auto_SOURCES += auto_xml_test.cpp
auto_SOURCES += gtest/gtest-all.cc

auto_LDADD=
auto_LDADD += $(LIBEVENT_LIBS)
auto_LDADD += $(LIBCURL_LIBS)
auto_LDADD += $(LIBXML2_LIBS)
auto_LDADD += $(PROTOBUF_LIBS)
auto_LDADD += $(MYSQL_CLIENT_LIBS)

CLEANFILES = $(BUILT_SOURCES)
