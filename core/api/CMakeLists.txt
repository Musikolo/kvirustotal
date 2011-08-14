# Very useful link: http://techbase.kde.org/Development/Tutorials/Creating_Libraries

include_directories(
# 		${CMAKE_SOURCE_DIR}
#  		${CMAKE_BINARY_DIR}
# 		${KDE4_INCLUDES} 
		${CMAKE_CURRENT_BINARY_DIR})

set(kvt_api_SRCS 
		apihttpconnector.cpp
		apiurlreport.cpp 
		apifilereport.cpp 
		apiservicereply.cpp )

set(kvt_api_LIBS
		kvt_core
		kvt_base
		${KDE4_KDECORE_LIBS})
# 		${KDE4_KDEUI_LIBS}
# 		${KDE4_KUTILS_LIBS})

kde4_add_library(kvt_api STATIC ${kvt_api_SRCS})
qt4_automoc(${kvt_api_SRCS})
target_link_libraries(kvt_api ${kvt_api_LIBS})