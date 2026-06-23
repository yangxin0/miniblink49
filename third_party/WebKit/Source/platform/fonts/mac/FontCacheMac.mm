/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "platform/fonts/FontCache.h"

#import <AppKit/AppKit.h>
#import <CoreText/CoreText.h>
#include "platform/LayoutTestSupport.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/fonts/FontDescription.h"
#include "platform/fonts/FontFaceCreationParams.h"
#include  "platform/fonts/FontPlatformData.h"
#include "platform/fonts/SimpleFontData.h"
#include "platform/fonts/mac/FontFamilyMatcherMac.h"
#include "public/platform/Platform.h"
#include "public/platform/WebTraceLocation.h"
#include <wtf/Functional.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>

namespace blink {

static void invalidateFontCache()
{
    if (!isMainThread()) {
        Platform::current()->mainThread()->postTask(FROM_HERE, bind(&invalidateFontCache));
        return;
    }
    FontCache::fontCache()->invalidate();
}

static void fontCacheRegisteredFontsChangedNotificationCallback(CFNotificationCenterRef, void* observer, CFStringRef name, const void *, CFDictionaryRef)
{
    ASSERT_UNUSED(observer, observer == FontCache::fontCache());
    ASSERT_UNUSED(name, CFEqual(name, kCTFontManagerRegisteredFontsChangedNotification));
    invalidateFontCache();
}

static bool useHinting()
{
    // Enable hinting when subpixel font scaling is disabled or
    // when running the set of standard non-subpixel layout tests,
    // otherwise use subpixel glyph positioning.
    return (LayoutTestSupport::isRunningLayoutTest() && !LayoutTestSupport::isFontAntialiasingEnabledForTest());
}

void FontCache::platformInit()
{
    CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter(), this, fontCacheRegisteredFontsChangedNotificationCallback, kCTFontManagerRegisteredFontsChangedNotification, 0, CFNotificationSuspensionBehaviorDeliverImmediately);
}

static int toAppKitFontWeight(FontWeight fontWeight)
{
    static int appKitFontWeights[] = {
        2,  // FontWeight100
        3,  // FontWeight200
        4,  // FontWeight300
        5,  // FontWeight400
        6,  // FontWeight500
        8,  // FontWeight600
        9,  // FontWeight700
        10, // FontWeight800
        12, // FontWeight900
    };
    return appKitFontWeights[fontWeight];
}

static inline bool isAppKitFontWeightBold(NSInteger appKitFontWeight)
{
    return appKitFontWeight >= 7;
}

PassRefPtr<SimpleFontData> FontCache::fallbackFontForCharacter(const FontDescription& fontDescription, UChar32 character, const SimpleFontData* fontDataToSubstitute)
{
    // FIXME: We should fix getFallbackFamily to take a UChar32
    // and remove this split-to-UChar16 code.
    UChar codeUnits[2];
    int codeUnitsLength;
    if (character <= 0xFFFF) {
        codeUnits[0] = character;
        codeUnitsLength = 1;
    } else {
        codeUnits[0] = U16_LEAD(character);
        codeUnits[1] = U16_TRAIL(character);
        codeUnitsLength = 2;
    }

    const FontPlatformData& platformData = fontDataToSubstitute->platformData();

    // Find a font that covers `character`. WebKit historically used the private
    // AppKit SPI -[NSFont findFontLike:forString:/forCharacter:], but that was
    // removed on modern macOS and returns nil there — so emoji (e.g. U+1F511) and
    // CJK got no substitute and fell through to the last-resort font, rendering
    // the wrong glyph at the wrong advance width. CTFontCreateForString is
    // Apple's supported replacement: it returns a font from the system cascade
    // that covers the range (Apple Color Emoji for emoji, a CJK face, etc.) at
    // the reference font's size.
    CFStringRef cfString = CFStringCreateWithCharacters(kCFAllocatorDefault, reinterpret_cast<const UniChar*>(codeUnits), codeUnitsLength);
    CTFontRef ctSubstitute = CTFontCreateForString(platformData.ctFont(), cfString, CFRangeMake(0, codeUnitsLength));
    CFRelease(cfString);
    if (!ctSubstitute)
        return nullptr;
    // If CoreText just echoed the reference font, nothing in the cascade covers
    // the character — give up so the caller falls back to its last-resort font.
    if (CFEqual(ctSubstitute, platformData.ctFont())) {
        CFRelease(ctSubstitute);
        return nullptr;
    }

    // Build the substitute straight from the CoreText font. The old code then ran
    // it through NSFontManager + -screenFont/-printerFont to match traits, but
    // those AppKit paths are deprecated and corrupt the resolved font on modern
    // macOS (turning the emoji/icon glyphs into tofu). Carry over the original's
    // synthetic-bold/italic flags and orientation instead. (CTFontRef is toll-free
    // bridged to NSFont*; the FontPlatformData ctor retains it, so we can release
    // our reference afterwards — this file is compiled without ARC.)
    NSFont *substituteFont = toNSFont(ctSubstitute);
    FontPlatformData alternateFont(substituteFont, platformData.size(),
        platformData.m_syntheticBold, platformData.m_syntheticItalic,
        platformData.orientation());
    CFRelease(ctSubstitute);

    return fontDataFromFontPlatformData(&alternateFont, DoNotRetain);
}

PassRefPtr<SimpleFontData> FontCache::getLastResortFallbackFont(const FontDescription& fontDescription, ShouldRetain shouldRetain)
{
    DEFINE_STATIC_LOCAL(AtomicString, timesStr, ("Times", AtomicString::ConstructFromLiteral));

    // FIXME: Would be even better to somehow get the user's default font here.  For now we'll pick
    // the default that the user would get without changing any prefs.
    RefPtr<SimpleFontData> simpleFontData = getFontData(fontDescription, timesStr, false, shouldRetain);
    if (simpleFontData)
        return simpleFontData.release();

    // The Times fallback will almost always work, but in the highly unusual case where
    // the user doesn't have it, we fall back on Lucida Grande because that's
    // guaranteed to be there, according to Nathan Taylor. This is good enough
    // to avoid a crash at least.
    DEFINE_STATIC_LOCAL(AtomicString, lucidaGrandeStr, ("Lucida Grande", AtomicString::ConstructFromLiteral));
    return getFontData(fontDescription, lucidaGrandeStr, false, shouldRetain);
}

FontPlatformData* FontCache::createFontPlatformData(const FontDescription& fontDescription, const FontFaceCreationParams& creationParams, float fontSize)
{
    NSFontTraitMask traits = fontDescription.style() ? NSFontItalicTrait : 0;
    NSInteger weight = toAppKitFontWeight(fontDescription.weight());
    float size = fontSize;

    NSFont *nsFont = MatchNSFontFamily(creationParams.family(),traits, weight, size);
    if (!nsFont)
        return 0;

    NSFontManager *fontManager = [NSFontManager sharedFontManager];
    NSFontTraitMask actualTraits = 0;
    if (fontDescription.style())
        actualTraits = [fontManager traitsOfFont:nsFont];
    NSInteger actualWeight = [fontManager weightOfFont:nsFont];

    NSFont *platformFont = useHinting() ? [nsFont screenFont] : [nsFont printerFont];
    bool syntheticBold = (isAppKitFontWeightBold(weight) && !isAppKitFontWeightBold(actualWeight)) || fontDescription.isSyntheticBold();
    bool syntheticItalic = ((traits & NSFontItalicTrait) && !(actualTraits & NSFontItalicTrait)) || fontDescription.isSyntheticItalic();

    // FontPlatformData::typeface() is null in the case of Chromium out-of-process font loading failing.
    // Out-of-process loading occurs for registered fonts stored in non-system locations.
    // When loading fails, we do not want to use the returned FontPlatformData since it will not have
    // a valid SkTypeface.
    OwnPtr<FontPlatformData> platformData = adoptPtr(new FontPlatformData(platformFont, size, syntheticBold, syntheticItalic, fontDescription.orientation()));
    if (!platformData->typeface()) {
        return nullptr;
    }
    return platformData.leakPtr();
}

} // namespace blink
