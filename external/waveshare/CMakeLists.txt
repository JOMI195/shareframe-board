option(ENABLE_EPD_HARDWARE "Build with e-Paper display hardware support" OFF)

if(ENABLE_EPD_HARDWARE)
    add_library(waveshare_epd STATIC
        EPD_7in5_V2.c
        DEV_Config.c
    )

    target_include_directories(waveshare_epd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_compile_definitions(waveshare_epd
        PUBLIC EPD_HARDWARE_ENABLED
        PRIVATE RPI USE_LGPIO_LIB
    )

    find_library(LGPIO_LIBRARY lgpio REQUIRED)
    find_path(LGPIO_INCLUDE_DIR lgpio.h REQUIRED)
    target_include_directories(waveshare_epd PRIVATE ${LGPIO_INCLUDE_DIR})
    target_link_libraries(waveshare_epd PUBLIC ${LGPIO_LIBRARY})
else()
    add_library(waveshare_epd INTERFACE)
    target_include_directories(waveshare_epd INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
