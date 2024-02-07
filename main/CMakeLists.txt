idf_component_register(SRCS "main.cc"
                            "logger.cc"
                            "util.cc"
                            "rabbit_clock.cc"
                            "gpio_control.cc"
                            "task.cc"
                            "ble_device.cc"
                            "clock_management_task.cc"
                            "stepper_motor_task.cc"
                            "stepper_motor_util.cc"
                            "ble_service_task.cc"
                            "ble_services.cc"
                    INCLUDE_DIRS "")

component_compile_options(-Wno-error=format= -Wno-format)

