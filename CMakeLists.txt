cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project               (tta VERSION 2.3 LANGUAGES CXX)
set                   (CMAKE_BUILD_TYPE Release)
set                   (CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set                   (CMAKE_CXX_STANDARD 11)

add_compile_options   (-Wall -O2 -funroll-loops -fomit-frame-pointer)

set (PROJECT_FILES libtta.cpp libtta.h filter.h)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set (CPU_X86 true)
    if(ENABLE_AVX)
        add_compile_options(-march=haswell -mavx)
    elseif(ENABLE_SSE4)
        add_compile_options(-msse4)
    elseif(ENABLE_SSE2)
        add_compile_options(-msse2)
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(arm)|(ARM)")
    set (CPU_ARM true)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)|(ARM64)")
        if(ENABLE_ASM)
            message(("ENABLE_ASM not for ${CMAKE_SYSTEM_PROCESSOR}"))
            set(ENABLE_ASM 0)
        endif()
    elseif(ENABLE_ASM)
        set (PROJECT_FILES libtta.cpp libtta.h filter.h filter_arm.S)
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(mipsel)")
    add_compile_options(-mips32r2 -mtune=24kf)
endif ()

configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

add_library               (libtta SHARED ${PROJECT_FILES})
target_include_directories(libtta PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties     (libtta PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties     (libtta PROPERTIES OUTPUT_NAME tta)
set_target_properties     (libtta PROPERTIES PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/libtta.h)

add_library               (libtta.a STATIC ${PROJECT_FILES})
target_include_directories(libtta.a PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties     (libtta.a PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties     (libtta.a PROPERTIES OUTPUT_NAME tta)

add_executable            (tta.exe console/tta.cpp)
target_include_directories(tta.exe PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties     (tta.exe PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties     (tta.exe PROPERTIES OUTPUT_NAME tta)
target_link_libraries     (tta.exe PUBLIC libtta.a)

install(TARGETS libtta libtta.a tta.exe
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include)
