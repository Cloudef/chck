set(modules buffer pool lut atlas math bams dl fs sjis xdg string thread overflow unicode)

macro (install_headers)
   file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
   install(FILES ${ARGV} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${rel}")
endmacro ()

macro (install_libraries)
   string(REGEX MATCHALL "[0-9]+" VERSION_COMPONENTS ${PROJECT_VERSION})
   list(GET VERSION_COMPONENTS 0 SOVERSION)
   foreach (target ${ARGN})
      string(REGEX REPLACE "_" "-" name ${target})
      set_target_properties(${target} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${SOVERSION} OUTPUT_NAME ${name})
   endforeach ()
   install(TARGETS ${ARGV} DESTINATION "${CMAKE_INSTALL_LIBDIR}")
   set(CHCK_COMPONENTS ${CHCK_COMPONENTS} ${ARGV} PARENT_SCOPE)
endmacro ()

foreach (module ${modules})
   add_subdirectory(${module})
endforeach (module)

set(CHCK_COMPONENTS ${CHCK_COMPONENTS} PARENT_SCOPE)

install_headers(macros.h)
