# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(schc-full-sdk_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(schc-full-sdk_FRAMEWORKS_FOUND_RELEASE "${schc-full-sdk_FRAMEWORKS_RELEASE}" "${schc-full-sdk_FRAMEWORK_DIRS_RELEASE}")

set(schc-full-sdk_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET schc-full-sdk_DEPS_TARGET)
    add_library(schc-full-sdk_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET schc-full-sdk_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${schc-full-sdk_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${schc-full-sdk_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### schc-full-sdk_DEPS_TARGET to all of them
conan_package_library_targets("${schc-full-sdk_LIBS_RELEASE}"    # libraries
                              "${schc-full-sdk_LIB_DIRS_RELEASE}" # package_libdir
                              "${schc-full-sdk_BIN_DIRS_RELEASE}" # package_bindir
                              "${schc-full-sdk_LIBRARY_TYPE_RELEASE}"
                              "${schc-full-sdk_IS_HOST_WINDOWS_RELEASE}"
                              schc-full-sdk_DEPS_TARGET
                              schc-full-sdk_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "schc-full-sdk"    # package_name
                              "${schc-full-sdk_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${schc-full-sdk_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET schc-full-sdk::schc-full-sdk
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${schc-full-sdk_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${schc-full-sdk_LIBRARIES_TARGETS}>
                 )

    if("${schc-full-sdk_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET schc-full-sdk::schc-full-sdk
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     schc-full-sdk_DEPS_TARGET)
    endif()

    set_property(TARGET schc-full-sdk::schc-full-sdk
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${schc-full-sdk_LINKER_FLAGS_RELEASE}>)
    set_property(TARGET schc-full-sdk::schc-full-sdk
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${schc-full-sdk_INCLUDE_DIRS_RELEASE}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET schc-full-sdk::schc-full-sdk
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Release>:${schc-full-sdk_LIB_DIRS_RELEASE}>)
    set_property(TARGET schc-full-sdk::schc-full-sdk
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${schc-full-sdk_COMPILE_DEFINITIONS_RELEASE}>)
    set_property(TARGET schc-full-sdk::schc-full-sdk
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${schc-full-sdk_COMPILE_OPTIONS_RELEASE}>)

########## For the modules (FindXXX)
set(schc-full-sdk_LIBRARIES_RELEASE schc-full-sdk::schc-full-sdk)
