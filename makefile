
############################################################# 
# Makefile for shared library.
# 编译动态链接库
#############################################################
#（1）Makefile中的 符号 $@, $^, $< 的意思：
#　　$@  表示目标文件
#　　$^  表示所有的依赖文件
#　　$<  表示第一个依赖文件
#　　$?  表示比目标还要新的依赖文件列表
#
#（2）wildcard、notdir、patsubst的意思：
#
#　　wildcard : 扩展通配符
#　　notdir ： 去除路径
#　　patsubst ：替换通配符


#包含的头文件
INC= -I./AMQP_include  	-I./AMQP_include/MessageQueue  

#包含的源文件
DIR_SRC = ./AMQP_src

#生成的中间文件
DIR_OBJ = 

#生成的目标文件
DIR_BIN = ./lib

#使用C编译器
#CC = gcc

#使用C++编译器
CC = g++

#-fPIC 编译成位置无关的动态库,编译成动态库时必选
#-g 表示调试模式
#-Wall 表示打印所有警告
CC_FLAG =   -fPIC -g -Wall -I${INC} -D__linux

#需要的动态链接库
LIB = ./libcentos/librabbitmq.so.4.2.0

#生成的动态链接库，位置及文件名
PRG = ./libcentos/libmq.so

SRC = ./AMQP_src
#需要的源文件
OBJ = $(SRC)/RabbitMQ_Adapter.o  $(SRC)/RabbitMQ.o

#all target
all:$(PRG)

$(PRG):$(OBJ)
	$(CC)  -shared -o $@ $(OBJ)  $(LIB) 

.SUFFIXES: .c .o .cpp
.cpp.o:
	$(CC) $(CC_FLAG) $(INC) -c $*.cpp -o $*.o
#PRONY表示伪目标文件
.PRONY:clean
clean:
	@echo "Removing linked and compiled files......"
#清楚生成的中间文件和可执行文件或动态链接库
	-rm -f $(OBJ) $(PRG)