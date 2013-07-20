#include "stdlib.h"
#include "stdio.h"

#include "shim.h"

int test_func(shim_ctx_t* ctx, size_t argc, shim_obj_t** argv) {
  printf("we're in test_func, argc %zu\n", argc);

  int32_t i;
  uint32_t u;
  shim_ConvertArguments(ctx, argc, argv, "iu", &i, &u);

  printf("we have an argument of %d %u\n", i, u);

  SHIM_SET_RVAL(ctx, argv, shim_NumberValue(i));
  return TRUE;
}

int test_foo(shim_ctx_t* ctx, size_t argc, shim_obj_t** argv) {
  printf("we're in test_foo\n");
  shim_jstring_t* str;
  shim_ConvertArguments(ctx, argc, argv, "S", &str);
  char *cstr = shim_EncodeString(ctx, str);
  printf("we got %s\n", cstr);
  return TRUE;
}

int initialize(shim_ctx_t* ctx, shim_obj_t* exports, shim_obj_t* module)
{
  shim_FunctionSpec funcs[] = {
    SHIM_FS_DEF(test_func, 0),
    SHIM_FS_DEF(test_foo, 0),
    SHIM_FS_END,
  };

  shim_DefineFunctions(ctx, exports, funcs);
  
  printf("we're initializing the c side\n");
  return TRUE;
}

register_func shim_initialize = &initialize;
