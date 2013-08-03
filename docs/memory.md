# Memory Management

::shim_val_s is a thin opaque pointer to a V8 heap object. Memory management
of ::shim_val_s's is by and large explicit. Aside from the noted exceptions
below, all returned ::shim_val_s's should be passed to shim_value_release().
This will only cleanup any resources the addon layer has allocated. Releasing
a ::shim_val_s does not indicate to V8 that it may now garbage collect the
underlying object either.

It is not generally safe to hold on to a ::shim_val_s for longer than the
currently executing boundary function. That is the C function that was called
from JavaScript. Unless however you have first made a peristent with
shim_persistent_new(), which should only be used in conjunction with
shim_persistent_dispose() and **not** shim_value_release().

::shim_val_s's retrieved from shim_args_get(), shim_args_get_this(), or passed
to shim_args_set_rval() do **not** need to be shim_value_release()'d by you,
the addon layer itself will manage that for you when the boundary function has
finished executing.

shim_null() and shim_undefined() are unique sentinels/singletons, they are
ignored when passed to shim_value_release(), it's neither necessary or harmful
to do so.
