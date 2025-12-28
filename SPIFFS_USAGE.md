# SPIFFS Storage Usage Guide

This guide explains how to use the SPIFFS storage partition available on 8MB and 16MB flash configurations.

## SPIFFS Partition Sizes

| Flash Size | SPIFFS Size | Use Cases |
|------------|-------------|-----------|
| 4MB | None | Maximizes OTA partition size |
| 8MB | 1340KB (~1.3MB) | Logs, firmware backup, configuration |
| 16MB | 7276KB (~7.1MB) | Extensive logging, media files, data storage |

## What is SPIFFS?

SPIFFS (SPI Flash File System) is a file system designed for SPI NOR flash devices:
- **Wear leveling** - Distributes writes across flash
- **Power-loss resilient** - Handles unexpected power loss
- **Simple API** - POSIX-like file operations (fopen, fwrite, etc.)
- **No directories** - Flat file structure (but can simulate with paths)

## Enabling SPIFFS in Your Application

### Step 1: Add SPIFFS Component Requirement

Edit `main/CMakeLists.txt` and add `spiffs` to REQUIRES:

```cmake
idf_component_register(
    SRCS
        "app_main.c"
        "app_wifi.c"
        "rmaker_devices.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        ir_control
        rgb_led
        esp_rainmaker
        esp_wifi
        nvs_flash
        bt
        driver
        app_update
        esp_timer
        console
        spiffs        # Add this line
    PRIV_REQUIRES
        espressif__network_provisioning
)
```

### Step 2: Initialize SPIFFS in app_main.c

Add the initialization code:

```c
#include "esp_spiffs.h"

esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",           // Mount point
        .partition_label = "storage",       // Partition name from CSV
        .max_files = 5,                     // Max open files
        .format_if_mount_failed = true      // Auto-format on first use
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info("storage", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS: Total: %d KB, Used: %d KB", total / 1024, used / 1024);
    }

    return ESP_OK;
}

void app_main(void)
{
    // ... existing initialization ...

    // Initialize SPIFFS (only on 8MB/16MB flash)
    #ifdef CONFIG_PARTITION_TABLE_CUSTOM_FILENAME
    if (strstr(CONFIG_PARTITION_TABLE_CUSTOM_FILENAME, "8MB") ||
        strstr(CONFIG_PARTITION_TABLE_CUSTOM_FILENAME, "16MB")) {
        init_spiffs();
    }
    #endif

    // ... rest of app_main ...
}
```

### Step 3: Using SPIFFS - File Operations

After initialization, use standard C file operations:

#### Write a File
```c
void write_log_file(const char *message)
{
    FILE *f = fopen("/storage/system.log", "a");  // Append mode
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "[%lu] %s\n", esp_log_timestamp(), message);
    fclose(f);
    ESP_LOGI(TAG, "Log written to SPIFFS");
}
```

#### Read a File
```c
void read_config_file(void)
{
    FILE *f = fopen("/storage/config.json", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f) != NULL) {
        // Process line
        printf("%s", line);
    }

    fclose(f);
}
```

#### Delete a File
```c
void delete_old_logs(void)
{
    if (remove("/storage/old.log") == 0) {
        ESP_LOGI(TAG, "File deleted successfully");
    } else {
        ESP_LOGE(TAG, "Unable to delete file");
    }
}
```

#### Check if File Exists
```c
bool file_exists(const char *filename)
{
    struct stat st;
    char path[64];
    snprintf(path, sizeof(path), "/storage/%s", filename);
    return (stat(path, &st) == 0);
}
```

## Recommended Use Cases

### For 8MB Flash (1.3MB SPIFFS)

**1. System Logs**
```c
void log_ir_learning_event(const char *device, const char *action, bool success)
{
    FILE *f = fopen("/storage/ir_history.log", "a");
    if (f) {
        fprintf(f, "%lu,%s,%s,%s\n",
                esp_log_timestamp(), device, action,
                success ? "SUCCESS" : "FAILED");
        fclose(f);
    }
}
```

**2. Crash Dumps**
```c
void save_crash_info(const char *reason)
{
    FILE *f = fopen("/storage/crash.log", "w");
    if (f) {
        fprintf(f, "Crash Time: %lu\n", esp_log_timestamp());
        fprintf(f, "Reason: %s\n", reason);
        fprintf(f, "Free Heap: %d\n", esp_get_free_heap_size());
        fclose(f);
    }
}
```

**3. Firmware Update Backup**
```c
void backup_firmware_version(void)
{
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();
    FILE *f = fopen("/storage/fw_version.txt", "w");
    if (f) {
        fprintf(f, "Version: %s\n", app_desc->version);
        fprintf(f, "Date: %s %s\n", app_desc->date, app_desc->time);
        fclose(f);
    }
}
```

### For 16MB Flash (7.1MB SPIFFS)

All of the above, PLUS:

**4. IR Code Export/Import**
```c
// Export learned IR codes to file for backup
esp_err_t export_ir_codes_to_file(void)
{
    FILE *f = fopen("/storage/ir_codes_backup.bin", "wb");
    if (!f) return ESP_FAIL;

    // Export all IR codes from NVS to file
    for (int i = 0; i < IR_BTN_MAX; i++) {
        ir_code_t code;
        if (ir_load_code(i, &code) == ESP_OK) {
            fwrite(&code, sizeof(ir_code_t), 1, f);
            if (code.raw_data) {
                fwrite(code.raw_data, sizeof(uint16_t), code.raw_length, f);
            }
        }
    }

    fclose(f);
    return ESP_OK;
}
```

**5. Statistical Data**
```c
void save_usage_statistics(void)
{
    FILE *f = fopen("/storage/stats.json", "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "  \"uptime\": %lu,\n", esp_log_timestamp() / 1000);
        fprintf(f, "  \"ir_transmissions\": %d,\n", get_ir_tx_count());
        fprintf(f, "  \"wifi_reconnects\": %d,\n", get_wifi_reconnect_count());
        fprintf(f, "  \"free_heap\": %d\n", esp_get_free_heap_size());
        fprintf(f, "}\n");
        fclose(f);
    }
}
```

**6. Firmware Download Cache**
- Store downloaded OTA firmware before flashing
- Allows resume on interrupted downloads
- Validate before flashing

**7. Web Server Assets**
```c
// Serve configuration web UI from SPIFFS
void serve_web_interface(void)
{
    FILE *f = fopen("/storage/index.html", "r");
    if (f) {
        // Read and serve HTML
        fclose(f);
    }
}
```

## Important Considerations

### 1. SPIFFS Limitations

- **No directories** - Flat file structure (use "/" in filenames to simulate paths)
- **No timestamps** - Files don't track creation/modification time automatically
- **Wear leveling overhead** - ~10-15% of partition size used for wear leveling
- **Not ideal for frequent writes** - Better for read-mostly or append-only workloads

### 2. Performance

- **Read speed** - Good (~1 MB/s)
- **Write speed** - Moderate (~100-300 KB/s)
- **Best for** - Small to medium files (<1MB each)

### 3. Safety

```c
// Always check return values
FILE *f = fopen("/storage/data.txt", "w");
if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file");
    return;
}

// Always close files
fclose(f);

// Sync to flash if critical
fflush(f);  // Before closing
```

### 4. Cleanup

To unmount SPIFFS (before OTA update or shutdown):
```c
esp_vfs_spiffs_unregister("storage");
```

To format SPIFFS (erase all data):
```c
esp_spiffs_format("storage");
```

## Checking Available Space

```c
void check_spiffs_space(void)
{
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info("storage", &total, &used);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS Total: %d KB", total / 1024);
        ESP_LOGI(TAG, "SPIFFS Used: %d KB", used / 1024);
        ESP_LOGI(TAG, "SPIFFS Free: %d KB", (total - used) / 1024);
        ESP_LOGI(TAG, "SPIFFS Usage: %.1f%%", (100.0 * used) / total);
    }
}
```

## Sample: Rotating Log Files

```c
#define MAX_LOG_SIZE (100 * 1024)  // 100KB

void write_rotating_log(const char *message)
{
    struct stat st;
    if (stat("/storage/current.log", &st) == 0 && st.st_size > MAX_LOG_SIZE) {
        // Rotate logs
        remove("/storage/old.log");
        rename("/storage/current.log", "/storage/old.log");
    }

    FILE *f = fopen("/storage/current.log", "a");
    if (f) {
        fprintf(f, "[%lu] %s\n", esp_log_timestamp(), message);
        fclose(f);
    }
}
```

## Integration with ESP RainMaker

You can expose SPIFFS files via RainMaker for remote access:

```c
// Create a custom parameter to upload logs
esp_rmaker_param_t *log_param = esp_rmaker_param_create(
    "logs", NULL, esp_rmaker_str(""), PROP_FLAG_READ);
esp_rmaker_device_add_param(device, log_param);

// Update parameter with log content
void update_log_parameter(void)
{
    FILE *f = fopen("/storage/current.log", "r");
    if (f) {
        char buffer[1024];
        size_t read = fread(buffer, 1, sizeof(buffer)-1, f);
        buffer[read] = '\0';
        esp_rmaker_param_update_and_report(log_param, esp_rmaker_str(buffer));
        fclose(f);
    }
}
```

## Summary

For **16MB flash**, the 7.1MB SPIFFS partition provides significant storage for:
- ✅ Comprehensive logging and diagnostics
- ✅ IR code backup/restore functionality
- ✅ Firmware download cache
- ✅ Web interface assets
- ✅ Statistical data collection
- ✅ User configuration files

Just remember to:
1. Add `spiffs` to CMakeLists.txt REQUIRES
2. Initialize SPIFFS in app_main.c
3. Use standard file operations with `/storage/` prefix
4. Always check file operation return values
5. Close files after use

The SPIFFS partition makes 16MB flash extremely versatile for advanced features!
