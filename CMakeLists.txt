project(KVirusTotal)
cmake_minimum_required(VERSION 2.6)

find_package(KDE4 REQUIRED)
find_package(QCA2 REQUIRED)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)

include_directories(${KDE4_INCLUDES} ${QCA2_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(resources)
add_subdirectory(translations)

set(kvirustotal_SRCS 
		taskschedulerjob.cpp 
		taskscheduler.cpp 
		settingsdialog.cpp 
		welcomepage.cpp 
		welcomewizard.cpp 
		filehasher.cpp 
		reportrowviewhandler.cpp 
		urlreport.cpp 
		servicebasicreply.cpp 
		abstractreport.cpp 
		taskviewhandler.cpp 
		taskrowviewhandler.cpp 
		reportviewhandler.cpp 
		reportviewhandler.cpp 
		httpconnector.cpp 
		filereport.cpp 
		mainwindow.cpp 
		main.cpp)

kde4_add_kcfg_files(kvirustotal_SRCS settings.kcfgc)
kde4_add_ui_files(kvirustotal_SRCS mainwindow.ui)
qt4_automoc(${kvirustotal_SRCS})

kde4_add_executable(kvirustotal ${kvirustotal_SRCS})

set( QCA2_LIBS qca)
set( QJSON_LIBS qjson)
target_link_libraries(kvirustotal ${KDE4_KDEUI_LIBS} ${KDE4_LIBS} ${KDE4_KFILE_LIBRARY} ${QJSON_LIBS} ${QCA2_LIBS})
install( TARGETS kvirustotal  ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kvirustotal.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
#install(FILES settings.kcfg DESTINATION ${KCFG_INSTALL_DIR})