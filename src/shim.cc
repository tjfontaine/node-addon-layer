/*
 * Copyright Joyent, Inc. and other Node contributors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include "shim.h"

#if NODE_VERSION_AT_LEAST(0, 11, 3)
#define V8_USE_UNSAFE_HANDLES 1
#define V8_ALLOW_ACCESS_TO_RAW_HANDLE_CONSTRUCTOR 1
#endif

#include "v8.h"
#include "node.h"

#if NODE_VERSION_AT_LEAST(0, 11, 3)
using v8::FunctionCallbackInfo;
#else
using v8::Arguments;
#endif

using v8::Array;
using v8::Exception;
using v8::External;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::TryCatch;
using v8::Undefined;
using v8::Value;

#define SHIM_PROLOGUE                                                         \
  Isolate *isolate = Isolate::GetCurrent();                                   \
  HandleScope scope;                                                          \
  shim_ctx_t ctx;                                                             \
  TryCatch trycatch;                                                          \
  QUEUE_INIT(&ctx.allocs);                                                    \
  ctx.isolate = static_cast<void*>(isolate);                                  \
  ctx.scope = static_cast<void*>(&scope);                                     \
  ctx.trycatch = static_cast<void*>(&trycatch);                               \
do {} while(0)


#define SHIM_TO_VAL(obj) static_cast<Value*>((obj)->handle)

#define VAL_DEFINE(var, obj) Local<Value> var(SHIM_TO_VAL(obj))

namespace shim {

Persistent<String> hidden_private;

void
shim_cleanup_context(shim_ctx_t *ctx)
{
  QUEUE* q = NULL;
  QUEUE_FOREACH(q, &ctx->allocs) {
    shim_val_t* o = container_of(q, shim_val_t, member);
    free(o);
  }
}

shim_val_t*
shim_val_alloc(shim_ctx_t *ctx, Handle<Value> val)
{
  shim_val_t* obj = static_cast<shim_val_t*>(malloc(sizeof(shim_val_t)));
  QUEUE_INIT(&obj->member);
  if (ctx != NULL)
    QUEUE_INSERT_TAIL(&ctx->allocs, &obj->member);
  obj->handle = *val;
  return obj;
}

Local<Value>*
shim_vals_to_handles(size_t argc, shim_val_t** argv)
{
  Local<Value>* jsargs = new Local<Value>[argc];
  for (size_t i = 0; i < argc; i++) {
    Local<Value> tmp = SHIM_TO_VAL(argv[i]);
    jsargs[i] = tmp;
  }
  return jsargs;
}

void*
shim_external_value(Handle<Value> obj)
{
#if NODE_VERSION_AT_LEAST(0, 11, 3)
  Handle<External> ext = Handle<External>::Cast(obj);
  return ext->Value();
#else
  return External::Unwrap(obj);
#endif
}

Local<Value>
shim_external_new(void *data)
{
#if NODE_VERSION_AT_LEAST(0, 11, 3)
  return External::New(data);
#else
  return External::Wrap(data);
#endif
}

#if NODE_VERSION_AT_LEAST(0, 11, 3)
void
Static(const FunctionCallbackInfo<Value>& args)
#else
Handle<Value>
Static(const Arguments& args)
#endif
{
  SHIM_PROLOGUE;

  shim_func cfunc = reinterpret_cast<shim_func>(shim_external_value(args.Data()));

  size_t argc = args.Length();

  size_t argv_len = sizeof(shim_val_t*) * (argc + SHIM_ARG_PAD);
  shim_val_t** argv = static_cast<shim_val_t**>(malloc(argv_len));

  argv[SHIM_ARG_THIS] = shim_val_alloc(&ctx, args.This());
  argv[SHIM_ARG_RVAL] = NULL;

  for (size_t i = 0; i < argc; i++) {
    argv[i + SHIM_ARG_PAD] = shim_val_alloc(&ctx, args[i]);
  }

  if(!cfunc(&ctx, argc, argv + SHIM_ARG_PAD)) {
    /* the function failed do we need to do any more checking of exceptions? */
  }

  Local<Value> ret;

  if(!trycatch.HasCaught() && argv[SHIM_ARG_RVAL] != NULL) {
    ret = SHIM_TO_VAL(argv[SHIM_ARG_RVAL]);
  }

  shim_cleanup_context(&ctx);
  free(argv);

#if NODE_VERSION_AT_LEAST(0, 11, 3)
  if (!trycatch.HasCaught())
    args.GetReturnValue().Set(ret);
#else
  if (trycatch.HasCaught()) {
    return trycatch.Exception();
  } else {
    return scope.Close(ret);
  }
#endif
}

extern "C"
void shim_module_initialize(Handle<Object> exports, Handle<Value> module)
{
  SHIM_PROLOGUE;

  if (hidden_private.IsEmpty()) {
    Handle<String> str = String::NewSymbol("shim_private");
#if NODE_VERSION_AT_LEAST(0, 11, 3)
    hidden_private = Persistent<String>::New(isolate, str);
#else
    hidden_private = Persistent<String>::New(str);
#endif
  }

  shim_val_t sexport;
  sexport.handle = *exports;

  shim_val_t smodule;
  smodule.handle = *module;

  if (!shim_initialize(&ctx, &sexport, &smodule)) {
    if (!trycatch.HasCaught())
      shim_ThrowError(&ctx, "Failed to initialize module");
  }

  shim_cleanup_context(&ctx);
}

extern "C"
shim_obj_t*
shim_NewObject(shim_ctx_t* ctx, shim_class_t* clas, shim_obj_t* proto,
  shim_obj_t* parent)
{
  /* TODO if clas != NULL we should FunctionTemplate::New() */
  Local<Object> obj = Object::New();

  if (proto != NULL) {
    VAL_DEFINE(jsproto, proto);
    obj->SetPrototype(jsproto->ToObject());
  }

  /* TODO what does parent mean in v8 */

  return (shim_obj_t*)shim_val_alloc(ctx, obj);
}

extern "C"
void
shim_SetPrivate(shim_obj_t* obj, void* data)
{
  VAL_DEFINE(jsobj, obj);
  jsobj->ToObject()->SetHiddenValue(hidden_private, shim_external_new(data));
}

extern "C"
int
shim_SetProperty(shim_ctx_t* ctx, shim_val_t* obj, const char* name,
  shim_val_t* val)
{
  VAL_DEFINE(jsobj, obj);
  VAL_DEFINE(jsval, val);
  jsobj->ToObject()->Set(String::NewSymbol(name), jsval);
  return TRUE;
}

extern "C"
int
shim_SetPropertyById(shim_ctx_t* ctx, shim_obj_t* obj, uint32_t id,
  shim_val_t* val)
{
  VAL_DEFINE(jsobj, obj);
  VAL_DEFINE(jsval, val);
  jsobj->ToObject()->Set(id, jsval);
  return TRUE;
}

extern "C"
void*
shim_GetPrivate(shim_obj_t* obj)
{
  VAL_DEFINE(jsobj, obj);
  Local<Value> ext = jsobj->ToObject()->GetHiddenValue(hidden_private);
  return shim::shim_external_value(ext);
}

extern "C"
int
shim_GetProperty(shim_ctx_t* ctx, shim_obj_t* obj, const char* name,
  shim_val_t* val)
{
  VAL_DEFINE(jsobj, obj);
  Local<Value> tmp = jsobj->ToObject()->Get(String::New(name));
  val->handle = *tmp;
  return TRUE;
}

extern "C"
int
shim_GetPropertyById(shim_ctx_t*, shim_obj_t* obj, uint32_t id,
  shim_val_t* val)
{
  VAL_DEFINE(jsobj, obj);
  Local<Value> tmp = jsobj->ToObject()->Get(id);
  val->handle = *tmp;
  return TRUE;
}

Handle<Object>
shim_HandleFromFunc(shim_func cfunc, const char* name)
{
  Local<Value> data = shim_external_new(reinterpret_cast<void*>(cfunc));
  Local<FunctionTemplate> ft = FunctionTemplate::New(shim::Static, data);
  Local<Function> fh = ft->GetFunction();
  fh->SetName(String::New(name));
  return fh->ToObject();
}

extern "C"
shim_val_t*
shim_NewFunction(shim_ctx_t* ctx, shim_func cfunc, int argc, int flgas,
  shim_val_t* parent, const char* name)
{
  /* TODO */
  shim_val_t *ret = new shim_val_t;
  ret->handle = *shim_HandleFromFunc(cfunc, name);
  return ret;
}

extern "C"
shim_val_t*
shim_DefineFunction(shim_ctx_t* ctx, shim_val_t* obj, const char* name,
  shim_func cfunc, int argc, int flags)
{
  shim_val_t *func = shim::shim_NewFunction(ctx, cfunc, argc, flags, obj, name);
  shim::shim_SetProperty(ctx, obj, name, func);
  return func;
}

extern "C"
int
shim_DefineFunctions(shim_ctx_t* ctx, shim_val_t* obj,
  const shim_FunctionSpec* funcs)
{
  size_t i = 0;
  shim_FunctionSpec cur = funcs[i];
  while (cur.name != NULL) {
    shim::shim_DefineFunction(ctx, obj, cur.name, cur.cfunc, cur.nargs,
      cur.flags);
    i++;
    cur = funcs[i];
  }

  return TRUE;
}

extern "C"
int
shim_CallFunctionValue(shim_ctx_t* ctx, shim_val_t* obj, shim_val_t fval,
  size_t argc, shim_val_t** argv, shim_val_t* rval)
{
  Local<Object> jsobj;

  if (obj != NULL) {
    Local<Value> tmp = SHIM_TO_VAL(obj);
    jsobj = tmp->ToObject();
  } else {
    /* this isn't right, we probably want Null() */
    jsobj = Object::New();
  }

  /* TODO check is valid */
  Local<Value> prop = SHIM_TO_VAL(&fval);
  Local<Function> fn = Local<Function>::Cast(prop);
  Local<Value>* jsargs = shim_vals_to_handles(argc, argv);
  Local<Value> ret = fn->Call(jsobj, argc, jsargs);
  rval->handle = *ret;
  return TRUE;
}

extern "C"
int
shim_CallFunctionName(shim_ctx_t* ctx, shim_val_t* obj, const char* name,
  size_t argc, shim_val_t** argv, shim_val_t* rval)
{
  Local<Value> v = SHIM_TO_VAL(obj);
  Local<Object> jsobj = v->ToObject();
  Local<Value> prop = jsobj->Get(String::New(name));

  shim_val_t fval;
  QUEUE_INIT(&fval.member);
  fval.handle = *prop;

  return shim::shim_CallFunctionValue(ctx, obj, fval, argc, argv, rval);
}

extern "C"
int
shim_ValueToBoolean(shim_ctx_t* ctx, shim_val_t* obj, int* bp)
{
  VAL_DEFINE(tmp, obj);
  *bp = tmp->BooleanValue();
  return TRUE;
}

extern "C"
int
shim_ValueToECMAInt32(shim_ctx_t* ctx, shim_val_t* obj, int32_t* bp)
{
  VAL_DEFINE(tmp, obj);
  *bp = tmp->Int32Value();
  return TRUE;
}

extern "C"
int
shim_ValueToECMAUint32(shim_ctx_t* ctx, shim_val_t* obj, uint32_t* bp)
{
  VAL_DEFINE(tmp, obj);
  *bp = tmp->Uint32Value();
  return TRUE;
}

extern "C"
int
shim_ValueToUint16(shim_ctx_t* ctx, shim_val_t* obj, uint16_t* bp)
{
  VAL_DEFINE(tmp, obj);
  *bp = tmp->BooleanValue();
  return TRUE;
}

extern "C"
int
shim_ValueToNumber(shim_ctx_t* ctx, shim_val_t* obj, double* dp)
{
  VAL_DEFINE(tmp, obj);
  *dp = tmp->NumberValue();
  return TRUE;
}

extern "C"
shim_jstring_t*
shim_ValueToString(shim_ctx_t* ctx, shim_val_t* obj)
{
  VAL_DEFINE(tmp, obj);
  return shim_val_alloc(ctx, tmp->ToString());
}

extern "C"
int
shim_ConvertArguments(shim_ctx_t* ctx, size_t argc, shim_val_t** argv,
  const char* fmt, ...)
{
  size_t arg, cur;
  size_t len = strlen(fmt);

  shim_val_t* val;

  int* b;
  int32_t* i;
  uint32_t* u;
  double* d;
  shim_val_t** S = NULL;

  int ret = TRUE;

  /* todo set exception */
  if (argc < len)
    return FALSE;

  va_list list;
  va_start(list, fmt);

  for (arg = 0, cur = 0; arg < len; arg++) {
    val = argv[cur];
    switch(fmt[arg]) {
      case '/':
        continue;
        break;
      case '*':
        break;
      case 'b':
        b = va_arg(list, int*);
        ret = shim::shim_ValueToBoolean(ctx, val, b);
        break;
      case 'i':
        i = va_arg(list, int32_t*);
        ret = shim::shim_ValueToECMAInt32(ctx, val, i);
        break;
      case 'u':
        u = va_arg(list, uint32_t*);
        ret = shim::shim_ValueToECMAUint32(ctx, val, u);
        break;
      case 'd':
        d = va_arg(list, double*);
        ret = shim::shim_ValueToNumber(ctx, val, d);
        break;
      case 'S':
        S = va_arg(list, shim_jstring_t**);
        *S = shim::shim_ValueToString(ctx, val);
        break;
      default:
        printf("bad argument type %c\n", fmt[arg]);
        break;
    }
    if (!ret)
      goto END;
    cur++;
  }

END:
  va_end(list);
  return ret;
}

extern "C"
shim_val_t*
shim_NumberValue(double d)
{
  Local<Number> n = Number::New(d);
  return shim_val_alloc(NULL, n);
}

extern "C"
char*
shim_EncodeString(shim_ctx_t* ctx, shim_jstring_t* str)
{
  VAL_DEFINE(obj, str);
  String::Utf8Value jstr(obj);
  return strdup(*jstr);
}

extern "C"
size_t
shim_GetStringEncodingLength(shim_ctx_t* ctx, shim_jstring_t* str)
{
  VAL_DEFINE(obj, str);
  Local<String> jstr = Local<String>::Cast(obj);
  return jstr->Length();
}

extern "C"
size_t
shim_EncodeStringToBuffer(shim_jstring_t* str, char* buffer, size_t len)
{
  VAL_DEFINE(obj, str);
  Local<String> jstr = Local<String>::Cast(obj);
  return jstr->WriteUtf8(buffer, len);
}

extern "C"
shim_jstring_t*
shim_NewStringCopyN(shim_ctx_t* ctx, const char* src, size_t len)
{
  Local<String> jstr = String::New(src, len);
  return shim_val_alloc(ctx, jstr);
}

extern "C"
shim_jstring_t*
shim_NewStringCopyZ(shim_ctx_t* ctx, const char* src)
{
  Local<String> jstr = String::New(src);
  return shim_val_alloc(ctx, jstr);
}

extern "C"
int
shim_AddValueRoot(shim_ctx_t* ctx, shim_val_t* val)
{
  QUEUE_REMOVE(&val->member);
  VAL_DEFINE(obj, val);
#if NODE_VERSION_AT_LEAST(0, 11, 3)
  val->handle = *Persistent<Value>::New(static_cast<Isolate*>(ctx->isolate),
    obj);
#else
  val->handle = *Persistent<Value>::New(obj);
#endif
  return TRUE;
}

extern "C"
int
shim_RemoveValueRoot(shim_ctx_t* ctx, shim_val_t* val)
{
  QUEUE_INSERT_TAIL(&ctx->allocs, &val->member);
  Persistent<Value> v = SHIM_TO_VAL(val);
  Local<Value> tmp = Local<Value>::New(v);
  v.Dispose();
  val->handle = *tmp;
  return TRUE;
}

void
before_work(uv_work_t* req)
{
  shim_work_t* work = container_of(req, shim_work_t, req);
  work->work_cb(work);
}

void
#if NODE_VERSION_AT_LEAST(0, 10, 0)
before_after(uv_work_t* req, int status)
{
#else
before_after(uv_work_t* req)
{
  int status = 0;
#endif
  shim_work_t* work = container_of(req, shim_work_t, req);
  SHIM_PROLOGUE;
  work->after_cb(&ctx, work, status);
  shim_cleanup_context(&ctx);
}

extern "C"
void
shim_queue_work(shim_loop_t* loop, shim_work_t* work, shim_work_cb work_cb,
  shim_after_work after_cb)
{
  work->work_cb = work_cb;
  work->after_cb = after_cb;
  uv_queue_work(loop, &work->req, before_work, before_after);
}

extern "C"
shim_val_t*
shim_NewExternal(shim_ctx_t* ctx, void* data)
{
  return shim_val_alloc(ctx, shim_external_new(data));
}

extern "C"
void*
shim_ExternalValue(shim_ctx_t* ctx, shim_val_t* obj)
{
  VAL_DEFINE(jsobj, obj);
  return shim_external_value(jsobj);
}

typedef struct weak_baton_s {
  shim_weak_cb weak_cb;
  void* data;
} weak_baton_t;

void
#if NODE_VERSION_AT_LEAST(0, 11, 3)
common_weak_cb(Isolate* iso, Persistent<Value>* pobj, weak_baton_t* baton)
{
  Persistent<Value> obj = *pobj;
#else
common_weak_cb(Persistent<Value> obj, void* data)
{
  weak_baton_t* baton = static_cast<weak_baton_t*>(data);
#endif
  SHIM_PROLOGUE;
  shim_val_t* tmp = shim_val_alloc(NULL, obj);
  baton->weak_cb(tmp, baton->data);
  free(tmp);
}

extern "C"
void
shim_ValueMakeWeak(shim_ctx_t* ctx, shim_val_t* val, void* data,
  shim_weak_cb weak_cb)
{
  if(!QUEUE_EMPTY(&val->member))
    shim::shim_AddValueRoot(ctx, val);

  Persistent<Value> tmp(SHIM_TO_VAL(val));

  weak_baton_t *baton = new weak_baton_t;
  baton->weak_cb = weak_cb;
  baton->data = data;

  tmp.MakeWeak(baton, common_weak_cb);
}

extern "C"
void
shim_ValueDispose(shim_val_t* val)
{
  Persistent<Value> tmp(SHIM_TO_VAL(val));
  tmp.Dispose();
}

extern "C"
shim_val_t*
shim_NewArrayObject(shim_ctx_t* ctx, size_t len, shim_val_t* val)
{
  /* TODO implement vector initialization */
  return shim_val_alloc(ctx, Array::New(len));
}

extern "C"
int
shim_GetArrayLength(shim_ctx_t* ctx, shim_val_t* arr, uint32_t* len)
{
  VAL_DEFINE(jsobj, arr);
  Local<Array> jsarr = Local<Array>::Cast(jsobj);
  *len = jsarr->Length();
  return TRUE;
}

extern "C"
int
shim_GetElement(shim_ctx_t* ctx, shim_val_t* arr, int32_t idx, shim_val_t* val)
{
  VAL_DEFINE(jsobj, arr);
  Local<Array> jsarr = Local<Array>::Cast(jsobj);
  val->handle = *jsarr->Get(idx);
  return TRUE;
}

extern "C"
int
shim_SetElement(shim_ctx_t* ctx, shim_val_t* arr, int32_t idx, shim_val_t* val)
{
  VAL_DEFINE(jsobj, arr);
  VAL_DEFINE(jsval, val);
  Local<Array> jsarr = Local<Array>::Cast(jsobj);
  jsarr->Set(idx, jsval);
  return TRUE;
}

extern "C"
int
shim_IsExceptionPending(shim_ctx_t* ctx)
{
  TryCatch* trycatch = static_cast<TryCatch*>(ctx->trycatch);
  return trycatch->HasCaught() ? TRUE : FALSE;
}

extern "C"
void
shim_SetPendingException(shim_ctx_t* ctx, shim_val_t* val)
{
  VAL_DEFINE(err, val);
  ThrowException(err);
}

extern "C"
void
shim_ClearPendingException(shim_ctx_t* ctx)
{
  TryCatch* trycatch = static_cast<TryCatch*>(ctx->trycatch);
  trycatch->Reset();
}

extern "C"
int
shim_GetPendingException(shim_ctx_t* ctx, shim_val_t* rval)
{
  TryCatch* trycatch = static_cast<TryCatch*>(ctx->trycatch);
  rval->handle = *trycatch->Exception();
  return TRUE;
}

extern "C"
void
shim_ThrowError(shim_ctx_t* ctx, const char* msg)
{
  ThrowException(Exception::Error(String::New(msg)));
}

extern "C"
void
shim_ThrowTypeError(shim_ctx_t* ctx, const char* msg)
{
  ThrowException(Exception::TypeError(String::New(msg)));
}

extern "C"
void
shim_ThrowRangeError(shim_ctx_t* ctx, const char* msg)
{
  ThrowException(Exception::TypeError(String::New(msg)));
}

}
