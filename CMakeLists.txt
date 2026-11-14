cmake_minimum_required(VERSION 3.18.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ANDROID_ABI arm64-v8a)
set(ANDROID_PLATFORM android-26)

set(MODULE_NAME zygiskDump)
project(${MODULE_NAME})

if (NOT ANDROID)
    message(FATAL_ERROR "Set CMAKE_TOOLCHAIN_FILE to the Android NDK's build/cmake/android.toolchain.cmake")
endif ()

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 20)

set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(C_FLAGS "-Werror=format -fdata-sections -ffunction-sections")
set(CXX_FLAGS "${CXX_FLAGS} -fno-exceptions -fno-rtti")

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(C_FLAGS "${C_FLAGS} -O2 -fvisibility=hidden -fvisibility-inlines-hidden")
    set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,-exclude-libs,ALL -Wl,--gc-sections -Wl,--strip-all")
else ()
    set(C_FLAGS "${C_FLAGS} -O0")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS} ${CXX_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

include_directories(
        xdl/include
)

aux_source_directory(xdl xdl-src)

add_library(${MODULE_NAME} SHARED
        main.cpp
        hack.cpp
        il2cpp_dump.cpp
        ${xdl-src})
target_link_libraries(${MODULE_NAME} log)
target_link_options(${MODULE_NAME} PRIVATE
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384")

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment "${CMAKE_CURRENT_BINARY_DIR}/lib${MODULE_NAME}.so")
endif ()

set(MODULE_STAGE "${CMAKE_CURRENT_BINARY_DIR}/magisk-module")
file(MAKE_DIRECTORY "${MODULE_STAGE}/zygisk")

add_custom_target(magisk_zip
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/module/module.prop"
                "${CMAKE_CURRENT_SOURCE_DIR}/module/target.txt"
                "${CMAKE_CURRENT_SOURCE_DIR}/module/customize.sh"
                "${MODULE_STAGE}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${MODULE_NAME}>" "${MODULE_STAGE}/zygisk/arm64-v8a.so"
        COMMAND ${CMAKE_COMMAND} -E tar cf
                "${CMAKE_CURRENT_BINARY_DIR}/Zygisk-Il2CppDumper-configurable-arm64.zip"
                --format=zip module.prop target.txt customize.sh zygisk/arm64-v8a.so
        WORKING_DIRECTORY "${MODULE_STAGE}"
        DEPENDS ${MODULE_NAME}
        VERBATIM)
