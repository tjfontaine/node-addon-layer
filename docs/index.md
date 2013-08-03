Addon Layer {#mainpage}
===========

This documentation discusses the best practices and the API for the C API layer
for node.

## Getting Started

In `test.c`

~~~~~~~~~~~~~~~{.c}
#include "shim.h"

shim_bool_t
foobar(shim_ctx_t* ctx, shim_args_t* args)
{
  shim_args_t* ret = shim_string_new_copy(ctx, "Hello World");
  shim_args_set_rval(ctx, args, ret);
  return TRUE;
}

shim_bool_t
myinit(shim_ctx_t* ctx, shim_val_t* exports, shim_val_t* module)
{
  shim_fspec_t funcs[] = {
    SHIM_FS(foobar),
    SHIM_FS_END,
  }

  shim_obj_set_funcs(ctx, exports, funcs);

  return TRUE;
}

SHIM_MODULE(mymodule, myinit)
~~~~~~~~~~~~~~~

And then in `index.js`

~~~~~~~~~~~~~~~{.js}
var mymodule = require('bindings')('mymodule');

console.log('module return', mymodule.foobar());
~~~~~~~~~~~~~~~

## See Also

 * [Modules](modules.html)
 * [Memory Management](md_docs_memory.html)
