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
	g++ -std=c++11 -g -o co.o -c co.cpp
	g++ -std=c++11 -g -o main.o -c main.cpp
	g++ -std=c++11 -g -o co_mutex.o -c co_mutex.cpp
	g++ -std=c++11 -g -o co_semaphore.o -c co_semaphore.cpp
	g++ -std=c++11 -o main main.o co.o co_mutex.o co_semaphore.o -lpthread

clean:
	rm -f *.o main
