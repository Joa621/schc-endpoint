########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(schc-full-sdk_COMPONENT_NAMES "")
if(DEFINED schc-full-sdk_FIND_DEPENDENCY_NAMES)
  list(APPEND schc-full-sdk_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES schc-full-sdk_FIND_DEPENDENCY_NAMES)
else()
  set(schc-full-sdk_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(schc-full-sdk_PACKAGE_FOLDER_RELEASE "/home/joako/.conan2/p/b/schc-c10201a1398aa/p")
set(schc-full-sdk_BUILD_MODULES_PATHS_RELEASE )


set(schc-full-sdk_INCLUDE_DIRS_RELEASE "${schc-full-sdk_PACKAGE_FOLDER_RELEASE}/include")
set(schc-full-sdk_RES_DIRS_RELEASE )
set(schc-full-sdk_DEFINITIONS_RELEASE )
set(schc-full-sdk_SHARED_LINK_FLAGS_RELEASE )
set(schc-full-sdk_EXE_LINK_FLAGS_RELEASE )
set(schc-full-sdk_OBJECTS_RELEASE )
set(schc-full-sdk_COMPILE_DEFINITIONS_RELEASE )
set(schc-full-sdk_COMPILE_OPTIONS_C_RELEASE )
set(schc-full-sdk_COMPILE_OPTIONS_CXX_RELEASE )
set(schc-full-sdk_LIB_DIRS_RELEASE "${schc-full-sdk_PACKAGE_FOLDER_RELEASE}/lib")
set(schc-full-sdk_BIN_DIRS_RELEASE )
set(schc-full-sdk_LIBRARY_TYPE_RELEASE STATIC)
set(schc-full-sdk_IS_HOST_WINDOWS_RELEASE 0)
set(schc-full-sdk_LIBS_RELEASE fullsdk)
set(schc-full-sdk_SYSTEM_LIBS_RELEASE )
set(schc-full-sdk_FRAMEWORK_DIRS_RELEASE )
set(schc-full-sdk_FRAMEWORKS_RELEASE )
set(schc-full-sdk_BUILD_DIRS_RELEASE )
set(schc-full-sdk_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(schc-full-sdk_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${schc-full-sdk_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${schc-full-sdk_COMPILE_OPTIONS_C_RELEASE}>")
set(schc-full-sdk_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${schc-full-sdk_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${schc-full-sdk_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${schc-full-sdk_EXE_LINK_FLAGS_RELEASE}>")


set(schc-full-sdk_COMPONENTS_RELEASE )