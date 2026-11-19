file(GLOB_RECURSE ARCQML_CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE ARCQML_HPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "*.hpp")
file(GLOB_RECURSE ARCQML_QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "*.qml")
file(GLOB_RECURSE ARCQML_SVG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "res/*.svg")

set(QT_FORCE_CMP0156_TO_VALUE NEW)
find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.11)

qt_add_library(ArcQml STATIC)
qt_add_qml_module(ArcQml
  URI ArcQml
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ArcQml
  QML_FILES ${ARCQML_QML_FILES}
  SOURCES ${ARCQML_CPP_FILES} ${ARCQML_HPP_FILES}
  RESOURCES ${ARCQML_SVG_FILES}
)

target_link_libraries(ArcQml
  PUBLIC arca::lua
  PRIVATE Qt6::Gui Qt6::Quick Qt6::QuickControls2
)

# Its headers are included as "arcqml/...", from the root of arca. The type registrations Qt
# generates include them by file name, so each folder is an include path of the module too.
file(GLOB ARCQML_SUBDIRS LIST_DIRECTORIES true CONFIGURE_DEPENDS "*")
list(FILTER ARCQML_SUBDIRS EXCLUDE REGEX "\\.[^/]*$")
target_include_directories(ArcQml PUBLIC ${PROJECT_SOURCE_DIR} PRIVATE ${ARCQML_SUBDIRS})
