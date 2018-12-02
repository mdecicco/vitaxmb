cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(SHORT_NAME VitaXmb)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "VitaXmb")
set(VITA_TITLEID  "SHELL6969")
SET(PSVITAIP 192.168.1.16)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive -Wno-psabi")
set(PSP2CGC /usr/local/vitasdk/bin/psp2cgc.exe)

include_directories(
  ./src
)

file(GLOB_RECURSE VERTEX_SHADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    resources/shaders/*_v.cg
    resources/shaders/*_vertex.cg
)
file(GLOB_RECURSE FRAGMENT_SHADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    resources/shaders/*_f.cg
    resources/shaders/*_fragment.cg
)

foreach(shader ${VERTEX_SHADERS})
	get_filename_component(shader_we ${shader} NAME_WE)
	add_custom_command(OUTPUT "${shader_we}.gxp"
		COMMAND ${PSP2CGC} -profile sce_vp_psp2 "${PROJECT_SOURCE_DIR}/${shader}" -Wperf -fastprecision -O3 -o "build/${shader_we}.gxp"
		DEPENDS ${shader}
		COMMENT "Compiling ${shader} to ${shader_we}.gxp"
	)
	list(APPEND SHADER_GXPS "${shader_we}.gxp")
endforeach()

foreach(shader ${FRAGMENT_SHADERS})
	get_filename_component(shader_we ${shader} NAME_WE)
	add_custom_command(OUTPUT "${shader_we}.gxp"
		COMMAND ${PSP2CGC} -profile sce_fp_psp2 "${PROJECT_SOURCE_DIR}/${shader}" -Wperf -fastprecision -O3 -o "build/${shader_we}.gxp"
		DEPENDS ${shader}
		COMMENT "Compiling ${shader} to ${shader_we}.gxp"
	)
	list(APPEND SHADER_GXPS "${shader_we}.gxp")
endforeach()

file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    src/*.cpp
    src/system/*.cpp
    src/common/*.cpp
    src/tools/*.cpp
    src/rendering/*.cpp
)

add_executable(${SHORT_NAME}
    ${SOURCE_FILES}
    ${SHADER_GXPS}
    ${SHORT_NAME}.vpk
)

target_link_libraries(${SHORT_NAME}
    debugnet
    freetype
    png
    SceAppMgr_stub
    SceAppUtil_stub
    SceCtrl_stub
    SceDisplay_stub
    SceGxm_stub
    SceNet_stub
    SceNetCtl_stub
    ScePower_stub
    SceShellSvc_stub
    SceSysmodule_stub
    vitaGL
    z
)


file(GLOB_RECURSE SHADER_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    build/*.gxp
)
file(GLOB_RECURSE ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
     resources/icons/*.png
)
file(GLOB_RECURSE FONT_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
     resources/fonts/*.ttf
)
file(GLOB_RECURSE CONFIG_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
     resources/config/*.json
)

set(RESOURCES "")
SET(FONT_CURL_COMMANDS "")
SET(ICON_CURL_COMMANDS "")
SET(SHADER_CURL_COMMANDS "")
SET(CONFIG_CURL_COMMANDS "")
foreach(file ${SHADER_FILES})
    get_filename_component(filename ${file} NAME)
    list(APPEND RESOURCES "FILE")
    list(APPEND RESOURCES ${file})
    list(APPEND RESOURCES resources/shaders/${filename})
    
    list(APPEND SHADER_CURL_COMMANDS "COMMAND")
    list(APPEND SHADER_CURL_COMMANDS curl -s -T ${file} ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/resources/shaders/${filename})
endforeach()
foreach(file ${ICON_FILES})
    get_filename_component(filename ${file} NAME)
    list(APPEND RESOURCES "FILE")
    list(APPEND RESOURCES ${file})
    list(APPEND RESOURCES resources/icons/${filename})
    
    list(APPEND ICON_CURL_COMMANDS "COMMAND")
    list(APPEND ICON_CURL_COMMANDS curl -s -T ${file} ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/resources/icons/${filename})
endforeach()
foreach(file ${FONT_FILES})
    get_filename_component(filename ${file} NAME)
    list(APPEND RESOURCES "FILE")
    list(APPEND RESOURCES ${file})
    list(APPEND RESOURCES resources/fonts/${filename})
    
    list(APPEND FONT_CURL_COMMANDS "COMMAND")
    list(APPEND FONT_CURL_COMMANDS curl -s -T ${file} ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/resources/fonts/${filename})
endforeach()
foreach(file ${CONFIG_FILES})
    get_filename_component(filename ${file} NAME)
    list(APPEND RESOURCES "FILE")
    list(APPEND RESOURCES ${file})
    list(APPEND RESOURCES resources/config/${filename})
    
    list(APPEND CONFIG_CURL_COMMANDS "COMMAND")
    list(APPEND CONFIG_CURL_COMMANDS curl -s -T ${file} ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/resources/config/${filename})
endforeach()

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME} UNSAFE)
vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  ${RESOURCES}
)

add_custom_target(s
    COMMAND curl -s -T ${SHORT_NAME}.vpk ftp://${PSVITAIP}:1337/ux0:/data/
    COMMENT "Sending ${SHORT_NAME}.vpk to ftp://${PSVITAIP}:1337/ux0:/data/"
)
add_custom_target(u
  	COMMAND curl -s -T ${SHORT_NAME}.self ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/eboot.bin
    COMMENT "Sending ${SHORT_NAME}.self to ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/eboot.bin"
)
add_custom_target(uc
  	${CONFIG_CURL_COMMANDS}
    COMMENT "Sending config files to ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/resources/config/"
)
add_custom_target(uf
    ${FONT_CURL_COMMANDS}
    COMMENT "Sending fonts to ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/resources/fonts/"
)
add_custom_target(ui
    ${ICON_CURL_COMMANDS}
    COMMENT "Sending icons to ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/resources/icons/"
)
add_custom_target(us
    ${SHADER_CURL_COMMANDS}
    COMMENT "Sending shaders to ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/resources/shaders/"
)
add_custom_target(m
    COMMAND socat udp-recv:1996 stdout
    COMMENT "Listening for Vita..."
)
