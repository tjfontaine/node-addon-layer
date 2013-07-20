#include "v8.h"
#include "node.h"

#include "shim.h"

using v8::Arguments;
using v8::External;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Isolate;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Undefined;
using v8::Value;

#define SHIM_PROLOGUE \
Isolate *isolate = Isolate::GetCurrent(); \
HandleScope scope; \
shim_ctx_t ctx; \
QUEUE_INIT(&ctx.allocs); \
ctx.isolate = static_cast<void*>(isolate); \
ctx.scope = static_cast<void*>(&scope); \
do {} while(0)


#define SHIM_TO_OBJ(obj) static_cast<Object*>((obj)->handle)

#define OBJ_DEFINE(var, obj) Handle<Object> var(SHIM_TO_OBJ(obj))


namespace shim {

void
shim_cleanup_context(shim_ctx_t *ctx)
{
  QUEUE* q = NULL;
  QUEUE_FOREACH(q, &ctx->allocs) {
    shim_obj_t* o = container_of(q, shim_obj_t, member);
    free(o);
  }
}

shim_obj_t*
shim_obj_alloc(shim_ctx_t *ctx, Handle<Value> val)
{
  shim_obj_t* obj = (shim_obj_t*)malloc(sizeof(shim_obj_t));
  QUEUE_INIT(&obj->member);
  if (ctx != NULL)
    QUEUE_INSERT_TAIL(&ctx->allocs, &obj->member);
  Handle<Object> tmp = Handle<Object>::Cast(val);
  obj->handle = *tmp;
  return obj;
}

Handle<Value>
Static(const Arguments& args)
{
  SHIM_PROLOGUE;

  Handle<External> data = Handle<External>::Cast(args.Data());
  shim_func cfunc = reinterpret_cast<shim_func>(data->Value());

  size_t argc = args.Length();

  shim_args_t sargs;
  shim_obj_t* to = (shim_obj_t*)malloc(sizeof(shim_obj_t*) * argc);
  sargs.argv = to;
  sargs.ret = NULL;

  shim_obj_t** argv = &sargs.argv;

  for (size_t i = 0; i < argc; i++) {
    argv[i] = shim_obj_alloc(&ctx, args[i]);
  }

  if(!cfunc(&ctx, argc, &sargs.argv)) {
    // TODO XXX FIXME
    printf("function failed\n");
  }

  Handle<Value> ret;

  if(sargs.ret != NULL) {
    OBJ_DEFINE(tmp, sargs.ret);
    ret = tmp;
  } else {
    ret = Undefined();
  }

  shim_cleanup_context(&ctx);
  free(to);

  return scope.Close(ret);
}

void Initialize(Handle<Object> exports, Handle<Value> module)
{
  SHIM_PROLOGUE;

  shim_obj_t sexport;
  sexport.handle = *exports;

  shim_obj_t smodule;
  smodule.handle = *module;

  if (shim_initialize(&ctx, &sexport, &smodule)) {
    printf("initialized\n");
  } else {
    printf("failed to initialize\n");
  }

  shim_cleanup_context(&ctx);
}

extern "C"
void
shim_SetProperty(shim_ctx_t* ctx, shim_obj_t* obj, const char* name,
  shim_obj_t* val)
{
  OBJ_DEFINE(jsobj, obj);
  OBJ_DEFINE(jsval, val);
  jsobj->Set(String::NewSymbol(name), jsval);
}

Handle<Object>
shim_HandleFromFunc(shim_func cfunc, const char* name)
{
  Handle<External> data = External::New(reinterpret_cast<void*>(cfunc));
  Handle<FunctionTemplate> ft = FunctionTemplate::New(shim::Static, data);
  Handle<Function> fh = ft->GetFunction();
  fh->SetName(String::New(name));
  return fh->ToObject();
}

extern "C"
shim_obj_t*
shim_NewFunction(shim_ctx_t* ctx, shim_func cfunc, int argc, int flgas,
  shim_obj_t* parent, const char* name)
{
  /* TODO */
  shim_obj_t *ret = new shim_obj_t;
  ret->handle = *shim_HandleFromFunc(cfunc, name);
  return ret;
}

extern "C"
shim_obj_t*
shim_DefineFunction(shim_ctx_t* ctx, shim_obj_t* obj, const char* name,
  shim_func cfunc, int argc, int flags)
{
  shim_obj_t *func = shim::shim_NewFunction(ctx, cfunc, argc, flags, obj, name);
  shim::shim_SetProperty(ctx, obj, name, func);
  return func;
}

extern "C"
int
shim_DefineFunctions(shim_ctx_t* ctx, shim_obj_t* obj,
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
shim_ValueToBoolean(shim_ctx_t* ctx, shim_obj_t* obj, int* bp)
{
  OBJ_DEFINE(tmp, obj);
  *bp = tmp->BooleanValue();
  return TRUE;
}

extern "C"
int
shim_ValueToECMAInt32(shim_ctx_t* ctx, shim_obj_t* obj, int32_t* bp)
{
  OBJ_DEFINE(tmp, obj);
  *bp = tmp->Int32Value();
  return TRUE;
}

extern "C"
int
shim_ValueToECMAUint32(shim_ctx_t* ctx, shim_obj_t* obj, uint32_t* bp)
{
  OBJ_DEFINE(tmp, obj);
  *bp = tmp->Uint32Value();
  return TRUE;
}

extern "C"
int
shim_ValueToUint16(shim_ctx_t* ctx, shim_obj_t* obj, uint16_t* bp)
{
  OBJ_DEFINE(tmp, obj);
  *bp = tmp->BooleanValue();
  return TRUE;
}

extern "C"
int
shim_ValueToNumber(shim_ctx_t* ctx, shim_obj_t* obj, double* dp)
{
  OBJ_DEFINE(tmp, obj);
  *dp = tmp->NumberValue();
  return TRUE;
}

extern "C"
shim_jstring_t*
shim_ValueToString(shim_ctx_t* ctx, shim_obj_t* obj)
{
  OBJ_DEFINE(tmp, obj);
  return shim_obj_alloc(ctx, tmp->ToString());
}

extern "C"
int
shim_ConvertArguments(shim_ctx_t* ctx, size_t argc, shim_obj_t** argv,
  const char* fmt, ...)
{
  size_t arg, cur;
  size_t len = strlen(fmt);

  shim_obj_t* val;

  int* b;
  int32_t* i;
  uint32_t* u;
  double* d;
  shim_obj_t** S = NULL;

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
shim_obj_t*
shim_NumberValue(double d)
{
  Handle<Number> n = Number::New(d);
  return shim_obj_alloc(NULL, n);
}

extern "C"
char*
shim_EncodeString(shim_ctx_t* ctx, shim_jstring_t* str)
{
  OBJ_DEFINE(obj, str);
  String::Utf8Value jstr(obj);
  return strdup(*jstr);
}

extern "C"
size_t
shim_GetStringEncodingLength(shim_ctx_t* ctx, shim_jstring_t* str)
{
  OBJ_DEFINE(obj, str);
  Handle<String> jstr = Handle<String>::Cast(obj);
  return jstr->Length();
}

extern "C"
size_t
shim_EncodeStringToBuffer(shim_jstring_t* str, char* buffer, size_t len)
{
  OBJ_DEFINE(obj, str);
  Handle<String> jstr = Handle<String>::Cast(obj);
  return jstr->WriteUtf8(buffer, len);
}

}

NODE_MODULE(shim, shim::Initialize)
