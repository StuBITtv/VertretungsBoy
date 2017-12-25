cmake_minimum_required(VERSION 3.7)
project(VertretungsBoy)

set(CMAKE_CXX_STANDARD 14)

link_directories(${CURL_INLCUDE_DIR})
link_directories(${SQLITE3_INLCUDE_DIR})

set(SOURCE_FILES main.cpp plan.cpp plan.hpp)
add_executable(VertretungsBoy ${SOURCE_FILES})

target_link_libraries(VertretungsBoy curl sqlite3)