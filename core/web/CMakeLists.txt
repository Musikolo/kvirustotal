# Very useful link: http://techbase.kde.org/Development/Tutorials/Creating_Libraries

include_directories(
# 		${CMAKE_SOURCE_DIR}
#  		${CMAKE_BINARY_DIR}
# 		${KDE4_INCLUDES}
		${CMAKE_CURRENT_BINARY_DIR})

set(kvt_web_SRCS
		webfilereport.cpp
		weburlreport.cpp
		webservicereply.cpp 
		webhttpconnector.cpp)

set(kvt_web_LIBS
		kvt_core
		kvt_base
		${KDE4_KDECORE_LIBS})
# 		${KDE4_KDEUI_LIBS}
# 		${KDE4_KUTILS_LIBS})

kde4_add_library(kvt_web STATIC ${kvt_web_SRCS})
qt4_automoc(${kvt_web_SRCS})
target_link_libraries(kvt_web ${kvt_web_LIBS})
