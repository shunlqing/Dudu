cmake_minimum_required(VERSION 3.12)
project(Dudu)

set(CMAKE_CXX_STANDARD 14)

add_executable(Dudu main.cpp EventLoop.cpp Poller.cpp Channel.cpp Util.cpp Buffer.cpp Socket.cpp StreamSocket.cpp TcpServer.cpp Timer.cpp ThreadPool.cpp TCPClient.cpp)
add_executable(cli cli.cpp EventLoop.cpp Poller.cpp Channel.cpp Util.cpp Buffer.cpp Socket.cpp StreamSocket.cpp TcpServer.cpp Timer.cpp ThreadPool.cpp TCPClient.cpp )
INCLUDE_DIRECTORIES(./)

TARGET_LINK_LIBRARIES(Dudu -pthread)
TARGET_LINK_LIBRARIES(cli -pthread)