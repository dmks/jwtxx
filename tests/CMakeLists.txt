enable_language(CXX)

find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs -Wno-unused-parameter")

add_definitions(-DBOOST_TEST_DYN_LINK)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(nonetest EXCLUDE_FROM_ALL nonetest.cpp)
target_link_libraries(nonetest jwtxx ${OPENSSL_CRYPTO_LIBRARY} ${JANSSON_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} dl Threads::Threads)
add_executable(hmactest EXCLUDE_FROM_ALL hmactest.cpp)
target_link_libraries(hmactest jwtxx ${OPENSSL_CRYPTO_LIBRARY} ${JANSSON_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} dl Threads::Threads)
add_executable(rsatest EXCLUDE_FROM_ALL rsatest.cpp)
target_link_libraries(rsatest jwtxx ${OPENSSL_CRYPTO_LIBRARY} ${JANSSON_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} dl Threads::Threads)
add_executable(ecdsatest EXCLUDE_FROM_ALL ecdsatest.cpp)
target_link_libraries(ecdsatest jwtxx ${OPENSSL_CRYPTO_LIBRARY} ${JANSSON_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} dl Threads::Threads)

add_test(none nonetest)
add_test(hmac hmactest)
add_test(rsa rsatest)
add_test(ecdsa ecdsatest)

add_custom_target(tests DEPENDS nonetest hmactest rsatest ecdsatest)

configure_file(rsa-2048-key-pair.pem rsa-2048-key-pair.pem COPYONLY)
configure_file(rsa-2048-key-pair-pw.pem rsa-2048-key-pair-pw.pem COPYONLY)
configure_file(public-rsa-2048-key.pem public-rsa-2048-key.pem COPYONLY)
configure_file(ecdsa-256-key-pair.pem ecdsa-256-key-pair.pem COPYONLY)
configure_file(ecdsa-256-key-pair-pw.pem ecdsa-256-key-pair-pw.pem COPYONLY)
configure_file(public-ecdsa-256-key.pem public-ecdsa-256-key.pem COPYONLY)
configure_file(rsa-cert.pem rsa-cert.pem COPYONLY)
configure_file(ecdsa-cert.pem ecdsa-cert.pem COPYONLY)
