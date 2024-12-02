cmake_minimum_required(VERSION 3.10)

project(GLAD VERSION 1.0)

# Add the GLAD library
add_library(glad STATIC
  src/glad.c
)

# Specify include directories
target_include_directories(glad PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set C standard (optional, adjust as needed)
set_target_properties(glad PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
)

# Install rules (optional)
install(TARGETS glad
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
