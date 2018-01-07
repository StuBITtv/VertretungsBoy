cmake_minimum_required(VERSION 3.7)
project(VertretungsBoy)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(lib/discordpp)
add_subdirectory(lib/rest-curlpp)
add_subdirectory(lib/websocket-websocketpp)

include_directories(${discordpp_SOURCE_DIR}
                    ${discordpp-rest-curlpp_SOURCE_DIR}
                    ${discordpp-websocket-websocketpp_SOURCE_DIR})

include_directories(include)

find_package(Boost REQUIRED COMPONENTS system filesystem)

link_directories(${CURL_INLCUDE_DIR})
link_directories(${SQLITE3_INLCUDE_DIR})
link_directories(${CRYPTO_INLCUDE_DIR})
link_directories(${PTHREAD_INLCUDE_DIR})
link_directories(${Boost_INLCUDE_DIRS})
link_directories(${SSL_INLCUDE_DIR})

set(SOURCE_FILES main.cpp src/plan.cpp include/plan.hpp include/management.hpp src/management.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} curl sqlite3 crypto pthread ${Boost_LIBRARIES} ssl curlpp)
