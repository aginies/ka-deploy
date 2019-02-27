AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes

lib_LTLIBRARIES=libRMTP.la

libRMTP_la_SOURCES=RMTPserver.cc RMTPclient.cc

libRMTP_la_LDFLAGS=-lpthread -no-undefined -version-info 1:0:0

include_HEADERS=RMTPlib.h RMTPlist.h


