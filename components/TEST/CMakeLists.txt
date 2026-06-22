set(src_dirs
            GPIO_ESP_TIME_TEST
            MCU_GPIO_SETUP
   )

set(include_dirs
            GPIO_ESP_TIME_TEST
            MCU_GPIO_SETUP
   )

set(requires
            driver
            esp_timer
            BSP
   )

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)