#!/bin/bash
set -e

# WSL2 / Arch Linux Build Script for VOID
echo "=== VOID Horror FPS — Build Script ==="
echo "Platform: $(uname -s) $(uname -r)"

# WSL2 ortam kontrolü
if [ -z "$DISPLAY" ]; then
    echo ""
    echo "UYARI: DISPLAY ortam degiskeni ayarli degil!"
    echo "  WSLg (Windows 11) icin:  export DISPLAY=:0"
    echo "  VcXsrv icin:             export DISPLAY=\$(cat /etc/resolv.conf | grep nameserver | awk '{print \$2}'):0.0"
    echo "Derlemeye devam ediliyor..."
    echo ""
fi

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
echo ""
echo "=== Build tamamlandi: build/void_game ==="
ls -lh void_game

echo ""
echo "Calistirmak icin:"
echo "  ./void_game"
echo "  LIBGL_ALWAYS_SOFTWARE=1 ./void_game   (OpenGL hatasi alirsan)"
