ifndef PREFIX
PREFIX  = arm-linux-gnueabihf-#工具链
endif

CC 	    = $(PREFIX)gcc
STRIP   = $(PREFIX)strip
AR      = $(PREFIX)ar
CFLAGS  += -g -Wall -std=gnu99#-Werror
C_SRC = 
CPP_SRC = 
INCLUDE_PATH = -I ../h/frameWork \
			   -I ../log/src
LIB_PATH = ../module  #库路径，.表示当前路径
OBJS    =  shellserver.O
#TARGET 	=  log.a 
LIB     = libShellServer.a
LIB_FRAMEWORK = -L$(LIB_PATH) \
                -lShellServer \
				-lFrameWork   \
				-llog         \
				-lpthread     \
				-lrt
#TARGET = demo
all: $(LIB)

# 生成.o 文件
shellserver.O : shellserver.c
	$(CC) $(CFLAGS) $(INCLUDE_PATH) -c $< -o $@ $(LIB_FRAMEWORK)

# 生成库文件
$(LIB):$(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS) 
	rm -f $(OBJS)
	cp $(LIB) ../module
#ShellServer 依赖 Data，因此必须在data前面, 换行符\后不能有空格或tab
#LIB_FRAMEWORK = -L$(LIB_PATH) -lShellServer  -lFrameWork \
				-llog -lpthread 
# -Dxxxx 打开宏xxxx
#demo:
#	$(CC) $(CFLAGS) $(INCLUDE_PATH) -o  $(TARGET) LogMng.c demo.c $(LIB_FRAMEWORK) 
shelltools:
	$(CC) -o  shelltools shelltools.c
# 删除中间文件
clean:
	rm -f $(OBJS) $(TARGET) 

ifeq (0,1)
你的变量在整个Makefile中都不会改变，使用:=是更好的选择，
因为它可以避免一些意外的问题。如果变量的值可能会在Makefile的执行过程中发生变化，
使用=可能更适合，因为它允许变量的值在需要时动态改变。
endif
