// macOS case-insensitive-FS shim. mc's Chromium-derived compositor does
// #include "cc/tiles/tile.h" meaning orig_chrome/cc/tiles/tile.h, but the legacy
// root cc/tiles/Tile.h matches case-insensitively and wins when the repo root is
// on the include path. This dir is placed BEFORE the repo root so this one header
// wins the lookup and forwards to the canonical Chromium copy; every other cc/*
// include falls through unchanged. Windows builds never use this dir.
#include "orig_chrome/cc/tiles/tile.h"
