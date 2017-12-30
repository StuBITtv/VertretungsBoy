cmake_minimum_required(VERSION 3.7)
project(VertretungsBoy)

set(CMAKE_CXX_STANDARD 14)

link_directories(${CURL_INLCUDE_DIR})
link_directories(${SQLITE3_INLCUDE_DIR})

set(SOURCE_FILES main.cpp src/plan.cpp include/plan.hpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} curl sqlite3)