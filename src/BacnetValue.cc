#include <iostream>
#include <node.h>
#include <node_object_wrap.h>
#include <bacapp.h>
#include <nan.h>
#include "BacnetValue.h"
#include "conversion.h"

using v8::String;
using v8::FunctionTemplate;
using v8::Local;
using v8::Object;
using v8::Isolate;
using v8::Persistent;
using v8::Function;
using v8::Number;
using v8::Uint32;
using v8::MaybeLocal;
using v8::Context;

BacnetValue::BacnetValue() {
}
BacnetValue::~BacnetValue() {}

NAN_MODULE_INIT(BacnetValue::Init) {
    // Prepare constructor template
    Local<FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(FromJs);
    tpl->SetClassName(Nan::New("BacnetValue").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    Nan::SetPrototypeMethod(tpl, "bytes", Bytes);
    Nan::SetPrototypeMethod(tpl, "toString", ToString);
    Nan::SetPrototypeMethod(tpl, "valueOf", ValueOf);

    // Constructor functions
    Nan::SetMethod(tpl->GetFunction(), "fromBytes", FromBytes);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("BacnetValue").ToLocalChecked(),
        Nan::GetFunction(tpl).ToLocalChecked());
}

// Default constructor in js
// new BacnetValue(value, [tag])
NAN_METHOD(BacnetValue::FromJs) {
    if (info.IsConstructCall()) {
        // Invoked as constructor: `new BacnetValue(value [, tag])`
        if (info[1]->IsUndefined()) {
            Nan::ThrowError("Type inference unsupported, supply tag");
            return;
        }
        uint8_t type_tag = Nan::To<uint32_t>(info[1]).FromJust();
        BacnetValue* obj = new BacnetValue();
        std::cout << "creating value " << extractString(info[0]->ToString()) << " with type " << +type_tag << std::endl;
        obj->Wrap(info.This());
        Nan::Set(info.This(), Nan::New("value").ToLocalChecked(), info[0]);
        Nan::Set(info.This(), Nan::New("tag").ToLocalChecked(), info[1]);
        info.GetReturnValue().Set(info.This());
    } else {
        // Invoked as plain function `BacnetValue(...)`, turn into construct call.
        const int argc = 2;
        Local<Value> argv[argc] = { info[0], info[1] };
        Local<Function> cons = Nan::New(constructor());
        info.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }
}

// Constructor from bacnet app buffer
// BacnetValue.fromBytes(buffer)
NAN_METHOD(BacnetValue::FromBytes) {
    Nan::HandleScope scope;

    size_t buflen = node::Buffer::Length(info[0]);
    if (buflen < sizeof(BACNET_APPLICATION_DATA_VALUE)) { // risk of segfault
        Nan::ThrowError("Unsure if buffer is big enough to provide BACNET_APPLICATION_DATA_VALUE, avoiding risk of segfault");
        return;
    }
    char * buf = node::Buffer::Data(info[0]);
    BACNET_APPLICATION_DATA_VALUE * value = reinterpret_cast<BACNET_APPLICATION_DATA_VALUE*>(buf);

    Local<Value> jsValue = bacnetApplicationValueToJ(&scope, value);

    std::cout << "interpreted value as having " << extractString(jsValue->ToString()) << " with type " << +value->tag << std::endl;
    const int argc = 2;
    Local<Value> argv[argc] = { jsValue, Nan::New<Number>(value->tag) };
    Local<Function> cons = Nan::New(constructor());
    info.GetReturnValue().Set(cons->NewInstance(argc, argv));
}

NAN_METHOD(BacnetValue::Bytes) {
    BacnetValue* obj = ObjectWrap::Unwrap<BacnetValue>(info.Holder());
    Local<Value> buffer = Nan::Encode(obj->bacnetValue(), sizeof(BACNET_APPLICATION_DATA_VALUE), Nan::Encoding::BUFFER); // TODO : depending upon the tag we can cut off the buffer at a shorter length
    /// TODO : check handling and cleanup of bacnet Value

    info.GetReturnValue().Set(buffer);
}

NAN_METHOD(BacnetValue::ToString) {
    BacnetValue* obj = ObjectWrap::Unwrap<BacnetValue>(info.Holder());
    Local<Value> value = obj->value();
    std::cout << "doing tostring of " << extractString(value->ToString()) << std::endl;
    Local<Value> string = value->ToString();

    info.GetReturnValue().Set(string);
}

NAN_METHOD(BacnetValue::ValueOf) {
    Local<Value> value = Nan::Get(info.Holder(), Nan::New("value").ToLocalChecked()).ToLocalChecked();

    info.GetReturnValue().Set(value);
}

Local<Value> BacnetValue::value() {
    return Nan::Get(handle(), Nan::New("value").ToLocalChecked()).ToLocalChecked(); // not handling the value not being set - could fail if its deleted
}

BACNET_APPLICATION_TAG BacnetValue::tag() {
    Local<String> key = Nan::New("tag").ToLocalChecked();
    Local<Value> value = Nan::Get(handle(), key).ToLocalChecked();
    uint32_t number = Nan::To<uint32_t>(value).FromJust();
    return static_cast<BACNET_APPLICATION_TAG>(number); // not handling the value not being set - could fail if its deleted
}

BACNET_APPLICATION_DATA_VALUE * BacnetValue::bacnetValue() {
    BACNET_APPLICATION_DATA_VALUE * cvalue = new BACNET_APPLICATION_DATA_VALUE;
    bacnetAppValueToC(cvalue, value(), tag()); // TODO : use tag to make this more effective
    return cvalue;
}