idf_component_register(SRCS "web.c"
                    PRIV_REQUIRES esp_http_server esp_event app_update json decodervm storage
                    INCLUDE_DIRS ".;../../main"
                    EMBED_FILES "assets/index.html")
