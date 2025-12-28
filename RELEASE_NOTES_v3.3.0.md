# Release Notes - Version 3.3.0

**Release Date:** December 29, 2025
**Type:** Feature Release - Multi-SOC Support & Storage Optimization

## 🎯 What's New in v3.3.0

### 🔧 Multi-SOC Platform Support

**Flexible Target Architecture:**
- ✅ **ESP32** - Original platform (4MB flash)
- ✅ **ESP32-S3** - High-performance variant (4MB/8MB/16MB flash)
- ✅ **ESP32-C3** - RISC-V compact variant (4MB flash)
- ✅ **ESP32-S2** - Cost-optimized variant (4MB flash)

**No code changes needed** - Simply use `idf.py set-target <target>` to build for any platform!

### 💾 Intelligent Partition Table System

**Three Optimized Configurations:**

1. **4MB Flash** (partitions_4MB.csv - Default)
   - OTA Partitions: 1920KB each (1.875 MB)
   - IR Storage: 140KB (~350 IR codes)
   - Optimized for: ESP32, ESP32-C3, ESP32-S3 (4MB)

2. **8MB Flash** (partitions_8MB.csv)
   - OTA Partitions: 3072KB each (3 MB)
   - IR Storage: 256KB (~640 IR codes)
   - SPIFFS Storage: 1340KB (logs, backups, data)
   - Optimized for: ESP32-S3, ESP32-WROVER

3. **16MB Flash** (partitions_16MB.csv)
   - OTA Partitions: 4096KB each (4 MB)
   - IR Storage: 512KB (~1280 IR codes)
   - SPIFFS Storage: 7276KB (extensive data, media)
   - Optimized for: ESP32-S3 with PSRAM

**Key Benefits:**
- 📦 Dedicated `ir_storage` partition survives OTA updates
- 🔄 Automatic wear leveling via NVS
- 📈 Scales with flash size - more storage = more IR codes
- 🛡️ OTA-safe architecture on all configurations

### 🔐 Critical Bug Fixes

**IR Storage Isolation (CRITICAL FIX):**
- **Issue:** IR codes were stored in default NVS partition, risking data loss during OTA
- **Fix:** All IR storage now uses dedicated `ir_storage` partition via `nvs_open_from_partition()`
- **Impact:** IR codes guaranteed to survive firmware updates
- **Files Modified:**
  - `components/ir_control/ir_action.c` - Action mappings
  - `components/ir_control/ir_ac_state.c` - AC state persistence
  - `components/ir_control/ir_control.c` - Learned button codes

**Partition Table Validation:**
- Fixed `fctry` partition marked as readonly (required for <12KB NVS)
- Proper 64KB alignment for app partitions
- Proper 4KB alignment for data partitions
- All partitions validated with no overlaps

**Multi-Target Configuration:**
- Removed hardcoded ESP32 target from `CMakeLists.txt`
- Removed hardcoded ESP32 target from `sdkconfig.defaults`
- Target now set dynamically via `idf.py set-target` command

## 📋 Migration Guide

### From v3.2.x to v3.3.0

**No action required for existing 4MB ESP32 deployments!**

The default configuration remains 4MB with backward-compatible partition layout.

### Upgrading to Larger Flash

**If migrating to 8MB or 16MB flash:**

1. **Check your flash size:**
   ```bash
   esptool.py flash_id
   ```

2. **Select appropriate partition table:**
   ```bash
   # For 8MB
   cp partitions_8MB.csv partitions.csv

   # For 16MB
   cp partitions_16MB.csv partitions.csv
   ```

3. **Update flash size in menuconfig:**
   ```bash
   idf.py menuconfig
   # Navigate to: Serial flasher config → Flash size → (8MB or 16MB)
   ```

4. **Erase flash (one-time):**
   ```bash
   esptool.py erase_flash
   ```

5. **Build and flash:**
   ```bash
   idf.py build flash
   ```

**⚠️ Warning:** Changing partition tables requires erasing flash. Backup IR codes if needed.

### Building for Different SOCs

**ESP32-S3:**
```bash
idf.py set-target esp32s3
idf.py build
```

**ESP32-C3:**
```bash
idf.py set-target esp32c3
idf.py build
```

**ESP32 (default):**
```bash
idf.py set-target esp32
idf.py build
```

## 🗂️ New Documentation

- **PARTITION_TABLES.md** - Complete guide to partition table selection
- **SPIFFS_USAGE.md** - How to use SPIFFS storage (8MB/16MB only)
- **RELEASE_NOTES_v3.3.0.md** - This file

## 🔍 Technical Details

### Partition Layout Improvements

**4MB Configuration:**
```
System NVS:        24KB   (WiFi credentials, system config)
OTA Data:          8KB    (OTA selection)
PHY Init:          4KB    (RF calibration)
Factory:           4KB    (readonly calibration)
OTA_0:             1920KB (App partition 0)
OTA_1:             1920KB (App partition 1)
NVS Keys:          4KB    (Encryption keys)
RainMaker:         20KB   (Cloud storage)
IR Storage:        140KB  (Dedicated IR codes)
Total:             ~3.99 MB
```

**8MB Configuration:**
```
(Same system partitions as 4MB)
OTA_0:             3072KB (Larger app partitions)
OTA_1:             3072KB
RainMaker:         32KB   (Increased cloud storage)
IR Storage:        256KB  (More IR codes)
SPIFFS:            1340KB (File system)
Total:             ~7.99 MB
```

**16MB Configuration:**
```
(Same system partitions as 4MB)
OTA_0:             4096KB (Maximum app size)
OTA_1:             4096KB
RainMaker:         48KB   (Maximum cloud storage)
IR Storage:        512KB  (Maximum IR codes)
SPIFFS:            7276KB (Large file system)
Total:             ~15.99 MB
```

### NVS Partition Architecture

**Dedicated IR Storage Partition:**
- Isolated from system NVS
- Survives OTA updates independently
- Automatic wear leveling
- Accessed via: `nvs_open_from_partition("ir_storage", ...)`

**Namespaces within ir_storage:**
- `ir_actions` - Action-to-IR-code mappings (TV, STB, Speaker, Fan)
- `ir_ac` - AC state persistence
- `ir_codes` - Legacy button-based IR codes

### Compatibility Matrix

| SOC | Flash Size | Partition Table | Status |
|-----|------------|-----------------|--------|
| ESP32 | 4MB | partitions_4MB.csv | ✅ Tested |
| ESP32-S3 | 4MB | partitions_4MB.csv | ✅ Tested |
| ESP32-S3 | 8MB | partitions_8MB.csv | ✅ Tested |
| ESP32-S3 | 16MB | partitions_16MB.csv | ✅ Tested |
| ESP32-C3 | 4MB | partitions_4MB.csv | ⚠️ Untested |
| ESP32-S2 | 4MB | partitions_4MB.csv | ⚠️ Untested |

## 🐛 Bug Fixes

### Critical
- **IR-001:** IR codes now properly isolated in dedicated partition
- **PART-001:** Factory partition validation error fixed (readonly flag)
- **BUILD-001:** Multi-target build support fixed (removed hardcoded targets)

### Minor
- **DOC-001:** Updated all documentation for v3.3.0
- **VER-001:** Version bumped to 3.3.0

## 📊 Binary Size

**ESP32 (4MB):**
- Binary Size: ~1.79 MB
- Partition Size: 1.875 MB (1920KB)
- Headroom: ~85KB (4.5%)

**ESP32-S3 (4MB):**
- Binary Size: ~1.82 MB (slightly larger due to additional features)
- Partition Size: 1.875 MB (1920KB)
- Headroom: ~55KB (2.9%)

**ESP32-S3 (8MB):**
- Binary Size: ~1.82 MB
- Partition Size: 3 MB (3072KB)
- Headroom: ~1.18 MB (39%)

**ESP32-S3 (16MB):**
- Binary Size: ~1.82 MB
- Partition Size: 4 MB (4096KB)
- Headroom: ~2.18 MB (53%)

## 🔄 Upgrade Path

### From v3.0.0 - v3.2.1 (Same Flash Size)

**No flash erase required!**

1. Build with new version:
   ```bash
   idf.py build
   ```

2. OTA update via ESP RainMaker app, OR:
   ```bash
   idf.py flash
   ```

3. IR codes automatically migrate to `ir_storage` partition on first boot

### From v3.0.0 - v3.2.1 (Larger Flash Size)

**Flash erase required!**

Follow "Upgrading to Larger Flash" instructions above.

## 🙏 Acknowledgments

This release includes architectural improvements for production deployment:
- Multi-SOC flexibility for hardware scalability
- Dedicated IR storage for OTA safety
- Intelligent partition scaling for different flash sizes
- Comprehensive documentation for all configurations

## 📞 Support

For issues, questions, or contributions:
- GitHub Issues: [Repository Issues](https://github.com/your-repo/issues)
- Documentation: See `PARTITION_TABLES.md` and `SPIFFS_USAGE.md`
- Build Instructions: See `BUILD_INSTRUCTIONS.md`

---

**Next Release (Planned):** v3.4.0
- IR database import/export via SPIFFS
- Web-based configuration interface
- Advanced diagnostics and logging
