# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(zlog_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(zlog_FRAMEWORKS_FOUND_RELEASE "${zlog_FRAMEWORKS_RELEASE}" "${zlog_FRAMEWORK_DIRS_RELEASE}")

set(zlog_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET zlog_DEPS_TARGET)
    add_library(zlog_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET zlog_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${zlog_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${zlog_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### zlog_DEPS_TARGET to all of them
conan_package_library_targets("${zlog_LIBS_RELEASE}"    # libraries
                              "${zlog_LIB_DIRS_RELEASE}" # package_libdir
                              "${zlog_BIN_DIRS_RELEASE}" # package_bindir
                              "${zlog_LIBRARY_TYPE_RELEASE}"
                              "${zlog_IS_HOST_WINDOWS_RELEASE}"
                              zlog_DEPS_TARGET
                              zlog_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "zlog"    # package_name
                              "${zlog_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${zlog_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET zlog::zlog
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${zlog_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${zlog_LIBRARIES_TARGETS}>
                 )

    if("${zlog_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET zlog::zlog
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     zlog_DEPS_TARGET)
    endif()

    set_property(TARGET zlog::zlog
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${zlog_LINKER_FLAGS_RELEASE}>)
    set_property(TARGET zlog::zlog
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${zlog_INCLUDE_DIRS_RELEASE}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET zlog::zlog
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Release>:${zlog_LIB_DIRS_RELEASE}>)
    set_property(TARGET zlog::zlog
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${zlog_COMPILE_DEFINITIONS_RELEASE}>)
    set_property(TARGET zlog::zlog
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${zlog_COMPILE_OPTIONS_RELEASE}>)

########## For the modules (FindXXX)
set(zlog_LIBRARIES_RELEASE zlog::zlog)
