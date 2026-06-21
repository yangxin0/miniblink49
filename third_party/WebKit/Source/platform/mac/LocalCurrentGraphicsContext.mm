/*
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
// Pre-import full Cocoa (this file defines an ObjC category but only imported a
// single AppKit header), with CarbonCore's TextEncoding renamed to avoid the
// WTF::TextEncoding collision.
#define TextEncoding CarbonTextEncoding
#import <Cocoa/Cocoa.h>
#undef TextEncoding
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/mac/LocalCurrentGraphicsContext.h"
#include "platform/mac/ThemeMac.h"

#include <AppKit/NSGraphicsContext.h>
#include "platform/graphics/GraphicsContext.h"
#include "platform_canvas.h"

namespace blink {

LocalCurrentGraphicsContext::LocalCurrentGraphicsContext(GraphicsContext* graphicsContext, const IntRect& dirtyRect)
    : LocalCurrentGraphicsContext(graphicsContext->canvas(), graphicsContext->deviceScaleFactor(), nullptr, dirtyRect)
{
}

LocalCurrentGraphicsContext::LocalCurrentGraphicsContext(GraphicsContext* graphicsContext, const IntRect* interestRect,
                                                         const IntRect& dirtyRect)
    : LocalCurrentGraphicsContext(graphicsContext->canvas(), graphicsContext->deviceScaleFactor(), interestRect, dirtyRect)
{
}

LocalCurrentGraphicsContext::LocalCurrentGraphicsContext(SkCanvas* canvas, float deviceScaleFactor, const IntRect* interestRect,
                                                         const IntRect& dirtyRect)
    : m_didSetGraphicsContext(false)
    , m_inflatedDirtyRect(ThemeMac::inflateRectForAA(dirtyRect))
    // This skia copy's SkiaBitLocker only has the 1-arg ctor; the dirty-rect/scale
    // form was a later optimization (lock just the dirty region). Locking the whole
    // canvas is functionally equivalent here.
    , m_skiaBitLocker(canvas)
{
    m_savedCanvas = canvas;
    canvas->save();

    bool clipToInterest = interestRect && RuntimeEnabledFeatures::slimmingPaintEnabled() && !interestRect->contains(m_inflatedDirtyRect);
    if (clipToInterest) {
        IntRect clippedBounds(m_inflatedDirtyRect);
        clippedBounds.intersect(*interestRect);
        canvas->clipRect(clippedBounds, SkRegion::kIntersect_Op);
    }

    CGContextRef cgContext = this->cgContext();
    if (cgContext == [[NSGraphicsContext currentContext] graphicsPort]) {
        m_savedNSGraphicsContext = 0;
        return;
    }

    m_savedNSGraphicsContext = [[NSGraphicsContext currentContext] retain];
    NSGraphicsContext* newContext = [NSGraphicsContext graphicsContextWithGraphicsPort:cgContext flipped:YES];
    [NSGraphicsContext setCurrentContext:newContext];
    m_didSetGraphicsContext = true;
}

LocalCurrentGraphicsContext::~LocalCurrentGraphicsContext()
{
    if (m_didSetGraphicsContext) {
        [NSGraphicsContext setCurrentContext:m_savedNSGraphicsContext];
        [m_savedNSGraphicsContext release];
    }

    m_savedCanvas->restore();
}

CGContextRef LocalCurrentGraphicsContext::cgContext()
{
    // This synchronizes the CGContext to reflect the current SkCanvas state.
    // The implementation may not return the same CGContext each time.
    CGContextRef cgContext = m_skiaBitLocker.cgContext();

    return cgContext;
}

}
