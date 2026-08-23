# Releasing

1. Update `VERSION`, `kFirmwareVersion`, library metadata, and release notes.
2. Run native tests, both firmware builds, repository verification, and the
   security audit.
3. Test the resulting `crowpanel_idf5` image on physical hardware.
4. Tag the exact version as `v<VERSION>` and push the tag.
5. The release workflow verifies the tag, builds firmware, creates a clean
   checksummed source archive, and uploads release assets.
6. Download the published artifacts, verify checksums, and smoke-test flashing.

Releases never contain local credentials. PlatformIO dependencies are pinned;
Dependabot groups Python tooling and GitHub Actions updates, while PlatformIO
and firmware-library pins require a tested manual update.
