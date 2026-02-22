#!/bin/bash
# Quick script to test ESP32 device connection

set -e

PORT="/dev/tty.usbserial-110"
ESP_IDF_PATH="${ESP_IDF_PATH:-$HOME/esp/v5.5.2/esp-idf}"

echo "=== ESP32 Device Connection Test ==="
echo ""

# Check if port exists
if [ -e "$PORT" ]; then
    echo "✓ Device port found: $PORT"
    ls -l "$PORT"
else
    echo "✗ Device port not found: $PORT"
    echo "Available USB serial ports:"
    ls -la /dev/tty.* 2>/dev/null | grep -E '(USB|usb)' || echo "  None found"
    exit 1
fi

echo ""
echo "=== Testing port access ==="
if stty -f "$PORT" 115200 raw -echo 2>/dev/null; then
    echo "✓ Port is accessible"
else
    echo "✗ Cannot access port (might be in use or need permissions)"
fi

echo ""
echo "=== ESP-IDF Detection ==="
if [ -f "$ESP_IDF_PATH/export.sh" ]; then
    echo "✓ ESP-IDF found at: $ESP_IDF_PATH"
    echo ""
    echo "To use ESP-IDF tools, run:"
    echo "  . $ESP_IDF_PATH/export.sh"
    echo "  idf.py -p $PORT chip_id"
else
    echo "✗ ESP-IDF not found at: $ESP_IDF_PATH"
    echo ""
    echo "Please source ESP-IDF environment first:"
    echo "  . \$HOME/esp/esp-idf/export.sh  # or wherever ESP-IDF is installed"
fi

echo ""
echo "=== Quick Test Commands ==="
echo ""
echo "1. Source ESP-IDF:"
echo "   . $ESP_IDF_PATH/export.sh"
echo ""
echo "2. Test chip detection:"
echo "   idf.py -p $PORT chip_id"
echo ""
echo "3. Flash firmware:"
echo "   idf.py -p $PORT flash"
echo ""
echo "4. Monitor output:"
echo "   idf.py -p $PORT monitor"
