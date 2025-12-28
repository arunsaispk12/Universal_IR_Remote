# Partition Table Configuration Guide

This project supports multiple flash sizes with optimized partition tables for each configuration.

## Available Partition Tables

### 1. **partitions_4MB.csv** (Default)
- **Flash Size:** 4MB (ESP32, ESP32-C3, ESP32-S3)
- **OTA Partitions:** 1920KB each (1.875 MB)
- **IR Storage:** 140KB (~350 IR codes)
- **Additional Storage:** None (maximizes OTA size)

**Layout:**
```
- nvs: 24KB (system NVS)
- otadata: 8KB (OTA selection)
- phy_init: 4KB (RF calibration)
- fctry: 4KB (factory NVS, readonly)
- ota_0: 1920KB (app partition 0)
- ota_1: 1920KB (app partition 1)
- nvs_key: 4KB (NVS encryption)
- rmaker: 20KB (RainMaker storage)
- ir_storage: 140KB (IR codes - dedicated)
```

### 2. **partitions_8MB.csv**
- **Flash Size:** 8MB (ESP32-S3, ESP32-WROVER)
- **OTA Partitions:** 3072KB each (3 MB)
- **IR Storage:** 256KB (~640 IR codes)
- **Additional Storage:** 1340KB SPIFFS (logs, firmware backup)

**Layout:**
```
- nvs: 24KB
- otadata: 8KB
- phy_init: 4KB
- fctry: 4KB (readonly)
- ota_0: 3072KB (app partition 0)
- ota_1: 3072KB (app partition 1)
- nvs_key: 4KB
- rmaker: 32KB (increased for 8MB)
- ir_storage: 256KB (IR codes - dedicated)
- storage: 1340KB (SPIFFS for user data)
```

### 3. **partitions_16MB.csv**
- **Flash Size:** 16MB (ESP32-S3 with PSRAM)
- **OTA Partitions:** 4096KB each (4 MB)
- **IR Storage:** 512KB (~1280 IR codes)
- **Additional Storage:** 7276KB SPIFFS (extensive logging, media files)

**Layout:**
```
- nvs: 24KB
- otadata: 8KB
- phy_init: 4KB
- fctry: 4KB (readonly)
- ota_0: 4096KB (app partition 0)
- ota_1: 4096KB (app partition 1)
- nvs_key: 4KB
- rmaker: 48KB (increased for 16MB)
- ir_storage: 512KB (IR codes - dedicated)
- storage: 7276KB (SPIFFS for extensive data)
```

## How to Switch Partition Tables

### Method 1: Update partitions.csv (Recommended for Build)

Copy the desired partition table to `partitions.csv`:

```bash
# For 4MB flash (default)
cp partitions_4MB.csv partitions.csv

# For 8MB flash
cp partitions_8MB.csv partitions.csv

# For 16MB flash
cp partitions_16MB.csv partitions.csv
```

Then build normally:
```bash
idf.py build
idf.py flash
```

### Method 2: Use Custom Partition Table in menuconfig

```bash
idf.py menuconfig
```

Navigate to:
```
Partition Table →
  Partition Table → Custom partition table CSV
  Custom partition CSV file → (change to partitions_8MB.csv or partitions_16MB.csv)
```

Save and exit, then build:
```bash
idf.py build
idf.py flash
```

### Method 3: Command Line Override

Specify partition table during build:
```bash
idf.py -D PARTITION_CSV_FILE=partitions_8MB.csv build
```

## Verifying Your Flash Size

To check your module's flash size:

```bash
esptool.py flash_id
```

Look for the line showing flash size (e.g., "Detected flash size: 8MB").

## Important Notes

### IR Storage Partition
All partition tables include a dedicated `ir_storage` partition that:
- **Survives OTA updates** - IR codes are preserved when updating firmware
- **Uses NVS wear leveling** - Automatic flash wear management
- **Scales with flash size** - More IR codes on larger flash
- **Accessed via** `nvs_open_from_partition("ir_storage", ...)`

### OTA Safety
The dual OTA partition scheme ensures:
- Safe firmware updates with automatic rollback on failure
- Always one working partition available
- No downtime during OTA updates

### Migration Between Flash Sizes

When migrating from one flash size to another:

1. **Backup IR codes** (if needed - they're stored in NVS)
2. **Erase flash completely:**
   ```bash
   esptool.py erase_flash
   ```
3. **Update partition table** using one of the methods above
4. **Build and flash:**
   ```bash
   idf.py build flash
   ```

### Storage Partition Usage (8MB/16MB only)

The optional SPIFFS storage partition can be used for:
- Application logs
- Firmware backup/history
- User configuration files
- Temporary data
- Media files (on 16MB)

To mount SPIFFS in your application:
```c
esp_vfs_spiffs_conf_t conf = {
    .base_path = "/storage",
    .partition_label = "storage",
    .max_files = 5,
    .format_if_mount_failed = true
};
esp_vfs_spiffs_register(&conf);
```

## Partition Table Validation

After changing partition tables, verify with:

```bash
idf.py partition-table
```

This will show the partition layout and flag any errors.
