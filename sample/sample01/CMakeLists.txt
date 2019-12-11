set(target "sample01")
message(STATUS "${target}")

project(${target})

file(GLOB MAIN *.h *.hpp *.cpp)
source_group("main" FILES ${MAIN})

add_executable(${target} ${MAIN})

set_target_properties(${target} PROPERTIES
    FOLDER "spio"
)
