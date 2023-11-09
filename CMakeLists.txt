cmake_minimum_required(VERSION 3.26)
project(Proyecto3_Raytracer)

set(CMAKE_CXX_STANDARD 23)

add_executable(Proyecto3_Raytracer main.cpp
        cube.cpp
        include/print.h
        camera.cpp
        skybox.cpp)

if(NOT PNG AND WIN32 AND NOT MSVC)
    set(PNG_LIBRARY "C:/Program Files(x86)/libpng")
    set(PNG_PNG_INCLUDE_DIR "C:/Program Files(x86)/libpng/include")
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

target_include_directories(${PROJECT_NAME}
        PUBLIC include/
        PUBLIC ${SDL2_INCLUDE_DIRS}
        PUBLIC ${SDL2IMAGE_INCLUDE_DIRS}
)

if(WIN32 AND NOT MSVC)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES} SDL2_image::SDL2_image mingw32)
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES} SDL2_image::SDL2_image)
endif()
