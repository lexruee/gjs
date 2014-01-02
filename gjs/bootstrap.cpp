/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright 2013 Red Hat, Inc.
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

#include "config.h"

#include <gio/gio.h>

#include "bootstrap.h"
#include "gjs/jsapi-util.h"
#include "gjs/jsapi-wrapper.h"

#define BOOTSTRAP_FILE "resource:///org/gnome/gjs/modules/bootstrap.js"

bool
gjs_run_bootstrap(JSContext *cx)
{
    JS::RootedObject environment(cx,
        JS_NewObject(cx, NULL, JS::NullPtr(), JS::NullPtr()));

    g_autoptr(GFile) file = g_file_new_for_uri(BOOTSTRAP_FILE);
    g_autofree char *script = NULL;
    size_t script_len = 0;

    if (!g_file_load_contents(file, NULL, &script, &script_len, NULL, NULL))
        return false;

    JS::RootedValue ignored(cx);
    return gjs_eval_with_scope(cx, environment, script, script_len,
                               BOOTSTRAP_FILE, &ignored);
}
