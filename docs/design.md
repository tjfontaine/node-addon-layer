# Module Design

By writing a native module you are explicitly preventing that logic from being
optimized by the JIT, it will only be as optimized as the target compiler
provides. So things like function inlining and lookup caching can't be done by
the JIT for logic defined in C.

There is a cost for a boundary jump between JavaScript and C, and it is more
than the cost of a function call in pure JavaScript (espcially if the JIT
inlined the call). But while the boundary cost is measurable, it is more
expensive to actually mutate an object (i.e. set properties) in C compared to
mutating the object in JavaScript.

To that end it is important to try and interact with your module using
primitives, like:

 * [Integer, Number](group__numbers.html)
 * [String](group__strings.html)
 * [Buffer](group__buffers.html)
 * [External](group__externals.html)
 * [Function](group__functions.html)

That is, you should pass and return these primitves. You can pass and return an
object, but don't modify the object in a hot path. If you want to construct an
object in a hot path it is generally cheaper to also pass a function to create
and return that object:

~~~~~~~~~~~~~~~{.c}
shim_bool_t
do_something(shim_ctx_t* ctx, shim_args_t* args)
{
  int i;

  /* grab the first argument */
  shim_val_t* blah = shim_args_get(args, 0);
  /* grab the second argument which is the callback */
  shim_val_t* factory = shim_args_get(args, 1);
  shim_val_t* rval;

  /* construct the set of arguments to be passed to the callback */
  shim_val_t* argv[] = {
    shim_integer_new(ctx, 10),
    shim_string_new_copy(ctx, "baz"),
    shim_undefined(),
  };

  /* call the callback without specifying a `this` for the function */
  shim_bool_t = shim_func_call_val(ctx, NULL, factory, 3, argv, &rval);


  /*
   * we only want to release the values we've created, incoming arguments
   * don't need released because the addon layer handles those for us
   */
  for (i = 0; i < 3; i++)
    shim_value_release(argv[i]);

  if (ret) {
    /* the callback was fine, use its return value */
    shim_args_set_rval(ctx, args, rval);
  } else {
    /* the callback failed, don't leak memory */
    shim_value_release(rval);
  }

  return ret;
}
~~~~~~~~~~~~~~~

And then in JavaScript

~~~~~~~~~~~~~~~{.js}
function createObj(a, b, c) {
  return {
    foo: a,
    bar: b,
    baz: c,
  };
}

var myObj = module.do_something('foobarbaz', createObj);
~~~~~~~~~~~~~~~
