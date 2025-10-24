cmake_minimum_required(VERSION 3.10)		# 指定最低CMake版本
project(EpollServer)						# 工程名

set(CMAKE_CXX_STANDARD 11)					# 使用C++11标准
add_executable(EpollServer Main.cc log.cpp) # 生成可执行文件 main