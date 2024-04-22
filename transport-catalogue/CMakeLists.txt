cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto)
protobuf_generate_cpp(PROTO_SRCS2 PROTO_HDRS2 map_renderer.proto)
protobuf_generate_cpp(PROTO_SRCS3 PROTO_HDRS3 transport_router.proto)
protobuf_generate_cpp(PROTO_SRCS4 PROTO_HDRS4 graph.proto)

set(PROTO_FILES 
    ${PROTO_SRCS} ${PROTO_HDRS} 
    ${PROTO_SRCS2} ${PROTO_HDRS2} 
    ${PROTO_SRCS3} ${PROTO_HDRS3} 
    ${PROTO_SRCS4} ${PROTO_HDRS4})

set(TRANSPORTCATALOGUE_FILES main.cpp 
    domain.h
    geo.h geo.cpp 
    json.h json.cpp 
    json_builder.h json_builder.cpp 
    json_reader.h json_reader.cpp 
    map_renderer.h map_renderer.cpp 
    request_handler.h request_handler.cpp 
    serialization.h serialization.cpp 
    svg.h svg.cpp 
    transport_catalogue.h transport_catalogue.cpp 
    transport_router.h transport_router.cpp router.h
    transport_catalogue.proto map_renderer.proto transport_router.proto graph.proto)

add_executable(transport_catalogue ${PROTO_FILES} ${TRANSPORTCATALOGUE_FILES})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)