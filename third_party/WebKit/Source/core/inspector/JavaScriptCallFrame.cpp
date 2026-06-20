/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/inspector/JavaScriptCallFrame.h"

#include "bindings/core/v8/V8Binding.h"
#include "bindings/core/v8/V8ScriptRunner.h"
#include <v8-debug.h>

namespace blink {

JavaScriptCallFrame::JavaScriptCallFrame(v8::Local<v8::Context> debuggerContext, v8::Local<v8::Object> callFrame)
    : m_isolate(v8::Isolate::GetCurrent())
    , m_debuggerContext(m_isolate, debuggerContext)
    , m_callFrame(m_isolate, callFrame)
{
}

JavaScriptCallFrame::~JavaScriptCallFrame()
{
}

JavaScriptCallFrame* JavaScriptCallFrame::caller()
{
    if (!m_caller) {
        v8::HandleScope handleScope(m_isolate);
        v8::Local<v8::Context> debuggerContext = m_debuggerContext.newLocal(m_isolate);
        v8::Context::Scope contextScope(debuggerContext);
#if V8_MAJOR_VERSION<8
        v8::Local<v8::Value> callerFrame = m_callFrame.newLocal(m_isolate)->Get(v8AtomicString(m_isolate, "caller"));
#else
        v8::Local<v8::Value> callerFrame;
        if (!m_callFrame.newLocal(m_isolate)->Get(debuggerContext, v8AtomicString(m_isolate, "caller")).ToLocal(&callerFrame))
            return 0;
#endif
        if (callerFrame.IsEmpty() || !callerFrame->IsObject())
            return 0;
        m_caller = JavaScriptCallFrame::create(debuggerContext, v8::Local<v8::Object>::Cast(callerFrame));
    }
    return m_caller.get();
}

int JavaScriptCallFrame::callV8FunctionReturnInt(const char* name) const
{
    v8::HandleScope handleScope(m_isolate);
    v8::Context::Scope contextScope(m_debuggerContext.newLocal(m_isolate));
    v8::Local<v8::Object> callFrame = m_callFrame.newLocal(m_isolate);
#if V8_MAJOR_VERSION<8
    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(callFrame->Get(v8AtomicString(m_isolate, name)));
#else
    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(callFrame->Get(m_isolate->GetCurrentContext(), v8AtomicString(m_isolate, name)).ToLocalChecked());
#endif
    v8::Local<v8::Value> result;
    if (!V8ScriptRunner::callInternalFunction(func, callFrame, 0, 0, m_isolate).ToLocal(&result) || !result->IsInt32())
        return 0;
    return result.As<v8::Int32>()->Value();
}

String JavaScriptCallFrame::callV8FunctionReturnString(const char* name) const
{
    v8::HandleScope handleScope(m_isolate);
    v8::Context::Scope contextScope(m_debuggerContext.newLocal(m_isolate));
    v8::Local<v8::Object> callFrame = m_callFrame.newLocal(m_isolate);
#if V8_MAJOR_VERSION<8
    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(callFrame->Get(v8AtomicString(m_isolate, name)));
#else
    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(callFrame->Get(m_isolate->GetCurrentContext(), v8AtomicString(m_isolate, name)).ToLocalChecked());
#endif
    v8::Local<v8::Value> result;
    if (!V8ScriptRunner::callInternalFunction(func, callFrame, 0, 0, m_isolate).ToLocal(&result))
        return String();
    return toCoreStringWithUndefinedOrNullCheck(result);
}

int JavaScriptCallFrame::sourceID() const
{
    return callV8FunctionReturnInt("sourceID");
}

int JavaScriptCallFrame::line() const
{
    return callV8FunctionReturnInt("line");
}

int JavaScriptCallFrame::column() const
{
    return callV8FunctionReturnInt("column");
}

String JavaScriptCallFrame::scriptName() const
{
    return callV8FunctionReturnString("scriptName");
}

String JavaScriptCallFrame::functionName() const
{
    return callV8FunctionReturnString("functionName");
}

int JavaScriptCallFrame::functionLine() const
{
    return callV8FunctionReturnInt("functionLine");
}

int JavaScriptCallFrame::functionColumn() const
{
    return callV8FunctionReturnInt("functionColumn");
}

v8::Local<v8::Value> JavaScriptCallFrame::scopeChain() const
{
    v8::Local<v8::Object> callFrame = m_callFrame.newLocal(m_isolate);
#if V8_MAJOR_VERSION<8
    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(callFrame->Get(v8AtomicString(m_isolate, "scopeChain")));
#else
    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(callFrame->Get(m_isolate->GetCurrentContext(), v8AtomicString(m_isolate, "scopeChain")).ToLocalChecked());
#endif
    v8::Local<v8::Array> scopeChain = v8::Local<v8::Array>::Cast(V8ScriptRunner::callInternalFunction(func, callFrame, 0, 0, m_isolate).ToLocalChecked());
    v8::Local<v8::Array> result = v8::Array::New(m_isolate, scopeChain->Length());
    for (uint32_t i = 0; i < scopeChain->Length(); i++) {
#if V8_MAJOR_VERSION<8
        result->Set(i, scopeChain->Get(i));
#else
        v8::Local<v8::Context> context = m_isolate->GetCurrentContext();
        result->Set(context, i, scopeChain->Get(context, i).ToLocalChecked()).FromMaybe(false);
#endif
    }
    return result;
}

int JavaScriptCallFrame::scopeType(int scopeIndex) const
{
    v8::Local<v8::Object> callFrame = m_callFrame.newLocal(m_isolate);
#if V8_MAJOR_VERSION<8
    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(callFrame->Get(v8AtomicString(m_isolate, "scopeType")));
    v8::Local<v8::Array> scopeType = v8::Local<v8::Array>::Cast(V8ScriptRunner::callInternalFunction(func, callFrame, 0, 0, m_isolate).ToLocalChecked());

    return scopeType->Get(scopeIndex)->Int32Value(m_isolate->GetCurrentContext()).FromJust();
#else
    v8::Local<v8::Context> context = m_isolate->GetCurrentContext();
    v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(callFrame->Get(context, v8AtomicString(m_isolate, "scopeType")).ToLocalChecked());
    v8::Local<v8::Array> scopeType = v8::Local<v8::Array>::Cast(V8ScriptRunner::callInternalFunction(func, callFrame, 0, 0, m_isolate).ToLocalChecked());

    return scopeType->Get(context, scopeIndex).ToLocalChecked()->Int32Value(context).FromJust();
#endif
}

v8::Local<v8::Value> JavaScriptCallFrame::thisObject() const
{
#if V8_MAJOR_VERSION<8
    return m_callFrame.newLocal(m_isolate)->Get(v8AtomicString(m_isolate, "thisObject"));
#else
    return m_callFrame.newLocal(m_isolate)->Get(m_isolate->GetCurrentContext(), v8AtomicString(m_isolate, "thisObject")).ToLocalChecked();
#endif
}

String JavaScriptCallFrame::stepInPositions() const
{
    return callV8FunctionReturnString("stepInPositions");
}

bool JavaScriptCallFrame::isAtReturn() const
{
    v8::HandleScope handleScope(m_isolate);
    v8::Context::Scope contextScope(m_debuggerContext.newLocal(m_isolate));
#if V8_MAJOR_VERSION<8
    v8::Local<v8::Value> result = m_callFrame.newLocal(m_isolate)->Get(v8AtomicString(m_isolate, "isAtReturn"));
#else
    v8::Local<v8::Value> result;
    if (!m_callFrame.newLocal(m_isolate)->Get(m_isolate->GetCurrentContext(), v8AtomicString(m_isolate, "isAtReturn")).ToLocal(&result))
        return false;
#endif
    if (result.IsEmpty() || !result->IsBoolean())
        return false;
    return result->BooleanValue(m_isolate);
}

v8::Local<v8::Value> JavaScriptCallFrame::returnValue() const
{
#if V8_MAJOR_VERSION<8
    return m_callFrame.newLocal(m_isolate)->Get(v8AtomicString(m_isolate, "returnValue"));
#else
    return m_callFrame.newLocal(m_isolate)->Get(m_isolate->GetCurrentContext(), v8AtomicString(m_isolate, "returnValue")).ToLocalChecked();
#endif
}

v8::Local<v8::Value> JavaScriptCallFrame::evaluateWithExceptionDetails(v8::Local<v8::Value> expression, v8::Local<v8::Value> scopeExtension)
{
    v8::Local<v8::Object> callFrame = m_callFrame.newLocal(m_isolate);
#if V8_MAJOR_VERSION<8
    v8::Local<v8::Function> evalFunction = v8::Local<v8::Function>::Cast(callFrame->Get(v8AtomicString(m_isolate, "evaluate")));
#else
    v8::Local<v8::Context> context = m_isolate->GetCurrentContext();
    v8::Local<v8::Function> evalFunction = v8::Local<v8::Function>::Cast(callFrame->Get(context, v8AtomicString(m_isolate, "evaluate")).ToLocalChecked());
#endif
    v8::Local<v8::Value> argv[] = {
        expression,
        scopeExtension
    };

    v8::TryCatch tryCatch(m_isolate);
    v8::Local<v8::Object> wrappedResult = v8::Object::New(m_isolate);
    v8::Local<v8::Value> result;
    if (V8ScriptRunner::callInternalFunction(evalFunction, callFrame, WTF_ARRAY_LENGTH(argv), argv, m_isolate).ToLocal(&result)) {
#if V8_MAJOR_VERSION<8
        wrappedResult->Set(v8::String::NewFromUtf8(m_isolate, "result"), result);
        wrappedResult->Set(v8::String::NewFromUtf8(m_isolate, "exceptionDetails"), v8::Undefined(m_isolate));
#else
        wrappedResult->Set(context, v8::String::NewFromUtf8(m_isolate, "result", v8::NewStringType::kNormal).ToLocalChecked(), result).FromMaybe(false);
        wrappedResult->Set(context, v8::String::NewFromUtf8(m_isolate, "exceptionDetails", v8::NewStringType::kNormal).ToLocalChecked(), v8::Undefined(m_isolate)).FromMaybe(false);
#endif
    } else {
#if V8_MAJOR_VERSION<8
        wrappedResult->Set(v8::String::NewFromUtf8(m_isolate, "result"), tryCatch.Exception());
        wrappedResult->Set(v8::String::NewFromUtf8(m_isolate, "exceptionDetails"), createExceptionDetails(m_isolate, tryCatch.Message()));
#else
        wrappedResult->Set(context, v8::String::NewFromUtf8(m_isolate, "result", v8::NewStringType::kNormal).ToLocalChecked(), tryCatch.Exception()).FromMaybe(false);
        wrappedResult->Set(context, v8::String::NewFromUtf8(m_isolate, "exceptionDetails", v8::NewStringType::kNormal).ToLocalChecked(), createExceptionDetails(m_isolate, tryCatch.Message())).FromMaybe(false);
#endif
    }
    return wrappedResult;
}

v8::MaybeLocal<v8::Value> JavaScriptCallFrame::restart()
{
    v8::Local<v8::Object> callFrame = m_callFrame.newLocal(m_isolate);
#if V8_MAJOR_VERSION<8
    v8::Local<v8::Function> restartFunction = v8::Local<v8::Function>::Cast(callFrame->Get(v8AtomicString(m_isolate, "restart")));
    v8::Debug::SetLiveEditEnabled(m_isolate, true);
    v8::MaybeLocal<v8::Value> result = V8ScriptRunner::callInternalFunction(restartFunction, callFrame, 0, 0, m_isolate);
    v8::Debug::SetLiveEditEnabled(m_isolate, false);
#else
    v8::Local<v8::Function> restartFunction = v8::Local<v8::Function>::Cast(callFrame->Get(m_isolate->GetCurrentContext(), v8AtomicString(m_isolate, "restart")).ToLocalChecked());
    v8::MaybeLocal<v8::Value> result = V8ScriptRunner::callInternalFunction(restartFunction, callFrame, 0, 0, m_isolate);
#endif
    return result;
}

v8::MaybeLocal<v8::Value> JavaScriptCallFrame::setVariableValue(int scopeNumber, v8::Local<v8::Value> variableName, v8::Local<v8::Value> newValue)
{
    v8::Local<v8::Object> callFrame = m_callFrame.newLocal(m_isolate);
#if V8_MAJOR_VERSION<8
    v8::Local<v8::Function> setVariableValueFunction = v8::Local<v8::Function>::Cast(callFrame->Get(v8AtomicString(m_isolate, "setVariableValue")));
#else
    v8::Local<v8::Function> setVariableValueFunction = v8::Local<v8::Function>::Cast(callFrame->Get(m_isolate->GetCurrentContext(), v8AtomicString(m_isolate, "setVariableValue")).ToLocalChecked());
#endif
    v8::Local<v8::Value> argv[] = {
        v8::Local<v8::Value>(v8::Integer::New(m_isolate, scopeNumber)),
        variableName,
        newValue
    };
    return V8ScriptRunner::callInternalFunction(setVariableValueFunction, callFrame, WTF_ARRAY_LENGTH(argv), argv, m_isolate);
}

v8::Local<v8::Object> JavaScriptCallFrame::createExceptionDetails(v8::Isolate* isolate, v8::Local<v8::Message> message)
{
    v8::Local<v8::Object> exceptionDetails = v8::Object::New(isolate);
#if V8_MAJOR_VERSION<8
    exceptionDetails->Set(v8::String::NewFromUtf8(isolate, "text"), message->Get());
    exceptionDetails->Set(v8::String::NewFromUtf8(isolate, "url"), message->GetScriptOrigin().ResourceName());
    exceptionDetails->Set(v8::String::NewFromUtf8(isolate, "scriptId"), v8::Integer::New(isolate, message->GetScriptOrigin().ScriptID()->Value()));
    exceptionDetails->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "line"), v8::Integer::New(isolate, message->GetLineNumber(isolate->GetCurrentContext()).FromJust()));
    exceptionDetails->Set(v8::String::NewFromUtf8(isolate, "column"), v8::Integer::New(isolate, message->GetStartColumn()));
    if (!message->GetStackTrace().IsEmpty())
        exceptionDetails->Set(v8::String::NewFromUtf8(isolate, "stackTrace"), message->GetStackTrace()->AsArray());
    else
        exceptionDetails->Set(v8::String::NewFromUtf8(isolate, "stackTrace"), v8::Undefined(isolate));
#else
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    auto setKey = [&](const char* key, v8::Local<v8::Value> value) {
        exceptionDetails->Set(context, v8::String::NewFromUtf8(isolate, key, v8::NewStringType::kNormal).ToLocalChecked(), value).FromMaybe(false);
    };
    setKey("text", message->Get());
    setKey("url", message->GetScriptOrigin().ResourceName());
    setKey("scriptId", v8::Integer::New(isolate, message->GetScriptOrigin().ScriptID()->Value()));
    setKey("line", v8::Integer::New(isolate, message->GetLineNumber(context).FromMaybe(0)));
    setKey("column", v8::Integer::New(isolate, message->GetStartColumn()));
    // v8::StackTrace::AsArray() was removed in V8 8.x; stack-trace wiring is
    // brought up with the inspector subsystem. Store undefined here.
    setKey("stackTrace", v8::Undefined(isolate));
#endif
    return exceptionDetails;
}

} // namespace blink
