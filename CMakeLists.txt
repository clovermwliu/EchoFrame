cmake_minimum_required(VERSION 3.8)
project(MyFramework2)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        net/buffer/myIOBuf.h
        net/buffer/MyIOReader.h
        net/buffer/MyIOWriter.h
        net/buffer/MySKBuffer.h
        net/ev/MyLoop.h
        net/ev/MyWatcher.h
        net/protocol/MyCodec.h
        net/server/MyContext.cc
        net/server/MyContext.h
        net/server/MyServer.cc
        net/server/MyServer.h
        net/socket/MySocket.cc
        net/socket/MySocket.h
        net/MyGlobal.cc
        net/MyGlobal.h
        util/MyCommon.cc
        util/MyCommon.h
        util/MyConfig.cc
        util/MyConfig.h
        util/MyException.h
        util/MyFactory.h
        util/MyPropertyTree.cc
        util/MyPropertyTree.h
        util/MyQueue.h
        util/MyRandom.h
        util/MySingleton.h
        util/MyThreadPool.h
        util/MyTimeoutMap.h
        util/MyTimeProvider.cc
        util/MyTimeProvider.h
        util/MyVariant.h
        net/server/MyServant.cc
        net/server/MyServant.h
        net/server/MyDispatcher.cc net/server/MyDispatcher.h
        net/server/MyChannel.cc net/server/MyChannel.h
        net/protocol/MyCodec.h
        net/protocol/MyMessage.cc net/protocol/MyMessage.h
        net/demo/MyDemoDispatcher.cc net/demo/MyDemoDispatcher.h
        net/demo/MyDemoMessage.h
        main.cpp net/server/MyFilter.cc net/server/MyFilter.h
        net/demo/MyDemoHandler.cc net/demo/MyDemoHandler.h
        net/server/MyHandler.h
        net/demo/MyDemoCodec.h
        net/server/EventLoop.cc net/server/EventLoop.h)

# 头文件引用目录
include_directories(
        ./
        /usr/local/Cellar/glog/0.3.5_3/include
        /usr/local/Cellar/gflags/2.2.1/include
        /usr/local/Cellar/tinyxml2/3.0.0/include
        /usr/local/Cellar/libev/4.22/include/
)

# 库文件目录
link_directories(
        ./
        /usr/local/Cellar/libev/4.22/lib/
        /usr/local/Cellar/tinyxml2/3.0.0/lib/
        /usr/local/Cellar/glog/0.3.5_3/lib
)

# 链接的库文件
link_libraries(ev tinyxml2 glog)

# 可执行程序列表
add_executable(MyFramework2 ${SOURCE_FILES})