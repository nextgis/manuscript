#***************************************************************************
# Project:  NextGIS Manuscript
# Purpose:  cmake script
# Author:   Dmitry Baryshnikov, dmitry.baryshnikov@nextgis.com
#***************************************************************************
#   Copyright (C) 2018 NextGIS, info@nextgis.com
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

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)

set(TRANSLATION_MESSAGES_FULL)
set(TRANSLATIONS
    ${PROJECT_NAME}_ru.ts
    ${PROJECT_NAME}_en.ts
)

file(GLOB_RECURSE TRANSLATION_CPP ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE TRANSLATION_H ${PROJECT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE TRANSLATION_UI ${PROJECT_SOURCE_DIR}/src/*.ui)
set(FILES_TO_TRANSLATE ${TRANSLATION_H} ${TRANSLATION_CPP} ${TRANSLATION_UI})

if(FILES_TO_TRANSLATE AND TRANSLATION_UPDATE)
    find_anyproject(Qt5 REQUIRED COMPONENTS Core LinguistTools)
    qt5_create_translation(TRANSLATION_MESSAGES ${FILES_TO_TRANSLATE} ${TRANSLATIONS})
    set(TRANSLATION_MESSAGES_FULL ${TRANSLATION_MESSAGES_FULL} ${TRANSLATION_MESSAGES})

    add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES_FULL})
endif()
