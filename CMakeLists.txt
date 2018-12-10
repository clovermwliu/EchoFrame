cmake_minimum_required(VERSION 3.8)
project(MyFramework2)
#include(proto/CMakeLists.txt)
find_package(Protobuf REQUIRED)

set(CMAKE_CXX_STANDARD 17)
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
        net/client/MySession.h
        net/demo/MyDemoProxy.cc net/demo/MyDemoProxy.h
        net/magic/MyMagicCodec.h
        net/magic/MyMagicDispatcher.h net/magic/MyMagicDispatcher.cc
        net/client/MyCommunicator.h
        route/MyRouteMessage.h
        route/MyRouteHandler.cc route/MyRouteHandler.h
        proto/BoxedMessage.h
        route/MyRouteDispatcher.cc route/MyRouteDispatcher.h
        route/MyRouteProxy.cc route/MyRouteProxy.h net/application/MyApplication.cc net/application/MyApplication.h)
#        presistence/MyRedis.h presistence/MyRedis.cc)

set(SERVER_MAIN main.cpp)
set(CLIENT_MAIN client.cc)
set(ROUTE_MAIN route/route.cc)

#protobuf
set(PROTOBUF_IMPORT_DIRS proto) #proto 文件目录
set(PROTOBUF_INCLUDE_DIRS /usr/local/include/protobuf/) #protobuf 头文件目录
set(PROTOBUF_LIBRARIES /usr/local/lib/) #protobuf 库文件目录
set(PROTOBUF_PROTOC_EXECUTABLE /usr/local/bin/protoc) #protoc文件目录

# 头文件引用目录
include_directories(
        ./
        /usr/local/include/
        ${PROTOBUF_INCLUDE_DIRS}
)

# 库文件目录
link_directories(
        ./
        /usr/local/lib/
        ${PROTOBUF_LIBRARIES}
)

#生成protobuf
FILE(GLOB MY_PROTOS "proto/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${MY_PROTOS})

# 链接的库文件
link_libraries(ev tinyxml2 glog protobuf)

# 可执行程序列表
add_executable(server ${SOURCE_FILES} ${SERVER_MAIN} ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(client ${SOURCE_FILES} ${CLIENT_MAIN} ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(route ${SOURCE_FILES} ${ROUTE_MAIN} ${PROTO_SRCS} ${PROTO_HDRS})

# 执行后置代码
add_custom_target(
        AllTarget ALL
        DEPENDS server client route
)
add_custom_command(
        TARGET AllTarget POST_BUILD
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/*.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/proto
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/*.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/proto
)
