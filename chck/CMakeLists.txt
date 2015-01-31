set(modules buffer pool dl fs sjis xdg)

foreach (module ${modules})
   add_subdirectory(${module})
endforeach (module)
