#!/bin/bash
# Create a distributable DMG for PChat

set -e

VERSION="${1:-1.5.4}"
BUILD_DIR="${2:-build}"
SOURCE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUNDLE_PATH="${BUILD_DIR}/PChat.app"
DMG_NAME="PChat-${VERSION}.dmg"

if [ ! -d "$BUNDLE_PATH" ]; then
    echo "Error: Bundle not found at ${BUNDLE_PATH}"
    echo "Run 'make bundle' or './osx/create-bundle.sh' first"
    exit 1
fi

echo "Creating DMG: ${DMG_NAME}"
echo "Source bundle: ${BUNDLE_PATH}"

# Remove existing DMG if present
rm -f "${BUILD_DIR}/${DMG_NAME}"

# Create temporary directory for DMG contents
TEMP_DMG_DIR="${BUILD_DIR}/dmg_temp"
rm -rf "${TEMP_DMG_DIR}"
mkdir -p "${TEMP_DMG_DIR}"

# Copy the bundle
echo "Copying application bundle..."
cp -R "${BUNDLE_PATH}" "${TEMP_DMG_DIR}/"

# Create Applications symlink
echo "Creating Applications symlink..."
ln -s /Applications "${TEMP_DMG_DIR}/Applications"

# Create a README if it exists
if [ -f "${SOURCE_DIR}/README.md" ]; then
    cp "${SOURCE_DIR}/README.md" "${TEMP_DMG_DIR}/README.txt"
fi

# Create the DMG
echo "Creating disk image..."
hdiutil create -volname "PChat ${VERSION}" \
    -srcfolder "${TEMP_DMG_DIR}" \
    -ov \
    -format UDZO \
    -imagekey zlib-level=9 \
    "${BUILD_DIR}/${DMG_NAME}"

# Clean up
rm -rf "${TEMP_DMG_DIR}"

echo ""
echo "✓ DMG created successfully: ${BUILD_DIR}/${DMG_NAME}"
echo ""

# Show DMG info
DMG_SIZE=$(du -h "${BUILD_DIR}/${DMG_NAME}" | cut -f1)
echo "DMG size: ${DMG_SIZE}"
echo ""
echo "To verify the signature:"
echo "  codesign --verify --deep --strict ${BUNDLE_PATH}"
echo ""
echo "To notarize (requires Apple Developer account):"
echo "  xcrun notarytool submit ${BUILD_DIR}/${DMG_NAME} --apple-id YOUR_APPLE_ID --team-id TEAMID --password APP_SPECIFIC_PASSWORD --wait"
echo ""
