# Prerequisites

Install ESP-IDF toolchain: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-macos-setup.html

Set environment variables: ```. {IDF PATH}/export.sh```

# Build

```
cd esp32

# choose target: esp32 or esp32s3
idf.py set-target esp32

idf.py build flash monitor
```

# Usage

Connect to WiFi train/12345678
