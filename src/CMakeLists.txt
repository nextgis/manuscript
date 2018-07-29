#***************************************************************************
# Project:  NextGIS Manuscript
# Purpose:  cmake script
# Author:   Dmitry Baryshnikov, dmitry.baryshnikov@nextgis.com
#***************************************************************************
#   Copyright (C) 2018 NextGIS, info@nextgis.ru
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#***************************************************************************

# Widgets finds its own dependencies.
find_anyproject(Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
if(WITH_Qt5_EXTERNAL)
    find_anyproject(ZLIB DEFAULT ON)
    if(ZLIB_FOUND AND WITH_ZLIB_EXTERNAL)
        # ZLIB needed for uic, rcc, lrelease and lupdate
        get_target_property(ZLIB_IMPORTED_LOCATION ${ZLIB_LIBRARIES} IMPORTED_LOCATION_RELEASE)
        get_target_property(RCC_PATH Qt5::rcc IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR ${RCC_PATH} PATH)
        if(WIN32)
            get_filename_component(PATH_DIR ${ZLIB_IMPORTED_LOCATION} PATH)
            set(ENV{PATH} "$ENV{PATH};${PATH_DIR}")
            if(EXISTS ${ZLIB_IMPORTED_LOCATION})
                file(COPY ${ZLIB_IMPORTED_LOCATION} DESTINATION ${QT_BIN_DIR})
            endif()
        elseif(OSX_FRAMEWORK)
            get_filename_component(PATH_DIR ${ZLIB_IMPORTED_LOCATION} PATH)
            get_filename_component(PATH_DIR ${PATH_DIR} PATH)
            get_filename_component(PATH_DIR ${PATH_DIR} PATH)
            if(EXISTS ${PATH_DIR})
                file(COPY ${PATH_DIR} DESTINATION ${QT_BIN_DIR}/Frameworks)
            endif()
        endif()
    endif()
endif()
find_anyproject(NGSTD REQUIRED COMPONENTS core5 framework5)

if(CMAKE_VERSION VERSION_LESS "3.1")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif()
else()
    set(CMAKE_CXX_STANDARD 11)
endif()

qt5_add_resources(RESOURCES res/manuscript.qrc)

set(PROJECT_HEADERS
    application.h
    mainwindow.h
    version.h
    panes.h
    project.h
    centralwidget.h
    navigationwidget.h
    bottomwidget.h
    welcomewidget.h
    tooltip.h
)

set(PROJECT_SOURCES
    application.cpp
    main.cpp
    mainwindow.cpp
    panes.cpp
    project.cpp
    centralwidget.cpp
    navigationwidget.cpp
    bottomwidget.cpp
    welcomewidget.cpp
    tooltip.cpp
)

set(DLG_HEADERS
    dialogs/firstlaunchdlg.h
    dialogs/addarticledlg.h
    dialogs/preferences.h
)

set(DLG_SOURCES
    dialogs/firstlaunchdlg.cpp
    dialogs/addarticledlg.cpp
    dialogs/preferences.cpp
)

set(EDITOR_HEADERS
    editor/editorwidget.h
    editor/texteditorwidget.h
    editor/rsthighlighter.h
    editor/highlightscrollbarcontroller.h
)

set(EDITOR_SOURCES
    editor/editorwidget.cpp
    editor/texteditorwidget.cpp
    editor/rsthighlighter.cpp
    editor/highlightscrollbarcontroller.cpp
)

set(LOGS_HEADERS
    logs/outputwidget.h
    logs/ansiescapecodehandler.h
)

set(LOGS_SOURCES
    logs/outputwidget.cpp
    logs/ansiescapecodehandler.cpp
)

set(GIT_HEADERS
    git/importfromgitwizard.h
    git/repopathwizardpage.h
    git/finishwizardpage.h
    git/submoduleswizardpage.h
    git/brancheswizardpage.h
)

set(GIT_SOURCES
    git/importfromgitwizard.cpp
    git/repopathwizardpage.cpp
    git/finishwizardpage.cpp
    git/submoduleswizardpage.cpp
    git/brancheswizardpage.cpp
)

set(PROJECT_HEADERS
    ${PROJECT_HEADERS} ${DLG_HEADERS} ${EDITOR_HEADERS} ${LOGS_HEADERS}
    ${GIT_HEADERS}
)

set(PROJECT_SOURCES
    ${PROJECT_SOURCES} ${DLG_SOURCES} ${EDITOR_SOURCES} ${LOGS_SOURCES}
    ${GIT_SOURCES}
)

qt5_wrap_cpp(MOC_SRCS ${PROJECT_HEADERS})

set(UI_SOURCES
    dialogs/firstlaunchdlg.ui
    dialogs/addarticledlg.ui
    dialogs/preferences.ui
    git/repopathwizardpage.ui
    git/finishwizardpage.ui
    git/submoduleswizardpage.ui
    git/brancheswizardpage.ui
)

qt5_wrap_ui(UI_HEADERS
    ${UI_SOURCES}
)

set(TRANSLATIONS
    ${PROJECT_SOURCE_DIR}/translations/${PROJECT_NAME}_ru.ts
    ${PROJECT_SOURCE_DIR}/translations/${PROJECT_NAME}_en.ts
)

unset(QM_FILES)
foreach(TRANSLATION ${TRANSLATIONS})
    if(EXISTS ${TRANSLATION})
        qt5_add_translation(QM_FILES ${TRANSLATION})
    endif()
endforeach()

if(MSVC)
    set(PROJECT_SOURCES ${PROJECT_SOURCES} res/manuscript.rc)
endif()

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${MOC_SRCS} ${PROJECT_HEADERS} ${PROJECT_SOURCES} ${RESOURCES} ${UI_HEADERS} ${QM_FILES})

if(OSX_FRAMEWORK)
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../../../../Library/Frameworks;@executable_path/../Frameworks")

    set_property(SOURCE ${QM_FILES} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/translations")
endif()

target_link_libraries(${PROJECT_NAME} Qt5::Widgets ${NGSTD_LIBRARIES})

configure_file(${PROJECT_SOURCE_DIR}/cmake/manuscript.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/manuscript.desktop @ONLY)

if(NOT SKIP_INSTALL_EXECUTABLES AND NOT SKIP_INSTALL_ALL )
    install(TARGETS ${PROJECT_NAME}
    #    EXPORT ${PROJECT_NAME}
        RUNTIME DESTINATION ${INSTALL_BIN_DIR} COMPONENT applications
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR} COMPONENT applications
        LIBRARY DESTINATION ${INSTALL_LIB_DIR} COMPONENT applications
        FRAMEWORK DESTINATION ${INSTALL_LIB_DIR}
        BUNDLE DESTINATION ${INSTALL_BIN_DIR}
    )
endif()

if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
    install(FILES ${QM_FILES} DESTINATION ${INSTALL_TRANSLATIONS_DIR})
endif()

if(UNIX AND NOT OSX_FRAMEWORK)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/art/manuscript_logo.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/manuscript.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()
