project(KVirusTotal)
cmake_minimum_required(VERSION 2.6)

#if you don't want the full compiler output, remove the following line
# SET(CMAKE_VERBOSE_MAKEFILE ON)

find_package(KDE4 REQUIRED)
find_package(QCA2 REQUIRED)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)

include_directories(
		${CMAKE_SOURCE_DIR}/core/ 
# 		${CMAKE_SOURCE_DIR}/common/ 
# 		${CMAKE_BINARY_DIR}/common/ 
		${KDE4_INCLUDES} 
		${QCA2_INCLUDE_DIR} 
		${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(core)
add_subdirectory(resources)
add_subdirectory(translations)

set(kvirustotal_SRCS 
		settingsdialog.cpp 
		welcomepage.cpp 
		welcomewizard.cpp
		reportrowviewhandler.cpp 
		taskviewhandler.cpp 
		taskrowviewhandler.cpp 
		reportviewhandler.cpp 
		reportviewhandler.cpp 
		mainwindow.cpp 
		main.cpp)

kde4_add_kcfg_files(kvirustotal_SRCS settings.kcfgc)
kde4_add_ui_files(kvirustotal_SRCS mainwindow.ui)
qt4_automoc(${kvirustotal_SRCS})

kde4_add_executable(kvirustotal ${kvirustotal_SRCS})

set( kvirustotal_LIBS
		qjson
		qca
		kvt_core
		kvt_base
		kvt_api
		kvt_web
		${KDE4_KDEUI_LIBS} 
		${KDE4_LIBS} 
		${KDE4_KFILE_LIBRARY} )

target_link_libraries(kvirustotal ${kvirustotal_LIBS})
install( TARGETS kvirustotal  ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kvirustotal.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
#install(FILES settings.kcfg DESTINATION ${KCFG_INSTALL_DIR})