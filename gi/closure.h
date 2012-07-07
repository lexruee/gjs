/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2008  litl, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __GJS_CLOSURE_H__
#define __GJS_CLOSURE_H__

#include <glib-object.h>

#include <jsapi.h>

G_BEGIN_DECLS

GClosure*  gjs_closure_new           (JSContext    *context,
                                      JSObject     *callable,
                                      const char   *description,
                                      gboolean      root_function);
JSBool     gjs_closure_invoke        (GClosure     *closure,
                                      JSObject     *this_obj,
                                      int           argc,
                                      jsval        *argv,
                                      jsval        *retval);
JSRuntime* gjs_closure_get_runtime   (GClosure     *closure);
JSContext* gjs_closure_get_context   (GClosure     *closure);
gboolean   gjs_closure_is_valid      (GClosure     *closure);
JSObject*  gjs_closure_get_callable  (GClosure     *closure);

void       gjs_closure_trace         (GClosure     *closure,
                                      JSTracer     *tracer);

G_END_DECLS

#endif  /* __GJS_CLOSURE_H__ */
