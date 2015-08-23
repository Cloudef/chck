find_package(Doxygen)
set_package_properties(Doxygen PROPERTIES TYPE PURPOSE "Enables documentation")

if (DOXYGEN_FOUND)
    configure_file(Doxyfile.in Doxyfile @ONLY)
    add_custom_target(doxygen
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating chck documentation with Doxygen" VERBATIM)
    message("-!- Use 'make doxygen' to generate documentation")
endif (DOXYGEN_FOUND)
