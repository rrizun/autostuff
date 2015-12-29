
AM_CPPFLAGS=
AM_LDFLAGS=
bin_PROGRAMS=
BUILT_SOURCES=

AM_CPPFLAGS += $(LIBXML2_CFLAGS)

%.pb.h %.pb.cc: %.proto
	protoc -I. --cpp_out=. $<

bin_PROGRAMS += main

BUILT_SOURCES += person.pb.h person.pb.cc
BUILT_SOURCES += wozconfig.pb.h wozconfig.pb.cc
BUILT_SOURCES += wozstatus.pb.h wozstatus.pb.cc

main_SOURCES = main.cpp
main_SOURCES += person.pb.cc
main_SOURCES += wozconfig.pb.cc
main_SOURCES += wozstatus.pb.cc
main_SOURCES += auto_xml_test.cpp
main_SOURCES += gtest/gtest-all.cc

main_LDADD=
main_LDADD += $(LIBEVENT_LIBS)
main_LDADD += $(LIBCURL_LIBS)
main_LDADD += $(LIBXML2_LIBS)
main_LDADD += $(PROTOBUF_LIBS)
main_LDADD += $(MYSQL_CLIENT_LIBS)

CLEANFILES = $(BUILT_SOURCES)
