# Memory Management

::shim_val_s is a thin opaque pointer to a V8 heap object. Memory management
of ::shim_val_s's is by and large explicit. Aside from the noted exceptions
below, all returned ::shim_val_s's should be passed to shim_value_release().
This will only cleanup any resources the addon layer has allocated. Releasing
a ::shim_val_s does not indicate to V8 that it may now garbage collect the
underlying object.

## Local vs Persistent

In V8 there are two basic kinds of values:

 * Local
  - A value that is valid until the end of a scope, i.e. after the current
scope is done executing, if no more references to the value are held the GC may
free the resources associated with the value.
 * Persistent
  - A value which is valid beyond the current scope

::shim_val_s's will be a Local unless it was returned by
shim_persistent_new(), in which case you'll be returned a ::shim_val_s that
wraps a Persistent.

Wrapped Persistents should **only** be released by calling
shim_persistent_dispose(). Do **not** use shim_value_release().

If you want to be notified when the GC is about to collect a value, see
shim_obj_make_weak().

## Arguments

::shim_val_s's retrieved from:

 * shim_args_get()
 * shim_args_get_this()

or passed to:

 * shim_args_set_rval()

do **not** need to be shim_value_release()'d, the addon layer will handle the
releasing when the boundary function has finished executing.

## Singletons

shim_null() and shim_undefined() are singletons, they are ignored when passed
to shim_value_release(), it's neither necessary or harmful to do so.
