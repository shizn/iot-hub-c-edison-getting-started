#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if (CMAKE_VERSION VERSION_LESS "3.1")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
  endif()
else()
  set (CMAKE_C_STANDARD 99)
  set (CMAKE_CXX_STANDARD 11)
endif()

include_directories(~/azure-iot-sdks/c/c-utility/inc
                    ~/azure-iot-sdks/c/iothub_client/inc
                    ~/azure-iot-sdks/c/serializer/inc)

link_directories(~/azure-iot-sdks/c/cmake/iotsdk_linux/serializer
                 ~/azure-iot-sdks/c/cmake/iotsdk_linux/iothub_client
                 ~/azure-iot-sdks/c/cmake/iotsdk_linux/c-utility)

add_executable(app main.c)
target_link_libraries(app mraa
                          serializer
                          iothub_client
                          iothub_client_http_transport
                          aziotsharedutil
                          ssl
                          crypto
                          curl
                          pthread
                          m
                          ssl
                          crypto)

