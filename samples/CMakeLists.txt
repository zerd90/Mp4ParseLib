# 最低CMake版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(Mp4ParseSample)

file(GLOB samples "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(sample_source_path IN LISTS samples)
    get_filename_component(sample_name ${sample_source_path} NAME_WE)
    message(${sample_name})

    add_executable(${sample_name} ${sample_source_path})
    target_link_libraries(${sample_name} Mp4ParseLib)

    install(TARGETS ${sample_name} DESTINATION ${CMAKE_SOURCE_DIR}/bin/)
endforeach()
