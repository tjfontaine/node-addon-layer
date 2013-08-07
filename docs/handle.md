# Native Handles

A common pattern is to write a binding to a library that intializes and returns
a handle that is used for all subsequent library operations. Similarly you will
want to know when it is necessary to clean up the underlying resources
(without necessarily being explicit in JavaScript).

~~~~~~~~~~~~~~~{.c}
/* Called when the handle is about to be collected by the VM */
void
handle_free_cb(shim_val_t* phandle, void* data)
{
  /* unwrap the handle */
  void* handle = shim_external_value(phandle);

  /* cleanup any resources for the library */
  some_library_free(handle);

  /* cleanup any resources for the value */
  shim_persistent_dispose(phandle);
}

shim_bool_t
library_init(shim_ctx_t* ctx, shim_args_t* args)
{
  /* Initialize some library */
  void* handle = some_library_init();

  /* Wrap the library handle in an external */
  shim_val_t* ehandle = shim_external_new(ctx, handle);

  /* Turn the handle into a Persistent */
  shim_val_t* phandle = shim_persistent_new(ctx, ehandle);

  /* Release the original wrapper */
  shim_value_release(ehandle);

  /*
   * Only valid for persistents, use this to get notified if JavaScript
   * is about to collect your object, this lets you handle any finalizer
   * work on your own
   */
  shim_obj_make_weak(ctx, phandle, NULL, handle_free_cb);

  /* return the handle to JavaScript */
  shim_args_set_rval(ctx, args, phandle);
  return TRUE;
}
~~~~~~~~~~~~~~~
