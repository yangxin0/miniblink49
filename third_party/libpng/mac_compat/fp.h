// Shim: modern macOS defines TARGET_OS_MAC, which makes libpng pngpriv.h try the
// classic Mac OS <fp.h>. Redirect it to <math.h>.
#include <math.h>
