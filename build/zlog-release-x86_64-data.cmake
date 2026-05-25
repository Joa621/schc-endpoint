########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(zlog_COMPONENT_NAMES "")
if(DEFINED zlog_FIND_DEPENDENCY_NAMES)
  list(APPEND zlog_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES zlog_FIND_DEPENDENCY_NAMES)
else()
  set(zlog_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(zlog_PACKAGE_FOLDER_RELEASE "/home/joako/.conan2/p/b/zlog995d70ea2dfe7/p")
set(zlog_BUILD_MODULES_PATHS_RELEASE )


set(zlog_INCLUDE_DIRS_RELEASE "${zlog_PACKAGE_FOLDER_RELEASE}/include")
set(zlog_RES_DIRS_RELEASE )
set(zlog_DEFINITIONS_RELEASE )
set(zlog_SHARED_LINK_FLAGS_RELEASE )
set(zlog_EXE_LINK_FLAGS_RELEASE )
set(zlog_OBJECTS_RELEASE )
set(zlog_COMPILE_DEFINITIONS_RELEASE )
set(zlog_COMPILE_OPTIONS_C_RELEASE )
set(zlog_COMPILE_OPTIONS_CXX_RELEASE )
set(zlog_LIB_DIRS_RELEASE "${zlog_PACKAGE_FOLDER_RELEASE}/lib")
set(zlog_BIN_DIRS_RELEASE )
set(zlog_LIBRARY_TYPE_RELEASE STATIC)
set(zlog_IS_HOST_WINDOWS_RELEASE 0)
set(zlog_LIBS_RELEASE zlog)
set(zlog_SYSTEM_LIBS_RELEASE )
set(zlog_FRAMEWORK_DIRS_RELEASE )
set(zlog_FRAMEWORKS_RELEASE )
set(zlog_BUILD_DIRS_RELEASE )
set(zlog_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(zlog_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${zlog_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${zlog_COMPILE_OPTIONS_C_RELEASE}>")
set(zlog_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${zlog_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${zlog_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${zlog_EXE_LINK_FLAGS_RELEASE}>")


set(zlog_COMPONENTS_RELEASE )