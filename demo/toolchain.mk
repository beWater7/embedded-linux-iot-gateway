#toolchain.mk
ifndef CROSS_COMPILE
CROSS_COMPILE  = arm-linux-gnueabihf-#工具链
endif

CC 	    = $(CROSS_COMPILE)gcc
CXX     = $(CROSS_COMPILE)g++
STRIP   = $(CROSS_COMPILE)strip
AR      = $(CROSS_COMPILE)ar
CFLAGS  += -g -Wall -std=gnu99 -Wno-missing-braces#-Werror