include(BundleUtilities)

# Check for pkgx installation
find_program(PKGX_EXECUTABLE pkgx)

# Xcode generator puts the build type in the build directory
set(BUILD_PREFIX "")
if (CMAKE_GENERATOR STREQUAL "Xcode")
    set(BUILD_PREFIX "${CMAKE_BUILD_TYPE}/")
endif()

# Use generator expressions to get the absolute path to the bundle
if(NOT DEFINED APP_BUNDLE)
    message(FATAL_ERROR "Pass APP_BUNDLE to fix up the exact built app")
endif()
set(APPS "${APP_BUNDLE}")

# Set up framework search paths
set(DIRS "${APP_BUNDLE}/Contents/Frameworks")

# Homebrew dependencies can refer to one another through @rpath rather than
# absolute paths (e.g. FreeType -> Brotli). Give BundleUtilities the actual
# library search roots so it can copy and rewrite the whole dependency graph.
find_program(BREW_EXECUTABLE brew)
if(BREW_EXECUTABLE)
    execute_process(COMMAND "${BREW_EXECUTABLE}" --prefix
        OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    file(GLOB BREW_LIBRARY_DIRS "${BREW_PREFIX}/opt/*/lib")
    list(APPEND DIRS "${BREW_PREFIX}/lib" ${BREW_LIBRARY_DIRS})
endif()

# Detect if we're using pkgx
if(PKGX_EXECUTABLE)
    message(STATUS "pkgx detected, adding pkgx directories to framework search path")
    list(APPEND DIRS "$ENV{HOME}/.pkgx/")
endif()

# Convert all paths to absolute paths
file(REAL_PATH ${APPS} APPS)

set(RESOLVED_DIRS "")
foreach(DIR IN LISTS DIRS)
    # Handle home directory expansion
    string(REPLACE "~" "$ENV{HOME}" DIR "${DIR}")
    # Convert to absolute path, but don't fail if directory doesn't exist
    if(EXISTS "${DIR}")
        file(REAL_PATH "${DIR}" RESOLVED_DIR)
        list(APPEND RESOLVED_DIRS "${RESOLVED_DIR}")
    endif()
endforeach()

# Debug output
message(STATUS "Bundle fixup paths:")
message(STATUS "  App: ${APPS}")
message(STATUS "  Search dirs: ${RESOLVED_DIRS}")

# Fix up the bundle
fixup_bundle("${APPS}" "" "${RESOLVED_DIRS}")
