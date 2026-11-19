cmake_minimum_required(VERSION 3.25)
project(blockworks360 C)

add_executable(blockworks360
    src/main.c
    src/core/input.c
    src/game/world.c
    src/game/physics.c
    src/game/player.c
    src/render/renderer.c
)
set_target_properties(blockworks360 PROPERTIES C_STANDARD 17 C_EXTENSIONS ON)
target_compile_options(blockworks360 PRIVATE -Wall -Wextra -Werror)
xsdk_add_shaders(blockworks360 src/render/world.vert src/render/world.frag)
xsdk_add_xex(blockworks360)
