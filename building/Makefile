
CROSS_COMPILE = mips-linux-uclibc-gnu-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
CPLUSPLUS = $(CROSS_COMPILE)g++
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

# export导出的变量是给子目录下的Makefile使用
export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP


# 编译器在编译时的参数设置 -Wall显示所有警告， -O2优化等级 -g调试信息
CFLAGS := -Wall -O2 -march=mips32r2
BULD_ROOT := $(shell pwd)
include $(BULD_ROOT)/build/base.mak

# 添加头文件路径
CFLAGS += -I $(shell pwd)/include	
#CFLAGS += $(LUX_INC_PATH)


LDFLAGS := -lpthread -lm -lrt -ldl
LDFLAGS += -Wl,-gc-section
#静态库链接,使用动态库链接需要把 all中的 $(LIBS) 去掉
LIBS = $(shell pwd)/libs/ingenic/libimp.a  $(shell pwd)/libs/ingenic/libalog.a

#使用动态库链接
#LDFLAGS += -L$(shell pwd)/libs/ingenic -lalog -limp -lsysutils
#LDFLAGS += -L $(shell pwd)/libs/tuya

 

export CFLAGS LDFLAGS

TOPDIR := $(shell pwd)
export TOPDIR

TARGET := Luxshare_IPC



 obj-y += common/
 obj-y += hal/
 obj-y += imp/	
 obj-y += sysutils/
 obj-y += modules/
 obj-y += sample/
all : 
	make -C ./ -f $(TOPDIR)/Makefile.build
	$(CPLUSPLUS) $(LDFLAGS) -o $(TARGET)  built-in.o $(LIBS)
	$(STRIP) $(TARGET)

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)
.PHONY:all clean 
