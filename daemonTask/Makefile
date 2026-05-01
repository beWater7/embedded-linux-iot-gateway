ifndef PREFIX
PREFIX  = arm-linux-gnueabihf-#工具链
endif

CC 	    = $(PREFIX)gcc
CXX     = $(PREFIX)g++
STRIP   = $(PREFIX)strip
AR      = $(PREFIX)ar
CFLAGS  += -g -Wall -std=gnu99 -Wno-missing-braces#-Werror
#LDFLAGS += -lrt
C_SRC = daemonTask.c 

CPP_SRC = 

INCLUDE_PATH = -I ../h/frameWork   \
			   -I ../h/log         \
			   -I ../h/logMng      \
			   -I ../h/node_tree   \
			   -I ../h/shellServer

LIB_PATH = ../module  #库路径，.表示当 

TARGET = daemonTask

#ShellServer 依赖 Data，因此必须在data前面, 换行符\后不能有空格或tab
LIB_FRAMEWORK = -L$(LIB_PATH) \
                -lShellServer \
				-lFrameWork   \
				-llog         \
				-lpthread     \
				-lrt
# -Dxxxx 打开宏xxxx
daemonTask:
	$(CC) $(CFLAGS) -o  $(TARGET) $(C_SRC)
# 删除中间文件
clean:
	rm -f *.o $(TARGET) 

ifeq (0,1)
你的变量在整个Makefile中都不会改变，使用:=是更好的选择，
因为它可以避免一些意外的问题。如果变量的值可能会在Makefile的执行过程中发生变化，
使用=可能更适合，因为它允许变量的值在需要时动态改变。
endif
