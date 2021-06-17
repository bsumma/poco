#
# MySQL.make
#
# Makefile fragment for finding MySQL library
#

ifndef POCO_MYSQL_INCLUDE
ifeq (0, $(shell test -d /usr/local/include/mysql; echo $$?))
POCO_MYSQL_INCLUDE = /usr/local/include
else
ifeq (0, $(shell test -d /usr/local/opt/mysql-client; echo $$?))
POCO_MYSQL_INCLUDE = /usr/local/opt/mysql-client/include
else
ifeq (0, $(shell test -d /usr/local/opt/mysql; echo $$?))
POCO_MYSQL_INCLUDE = /usr/local/opt/mysql/include
endif
endif
endif
endif

ifndef POCO_MYSQL_LIB
ifeq (0, $(shell test -d /usr/local/include/mysql; echo $$?))
POCO_MYSQL_LIB = /usr/local/lib
else
ifeq (0, $(shell test -d /usr/local/opt/mysql-client/lib; echo $$?))
POCO_MYSQL_INCLUDE = /usr/local/opt/mysql-client/lib
else
ifeq (0, $(shell test -d /usr/local/opt/mysql/lib; echo $$?))
POCO_MYSQL_INCLUDE = /usr/local/opt/mysql/lib
endif
endif
endif
endif

ifdef POCO_MYSQL_INCLUDE
INCLUDE += -I$(POCO_MYSQL_INCLUDE)
endif
ifdef POCO_MYSQL_LIB
SYSLIBS += -L$(POCO_MYSQL_LIB)
endif
