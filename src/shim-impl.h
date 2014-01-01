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

#include "shim.h"

#if NODE_VERSION_AT_LEAST(0, 11, 3)
#define V8_USE_UNSAFE_HANDLES 1
#define V8_ALLOW_ACCESS_TO_RAW_HANDLE_CONSTRUCTOR 1
#endif

#include "v8.h"
#include "node.h"
#include "node_buffer.h"

struct shim_val_s {
  void* handle;
  enum shim_type type;
};


struct shim_persistent_s {
  v8::Persistent<v8::Value> handle;
};


struct shim_ctx_s {
  void* scope;
  v8::Isolate* isolate;
  void* trycatch;
  void* allocs;
};


struct shim_args_s {
  size_t argc;
  shim_val_s *self;
  shim_val_s **argv;
  shim_val_s *ret;
  void* data;
};


struct shim_work_s {
  shim_work_cb work_cb;
  shim_after_work after_cb;
  void* hint;
};


extern shim_val_s shim__undefined;
extern shim_val_s shim__null;


typedef struct weak_baton_s {
  shim_weak_cb weak_cb;
  void* data;
} weak_baton_t;

#endif
