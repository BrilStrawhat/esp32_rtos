idf_component_register(SRCS "clock.c"
                    INCLUDE_DIRS "."
                    REQUIRES sh1106 uart_console dht11)
