cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)

project(MiniCI C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE})

# pthread podrška
find_package(Threads REQUIRED)

add_executable(MiniCI src/main.c)

target_include_directories(MiniCI PRIVATE ${CMAKE_SOURCE_DIR}/include)

# poveži pthread biblioteku
target_link_libraries(MiniCI PRIVATE Threads::Threads)

file(GLOB EXTRA_SOURCES "src/*.c" "src/*/*.c")

list(REMOVE_ITEM EXTRA_SOURCES "${CMAKE_SOURCE_DIR}/src/main.c")

target_sources(MiniCI PRIVATE ${EXTRA_SOURCES})


install(TARGETS MiniCI 
        RUNTIME DESTINATION bin)

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
