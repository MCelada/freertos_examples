idf_component_register(SRCS "main.c" UI/screens/ui_Screen1.c
    UI/ui.c
    UI/components/ui_comp_hook.c
    UI/ui_helpers.c
                    INCLUDE_DIRS "." UI
                    REQUIRES esp_lcd esp_timer)