// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/Source/platform/heap/UnifiedHeapController.h"

#include "bindings/core/v8/ScriptWrappable.h"
#include "bindings/core/v8/WrapperTypeInfo.h"
#include "third_party/WebKit/Source/platform/heap/heap.h"
#include "third_party/WebKit/Source/platform/heap/MarkingVisitor.h"
#include "third_party/WebKit/Source/core/dom/ActiveDOMObject.h"
#include "third_party/WebKit/Source/core/events/EventTarget.h"
#include "third_party/WebKit/Source/bindings/core/v8/V8GCController.h"
#include "third_party/WebKit/Source/bindings/core/v8/V8PerIsolateData.h"
#include "third_party/WebKit/Source/bindings/core/v8/DOMDataStore.h"

#if V8_MAJOR_VERSION >= 7

namespace blink {

UnifiedHeapController::UnifiedHeapController(ThreadState* threadState)
    : m_threadState(threadState)
{
}

UnifiedHeapController::~UnifiedHeapController()
{
    leakV8References();
}

std::vector<std::pair<void*, void*>>* UnifiedHeapController::leakV8References()
{
    for (WTF::HashMap<void*, void*>::iterator it = m_v8References.begin(); it != m_v8References.end(); ++it) {
        if (it->value)
            continue;
        
        v8::Persistent<v8::Value>* key = (v8::Persistent<v8::Value>*)it->key;
        //key->SetWeak();
        //key->ClearWeak();
        //key->Reset();
        delete key;
    }

    m_v8References.clear();
    return nullptr;
}

#if V8_MAJOR_VERSION < 8
void UnifiedHeapController::TracePrologue()
{
    m_isTracingDone = false;
}
#else
// V8 8.7 added a TraceFlags argument to this hook.
void UnifiedHeapController::TracePrologue(v8::EmbedderHeapTracer::TraceFlags)
{
    m_isTracingDone = false;
}
#endif

void UnifiedHeapController::EnterFinalPause(EmbedderStackState stack_state)
{

}

#if V8_MAJOR_VERSION < 8
void UnifiedHeapController::TraceEpilogue()
{
}
#else
// V8 8.7 added a TraceSummary* argument to this hook.
void UnifiedHeapController::TraceEpilogue(v8::EmbedderHeapTracer::TraceSummary*)
{
}
#endif

void UnifiedHeapController::RegisterV8References(const std::vector<std::pair<void*, void*>>& refs)
{
    v8::Isolate* isolate = m_threadState->isolate();
    v8::EmbedderHeapTracer* tracer = V8PerIsolateData::from(isolate)->getEmbedderHeapTracer(isolate);

    for (size_t i = 0; i < refs.size(); ++i) {
        std::pair<void*, void*> internalFields = refs[i];
        void* first = internalFields.first;
        WTF::HashMap<void*, void*>::iterator it = m_v8References.find(first);

        if (!internalFields.second) { // ��������������˵����ScopedPersistent���ø�MajorGCWrapperVisitor::VisitPersistentHandle
            if (it != m_v8References.end())
                continue;
            m_v8References.add(internalFields.first, nullptr);
            continue;
        }

        WrapperTypeInfo* wrapperTypeInfo = (WrapperTypeInfo*)(internalFields.first);
        ScriptWrappable* scriptWrappable = (ScriptWrappable*)(internalFields.second);

        if (wrapperTypeInfo->ginEmbedder != gin::GinEmbedder::kEmbedderBlink)
            continue;

        if (it != m_v8References.end()) {
            void* value = it->value;
            RELEASE_ASSERT(value == scriptWrappable);
            continue;
        }

        m_v8References.add(scriptWrappable, wrapperTypeInfo); // ע��˳����һ��
    }
    m_isTracingDone = false;
}

bool UnifiedHeapController::AdvanceTracing(double deadline_in_ms)
{
    m_isTracingDone = true;

    if (0 == m_v8References.size())
        return true;

    v8::Isolate* isolate = m_threadState->isolate();
    v8::EmbedderHeapTracer* tracer = V8PerIsolateData::from(isolate)->getEmbedderHeapTracer(isolate);
    v8::HandleScope scope(isolate);

    for (WTF::HashMap<void*, void*>::iterator it = m_v8References.begin(); it != m_v8References.end(); ++it) {
        if (!it->value) {
            v8::Persistent<v8::Value>* value = (v8::Persistent<v8::Value>*)it->key;
            v8::Local<v8::Value> wrapper = v8::Local<v8::Value>::New(isolate, v8::Persistent<v8::Value>::Cast(*value));
            v8::TracedGlobal<v8::Value> traceObj(isolate, wrapper);
#if V8_MAJOR_VERSION < 8
            tracer->RegisterEmbedderReference(traceObj);
#else
            // V8 8.7 takes a TracedReferenceBase<v8::Data>&.
            tracer->RegisterEmbedderReference(traceObj.As<v8::Data>());
#endif

//             v8::TracedGlobal<v8::Value>* value = (v8::TracedGlobal<v8::Value>*)it->key;
//             tracer->RegisterEmbedderReference(*value);

            continue;
        }

        WrapperTypeInfo* wrapperTypeInfo = (WrapperTypeInfo*)(it->value);
        ScriptWrappable* scriptWrappable = (ScriptWrappable*)(it->key);

        if (wrapperTypeInfo->ginEmbedder != gin::GinEmbedder::kEmbedderBlink)
            continue;

        const v8::TracedGlobal<v8::Object>& traceObj = scriptWrappable->GetMainWorldWrapper();
        if (!traceObj.IsEmpty()) {
#if V8_MAJOR_VERSION < 8
            tracer->RegisterEmbedderReference(traceObj.As<v8::Value>());
#else
            // V8 8.7 takes a TracedReferenceBase<v8::Data>&.
            tracer->RegisterEmbedderReference(traceObj.As<v8::Data>());
#endif
        } else if (!(WTF::isMainThread())) {
            if (isolate->GetCurrentContext().IsEmpty()) {
#if defined(_WIN32)
                char* output = (char*)malloc(0x100);
                sprintf_s(output, 0x99, "UnifiedHeapController::AdvanceTracing fail: %p\n", isolate);
                OutputDebugStringA(output);
                free(output);
#endif
                continue;
            }

            v8::Local<v8::Object> wrapper = DOMDataStore::getWrapper(scriptWrappable, isolate);
            v8::TracedGlobal<v8::Value> traceWrapObj(isolate, wrapper.As<v8::Value>());
#if V8_MAJOR_VERSION < 8
            tracer->RegisterEmbedderReference(traceWrapObj.As<v8::Value>());
#else
            // V8 8.7 takes a TracedReferenceBase<v8::Data>&.
            tracer->RegisterEmbedderReference(traceWrapObj.As<v8::Data>());
#endif
        }

//         char* output = (char*)malloc(0x100);
//         sprintf_s(output, 0x99, "UnifiedHeapController::AdvanceTracing: %p %p\n", wrapperTypeInfo, scriptWrappable);
//         OutputDebugStringA(output);
//         free(output);
    }

    tracer->FinalizeTracing();

    return true;
}

bool UnifiedHeapController::IsTracingDone()
{
    return m_isTracingDone;
}

const WrapperTypeInfo* toWrapperTypeInfo(const v8::TracedGlobal<v8::Object>& wrapper);

bool UnifiedHeapController::IsRootForNonTracingGCInternal(const v8::TracedGlobal<v8::Value>& handle)
{
    const uint16_t classId = handle.WrapperClassId();
    // Stand-alone TracedGlobal reference or kCustomWrappableId. Keep as root as we don't know better.
    if (classId != WrapperTypeInfo::NodeClassId && classId != WrapperTypeInfo::ObjectClassId)
        return true;

    const v8::TracedGlobal<v8::Object>& traced = handle.As<v8::Object>();
    v8::Local<v8::Object> wrapper = traced.Get(v8::Isolate::GetCurrent());

    const WrapperTypeInfo* type = toWrapperTypeInfo(wrapper);
    if (!type)
        return false;

    ActiveDOMObject* activeDOMObject = type->toActiveDOMObject(wrapper);
    ScriptWrappable* scriptWrap = toScriptWrappable(wrapper);

    if (activeDOMObject && activeDOMObject->hasPendingActivity())
        return true;

    if (scriptWrap && scriptWrap->hasScriptWrappableEventListeners())
        return true;

    return false;
}

bool UnifiedHeapController::IsRootForNonTracingGC(const v8::TracedGlobal<v8::Value>& handle)
{
    return IsRootForNonTracingGCInternal(handle);
}

}  // namespace blink

#endif
