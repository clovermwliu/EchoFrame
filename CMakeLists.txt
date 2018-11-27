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
        net/server/MyContext.cc net/server/MyContext.h
        net/server/MyServer.cc net/server/MyServer.h
        net/socket/MySocket.cc net/socket/MySocket.h
        net/MyGlobal.cc net/MyGlobal.h
        util/MyCommon.cc util/MyCommon.h
        util/MyConfig.cc util/MyConfig.h
        util/MyException.h
        util/MyFactory.h
        util/MyPropertyTree.cc util/MyPropertyTree.h
        util/MyQueue.h
        util/MyRandom.h
        util/MySingleton.h
        util/MyThreadPool.h
        util/MyTimeoutMap.h
        util/MyTimeProvider.cc util/MyTimeProvider.h
        util/MyVariant.h
        net/server/MyServant.cc net/server/MyServant.h
        net/server/MyDispatcher.cc net/server/MyDispatcher.h
        net/server/MyChannel.cc net/server/MyChannel.h
        net/protocol/MyCodec.h
        net/protocol/MyMessage.cc net/protocol/MyMessage.h
        net/demo/MyDemoDispatcher.cc net/demo/MyDemoDispatcher.h
        net/demo/MyDemoMessage.h
        net/server/MyFilter.cc net/server/MyFilter.h
        net/demo/MyDemoHandler.cc net/demo/MyDemoHandler.h
        net/server/MyHandler.h
        net/server/EventLoop.cc net/server/EventLoop.h
        net/client/MyClient.cc net/client/MyClient.h
        net/client/MyProxy.cc net/client/MyProxy.h
        net/client/ClientLoop.cc net/client/ClientLoop.h
        net/client/MyRequest.h
        net/demo/MyDemoProxy.cc net/demo/MyDemoProxy.h
        net/magic/MyMagicCodec.h
        net/magic/MyMagicDispatcher.h net/magic/MyMagicDispatcher.cc
        net/client/MyCommunicator.h)

set(SERVER_MAIN main.cpp)

set(CLIENT_MAIN client.cc)

# 头文件引用目录
include_directories(
        ./
        /usr/local/include/
)

# 库文件目录
link_directories(
        ./
        /usr/local/lib/
)

# 链接的库文件
link_libraries(ev tinyxml2 glog)

# 可执行程序列表
add_executable(Server ${SOURCE_FILES} ${SERVER_MAIN})
add_executable(Client ${SOURCE_FILES} client.cc ${CLIENT_MAIN})