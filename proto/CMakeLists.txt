cmake_minimum_required(VERSION 3.8)
find_package(Protobuf REQUIRED)

#protobuf
set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}) #proto 文件目录
set(PROTOBUF_INCLUDE_DIRS /usr/local/include/protobuf/) #protobuf 头文件目录
set(PROTOBUF_LIBRARIES /usr/local/lib/) #protobuf 库文件目录
set(PROTOBUF_PROTOC_EXECUTABLE /usr/local/bin/protoc) #protoc文件目录
SET(PROTO_META_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
LIST(APPEND PROTO_FLAGS -I${CMAKE_CURRENT_SOURCE_DIR})

# 头文件引用目录
include_directories(
        ${PROTOBUF_INCLUDE_DIRS}
)

# 库文件目录
link_directories(
        ${PROTOBUF_LIBRARIES}
)

#生成proto协议文件
#set(PROTO_SRCS proto)
#set(PROTO_HDRS proto)
#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/route.proto)

FILE(GLOB MY_PROTOS "*.proto")

FOREACH(FIL ${MY_PROTOS})
    GET_FILENAME_COMPONENT(ABS_FIL ${FIL} ABSOLUTE)
    GET_FILENAME_COMPONENT(FIL_WE ${FIL} NAME_WE)

    LIST(APPEND MY_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")
    LIST(APPEND MY_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")

    EXECUTE_PROCESS(
            COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} ${PROTO_FLAGS} --cpp_out=${PROTO_META_BASE_DIR} ${FIL}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
ENDFOREACH()
