cmake_minimum_required (VERSION 2.8.5)
project(DissolveMapBaker)

option(MAKE_CLI "Make the CLI too" on)

set( DMB_SRCS
	src/ComputeRunner.cpp
	src/DissolveMapBaker.cpp
	)

set (DMB_HDRS
	include/ComputeRunner.h
	include/DissolveMapBaker.h
	)

set (DMB_RSRC
	resources/dissolveMapBaker.hlsl
	)

include_directories(
	"${PROJECT_SOURCE_DIR}/include"
)

add_library(DissolveMapBaker ${DMB_SRCS} ${DMB_HDRS} ${DMB_RSRC})

add_custom_target(COPY_SHADER
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/resources
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/resources/dissolveMapBaker.hlsl ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/resources/
	)

add_dependencies(DissolveMapBaker COPY_SHADER)	

if (MAKE_CLI)
	set (CLI_SRCS
		CLI/src/main.cpp
		)

	set (CLI_HDRS
		)

	include_directories(
		"${PROJECT_SOURCE_DIR}/include"
		"${PROJECT_SOURCE_DIR}/CLI/include"
	)	

	add_executable(DissolveMapBakerCLI WIN32 ${CLI_SRCS} ${CLI_HDRS})
	target_link_libraries(DissolveMapBakerCLI DissolveMapBaker)

endif(MAKE_CLI)
