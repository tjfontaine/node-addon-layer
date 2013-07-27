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
#include "uv.h"
#include "shim-impl.h"

#if NODE_VERSION_AT_LEAST(0, 11, 3)
#define V8_USE_UNSAFE_HANDLES 1
#define V8_ALLOW_ACCESS_TO_RAW_HANDLE_CONSTRUCTOR 1
#endif

#include "v8.h"
#include "node.h"
#include "node_buffer.h"

namespace shim {

#if NODE_VERSION_AT_LEAST(0, 11, 3)
using v8::FunctionCallbackInfo;
#else
using node::Buffer;
using v8::Arguments;
#endif

using v8::Array;
using v8::Exception;
using v8::External;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Null;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::TryCatch;
using v8::Undefined;
using v8::Value;


#define SHIM_PROLOGUE(ctx)                                                    \
  Isolate* ctx ## _isolate = Isolate::GetCurrent();                           \
  HandleScope ctx ## _scope;                                                  \
  TryCatch ctx ## _trycatch;                                                  \
  shim_ctx_t ctx;                                                             \
  ctx.isolate = static_cast<void*>(ctx ## _isolate);                          \
  ctx.scope = static_cast<void*>(&ctx ## _scope);                             \
  ctx.trycatch = static_cast<void*>(&ctx ## _trycatch);                       \
do {} while(0)


#define SHIM_TO_VAL(obj) Local<Value>(static_cast<Value*>((obj)->handle))

#define VAL_DEFINE(var, obj) Local<Value> var(SHIM_TO_VAL(obj))


Persistent<String> hidden_private;

void
shim_context_cleanup(shim_ctx_t* ctx)
{
}


shim_val_t*
shim_val_alloc(shim_ctx_t* ctx, Handle<Value> val,
  shim_type_t type = SHIM_TYPE_UNKNOWN)
{
  shim_val_t* obj = static_cast<shim_val_t*>(malloc(sizeof(shim_val_t)));
  obj->handle = *val;
  obj->type = type;
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


#if NODE_VERSION_AT_LEAST(0, 11, 3)
void
Static(const FunctionCallbackInfo<Value>& args)
#else
Handle<Value>
Static(const Arguments& args)
#endif
{
  SHIM_PROLOGUE(ctx);

  Local<Value> data = args.Data();

  assert(data->IsExternal());

  Local<External> ext = data.As<External>();
  shim_func cfunc = reinterpret_cast<shim_func>(ext->Value());

  shim_args_t sargs;
  sargs.argc = args.Length();
  sargs.argv = NULL;
  sargs.ret = NULL;
  sargs.self = shim_val_alloc(&ctx, args.This());

  size_t argv_len = sizeof(shim_val_t*) * sargs.argc;

  if (argv_len > 0)
    sargs.argv = static_cast<shim_val_t**>(malloc(argv_len));

  size_t i;

  for (i = 0; i < sargs.argc; i++) {
    sargs.argv[i] = shim_val_alloc(&ctx, args[i]);
  }

  if(!cfunc(&ctx, &sargs)) {
    /* the function failed do we need to do any more checking of exceptions? */
  }

  Value* ret;

  if(sargs.ret != NULL) {
    ret = static_cast<Value*>(sargs.ret->handle);
    free(sargs.ret);
  }

  for (i = 0; i < sargs.argc; i++) {
    free(sargs.argv[i]);
  }

  shim_context_cleanup(&ctx);

#if NODE_VERSION_AT_LEAST(0, 11, 3)
  if (!ctx_trycatch.HasCaught())
    args.GetReturnValue().Set(ret);
#else
  if (ctx_trycatch.HasCaught()) {
    return ctx_trycatch.Exception();
  } else {
    return ctx_scope.Close(Handle<Value>(ret));
  }
#endif
}

extern "C"
{

void shim_module_initialize(Handle<Object> exports, Handle<Value> module)
{
  SHIM_PROLOGUE(ctx);

  if (hidden_private.IsEmpty()) {
    Handle<String> str = String::NewSymbol("shim_private");
#if NODE_VERSION_AT_LEAST(0, 11, 3)
    hidden_private = Persistent<String>::New(ctx_isolate, str);
#else
    hidden_private = Persistent<String>::New(str);
#endif
  }

  shim_val_t sexport;
  sexport.handle = *exports;

  shim_val_t smodule;
  smodule.handle = *module;

  if (!shim_initialize(&ctx, &sexport, &smodule)) {
    if (!ctx_trycatch.HasCaught())
      shim_throw_error(&ctx, "Failed to initialize module");
  }

  shim_context_cleanup(&ctx);
}


shim_bool_t
shim_value_is(shim_val_t* val, shim_type_t type)
{
  if (val->type == type)
    return TRUE;

  VAL_DEFINE(obj, val);
  shim_bool_t ret = FALSE;

  switch (type)
  {
    case SHIM_TYPE_OBJECT:
      ret = obj->IsObject();
      break;
    case SHIM_TYPE_STRING:
      ret = obj->IsString();
      break;
    case SHIM_TYPE_NUMBER:
      ret = obj->IsNumber();
      break;
    case SHIM_TYPE_INTEGER:
      /* TODO */
      ret = obj->IsNumber();
      break;
    case SHIM_TYPE_INT32:
      ret = obj->IsInt32();
      break;
    case SHIM_TYPE_UINT32:
      ret = obj->IsUint32();
      break;
    case SHIM_TYPE_ARRAY:
      ret = obj->IsArray();
      break;
    case SHIM_TYPE_BOOL:
      ret = obj->IsBoolean();
      break;
    case SHIM_TYPE_UNDEFINED:
      ret = obj->IsUndefined();
      break;
    case SHIM_TYPE_NULL:
      ret = obj->IsNull();
      break;
    case SHIM_TYPE_EXTERNAL:
      ret = obj->IsExternal();
      break;
    case SHIM_TYPE_DATE:
      ret = obj->IsDate();
      break;
    case SHIM_TYPE_FUNCTION:
      ret = obj->IsFunction();
      break;
    case SHIM_TYPE_UNKNOWN:
    default:
      ret = FALSE;
      break;
  }

  if (ret)
    val->type = type;

  return ret;
}


#define OBJ_TO_ARRAY(obj) \
  ((obj)->IsArray() ? (obj).As<Array>() : Local<Array>::Cast(obj))

#define OBJ_TO_OBJECT(obj) \
  ((obj)->IsObject() ? (obj).As<Object>() : (obj)->ToObject())

#define OBJ_TO_STRING(obj) \
  ((obj)->IsObject() ? (obj).As<String>() : (obj)->ToString())

#define OBJ_TO_INT32(obj) \
  ((obj)->ToInt32())

#define OBJ_TO_UINT32(obj) \
  ((obj)->ToUint32())

#define OBJ_TO_NUMBER(obj) \
  ((obj)->IsNumber() ? (obj).As<Number>() : (obj)->ToNumber())

#define OBJ_TO_EXTERNAL(obj) \
  ((obj)->IsExternal() ? (obj).As<External>() : Local<External>::Cast(obj))

#define OBJ_TO_FUNCTION(obj) \
  ((obj)->IsFunction() ? (obj).As<Function>() : Local<Function>::Cast(obj))


shim_bool_t
shim_value_to(shim_ctx_t* ctx, shim_val_t* val, shim_type_t type,
  shim_val_t* rval)
{
  if (val->type == type) {
    rval->type = type;
    rval->handle = val->handle;
    return TRUE;
  }

  VAL_DEFINE(obj, val);

  switch (type) {
    case SHIM_TYPE_UNDEFINED:
      rval->handle = *Undefined();
      break;
    case SHIM_TYPE_NULL:
      rval->handle = *Null();
      break;
    case SHIM_TYPE_BOOL:
      rval->handle = *obj->ToBoolean();
      break;
    case SHIM_TYPE_ARRAY:
      rval->handle = *OBJ_TO_ARRAY(obj);
      break;
    case SHIM_TYPE_OBJECT:
      rval->handle = *OBJ_TO_OBJECT(obj);
      break;
    case SHIM_TYPE_INTEGER:
      /* TODO */
      rval->handle = *OBJ_TO_NUMBER(obj);
      break;
    case SHIM_TYPE_INT32:
      rval->handle = *OBJ_TO_INT32(obj);
      break;
    case SHIM_TYPE_UINT32:
      rval->handle = *OBJ_TO_UINT32(obj);
      break;
    case SHIM_TYPE_NUMBER:
      rval->handle = *OBJ_TO_NUMBER(obj);
      break;
    case SHIM_TYPE_EXTERNAL:
      rval->handle = *OBJ_TO_EXTERNAL(obj);
      break;
    case SHIM_TYPE_FUNCTION:
      rval->handle = *OBJ_TO_FUNCTION(obj);
      break;
    case SHIM_TYPE_STRING:
      rval->handle = *OBJ_TO_STRING(obj);
    case SHIM_TYPE_UNKNOWN:
    default:
      return FALSE;
  }

  rval->type = type;
  return TRUE;
}


void
shim_value_release(shim_val_t* val)
{
  free(val);
}


shim_val_t*
shim_new_object(shim_ctx_t* ctx, shim_val_t* klass, shim_val_t* proto)
{
  /* TODO if klass != NULL we should FunctionTemplate::New() */
  Local<Object> obj = Object::New();

  if (proto != NULL) {
    VAL_DEFINE(jsproto, proto);
    obj->SetPrototype(jsproto->ToObject());
  }

  return shim_val_alloc(ctx, obj);
}


shim_val_t*
shim_obj_new_instance(shim_ctx_t* ctx, shim_val_t* klass, size_t argc,
  shim_val_t** argv)
{
  return NULL;
}


shim_val_t*
shim_obj_clone(shim_ctx_t* ctx, shim_val_t* src)
{
  Local<Value> dst = Local<Value>::New(SHIM_TO_VAL(src));
  return shim_val_alloc(ctx, dst);
}


shim_bool_t
shim_obj_has_name(shim_ctx_t* ctx, shim_val_t* val, const char* name)
{
  Local<Object> obj = OBJ_TO_OBJECT(SHIM_TO_VAL(val));
  return obj->Has(String::NewSymbol(name)) ? TRUE : FALSE;
}


shim_bool_t
shim_obj_has_id(shim_ctx_t* ctx, shim_val_t* val, uint32_t id)
{
  Local<Object> obj = OBJ_TO_OBJECT(SHIM_TO_VAL(val));
  return obj->Has(id) ? TRUE : FALSE;
}


shim_bool_t
shim_obj_has_sym(shim_ctx_t* ctx, shim_val_t* val, shim_val_t* sym)
{
  Local<Object> obj = OBJ_TO_OBJECT(SHIM_TO_VAL(val));
  return obj->Has(OBJ_TO_STRING(SHIM_TO_VAL(sym)));
}


shim_bool_t
shim_obj_set_prop_name(shim_ctx_t* ctx, shim_val_t* obj, const char* name,
  shim_val_t* val)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  return jsobj->Set(String::NewSymbol(name), SHIM_TO_VAL(val));
}


shim_bool_t
shim_obj_set_prop_id(shim_ctx_t* ctx, shim_val_t* obj, uint32_t id,
  shim_val_t* val)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  return jsobj->Set(id, SHIM_TO_VAL(val));
}


shim_bool_t
shim_obj_set_prop_sym(shim_ctx_t* ctx, shim_val_t* obj, shim_val_t* sym,
  shim_val_t* val)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  return jsobj->Set(SHIM_TO_VAL(sym), SHIM_TO_VAL(val));
}


shim_bool_t
shim_obj_set_private(shim_ctx_t* ctx, shim_val_t* obj, void* data)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  return jsobj->SetHiddenValue(hidden_private, External::New(data));
}


int
shim_obj_set_funcs(shim_ctx_t* ctx, shim_val_t* recv,
  const shim_fspec_t* funcs)
{
  size_t i = 0;
  shim_fspec_t cur = funcs[i];
  while (cur.name != NULL) {
    shim_val_t* func = shim_func_new(ctx, cur.cfunc, cur.nargs, cur.flags,
      cur.name, cur.data);

    if (!shim::shim_obj_set_prop_name(ctx, recv, cur.name, func))
      return FALSE;

    cur = funcs[++i];
  }

  return TRUE;
}


shim_bool_t
shim_obj_get_prop_name(shim_ctx_t* ctx, shim_val_t* obj, const char* name,
  shim_val_t* rval)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  Local<Value> val = jsobj->Get(String::NewSymbol(name));
  rval->handle = *val;
  rval->type = SHIM_TYPE_UNKNOWN;
  return TRUE;
}


shim_bool_t
shim_obj_get_prop_id(shim_ctx_t* ctx, shim_val_t* obj, uint32_t idx,
  shim_val_t* rval)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  Local<Value> val = jsobj->Get(idx);
  rval->handle = *val;
  rval->type = SHIM_TYPE_UNKNOWN;
  return TRUE;
}


shim_bool_t
shim_obj_get_prop_sym(shim_ctx_t* ctx, shim_val_t* obj, shim_val_t* sym,
  shim_val_t* rval)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  Local<Value> val = jsobj->Get(SHIM_TO_VAL(sym));
  rval->handle = *val;
  rval->type = SHIM_TYPE_UNKNOWN;
  return TRUE;
}


void*
shim_obj_get_private(shim_ctx_t* ctx, shim_val_t* obj)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  Local<Value> ext = jsobj->GetHiddenValue(hidden_private);
  return ext.As<External>()->Value();
}


shim_val_t*
shim_persistent_new(shim_ctx_t* ctx, shim_val_t* val)
{
  VAL_DEFINE(obj, val);
  Persistent<Value> pobj = Persistent<Value>::New(
#if NODE_VERSION_AT_LEAST(0, 11, 3)
    static_cast<Isolate*>(ctx->isolate),
#endif
    obj);
  /* TODO flag a val as persistent */
  return shim_val_alloc(ctx, pobj);
}


void
shim_persistent_dispose(shim_val_t* val)
{
  Persistent<Value> tmp(SHIM_TO_VAL(val));
  tmp.Dispose();
  free(val);
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
  SHIM_PROLOGUE(ctx);
  shim_val_t* tmp = shim_val_alloc(NULL, obj);
  baton->weak_cb(tmp, baton->data);
}


void
shim_obj_make_weak(shim_ctx_t* ctx, shim_val_t* val, void* data,
  shim_weak_cb weak_cb)
{
  /* TODO check that this is not a persistent? */
  Persistent<Value> tmp(SHIM_TO_VAL(val));

  weak_baton_t *baton = new weak_baton_t;
  baton->weak_cb = weak_cb;
  baton->data = data;

  tmp.MakeWeak(baton, common_weak_cb);
}


void
shim_obj_clear_weak(shim_val_t* val)
{
  Persistent<Value> tmp(SHIM_TO_VAL(val));
  tmp.ClearWeak();
}


shim_val_t*
shim_func_new(shim_ctx_t* ctx, shim_func cfunc, size_t argc, int32_t flags,
  const char* name, void* hint)
{
  Local<External> ext = External::New(reinterpret_cast<void*>(cfunc));
  Local<FunctionTemplate> ft = FunctionTemplate::New(shim::Static, ext);
  Local<Function> fh = ft->GetFunction();
  fh->SetName(String::NewSymbol(name));
  return shim_val_alloc(ctx, fh);
}


shim_bool_t
shim_func_call_val(shim_ctx_t* ctx, shim_val_t* self, shim_val_t* fval,
  size_t argc, shim_val_t** argv, shim_val_t* rval)
{
  /* TODO check is valid */
  Local<Value> prop = SHIM_TO_VAL(fval);
  Local<Function> fn = Local<Function>::Cast(prop);
  Local<Value>* jsargs = shim_vals_to_handles(argc, argv);

  Local<Value> ret;

  if (self != NULL) {
    Local<Object> obj = OBJ_TO_OBJECT(SHIM_TO_VAL(self));
    ret = fn->Call(obj, argc, jsargs);
  } else {
    ret = fn->Call(Object::New(), argc, jsargs);
  }

  rval->handle = *ret;
  return TRUE;
}


int
shim_func_call_name(shim_ctx_t* ctx, shim_val_t* obj, const char* name,
  size_t argc, shim_val_t** argv, shim_val_t* rval)
{
  Local<Object> jsobj = OBJ_TO_OBJECT(SHIM_TO_VAL(obj));
  Local<Value> prop = jsobj->Get(String::NewSymbol(name));

  shim_val_t fval;
  fval.handle = *prop;

  return shim::shim_func_call_val(ctx, obj, &fval, argc, argv, rval);
}


shim_val_t*
shim_number_new(shim_ctx_t* ctx, double d)
{
  return shim_val_alloc(ctx, Number::New(d));
}


double
shim_number_val(shim_val_t* val)
{
  return SHIM_TO_VAL(val)->NumberValue();
}


shim_val_t*
shim_integer_new(shim_ctx_t* ctx, int32_t i)
{
  return shim_val_alloc(ctx, Integer::New(i));
}


shim_val_t*
shim_integer_uint(shim_ctx_t* ctx, uint32_t i)
{
  return shim_val_alloc(ctx, Integer::NewFromUnsigned(i));
}


int64_t
shim_integer_value(shim_val_t* val)
{
  return SHIM_TO_VAL(val)->IntegerValue();
}


int32_t
shim_integer_int32_value(shim_val_t* val)
{
  return SHIM_TO_VAL(val)->Int32Value();
}


uint32_t
shim_integer_uint32_value(shim_val_t* val)
{
  return SHIM_TO_VAL(val)->Uint32Value();
}


shim_val_t*
shim_string_new(shim_ctx_t* ctx)
{
  return shim_val_alloc(ctx, String::Empty());
}


shim_val_t*
shim_string_new_copy(shim_ctx_t* ctx, const char* data)
{
  return shim_val_alloc(ctx, String::New(data));
}


shim_val_t*
shim_string_new_copyn(shim_ctx_t* ctx, const char* data, size_t len)
{
  return shim_val_alloc(ctx, String::New(data, len));
}


size_t
shim_string_length(shim_val_t* val)
{
  return OBJ_TO_STRING(SHIM_TO_VAL(val))->Length();
}


size_t
shim_string_length_utf8(shim_val_t* val)
{
  return OBJ_TO_STRING(SHIM_TO_VAL(val))->Utf8Length();
}


const char*
shim_string_value(shim_val_t* val)
{
  return *String::Utf8Value(OBJ_TO_STRING(SHIM_TO_VAL(val)));
}


size_t
shim_string_write_ascii(shim_val_t* val, char* buff, size_t start, size_t len,
  int32_t options)
{
  return OBJ_TO_STRING(SHIM_TO_VAL(val))->WriteAscii(buff, start, len);
}


shim_val_t*
shim_array_new(shim_ctx_t* ctx, size_t len)
{
  return shim_val_alloc(ctx, Array::New(len));
}


size_t
shim_array_length(shim_val_t* arr)
{
  return OBJ_TO_ARRAY(SHIM_TO_VAL(arr))->Length();
}


shim_bool_t
shim_array_get(shim_ctx_t* ctx, shim_val_t* arr, int32_t idx, shim_val_t* rval)
{
  rval->handle = *OBJ_TO_ARRAY(SHIM_TO_VAL(arr))->Get(idx);
  return TRUE;
}


shim_bool_t
shim_array_set(shim_ctx_t* ctx, shim_val_t* arr, int32_t idx, shim_val_t* val)
{
  return OBJ_TO_ARRAY(SHIM_TO_VAL(arr))->Set(idx, SHIM_TO_VAL(val));
}


shim_val_t*
shim_buffer_new(shim_ctx_t* ctx, size_t len)
{
#if NODE_VERSION_AT_LEAST(0, 11, 3)
  return shim_val_alloc(ctx, node::Buffer::New(len));
#else
  return shim_val_alloc(ctx, Buffer::New(len)->handle_);
#endif
}


shim_val_t*
shim_buffer_new_copy(shim_ctx_t* ctx, const char* data, size_t len)
{
#if NODE_VERSION_AT_LEAST(0, 11, 3)
  return shim_val_alloc(ctx, node::Buffer::New(data, len));
#else
  return shim_val_alloc(ctx, Buffer::New(data, len)->handle_);
#endif
}


shim_val_t*
shim_buffer_new_external(shim_ctx_t* ctx, char* data, size_t len,
  shim_buffer_free cb, void* hint)
{
#if NODE_VERSION_AT_LEAST(0, 11, 3)
  return shim_val_alloc(ctx, node::Buffer::New(data, len, cb, hint));
#else
  return shim_val_alloc(ctx, Buffer::New(data, len, cb, hint)->handle_);
#endif
}


char*
shim_buffer_value(shim_val_t* val)
{
#if NODE_VERSION_AT_LEAST(0, 11, 3)
  return node::Buffer::Data(SHIM_TO_VAL(val));
#else
  return Buffer::Data(SHIM_TO_VAL(val));
#endif
}


size_t
shim_buffer_length(shim_val_t* val)
{
#if NODE_VERSION_AT_LEAST(0, 11, 3)
  return node::Buffer::Length(SHIM_TO_VAL(val));
#else
  return Buffer::Length(SHIM_TO_VAL(val));
#endif
}


shim_val_t*
shim_external_new(shim_ctx_t* ctx, void* data)
{
  return shim_val_alloc(ctx, External::New(data));
}


void*
shim_external_value(shim_ctx_t* ctx, shim_val_t* obj)
{
  return SHIM_TO_VAL(obj).As<External>()->Value();
}


shim_val_t*
shim_error_new(shim_ctx_t* ctx, const char* msg)
{
  return shim_val_alloc(ctx, Exception::Error(String::New(msg)));
}


shim_val_t*
shim_error_type_new(shim_ctx_t* ctx, const char* msg)
{
  return shim_val_alloc(ctx, Exception::TypeError(String::New(msg)));
}


shim_val_t*
shim_error_range_new(shim_ctx_t* ctx, const char* msg)
{
  return shim_val_alloc(ctx, Exception::RangeError(String::New(msg)));
}


shim_bool_t
shim_exception_pending(shim_ctx_t* ctx)
{
  TryCatch* trycatch = static_cast<TryCatch*>(ctx->trycatch);
  return trycatch->HasCaught();
}


void
shim_exception_set(shim_ctx_t* ctx, shim_val_t* val)
{
  ThrowException(SHIM_TO_VAL(val));
}


int
shim_exception_get(shim_ctx_t* ctx, shim_val_t* rval)
{
  TryCatch* trycatch = static_cast<TryCatch*>(ctx->trycatch);
  rval->handle = *trycatch->Exception();
  return TRUE;
}


void
shim_exception_clear(shim_ctx_t* ctx)
{
  TryCatch* trycatch = static_cast<TryCatch*>(ctx->trycatch);
  trycatch->Reset();
}


void
shim_throw_error(shim_ctx_t* ctx, const char* msg)
{
  ThrowException(Exception::Error(String::New(msg)));
}


void
shim_throw_type_error(shim_ctx_t* ctx, const char* msg)
{
  ThrowException(Exception::TypeError(String::New(msg)));
}


void
shim_throw_range_error(shim_ctx_t* ctx, const char* msg)
{
  ThrowException(Exception::TypeError(String::New(msg)));
}


shim_bool_t
shim_unpack_type(shim_ctx_t* ctx, shim_val_t* arg, shim_type_t type,
  void* rval)
{
  switch(type) {
    case SHIM_TYPE_BOOL:
      *(shim_bool_t*)rval = SHIM_TO_VAL(arg)->BooleanValue();
      break;
    case SHIM_TYPE_INTEGER:
      *(int64_t*)rval = SHIM_TO_VAL(arg)->IntegerValue();
      break;
    case SHIM_TYPE_UINT32:
      *(uint32_t*)rval = SHIM_TO_VAL(arg)->Uint32Value();
      break;
    case SHIM_TYPE_INT32:
      *(int32_t*)rval = SHIM_TO_VAL(arg)->Int32Value();
      break;
    case SHIM_TYPE_NUMBER:
      *(double*)rval = SHIM_TO_VAL(arg)->NumberValue();
      break;
    case SHIM_TYPE_EXTERNAL:
      *(void**)rval = shim::shim_external_value(ctx, arg);
      break;
    case SHIM_TYPE_STRING:
      if(!shim::shim_value_to(ctx, arg, SHIM_TYPE_STRING, *(shim_val_t**)rval))
        return FALSE;
      break;
    case SHIM_TYPE_UNDEFINED:
    case SHIM_TYPE_NULL:
    case SHIM_TYPE_DATE:
    case SHIM_TYPE_ARRAY:
    case SHIM_TYPE_OBJECT:
    case SHIM_TYPE_FUNCTION:
    default:
      return FALSE;
      break;
  }

  return TRUE;
}


shim_bool_t
shim_unpack_one(shim_ctx_t* ctx, shim_args_t* args, uint32_t idx,
  shim_type_t type, void* rval)
{
  shim_val_t* arg = args->argv[idx];
  return shim::shim_unpack_type(ctx, arg, type, rval);
}


shim_bool_t
shim_unpack(shim_ctx_t* ctx, shim_args_t* args, shim_type_t type, ...)
{
  size_t cur;
  shim_type_t ctype = type;
  va_list ap;

  va_start(ap, type);

  for (cur = 0, ctype = type; ctype != SHIM_TYPE_UNKNOWN && cur < args->argc; cur++)
  {
    void* rval = va_arg(ap, void*);

    if(!shim_unpack_one(ctx, args, cur, ctype, rval))
      return FALSE;

    ctype = static_cast<shim_type_t>(va_arg(ap, int));
  }

  va_end(ap);

  return TRUE;
}


size_t
shim_args_length(shim_args_t* args)
{
  return args->argc;
}


shim_val_t*
shim_args_get(shim_args_t* args, size_t idx)
{
  /* TODO assert */
  return args->argv[idx];
}


shim_bool_t
shim_args_set_rval(shim_ctx_t* ctx, shim_args_t* args, shim_val_t* val)
{
  /* TODO check if persistent */
  /*
  if (!shim_has_allocd(ctx, val))
    allocs_from_ctx(ctx)->insert(val);
  */
  args->ret = val;
  return TRUE;
}


shim_val_t*
shim_args_get_this(shim_ctx_t* ctx, shim_args_t* args)
{
  return args->self;
}


void*
shim_args_get_data(shim_ctx_t* ctx, shim_args_t* args)
{
  /* TODO */
  return NULL;
}


void
before_work(uv_work_t* req)
{
  shim_work_t* work = container_of(req, shim_work_t, req);
  work->work_cb(work, work->hint);
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
  SHIM_PROLOGUE(ctx);
  shim_work_t* work = container_of(req, shim_work_t, req);
  work->after_cb(&ctx, work, status, work->hint);
  shim_context_cleanup(&ctx);
  delete work;
}

void
shim_queue_work(shim_work_cb work_cb,
  shim_after_work after_cb, void* hint)
{
  shim_work_t* work = new shim_work_t;
  work->work_cb = work_cb;
  work->after_cb = after_cb;
  work->hint = hint;
  work->req.data = work;
  uv_queue_work(uv_default_loop(), &work->req, before_work, before_after);
}

}

}
