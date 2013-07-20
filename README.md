# addon layer

This is a thin shim layer for node that provides a C API similar to
[JSAPI](https://developer.mozilla.org/en-US/docs/SpiderMonkey/JSAPI_Reference).

**This is still just proof of concept**

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
HandleScope from the entry point. But it also means that to compile in V8 3.20+
you must define

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
could later be `alloca`d. When the shim allocates a `shim_val_t` and it has a
`shim_ctx_t` the allocated `shim_val_t` will be added to a list in the
`shim_ctx_t` as an active allocation.

This allows the shim to cleanup any allocated `shim_val_t`s when it is cleaning
up the context, such that the module author doesn't have to explicitly manage
as much memory.

However you can use `shim_AddValueRoot` to promote to a `v8::Persistent`, part
of the promotion removes the `shim_val_t` from the list of to be `free`d values
and is safe for the module author to keep.

## TODO

 * Actually use the isolate
 * finish multiple V8 version support
 * Object creation for specific class
 * Date
 * Buffer

The shim does some gratuitous boxing and `ToObject()`s, instead when we have
previously coerced a type we should store that information on the `shim_val_t`
to avoid unnecessary allocations and casts.

# License

MIT
