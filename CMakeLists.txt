cmake_minimum_required(VERSION 2.6)

project(ttml2srt C)

include(CheckSymbolExists)

add_subdirectory(compat)
add_executable(ttml2srt main.c process.c process.h args.c args.h)

target_link_libraries(ttml2srt compat)

find_package(EXPAT REQUIRED)
target_link_libraries(ttml2srt ${EXPAT_LIBRARIES})
target_include_directories(ttml2srt PUBLIC ${EXPAT_INCLUDE_DIR})

target_compile_definitions(ttml2srt
	PUBLIC _CRT_SECURE_NO_WARNINGS
	PUBLIC PACKAGE_NAME=\"ttml2srt\" PACKAGE_VERSION=\"0.1\")
