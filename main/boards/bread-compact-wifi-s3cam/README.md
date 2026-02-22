The hardware is based on the ESP32S3CAM development board, and the code is adapted from bread-compact-wifi-lcd.
The camera used is the OV2640.
Note: Since the camera uses many IOs, it occupies both USB 19 and 20 pins on the ESP32S3.
For wiring details, refer to the pin definitions in the config.h file.

# Build Configuration Commands

**Set build target to ESP32S3:**

```bash
idf.py set-target esp32s3
```

**Open menuconfig:**

```bash
idf.py menuconfig
```

**Select Board:**

```bash
Xiaozhi Assistant -> Board Type -> Breadboard New Wiring (WiFi) + LCD + Camera
```

**Configure Camera Sensor:**

> **Note:** Confirm the camera sensor model and ensure it is supported by esp_cam_sensor. The current board uses the OV2640, which is supported.

To enable support for the corresponding sensor model in menuconfig, follow these steps:

1. **Navigate to the sensor configuration:**

   ```
   (Top) → Component config → Espressif Camera Sensors Configurations → Camera Sensor Configuration → Select and Set Camera Sensor
   ```

2. **Select the sensor model:**

   - Choose the required sensor model (OV2640)

3. **Configure sensor parameters:**
   - Press → to enter the detailed sensor settings
   - Enable **Auto detect**
   - It is recommended to set the **default output format** to **YUV422** and select an appropriate resolution
   - (Currently YUV422 and RGB565 are supported; YUV422 saves more memory)

**Build and Flash Firmware:**

```bash
idf.py build flash
```
