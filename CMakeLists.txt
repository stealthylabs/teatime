cmake_minimum_required(VERSION 3.0)
if (WIN32)
    set(CMAKE_BUILD_TYPE "Debug")
    if (CMAKE_CL_64)
        set(WINARCH "x64")
    else (CMAKE_CL_64)
        set(WINARCH "Win32")
    endif (CMAKE_CL_64)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MDd")
    else (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MD")
    endif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(NUGET_URL https://dist.nuget.org/win-x86-commandline/latest/nuget.exe)
    set(NUGET_EXE ${CMAKE_CURRENT_BINARY_DIR}/nuget.exe)
    if (NOT EXISTS ${NUGET_EXE})
        file(DOWNLOAD ${NUGET_URL} ${NUGET_EXE} STATUS NUGET_DL_STATUS
            SHOW_PROGRESS TLS_VERIFY ON)
    endif (NOT EXISTS ${NUGET_EXE})
    if (EXISTS ${NUGET_EXE})
        ## Freeglut
        execute_process(COMMAND ${NUGET_EXE} install freeglut -NonInteractive -ExcludeVersion
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        RESULT_VARIABLE FREEGLUT_NUGET
                        OUTPUT_QUIET)
        if ("${FREEGLUT_NUGET}" STREQUAL "0")
            message(STATUS "Freeglut downloaded from Nuget successfully")
        else ("${FREEGLUT_NUGET}" STREQUAL "0")
            message(STATUS "Freeglut download from Nuget failed: ${FREEGLUT_NUGET}")
        endif ("${FREEGLUT_NUGET}" STREQUAL "0")
        set(FREEGLUT_INCLUDES
            ${CMAKE_CURRENT_BINARY_DIR}/freeglut/build/native/include)
        include_directories(${FREEGLUT_INCLUDES})
        set(FREEGLUT_LIB_DIR
            ${CMAKE_CURRENT_BINARY_DIR}/freeglut/build/native/lib/v110/${WINARCH}/${CMAKE_BUILD_TYPE}/dynamic)
        set(FREEGLUT_DLL
            ${CMAKE_CURRENT_BINARY_DIR}/freeglut.redist/build/native/bin/v110/${WINARCH}/${CMAKE_BUILD_TYPE}/dynamic/freeglut.dll)
        link_directories(${FREEGLUT_LIB_DIR})
        set(FREEGLUT_LIB freeglut)
        ## Glew
        execute_process(COMMAND ${NUGET_EXE} install glew -NonInteractive -ExcludeVersion
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        RESULT_VARIABLE GLEW_NUGET
                        OUTPUT_QUIET)
        if ("${GLEW_NUGET}" STREQUAL "0")
            message(STATUS "Glew downloaded from Nuget successfully")
        else ("${GLEW_NUGET}" STREQUAL "0")
            message(STATUS "Glew download from Nuget failed: ${GLEW_NUGET}")
        endif ("${GLEW_NUGET}" STREQUAL "0")
        set(GLEW_INCLUDES
            ${CMAKE_CURRENT_BINARY_DIR}/glew/build/native/include)
        include_directories(${GLEW_INCLUDES})
        set(GLEW_LIB_DIR
            ${CMAKE_CURRENT_BINARY_DIR}/glew/build/native/lib/v110/${WINARCH}/${CMAKE_BUILD_TYPE}/dynamic)
        set(GLEW_DLL
            ${CMAKE_CURRENT_BINARY_DIR}/glew.redist/build/native/bin/v110/${WINARCH}/${CMAKE_BUILD_TYPE}/dynamic/glew.dll)
        link_directories(${GLEW_LIB_DIR})
        set(GLEW_LIB glew)
    else (EXISTS ${NUGET_EXE})
        message(FATAL_ERROR "Failed to download Nuget from ${NUGET_URL}")
    endif (EXISTS ${NUGET_EXE})
    find_package(OpenGL)
    if (NOT OPENGL_FOUND)
        message(FATAL_ERROR "OpenGL is not found")
    endif (NOT OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    add_executable(teatime teatime.c teapot.c)
    target_link_libraries(teatime ${FREEGLUT_LIB} ${GLEW_LIB} ${OPENGL_LIBRARIES})
    install(TARGETS teatime RUNTIME DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)
    install(PROGRAMS ${GLEW_DLL} ${FREEGLUT_DLL} DESTINATION
        ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif (WIN32)
