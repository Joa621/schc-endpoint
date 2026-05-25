# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/schc-full-sdk-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${schc-full-sdk_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${schc-full-sdk_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET schc-full-sdk::schc-full-sdk)
    add_library(schc-full-sdk::schc-full-sdk INTERFACE IMPORTED)
    message(${schc-full-sdk_MESSAGE_MODE} "Conan: Target declared 'schc-full-sdk::schc-full-sdk'")
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/schc-full-sdk-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()