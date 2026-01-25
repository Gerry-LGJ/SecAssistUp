cmake_minimum_required(VERSION 3.16)

project(
    SecAssistUp
    DESCRIPTION "SecAssistUp"
    VERSION     0.1
    LANGUAGES   CXX
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Sql WinExtras)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Sql WinExtras)

# ########## config git tools ##########
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/.cmake/)
include(GetGitRevisionDescription)


# ########## Set the output directory for the executable file ##########
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
if (APPLE)
    set(APPLICATION_DIR_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/MacOS)
else ()
    set(APPLICATION_DIR_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif ()


# ########## When compiling with MSVC, pdb files are generated simultaneously. ##########
if (MSVC)
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif ()


# ########## Configurate Directory ##########
set(SRC_DIR src)
set(RES_DIR res)


# ########## config source file ##########
file(GLOB COMMON_SRC
    ${SRC_DIR}/common/*.h
    ${SRC_DIR}/common/*.cpp
)
file(GLOB HELPER_SRC
    ${SRC_DIR}/helper/*.h
    ${SRC_DIR}/helper/*.cpp
)
file(GLOB WINDOW_SRC
    ${SRC_DIR}/window/*.h
    ${SRC_DIR}/window/*.cpp
    ${SRC_DIR}/window/*.ui
)
file(GLOB WIDGETS_SRC
    ${SRC_DIR}/widgets/*.h
    ${SRC_DIR}/widgets/*.cpp
    ${SRC_DIR}/widgets/*.ui
)
file(GLOB SERVICE_SRC
    ${SRC_DIR}/service/*.h
    ${SRC_DIR}/service/*.cpp
)
set(RES_SRC ${RES_DIR}/${PROJECT_NAME}.qrc)
set(HEADER_FILE_VERSION_PATH ${CMAKE_SOURCE_DIR}/${SRC_DIR}/version.h)
configure_file(
    ${CMAKE_SOURCE_DIR}/.cmake/version.h.in
    ${HEADER_FILE_VERSION_PATH}
)
# Generate the RC file and the Inno Setup script through the template file.
set(APP_VERSION_RC_PATH "")
if (WIN32)
    set(APP_VERSION_RC_PATH ${CMAKE_CURRENT_BINARY_DIR}/version_${PROJECT_NAME}.rc)
    configure_file(
        ${CMAKE_SOURCE_DIR}/.cmake/version_exe.rc.in
        ${APP_VERSION_RC_PATH}
    )
    configure_file(
        ${CMAKE_SOURCE_DIR}/.cmake/InstallerScript.iss.in
        ${CMAKE_SOURCE_DIR}/package/InstallerScript.iss
    )
endif ()


# ########## config group file ##########
if (${CMAKE_VERSION} VERSION_LESS 3.8)
source_group(Common  FILES ${COMMON_SRC})
source_group(Window  FILES ${WINDOW_SRC})
source_group(Helper  FILES ${HELPER_SRC})
source_group(Service FILES ${SERVICE_SRC})
source_group(Widgets FILES ${WIDGETS_SRC})
else ()
file(GLOB GROUP_FILES
    ${SRC_DIR}/main.cpp
    ${COMMON_SRC}
    ${HEADER_FILE_VERSION_PATH}
    ${HELPER_SRC}
    ${SERVICE_SRC}
    ${WINDOW_SRC}
    ${WIDGETS_SRC}
)
message("GROUP_FILES :")
foreach (filepath ${GROUP_FILES})
    message(${filepath})
endforeach ()
message("GROUP_FILES :(END)")
source_group(
    TREE   ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_DIR}
    PREFIX ${PROJECT_NAME}
    FILES  ${GROUP_FILES}
)
endif ()


# ########## config project sources ##########
set(PROJECT_SOURCES
        ${SRC_DIR}/main.cpp
        ${HEADER_FILE_VERSION_PATH}
        ${WINDOW_SRC}
        ${HELPER_SRC}
        ${SERVICE_SRC}
        ${RES_SRC}
        ${APP_VERSION_RC_PATH}
        ${WIDGETS_SRC}
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET SecAssistUp APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# ########## Release also supports logging of the code location. ##########
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    QT_MESSAGELOGCONTEXT
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::WinExtras
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${PROJECT_NAME})
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()


#添加部署脚本
message(STATUS "CMAKE_COMMAND=${CMAKE_COMMAND}")
message(STATUS "PROJECT_NAME=${PROJECT_NAME}")
message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "APPLICATION_DIR_PATH=${APPLICATION_DIR_PATH}")
if (CMAKE_BUILD_TYPE MATCHES "Release")
    if (APPLE)
        find_program(QT_DEPLOY_QT NAMES macdeployqt)
        add_custom_target(Script-DeployRelease ALL
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/dist
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${CMAKE_SOURCE_DIR}/dist
                COMMAND ${QT_DEPLOY_QT} ${CMAKE_SOURCE_DIR}/dist/${PROJECT_NAME}.app -qmldir=${CMAKE_CURRENT_LIST_DIR}
                COMMENT "MacOs Deploying Qt Dependencies After Build........."
                SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif ()
    if (WIN32)
        find_program(QT_DEPLOY_QT NAMES windeployqt)
        add_custom_target(Script-DeployRelease ALL
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/dist
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${CMAKE_SOURCE_DIR}/dist
                COMMAND ${QT_DEPLOY_QT} --no-translations --compiler-runtime ${CMAKE_SOURCE_DIR}/dist/${PROJECT_NAME}.exe
                COMMENT "Windows Deploying Qt Dependencies After Build........."
                SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS ${PROJECT_NAME}
        )
    endif ()
endif ()
