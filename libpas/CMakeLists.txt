cmake_minimum_required(VERSION 3.21)

project(libpas)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to be used")
add_compile_options(-Wall)

# Get sources
AUX_SOURCE_DIRECTORY( "src/libpas"       libpasSources   )
AUX_SOURCE_DIRECTORY( "src/test"         testSources     )
AUX_SOURCE_DIRECTORY( "src/verifier"     verifierSources )

# Directories
include_directories(    ${CMAKE_SOURCE_DIR}/src/libpas 
                        ${CMAKE_SOURCE_DIR}/src/verifier
                        ${CMAKE_SOURCE_DIR}/src/test )

link_directories(       ${CMAKE_SOURCE_DIR}/src/libpas 
                        ${CMAKE_SOURCE_DIR}/src/verifier
                        ${CMAKE_SOURCE_DIR}/src/test )

# Create libraries
add_library(pas_lib                                 SHARED ${libpasSources})
add_library(verifier_lib                            SHARED ${verifierSources} )

# Create executables
add_executable(test_pas ${testSources} )

# Link Libraries
target_link_libraries(test_pas verifier_lib pas_lib chaos_lib)
target_link_libraries(verifier_lib pas_lib)

