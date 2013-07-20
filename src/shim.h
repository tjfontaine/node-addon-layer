#ifndef NODE_SHIM_H
#define NODE_SHIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "queue.h"

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

typedef struct shim_obj_s {
  QUEUE member;
  void* handle;
} shim_obj_t;

typedef struct shim_ctx_s {
  void* scope;
  void* isolate;
  shim_obj_t *err;
  QUEUE allocs;
} shim_ctx_t;

typedef struct shim_args_s {
  shim_obj_t *argv;
  shim_obj_t *ret;
} shim_args_t;

#define SHIM_SET_RVAL(ctx, args, val) \
do { \
  shim_args_t* sargs = container_of(args, shim_args_t, argv); \
  sargs->ret = val; \
  if(QUEUE_EMPTY(&(val)->member)) \
    QUEUE_INSERT_TAIL(&(ctx)->allocs, &(val)->member); \
} while(0)

typedef int (* register_func)(shim_ctx_t*, shim_obj_t*, shim_obj_t*);
typedef int (* shim_func)(shim_ctx_t*, size_t, shim_obj_t**);

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

extern register_func shim_initialize;

void shim_SetProperty(shim_ctx_t*, shim_obj_t*, const char*, shim_obj_t*);

shim_obj_t* shim_DefineFunction(shim_ctx_t*, shim_obj_t*, const char*,
  shim_func, int, int);
int shim_DefineFunctions(shim_ctx_t*, shim_obj_t*, const shim_FunctionSpec*);

shim_obj_t* shim_NewFunction(shim_ctx_t*, shim_func, int, int, shim_obj_t*,
  const char*);

int shim_ValueToBoolean(shim_ctx_t*, shim_obj_t*, int*);
int shim_ValueToECMAInt32(shim_ctx_t*, shim_obj_t*, int32_t*);
int shim_ValueToECMAUint32(shim_ctx_t*, shim_obj_t*, uint32_t*);
int shim_ValueToUint16(shim_ctx_t*, shim_obj_t*, uint16_t*);
int shim_ValueToNumber(shim_ctx_t*, shim_obj_t*, double*);

typedef shim_obj_t shim_jstring_t;

shim_jstring_t* shim_ValueToString(shim_ctx_t*, shim_obj_t*);

int shim_ConvertArguments(shim_ctx_t*, size_t, shim_obj_t**, const char*, ...);

shim_obj_t* shim_NumberValue(double);

char* shim_EncodeString(shim_ctx_t*, shim_jstring_t*);
size_t shim_GetStringEncodingLength(shim_ctx_t*, shim_jstring_t*);
size_t shim_EncodeStringToBuffer(shim_jstring_t*, char*, size_t);

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 1
#endif

#ifdef __cplusplus
}
#endif

#endif
