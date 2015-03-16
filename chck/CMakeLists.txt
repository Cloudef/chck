set(modules buffer pool lut atlas math bams dl fs sjis xdg string thread)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
foreach (module ${modules})
   add_subdirectory(${module})
endforeach (module)
