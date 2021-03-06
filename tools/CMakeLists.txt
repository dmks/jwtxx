project(jwttool VERSION 1.0.2)

enable_language(CXX)

find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)

include_directories(${PROJECT_BINARY_DIR}) # toolversion.h
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${jwtxx_BINARY_DIR}/include) # jwtxx/version.h
include_directories(${jwtxx_SOURCE_DIR}/src) # json.h

add_custom_target(generate_tool_version COMMAND ${CMAKE_COMMAND} -D SOURCE_FILE=${PROJECT_SOURCE_DIR}/toolversion.h.in -D DEST_FILE=${PROJECT_BINARY_DIR}/toolversion.h -D PROJECT_VERSION=${PROJECT_VERSION} -P ${jwtxx_SOURCE_DIR}/cmake/version.cmake)

add_executable(${PROJECT_NAME} jwttool.cpp)
add_dependencies(${PROJECT_NAME} generate_tool_version)
target_link_libraries(${PROJECT_NAME} jwtxx ${OPENSSL_CRYPTO_LIBRARY} ${JANSSON_LIBRARIES} dl Threads::Threads)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
