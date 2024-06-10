# CMakeList.txt : CMake project for TermPod, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (TermPod "TermPod.cpp" "TermPod.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TermPod PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
target_include_directories (TermPod PUBLIC "../Deps/libtermpod/src" "../Deps/libtermpod/Deps/libzip/Deps/zlib")
target_link_libraries (TermPod PUBLIC libtermpod)
