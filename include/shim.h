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
#include <stddef.h>

/**
 * \defgroup core Core types and helpers
 * @{
 */

/** The ABI version of the addon layer */
#define SHIM_ABI_VERSION 1

/** Defines the types natively supported by the addon layer */
typedef enum shim_type {
  SHIM_TYPE_UNKNOWN = 0,  /**< Unknown type */
  SHIM_TYPE_UNDEFINED,    /**< v8:Undefined */
  SHIM_TYPE_NULL,         /**< v8::Null */
  SHIM_TYPE_BOOL,         /**< `true` or `false` */
  SHIM_TYPE_DATE,         /**< v8::Date object */
  SHIM_TYPE_ARRAY,        /**< v8::Array object */
  SHIM_TYPE_OBJECT,       /**< v8::Object */
  SHIM_TYPE_INTEGER,      /**< v8::Integer */
  SHIM_TYPE_INT32,        /**< v8::Integer::Int32 */
  SHIM_TYPE_UINT32,       /**< v8::Integer::Uint32 */
  SHIM_TYPE_NUMBER,       /**< v8::Number */
  SHIM_TYPE_EXTERNAL,     /**< v8::External */
  SHIM_TYPE_FUNCTION,     /**< v8::Function */
  SHIM_TYPE_STRING,       /**< v8::String */
  SHIM_TYPE_BUFFER,       /**< node::Buffer */
} shim_type_t;

/**
 * The opaque handle that represents a javascript value
 *
 * You should use shim_value_release
 */
typedef struct shim_val_s shim_val_t;

/** The opaque handle that represents the currently executing context */
typedef struct shim_ctx_s shim_ctx_t;

/** The opaque handle that represents the arguments passed to this function */
typedef struct shim_args_s shim_args_t;

/** A wrapper for TRUE and FALSE that is merely an int */
typedef int shim_bool_t;

/** Entry point from node to register the addon layer */
typedef void (* node_register_func)(void*, void*);

/** Signature of for how a module will be initialized */
typedef int (* register_func)(shim_ctx_t*, shim_val_t*, shim_val_t*);
/** Location of your entry point */
extern register_func shim_initialize;

/**
 * \struct shim_module_struct
 * \brief Used by node to identify how to initialize this module
 */
struct shim_module_struct {
  int version;              /**< Indicates the node module ABI */
  void* handle;             /**< Used internally by node */
  const char* fname;        /**< Filename of module */
  node_register_func func;  /**< Always represents the C++ entry point */
  const char* mname;        /**< Module name */
};


/* Sigh, NODE_MODULE_VERSION isn't in node_version.h before 0.12 */
#if defined(NODE_MODULE_VERSION)
# define SHIM_NODE_ABI NODE_MODULE_VERSION
#else
# if NODE_VERSION_AT_LEAST(0, 11, 0)
#   define SHIM_NODE_ABI 0x000C
# elif NODE_VERSION_AT_LEAST(0, 10, 0)
#   define SHIM_NODE_ABI 0x000B
# elif NODE_VERSION_AT_LEAST(0, 8, 0)
#   define SHIM_NODE_ABI 1
# else
#   error "The shim requires at least node v0.8.0"
# endif /* checking versions */
#endif /* checking NODE_MODULE_VERSION defined */


/**
 * \def SHIM_MODULE(name, func)
 * Use this to define the \a name and \a func entry point of your module
 */
#define SHIM_MODULE(name, func)                                               \
register_func shim_initialize = &func;                                        \
struct shim_module_struct name ## _module = {                                 \
  SHIM_NODE_ABI,                                                              \
  NULL,                                                                       \
  __FILE__,                                                                   \
  NULL,									      \
  #name,                                                                      \
};									      \
const char* shim_modname = # name ;

/** The signature of the entry point for exported functions */
typedef int (* shim_func)(shim_ctx_t*, shim_args_t*);

/**
 * Describes a function signature
 * \sa shim_obj_set_funcs()
 */
typedef struct shim_fspec_s {
  const char* name; /**< Name of function */
  shim_func cfunc;  /**< C function pointer */
  uint16_t nargs;   /**< Number of args this function accepts */
  void* data;       /**< Arbitrary data associated with function */
  uint32_t flags;   /**< Flags for function */
  uint32_t reserved;/**< Reserved for future use */
} shim_fspec_t;


/** Define the all the properties fo the function */
#define SHIM_FS_FULL(name, cfunc, nargs, data, flags)                         \
  { name, &cfunc, nargs, data, flags, 0 }
/** Define only the function, args, and data */
#define SHIM_FS_DEF(cfunc, nargs, data)                                       \
  SHIM_FS_FULL(#cfunc, cfunc, nargs, data, 0)
/** Define just the function */
#define SHIM_FS(cfunc)                                                        \
  SHIM_FS_DEF(cfunc, 0, NULL)
/** Sentinel that indicates we're done defining functions */
#define SHIM_FS_END                                                           \
  { NULL, NULL, 0, NULL, 0, 0 }

/**@}*/

/**
 * \defgroup primitives Primitive methods
 * Methods for managing primitives
 * @{
 */

/** Check the value is of a given type */
shim_bool_t shim_value_is(shim_val_t* val, shim_type_t type);
/** Convert the value to a given type */
shim_bool_t shim_value_to(shim_ctx_t* ctx, shim_val_t* val, shim_type_t type,
  shim_val_t* rval);


/** Relase memory associated with this value */
void shim_value_release(shim_val_t* val);

/** Get the undefined value */
shim_val_t* shim_undefined();
/** Get the null value */
shim_val_t* shim_null();

const char* shim_type_str(shim_type_t type);
/**@}*/


/**
 * \defgroup objects Object methods
 * Methods for creating and manipulation javascript objects
 * @{
 */

/** Create a new object */
shim_val_t* shim_obj_new(shim_ctx_t* context, shim_val_t* klass,
  shim_val_t* proto);
/** Create a new object by calling the given constructor with arguments */
shim_val_t* shim_obj_new_instance(shim_ctx_t* context, shim_val_t* klass,
  size_t argc, shim_val_t** argv);

/** Create a second wrapper of the given object */
shim_val_t* shim_obj_clone(shim_ctx_t* ctx, shim_val_t* src);

/** Check the object has the given name */
shim_bool_t shim_obj_has_name(shim_ctx_t* ctx, shim_val_t* obj,
  const char* name);
/** Check the object has the given id */
shim_bool_t shim_obj_has_id(shim_ctx_t* ctx, shim_val_t* obj,
  uint32_t id);
/** Check the object has the given symbol */
shim_bool_t shim_obj_has_sym(shim_ctx_t* ctx, shim_val_t* obj, shim_val_t* sym);

/** Set the value for the named property */
shim_bool_t shim_obj_set_prop_name(shim_ctx_t* ctx, shim_val_t* recv,
  const char* name, shim_val_t* val);
/** Set the value for the given id */
shim_bool_t shim_obj_set_prop_id(shim_ctx_t* ctx, shim_val_t* recv,
  uint32_t id, shim_val_t* val);
/** Set the value for the given symbol */
shim_bool_t shim_obj_set_prop_sym(shim_ctx_t* ctx, shim_val_t* recv,
  shim_val_t* sym, shim_val_t* val);
/** Add arbitrary data to given object */
shim_bool_t shim_obj_set_private(shim_ctx_t* ctx, shim_val_t* obj, void* data);
/** Adds a set of functions to an object */
shim_bool_t shim_obj_set_funcs(shim_ctx_t* ctx, shim_val_t* recv,
  const shim_fspec_t* funcs);

/** Get the value for the property name */
shim_bool_t shim_obj_get_prop_name(shim_ctx_t* ctx, shim_val_t* obj,
  const char* name, shim_val_t* rval);
/** Get the value for the given id */
shim_bool_t shim_obj_get_prop_id(shim_ctx_t* ctx, shim_val_t* obj,
  uint32_t id, shim_val_t* rval);
/** Get the value for the given symbol */
shim_bool_t shim_obj_get_prop_sym(shim_ctx_t* ctx, shim_val_t* obj,
  shim_val_t* sym, shim_val_t* rval);
/** Get the arbitrary data associated with the object */
shim_bool_t shim_obj_get_private(shim_ctx_t* ctx, shim_val_t* obj, void** data);

/**@}*/


/**
 * \defgroup persistents Persistent methods
 * Methods for persistents
 * @{
 */

/** Make an existing value persistent */
shim_val_t* shim_persistent_new(shim_ctx_t* ctx, shim_val_t* val);
/** Dispose a perisstent value */
void shim_persistent_dispose(shim_val_t* val);


/** Callback fired when a peristent is about to be collected  */
typedef void (* shim_weak_cb)(shim_val_t*, void*);
/** Make a persistent value weak */
void shim_obj_make_weak(shim_ctx_t* ctx, shim_val_t* val, void* data,
  shim_weak_cb cb);
/** Make a persistent object strong */
void shim_obj_clear_weak(shim_val_t* val);

/**@}*/


/**
 * \defgroup functions Function methods
 * Methods for functions
 * @{
 */

/** Create a new javascript function that points to a C function */
shim_val_t* shim_func_new(shim_ctx_t* ctx, shim_func cfunc, size_t argc,
  int32_t flags, const char* name, void* data);


/** Get the symbol from the object as a function and call it */
shim_bool_t shim_func_call_sym(shim_ctx_t* ctx, shim_val_t* self,
  shim_val_t* name, size_t argc, shim_val_t** argv, shim_val_t* rval);
/** Get the function by name from an object and call it */
shim_bool_t shim_func_call_name(shim_ctx_t* ctx, shim_val_t* self,
  const char* name, size_t argc, shim_val_t** argv, shim_val_t* rval);
/** Call the given function */
shim_bool_t shim_func_call_val(shim_ctx_t* ctx, shim_val_t* self,
  shim_val_t* func, size_t argc, shim_val_t** argv, shim_val_t* rval);


/** Get a function by symbol and process the callback */
shim_bool_t shim_make_callback_sym(shim_ctx_t* ctx, shim_val_t* self,
  shim_val_t* sym, size_t argc, shim_val_t** argv, shim_val_t* rval);
/** Process the callback for the given function */
shim_bool_t shim_make_callback_val(shim_ctx_t* ctx, shim_val_t* self,
  shim_val_t* fval, size_t argc, shim_val_t** argv, shim_val_t* rval);
/** Process the callback for the given name */
shim_bool_t shim_make_callback_name(shim_ctx_t* ctx, shim_val_t* obj,
  const char* name, size_t argc, shim_val_t** argv, shim_val_t* rval);

/**@}*/

/**
 * \defgroup numbers Number methods
 * Methods for numbers
 * @{
 */

/** Create a new Number from a value */
shim_val_t* shim_number_new(shim_ctx_t* ctx, double d);
/** Get the value of a number */
double shim_number_value(shim_val_t* obj);

/** Create a new Integer */
shim_val_t* shim_integer_new(shim_ctx_t* ctx, int32_t i);
/** Create a new Integer from a uint32_t */
shim_val_t* shim_integer_uint(shim_ctx_t* ctx, uint32_t i);
/** Get the value of an integer */
int64_t shim_integer_value(shim_val_t* val);
/** Get the int32_t value */
int32_t shim_integer_int32_value(shim_val_t* val);
/** Get the uint32_t value */
uint32_t shim_integer_uint32_value(shim_val_t* val);

/**@}*/

/**
 * \defgroup strings String methods
 * Methods for strings
 * @{
 */

/** Create a new empty string */
shim_val_t* shim_string_new(shim_ctx_t* ctx);
/** Create a new string and from the null terminated C string */
shim_val_t* shim_string_new_copy(shim_ctx_t* ctx, const char* data);
/** Create a new string of the given length from the existing buffer */
shim_val_t* shim_string_new_copyn(shim_ctx_t* ctx, const char* data,
  size_t len);

/** Get the length of the string */
size_t shim_string_length(shim_val_t* val);
/** Get the UTF-8 encoded length of the string */
size_t shim_string_length_utf8(shim_val_t* val);

/** Get the value of the string */
const char* shim_string_value(shim_val_t* val);
/* TODO enum for options */
/** Write the value of the string to a buffer */
size_t shim_string_write_ascii(shim_val_t* val, char* buff, size_t start,
  size_t len, int32_t options);

/**@}*/

/**
 * \defgroup array Array methods
 * Methods for Arrays
 * @{
 */

/** Create a new array of given length */
shim_val_t* shim_array_new(shim_ctx_t* ctx, size_t len);
/** Get the length of the given array */
size_t shim_array_length(shim_val_t* arr);
/** Get the value at the given index */
shim_bool_t shim_array_get(shim_ctx_t* ctx, shim_val_t* arr, int32_t idx,
  shim_val_t* rval);
/** Set the value at the given index */
shim_bool_t shim_array_set(shim_ctx_t* ctx, shim_val_t* arr, int32_t idx,
  shim_val_t* val);

/**@}*/

/**
 * \defgroup buffers Buffer methods
 * Methods for Buffers
 * @{
 */

/** The callback that will be called when the Buffer is to be freed */
typedef void (* shim_buffer_free)(char*, void*);
/** Create a new buffer of length */
shim_val_t* shim_buffer_new(shim_ctx_t*, size_t);
/** Copy into a new buffer */
shim_val_t* shim_buffer_new_copy(shim_ctx_t*, const char*, size_t);
/** Create a new buffer and be notified when the buffer is to be freed */
shim_val_t* shim_buffer_new_external(shim_ctx_t*, char*, size_t,
  shim_buffer_free, void*);

/** Get the underlying memory for the Buffer */
char* shim_buffer_value(shim_val_t*);
/** Get the size of the buffer */
size_t shim_buffer_length(shim_val_t*);

/**@}*/

/**
 * \defgroup externals External methods
 * Methods for externals
 * @{
 */

/** Create a new external */
shim_val_t* shim_external_new(shim_ctx_t* ctx, void* data);
/** Get the underlying memory for the external */
void* shim_external_value(shim_ctx_t* ctx, shim_val_t* val);

/**@}*/

/**
 * \defgroup errors Error methods
 * Methods for errors
 * @{
 */

/** Create a new error */
shim_val_t* shim_error_new(shim_ctx_t* ctx, const char* msg, ...);
/** Create a new TypeError */
shim_val_t* shim_error_type_new(shim_ctx_t* ctx, const char* msg, ...);
/** create a new RangeError */
shim_val_t* shim_error_range_new(shim_ctx_t* ctx, const char* msg, ...);

/** Check if there is a pending exception */
shim_bool_t shim_exception_pending(shim_ctx_t* ctx);
/** Clear any pending exception */
void shim_exception_clear(shim_ctx_t* ctx);
/** Set the pending exception */
void shim_exception_set(shim_ctx_t* ctx, shim_val_t* val);
/** Get the pending exception */
shim_bool_t shim_exception_get(shim_ctx_t* ctx, shim_val_t* rval);

/** Throw an error */
void shim_throw_error(shim_ctx_t* ctx, const char* msg, ...);
/** Throw a TypeError */
void shim_throw_type_error(shim_ctx_t* ctx, const char* msg, ...);
/** Throw a RangeError */
void shim_throw_range_error(shim_ctx_t* ctx, const char* msg, ...);

/**@}*/

/**
 * \defgroup arguments Argument methods
 * Methods for arguments
 * @{
 */

/** Unpack the argument at the given index */
shim_bool_t shim_unpack_one(shim_ctx_t* ctx, shim_args_t* args, uint32_t idx,
  shim_type_t type, void* rval);
/** Unpack the value to the underlying type */
shim_bool_t shim_unpack_type(shim_ctx_t* ctx, shim_val_t* arg,
  shim_type_t type, void* rval);
/** Unpack the arguments into underlying types */
shim_bool_t shim_unpack(shim_ctx_t* ctx, shim_args_t* args, shim_type_t type,
  ...);
/** Unpack the arguments by format */
int shim_unpack_fmt(shim_ctx_t* ctx, shim_args_t* args, const char* fmt, ...);

/** How many arguments were passed to this function */
size_t shim_args_length(shim_args_t* args);
/** Get the argument at the given index */
shim_val_t* shim_args_get(shim_args_t*, size_t idx);
/** Set the return value for this function */
shim_bool_t shim_args_set_rval(shim_ctx_t* ctx, shim_args_t* args,
  shim_val_t* val);
/** Get the This for the given function */
shim_val_t* shim_args_get_this(shim_ctx_t* ctx, shim_args_t* args);
/** Get the arbitrary data associated with this function */
void* shim_args_get_data(shim_ctx_t* ctx, shim_args_t* args);

/**@}*/

/**
 * \defgroup async Async methods
 * Methods for asynchronous function calling
 * @{
 */

/**
 * \typedef shim_work_t
 * \brief Opaque pointer to the queued work
 */
typedef struct shim_work_s shim_work_t;

/** Callback on a different thread to do computation */
typedef void (* shim_work_cb)(shim_work_t*, void*);
/** Callback on main thread to return to JS */
typedef void (* shim_after_work)(shim_ctx_t*, shim_work_t*, int, void*);
/** Queue work to be done on background thread */
void shim_queue_work(shim_work_cb, shim_after_work, void* hint);

/**@}*/

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
