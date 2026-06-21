// sk_document_pdf_stub_mac.cpp — link-only stub for SkDocument's PDF backend.
//
// miniblink's wke print path (wke/wke2.cpp) uses SkDocument::CreatePDF +
// beginPage/endPage/close to render a page to PDF. The real implementations
// live in skia's src/pdf (SkDocument_PDF.cpp) and the cross-page SkDocument.cpp,
// neither of which is built in this macOS port (src/pdf is not in the skia
// CMake glob). PDF printing is a non-core feature, so we provide a minimal stub
// that satisfies the link: CreatePDF returns nullptr (callers AdoptRef a null
// SkDocument and the print becomes a no-op), and the public page-control methods
// are defined defensively so any stray call is harmless.
//
// This is posix/mac-only; on Windows the real skia PDF module is used.

#if !defined(_WIN32)

#include "third_party/skia/include/core/SkDocument.h"

// static
SkDocument* SkDocument::CreatePDF(SkWStream*, SkScalar)
{
    return nullptr;
}

// static
SkDocument* SkDocument::CreatePDF(const char[], SkScalar)
{
    return nullptr;
}

SkCanvas* SkDocument::beginPage(SkScalar width, SkScalar height, const SkRect* content)
{
    (void)width;
    (void)height;
    (void)content;
    // No backing document was created, so there is no page canvas.
    return nullptr;
}

void SkDocument::endPage()
{
}

bool SkDocument::close()
{
    return false;
}

#endif // !defined(_WIN32)
