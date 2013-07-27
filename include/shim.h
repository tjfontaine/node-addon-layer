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
#include "node_version.h"

typedef enum shim_type {
  SHIM_TYPE_UNKNOWN = 0,
  SHIM_TYPE_UNDEFINED,
  SHIM_TYPE_NULL,
  SHIM_TYPE_BOOL,
  SHIM_TYPE_DATE,
  SHIM_TYPE_ARRAY,
  SHIM_TYPE_OBJECT,
  SHIM_TYPE_INTEGER,
  SHIM_TYPE_INT32,
  SHIM_TYPE_UINT32,
  SHIM_TYPE_NUMBER,
  SHIM_TYPE_EXTERNAL,
  SHIM_TYPE_FUNCTION,
  SHIM_TYPE_STRING,
} shim_type_t;


typedef struct shim_val_s shim_val_t;
typedef struct shim_ctx_s shim_ctx_t;
typedef struct shim_args_s shim_args_t;
typedef int shim_bool_t;


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


/* Sigh, NODE_MODULE_VERSION isn't in node_version */
#if defined(NODE_MODULE_VERSION)
#define SHIM_MODULE_VERSION NODE_MODULE_VERSION
#elif NODE_VERSION_AT_LEAST(0, 11, 0)
#define SHIM_MODULE_VERSION 0x000C
#elif NODE_VERSION_AT_LEAST(0, 10, 0)
#define SHIM_MODULE_VERSION 0x000B
#elif NODE_VERSION_AT_LEAST(0, 8, 0)
#define SHIM_MODULE_VERSION 1
#else /* 0.8.0 */
#error "The shim requires at least node v0.8.0"
#endif /* 0.8.0 */


#define SHIM_MODULE(name, func)                                               \
register_func shim_initialize = &func;                                        \
struct shim_module_struct name ## _module = {                                 \
  SHIM_MODULE_VERSION,                                                        \
  NULL,                                                                       \
  __FILE__,                                                                   \
  (node_register_func)&shim_module_initialize,                                \
  #name,                                                                      \
};


typedef int (* shim_func)(shim_ctx_t*, shim_args_t*);


typedef struct {
  const char* name;
  shim_func cfunc;
  uint16_t nargs;
  void* data;
  uint32_t flags;
  uint32_t reserved;
} shim_fspec_t;


#define SHIM_FS_FULL(name, cfunc, nargs, data, flags)                         \
  { name, &cfunc, nargs, data, flags, 0 }
#define SHIM_FS_DEF(cfunc, nargs, data)                                       \
  SHIM_FS_FULL(#cfunc, cfunc, nargs, data, 0)
#define SHIM_FS(cfunc)                                                        \
  SHIM_FS_DEF(cfunc, 0, NULL)
#define SHIM_FS_END                                                           \
  { NULL, NULL, 0, NULL, 0, 0 }


shim_bool_t shim_value_is(shim_val_t* val, shim_type_t type);
shim_bool_t shim_value_to(shim_ctx_t* ctx, shim_val_t* val, shim_type_t type,
  shim_val_t* rval);


void shim_value_release(shim_val_t* val);


shim_val_t* shim_obj_new(shim_ctx_t* context, shim_val_t* klass,
  shim_val_t* proto);
shim_val_t* shim_obj_new_instance(shim_ctx_t* context, shim_val_t* klass,
  size_t argc, shim_val_t** argv);


shim_val_t* shim_obj_clone(shim_ctx_t* ctx, shim_val_t* src);


shim_bool_t shim_obj_has_name(shim_val_t* obj, const char* name);
shim_bool_t shim_obj_has_id(shim_val_t* obj, uint32_t id);
shim_bool_t shim_obj_has_sym(shim_val_t* obj, shim_val_t* sym);


shim_bool_t shim_obj_set_prop_name(shim_ctx_t* ctx, shim_val_t* recv,
  const char* name, shim_val_t* val);
shim_bool_t shim_obj_set_prop_id(shim_ctx_t* ctx, shim_val_t* recv,
  uint32_t id, shim_val_t* val);
shim_bool_t shim_obj_set_prop_sym(shim_ctx_t* ctx, shim_val_t* recv,
  shim_val_t* sym, shim_val_t* val);
shim_bool_t shim_obj_set_private(shim_ctx_t* ctx, shim_val_t* obj, void* data);
shim_bool_t shim_obj_set_funcs(shim_ctx_t* ctx, shim_val_t* recv,
  const shim_fspec_t* funcs);


shim_bool_t shim_obj_get_prop_name(shim_ctx_t* ctx, shim_val_t* obj,
  const char* name, shim_val_t* rval);
shim_bool_t shim_obj_get_prop_id(shim_ctx_t* ctx, shim_val_t* obj,
  uint32_t id, shim_val_t* rval);
shim_bool_t shim_obj_get_prop_sym(shim_ctx_t* ctx, shim_val_t* obj,
  shim_val_t* sym, shim_val_t* rval);
shim_bool_t shim_obj_get_private(shim_ctx_t* ctx, shim_val_t* obj, void** data);


shim_val_t* shim_persistent_new(shim_ctx_t* ctx, shim_val_t* val);
void shim_persistent_dispose(shim_val_t* val);


/* this should probably also get a context */
typedef void (* shim_weak_cb)(shim_val_t*, void*);
void shim_obj_make_weak(shim_ctx_t* ctx, shim_val_t* val, void* data,
  shim_weak_cb cb);
void shim_obj_clear_weak(shim_val_t* val);


shim_val_t* shim_func_new(shim_ctx_t* ctx, shim_func cfunc, size_t argc,
  int32_t flags, const char* name, void* data);


shim_bool_t shim_func_call_name(shim_ctx_t* ctx, shim_val_t* self,
  const char* name, size_t argc, shim_val_t** argv, shim_val_t* rval);
shim_bool_t shim_func_call_val(shim_ctx_t* ctx, shim_val_t* self,
  shim_val_t* func, size_t argc, shim_val_t** argv, shim_val_t* rval);


shim_val_t* shim_number_new(shim_ctx_t* ctx, double d);
double shim_number_value(shim_val_t* obj);


shim_val_t* shim_integer_new(shim_ctx_t* ctx, int32_t i);
shim_val_t* shim_integer_uint(shim_ctx_t* ctx, uint32_t i);
int64_t shim_integer_value(shim_val_t* val);
int32_t shim_integer_int32_value(shim_val_t* val);
uint32_t shim_integer_uint32_value(shim_val_t* val);


shim_val_t* shim_string_new(shim_ctx_t* ctx);
shim_val_t* shim_string_new_copy(shim_ctx_t* ctx, const char* data);
shim_val_t* shim_string_new_copyn(shim_ctx_t* ctx, const char* data,
  size_t len);


size_t shim_string_length(shim_val_t* val);
size_t shim_string_length_utf8(shim_val_t* val);


const char* shim_string_value(shim_val_t* val);
/* TODO enum for options */
size_t shim_string_write_ascii(shim_val_t* val, char* buff, size_t start,
  size_t len, int32_t options);


shim_val_t* shim_array_new(shim_ctx_t* ctx, size_t len);
size_t shim_array_length(shim_val_t* arr);
shim_bool_t shim_array_get(shim_ctx_t* ctx, shim_val_t* arr, int32_t idx,
  shim_val_t* rval);
shim_bool_t shim_array_set(shim_ctx_t* ctx, shim_val_t* arr, int32_t idx,
  shim_val_t* val);


typedef void (* shim_buffer_free)(char*, void*);
shim_val_t* shim_buffer_new(shim_ctx_t*, size_t);
shim_val_t* shim_buffer_new_copy(shim_ctx_t*, const char*, size_t);
shim_val_t* shim_buffer_new_external(shim_ctx_t*, char*, size_t,
  shim_buffer_free, void*);


char* shim_buffer_value(shim_val_t*);
size_t shim_buffer_length(shim_val_t*);


shim_val_t* shim_external_new(shim_ctx_t* ctx, void* data);
void* shim_external_value(shim_ctx_t* ctx, shim_val_t* val);


shim_val_t* shim_error_new(shim_ctx_t* ctx, const char* msg);
shim_val_t* shim_error_type_new(shim_ctx_t* ctx, const char* msg);
shim_val_t* shim_error_range_new(shim_ctx_t* ctx, const char* msg);


shim_bool_t shim_exception_pending(shim_ctx_t* ctx);
void shim_exception_clear(shim_ctx_t* ctx);
void shim_exception_set(shim_ctx_t* ctx, shim_val_t* val);
shim_bool_t shim_exception_get(shim_ctx_t* ctx, shim_val_t* rval);


void shim_throw_error(shim_ctx_t* ctx, const char* msg);
void shim_throw_type_error(shim_ctx_t* ctx, const char* msg);
void shim_throw_range_error(shim_ctx_t* ctx, const char* msg);


shim_bool_t shim_unpack_one(shim_ctx_t* ctx, shim_args_t* args, uint32_t idx,
  shim_type_t type, void** rval);
shim_bool_t shim_unpack_type(shim_ctx_t* ctx, shim_val_t* arg,
  shim_type_t type, void* rval);
shim_bool_t shim_unpack(shim_ctx_t* ctx, shim_args_t* args, ...);
int shim_unpack_fmt(shim_ctx_t* ctx, shim_args_t* args, const char* fmt, ...);


size_t shim_args_length(shim_args_t* args);
shim_val_t* shim_args_get(shim_args_t*, size_t idx);
shim_bool_t shim_args_set_rval(shim_ctx_t* ctx, shim_args_t* args,
  shim_val_t* val);
shim_val_t* shim_args_get_this(shim_ctx_t* ctx, shim_args_t* args);
void* shim_args_get_data(shim_ctx_t* ctx, shim_args_t* args);


typedef struct shim_work_s shim_work_t;

typedef void (* shim_work_cb)(shim_work_t*, void*);
typedef void (* shim_after_work)(shim_ctx_t*, shim_work_t*, int, void*);

void shim_queue_work(shim_work_cb, shim_after_work, void* hint);


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

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

#ifdef __cplusplus
}
#endif

#endif
