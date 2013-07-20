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

#ifndef NODE_SHIM_H
#define NODE_SHIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "queue.h"
#include "uv.h"

#ifndef offset_of
// g++ in strict mode complains loudly about the system offsetof() macro
// because it uses NULL as the base address.
# define offset_of(type, member) \
  ((intptr_t) ((char *) &(((type *) 8)->member) - 8))
#endif

#ifndef container_of
# define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offset_of(type, member)))
#endif

typedef struct shim_val_s {
  QUEUE member;
  void* handle;
} shim_val_t;

/* JSAPI has the concept of a few other types that we just wrap normally */
typedef shim_val_t shim_jstring_t;
typedef shim_val_t shim_obj_t;
typedef shim_val_t shim_class_t;

typedef struct shim_ctx_s {
  void* scope;
  void* isolate;
  void* trycatch;
  QUEUE allocs;
} shim_ctx_t;

typedef struct shim_args_s {
  shim_val_t **argv;
  shim_val_t *ret;
} shim_args_t;

typedef void (* node_register_func)(void*, void*);
void shim_module_initialize(void*, void*);

typedef int (* register_func)(shim_ctx_t*, shim_val_t*, shim_val_t*);
extern register_func shim_initialize;

struct shim_module_struct {
  int version;
  void* handle;
  const char* fname;
  node_register_func func;
  const char* mname;
};

#define SHIM_MODULE_VERSION 0x000B

#define SHIM_MODULE(name, func)                                               \
register_func shim_initialize = &func;                                        \
struct shim_module_struct name ## _module = {                                 \
  SHIM_MODULE_VERSION,                                                        \
  NULL,                                                                       \
  __FILE__,                                                                   \
  (node_register_func)&shim_module_initialize,                                \
  #name,                                                                      \
};

#define SHIM_ARG_PAD 2
#define SHIM_ARG_THIS 0
#define SHIM_ARG_RVAL 1

#define SHIM_GET_SARGS(args) (args) - SHIM_ARG_PAD

#define SHIM_SET_RVAL(ctx, args, val)                                         \
do {                                                                          \
  shim_val_t** sargs = SHIM_GET_SARGS(args);                                  \
  sargs[SHIM_ARG_RVAL] = val;                                                 \
  if(QUEUE_EMPTY(&(val)->member))                                             \
    QUEUE_INSERT_TAIL(&(ctx)->allocs, &(val)->member);                        \
} while(0)

typedef int (* shim_func)(shim_ctx_t*, size_t, shim_val_t**);

typedef struct {
  const char* name;
  shim_func cfunc;
  uint16_t nargs;
  uint16_t flags;
  uint32_t reserved;
} shim_FunctionSpec;

#define SHIM_FS(name, cfunc, nargs, flags) { name, &cfunc, nargs, flags, 0 }
#define SHIM_FS_DEF(cfunc, nargs) SHIM_FS(#cfunc, cfunc, nargs, 0)
#define SHIM_FS_END { NULL, NULL, 0, 0, 0 }


shim_obj_t* shim_NewObject(shim_ctx_t*, shim_class_t*, shim_obj_t*,
  shim_obj_t*);
/*shim_obj_t* shim_New(shim_ctx_t*, shim_obj_t*, size_t argc, shim_vat_t**);*/

/* uses hidden propety should use internal field? */
void shim_SetPrivate(shim_obj_t*, void*);
int shim_SetProperty(shim_ctx_t*, shim_obj_t*, const char*, shim_val_t*);
int shim_SetPropertyById(shim_ctx_t*, shim_obj_t*, uint32_t, shim_val_t*);

/* uses hidden propety should use internal field? */
void* shim_GetPrivate(shim_obj_t*);
int shim_GetProperty(shim_ctx_t*, shim_obj_t*, const char*, shim_val_t*);
int shim_GetPropertyById(shim_ctx_t*, shim_obj_t*, uint32_t, shim_val_t*);

shim_val_t* shim_DefineFunction(shim_ctx_t*, shim_val_t*, const char*,
  shim_func, int, int);
int shim_DefineFunctions(shim_ctx_t*, shim_val_t*, const shim_FunctionSpec*);

shim_val_t* shim_NewFunction(shim_ctx_t*, shim_func, int, int, shim_val_t*,
  const char*);
int shim_CallFunctionName(shim_ctx_t*, shim_val_t*, const char*, size_t,
  shim_val_t**, shim_val_t*);
int shim_CallFunctionValue(shim_ctx_t*, shim_val_t*, shim_val_t, size_t,
  shim_val_t**, shim_val_t*);

int shim_ValueToBoolean(shim_ctx_t*, shim_val_t*, int*);
int shim_ValueToECMAInt32(shim_ctx_t*, shim_val_t*, int32_t*);
int shim_ValueToECMAUint32(shim_ctx_t*, shim_val_t*, uint32_t*);
int shim_ValueToUint16(shim_ctx_t*, shim_val_t*, uint16_t*);
int shim_ValueToNumber(shim_ctx_t*, shim_val_t*, double*);

shim_jstring_t* shim_ValueToString(shim_ctx_t*, shim_val_t*);

int shim_ConvertArguments(shim_ctx_t*, size_t, shim_val_t**, const char*, ...);

shim_val_t* shim_NumberValue(double);

char* shim_EncodeString(shim_ctx_t*, shim_jstring_t*);
size_t shim_GetStringEncodingLength(shim_ctx_t*, shim_jstring_t*);
size_t shim_EncodeStringToBuffer(shim_jstring_t*, char*, size_t);

shim_jstring_t* shim_NewStringCopyN(shim_ctx_t*, const char*, size_t);
shim_jstring_t* shim_NewStringCopyZ(shim_ctx_t*, const char*);

int shim_AddValueRoot(shim_ctx_t*, shim_val_t*);
int shim_RemoveValueRoot(shim_ctx_t*, shim_val_t*);

shim_val_t* shim_NewArrayObject(shim_ctx_t*, size_t, shim_val_t*);
int shim_GetArrayLength(shim_ctx_t*, shim_val_t*, uint32_t*);
int shim_GetElement(shim_ctx_t*, shim_val_t*, int32_t, shim_val_t*);
int shim_SetElement(shim_ctx_t*, shim_val_t*, int32_t, shim_val_t*);

int shim_IsExceptionPending(shim_ctx_t*);
void shim_SetPendingException(shim_ctx_t*, shim_val_t*);
void shim_ClearPendingException(shim_ctx_t*);
int shim_GetPendingException(shim_ctx_t*, shim_val_t*);

/* API not found in JSAPI */

void shim_ThrowError(shim_ctx_t*, const char*);
void shim_ThrowTypeError(shim_ctx_t*, const char*);
void shim_ThrowRangeError(shim_ctx_t*, const char*);

typedef uv_loop_t shim_loop_t;
#define shim_default_loop uv_default_loop

typedef struct shim_work_s shim_work_t;

typedef void (* shim_work_cb)(shim_work_t*);
typedef void (* shim_after_work)(shim_ctx_t*, shim_work_t*, int);

struct shim_work_s {
  uv_work_t req;
  shim_work_cb work_cb;
  shim_after_work after_cb;
};

void shim_queue_work(shim_loop_t*, shim_work_t*, shim_work_cb, shim_after_work);

shim_val_t* shim_NewExternal(shim_ctx_t*, void*);
void* shim_ExternalValue(shim_ctx_t*, shim_val_t*);

/* this should probably also get a context */
typedef void (* shim_weak_cb)(shim_val_t*, void*);

void shim_ValueMakeWeak(shim_ctx_t*, shim_val_t*, void*, shim_weak_cb);
/* this doesn't seem like the right pattern */
void shim_ValueDispose(shim_val_t*);

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef __cplusplus
}
#endif

#endif
