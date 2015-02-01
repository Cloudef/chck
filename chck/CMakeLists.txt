set(modules buffer pool lut atlas dl fs sjis xdg)

foreach (module ${modules})
   add_subdirectory(${module})
endforeach (module)
