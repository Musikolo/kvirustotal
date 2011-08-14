# Very useful link: http://techbase.kde.org/Development/Tutorials/Creating_Libraries

include_directories(
#  		${CMAKE_SOURCE_DIR}/core/api/
#  		${CMAKE_BINARY_DIR}/core/api/
# 		${KDE4_INCLUDES} 
		${CMAKE_CURRENT_BINARY_DIR})

set(kvt_core_SRCS
		httpconnectorfactory.cpp 
		filehasher.cpp )

set(kvt_core_LIBS
# 		kvt_api
		${KDE4_KDECORE_LIBS})
# 		${KDE4_KDEUI_LIBS}
# 		${KDE4_KUTILS_LIBS})

kde4_add_kcfg_files(kvt_core_SRCS ${CMAKE_SOURCE_DIR}/settings.kcfgc)
kde4_add_library(kvt_core STATIC ${kvt_core_SRCS})
target_link_libraries(kvt_core ${kvt_core_LIBS})

add_subdirectory(base)
add_subdirectory(api)
add_subdirectory(web)