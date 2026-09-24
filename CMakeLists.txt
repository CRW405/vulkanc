cmake_minimum_required(VERSION 3.16)

project(FirstAppC VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(APP CVulkan)

file(GLOB SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB VERT_SHADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert)
file(GLOB FRAG_SHADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag)

set(SHADER_OUTPUTS "")
foreach(shader IN LISTS VERT_SHADERS FRAG_SHADERS)
	get_filename_component(name ${shader} NAME_WE)
	get_filename_component(ext  ${shader} EXT)
	set(spv ${CMAKE_CURRENT_BINARY_DIR}/shaders/${name}${ext}.spv)
	list(APPEND SHADER_OUTPUTS ${spv})
	get_filename_component(spv_dir ${spv} DIRECTORY)
	add_custom_command(
		OUTPUT ${spv}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${spv_dir}
		COMMAND glslc ${shader} -o ${spv}
		DEPENDS ${shader}
		COMMENT "Compiling ${name}${ext}"
	)
endforeach()

add_custom_target(compile-shaders DEPENDS ${SHADER_OUTPUTS})

add_executable(${APP} ${SOURCES} ${HEADERS})

target_compile_options(${APP} PRIVATE -Wall -Wextra -Wpedantic -Werror)

target_link_libraries(${APP} PRIVATE glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)

add_dependencies(${APP} compile-shaders)

add_custom_target(run
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${APP}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS ${APP}
)

add_custom_target(test
	COMMAND ${CMAKE_COMMAND} --build . --target clean --config $<CONFIG>
	COMMAND ${CMAKE_COMMAND} --build . --target run --config $<CONFIG>
	COMMENT "Testing ${APP}: clean + run"
)
