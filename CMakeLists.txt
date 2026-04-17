cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
project(VibeComb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
elseif(EXISTS "/Users/nerbai/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "/Users/nerbai/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

find_package(CLI11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

enable_testing()
find_package(GTest CONFIG REQUIRED)

add_executable(vibecomb
    src/main.cpp
    src/providers/store.cpp
)
target_include_directories(vibecomb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(vibecomb PRIVATE
    VIBECOMB_VERSION="${PROJECT_VERSION}"
)
target_link_libraries(vibecomb PRIVATE CLI11::CLI11 nlohmann_json::nlohmann_json)

install(TARGETS vibecomb DESTINATION bin)

# Create symlink for clangd
add_custom_command(TARGET vibecomb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
    COMMENT "Creating symlink for compile_commands.json"
)

# Tests
add_executable(provider_store_test
    tests/provider_store_test.cpp
    src/providers/store.cpp
)
target_include_directories(provider_store_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_link_libraries(provider_store_test PRIVATE
    GTest::gtest_main
    nlohmann_json::nlohmann_json
)

include(GoogleTest)
gtest_discover_tests(provider_store_test)
