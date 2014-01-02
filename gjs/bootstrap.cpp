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
#include "gjs/jsapi-util-args.h"
#include "gjs/jsapi-wrapper.h"
#include "native.h"

/* The bootstrap process is the thing that sets up the import system.
 * As such, we give it a hook to import any native modules it may need.
 *
 * The rest of the functionality that the bootstrap code needs should be
 * in independent native modules which can be imported by this API,
 * rather than in the bootstrap environment.
 */

static bool
import_native_module(JSContext *cx,
                     unsigned   argc,
                     jsval     *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    g_autofree char *module_name = NULL;

    if (!gjs_parse_call_args(cx, "importNativeModule", args, "s",
                             "moduleName", &module_name))
        return false;

    JS::RootedObject module_obj(cx);
    if (!gjs_import_native_module(cx, module_name, &module_obj))
        return false;

    args.rval().setObjectOrNull(module_obj);
    return true;
}

static JSFunctionSpec environment_funcs[] = {
    JS_FS("importNativeModule", import_native_module, 1, GJS_MODULE_PROP_FLAGS),
    JS_FS_END,
};

static bool
define_bootstrap_environment(JSContext              *cx,
                             JS::MutableHandleObject environment)
{
    environment.set(JS_NewObject(cx, NULL, JS::NullPtr(), JS::NullPtr()));

    if (!environment)
        return false;

    return JS_DefineFunctions(cx, environment, &environment_funcs[0]);
}

#define BOOTSTRAP_FILE "resource:///org/gnome/gjs/modules/bootstrap.js"

bool
gjs_run_bootstrap(JSContext *cx)
{
    JS::RootedObject environment(cx);
    define_bootstrap_environment(cx, &environment);
    if (!environment)
        return false;

    g_autoptr(GFile) file = g_file_new_for_uri(BOOTSTRAP_FILE);
    g_autofree char *script = NULL;
    size_t script_len = 0;

    if (!g_file_load_contents(file, NULL, &script, &script_len, NULL, NULL))
        return false;

    JS::RootedValue ignored(cx);
    return gjs_eval_with_scope(cx, environment, script, script_len,
                               BOOTSTRAP_FILE, &ignored);
}
