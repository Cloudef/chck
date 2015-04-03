set(modules buffer pool lut atlas math bams dl fs sjis xdg string thread overflow unicode)

foreach (module ${modules})
   add_subdirectory(${module})
endforeach (module)
