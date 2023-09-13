cmake_minimum_required(VERSION 3.16.0)
project("vulkan-playground" LANGUAGES C)

# require C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# all the errors
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -pedantic -Wextra")

# use extra cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/share/ECM/find-modules/")

find_package(Vulkan REQUIRED)
find_package(XCB REQUIRED)

add_subdirectory(src)

