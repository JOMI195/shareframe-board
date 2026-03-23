option(ENABLE_EPD_HARDWARE "Build with e-Paper display hardware support" OFF)

if(ENABLE_EPD_HARDWARE)
    add_library(waveshare_epd STATIC
        EPD_7in5_V2.c
        DEV_Config.c
        RPI_gpiod.c
        dev_hardware_SPI.c
        sysfs_gpio.c
        sysfs_software_spi.c
    )

    target_include_directories(waveshare_epd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_compile_definitions(waveshare_epd
        PUBLIC EPD_HARDWARE_ENABLED
        PRIVATE RPI USE_DEV_LIB
    )

    find_library(GPIOD_LIBRARY gpiod)
    if(GPIOD_LIBRARY)
        target_link_libraries(waveshare_epd PUBLIC ${GPIOD_LIBRARY})
    endif()
else()
    add_library(waveshare_epd INTERFACE)
    target_include_directories(waveshare_epd INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
