// sk_pdf_printing_stub_win.cpp — Windows link-only stubs for the PDF print path.
//
// miniblink's wke print path (wke/wke2.cpp) uses SkDocument::CreatePDF +
// beginPage/endPage/close, and wkeWebView.cpp references the mbvip PDF-viewer
// NPAPI plugin entry points. The real skia PDF backend (src/pdf) and the mbvip
// printing plugin are not built in this CMake tree, and PDF printing is a
// non-core feature, so provide minimal stubs that satisfy the link: CreatePDF
// returns nullptr (the print becomes a no-op) and the plugin entry points are
// harmless no-ops. (The macOS port has the equivalent sk_document_pdf_stub_mac.)
#if defined(_WIN32)

#include "third_party/skia/include/core/SkDocument.h"
#include "features/printing/PdfViewerPluginFunc.h"

SkDocument* SkDocument::CreatePDF(SkWStream*, SkScalar) { return nullptr; }
SkDocument* SkDocument::CreatePDF(const char[], SkScalar) { return nullptr; }
SkCanvas* SkDocument::beginPage(SkScalar, SkScalar, const SkRect*) { return nullptr; }
void SkDocument::endPage() {}
bool SkDocument::close() { return false; }

namespace printing {
NPError __stdcall PdfViewerPluginNPInitialize(NPNetscapeFuncs*) { return 0; }
NPError __stdcall PdfViewerPluginNPGetEntryPoints(NPPluginFuncs*) { return 0; }
void __stdcall PdfViewerPluginNPShutdown(void) {}
}  // namespace printing

#endif  // defined(_WIN32)
