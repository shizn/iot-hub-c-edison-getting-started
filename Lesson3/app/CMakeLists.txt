#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.
cmake_minimum_required(VERSION 2.8)

set (CMAKE_C_FLAGS "--std=gnu99 ${CMAKE_C_FLAGS}")

include_directories(~/azure-iot-sdks/c/c-utility/inc
                    ~/azure-iot-sdks/c/iothub_client/inc)

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

