# Generated firmware bundles

This directory intentionally contains no checked-in binary. After a successful
PlatformIO build, run:

```bash
python tools/export_firmware.py --env crowpanel
```

The generated versioned directory contains individual ESP32-S3 images, exact
flash offsets, SHA-256 checksums, an ELF file when available, and a merged image
when PlatformIO's esptool package can be located.
