# Very useful link: http://techbase.kde.org/Development/Tutorials/Creating_Libraries

include_directories(
# 		${CMAKE_SOURCE_DIR}/core/
#  		${CMAKE_BINARY_DIR}/core/
# 		${KDE4_INCLUDES} 
		${CMAKE_CURRENT_BINARY_DIR})

set(kvt_base_SRCS 
		basereport.cpp
		basefilereport.cpp
		baseurlreport.cpp
		baseservicereply.cpp
		basehttpconnector.cpp )

set(kvt_base_LIBS
		qjson
		qca
		${KDE4_KDECORE_LIBS})
# 		${KDE4_KDEUI_LIBS}
# 		${KDE4_KUTILS_LIBS})

kde4_add_kcfg_files(kvt_base_SRCS ${CMAKE_SOURCE_DIR}/settings.kcfgc)
kde4_add_library(kvt_base STATIC ${kvt_base_SRCS})
# qt4_automoc(${kvt_base_SRCS})
target_link_libraries(kvt_base ${kvt_base_LIBS})