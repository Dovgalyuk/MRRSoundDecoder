idf_component_register(SRCS "web.c"
                    PRIV_REQUIRES esp_http_server esp_event json decodervm storage
                    INCLUDE_DIRS ".;../../main"
                    EMBED_FILES "assets/index.html")
