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

#ifndef NODE_SHIM_IMPL_H
#define NODE_SHIM_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif


struct shim_val_s {
  void* handle;
  enum shim_type type;
};

struct shim_ctx_s {
  void* scope;
  void* isolate;
  void* trycatch;
  void* allocs;
};

struct shim_args_s {
  size_t argc;
  shim_val_t *self;
  shim_val_t **argv;
  shim_val_t *ret;
};

struct shim_work_s {
  shim_work_cb work_cb;
  shim_after_work after_cb;
  void* hint;
};

#ifdef __cplusplus
}
#endif

#endif
