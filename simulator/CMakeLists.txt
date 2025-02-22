find_package(SDL2 REQUIRED)
add_executable(Simulator src/simulator.c)
target_link_libraries(Simulator SDL2)
