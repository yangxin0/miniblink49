// macOS shim for <uxtheme.h>. content/ui/CustomTheme.h includes this Windows visual-
// styles header but uses none of its symbols on the code paths we build (only comment
// references). Empty shim so the include resolves; the real theming goes through
// blink's own LayoutThemeMac / native_theme path on macOS.
#pragma once
