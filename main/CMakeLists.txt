idf_component_register(SRCS "main.cc"
                            "logger.cc"
                            "util.cc"
                            "hare_tortoise_clock.cc"
                            "gpio_control.cc"
                            "task.cc"
                            "ble_device.cc"
                            "clock_management_task.cc"
                            "stepper_motor_controller.cc"
                            "ble_services.cc"
                    INCLUDE_DIRS "")

component_compile_options(-Wno-error=format= -Wno-format)

