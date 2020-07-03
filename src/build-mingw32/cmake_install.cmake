# Install script for directory: /home/jeremy/Desktop/Projects/Quiver/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "ON")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/tier0/tier0.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/tier0/tier0.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tier0.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tier0.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/tier0.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vstdlib/vstdlib.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vstdlib/vstdlib.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vstdlib.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vstdlib.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vstdlib.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/inputsystem/inputsystem.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/inputsystem/inputsystem.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/inputsystem.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/inputsystem.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/inputsystem.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/launcher/launcher.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/launcher/launcher.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/launcher.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/launcher.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/launcher.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/launcher_main/quiver.exe")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/quiver.exe" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/quiver.exe")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/quiver.exe")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/engine/engine.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/engine/engine.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/engine.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/engine.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/engine.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/networksystem/networksystem.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/utils/vrad/vrad.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/utils/vrad/vrad.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vrad.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vrad.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vrad.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/utils/vrad_launcher/vrad.exe")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vrad.exe" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vrad.exe")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vrad.exe")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vphysics/vphysics.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vphysics/vphysics.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vphysics.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vphysics.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/vphysics.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/soundsystem/soundsystem.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/soundsystem/soundsystem.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/soundsystem.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/soundsystem.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/soundsystem.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/materialsystem/materialsystem.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/materialsystem/materialsystem.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/materialsystem.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/materialsystem.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/materialsystem.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/soundemittersystem/soundemittersystem.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/soundemittersystem/soundemittersystem.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/soundemittersystem.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/soundemittersystem.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/soundemittersystem.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/datacache/datacache.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/datacache/datacache.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/datacache.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/datacache.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/datacache.dll")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/game/client/hl2/client.dll.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/game/client/hl2/client.dll")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/client.dll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/client.dll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/i686-w64-mingw32-strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/client.dll")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/mathlib/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/tier0/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/tier1/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/tier2/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/tier3/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/tier4/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vstdlib/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/filesystem/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/appframework/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/launcher/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/launcher_main/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/inputsystem/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vtf/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/bitmap/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/choreoobjects/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/studiorender/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/utils/bsppack/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/materialsystem/shaderlib/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/materialsystem/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/materialsystem/shaderapiempty/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/materialsystem/shaderapigl/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/game/client/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/engine/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/networksystem/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/utils/vrad/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/utils/vrad_launcher/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/dmxloader/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vgui2/matsys_controls/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vgui2/vgui_surfacelib/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/tools/toolutils/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/particles/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vphysics/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/thirdparty/bzip2/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/thirdparty/jpeglib/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/soundsystem/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/soundemittersystem/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/datacache/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/gameui/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/thirdparty/bullet/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/materialsystem/shaderapidx9/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vgui2/vgui_controls/cmake_install.cmake")
  include("/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/vgui2/src/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/jeremy/Desktop/Projects/Quiver/src/build-mingw32/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
