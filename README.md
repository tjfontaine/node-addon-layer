# addon layer

This is a thin shim layer for node that provides a C API somewhat inspired by
[JSAPI](https://developer.mozilla.org/en-US/docs/SpiderMonkey/JSAPI_Reference).

**This is still very alpha**

## implementation

Everything is wrapped by `shim_val_t` which holds a pointer to the actual heap
object managed by the GC

To quote:

 > It is safe to extract the object stored in the handle by
 > dereferencing the handle (for instance, to extract the Object* from
 > a Handle<Object>); the value will still be governed by a handle
 > behind the scenes and the same rules apply to these values as to
 > their handles.

This isn't as scary as it sounds, since the object is generally rooted in a
HandleScope from the boundary cross from JS to C. But it also means that to
compile in V8 3.20+ you must define:

 * V8_USE_UNSAFE_HANDLES
 * V8_ALLOW_ACCESS_TO_RAW_HANDLE_CONSTRUCTOR

There may be some other ways around this, I will need to investigate the
incurred overhead from such changes.

### function invocation

When you export a function from C to JS, the function pointer is wrapped in an
`External` and associated with the function such that upon invocation the shim
can then `Unwrap` marshal arguments and invoke your function pointer.

### allocation

Most `shim_val_t`s are allocated on the heap--though internally the arguments
could later be `alloca`d. You are not responsible for releasing the arguments
passed to your function, nor are you responsible for releasing any value you
return.

You *are* responsible to `shim_value_release` any value you create, or is
returned to you that isn't `shim_args_get`. With the exception of
`shim_persistent_new` which should be `shim_persistent_dispose`d.

## TODO

 * Actually use the isolate
 * Object creation for specific class
 * Date

The shim does some gratuitous boxing and `ToObject()`s, instead when we have
previously coerced a type we should store that information on the `shim_val_t`
to avoid unnecessary allocations and casts.

# License

MIT
