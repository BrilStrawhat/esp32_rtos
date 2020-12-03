idf_component_register(SRCS "creative.c" 
                    INCLUDE_DIRS "."
                    REQUIRES uart_console sh1106 clock dht11 led_cmd) 
