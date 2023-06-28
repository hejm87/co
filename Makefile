#
# @file  Makefile
# @author chenxueyou
# @version 0.1
# @brief   :A asymmetric coroutine library for C++
# History
#      1. Date: 2014-12-12 
#          Author: chenxueyou
#          Modification: this file was created 
#

all:
	g++ -std=c++11 co.cpp -g -c
	g++ -std=c++11 main.cpp -g -c
	g++ -std=c++11 -g -o main main.o co.o -lpthread
clean:
	rm -f co.o main.o main
