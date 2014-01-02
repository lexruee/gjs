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

#include "gi/ns.h"
#include "gjs/byteArray.h"
#include "gjs/context.h"
#include "gjs/jsapi-util-args.h"
#include "importer.h"

static bool
import_gi_module(JSContext *cx,
                 unsigned   argc,
                 jsval     *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    g_autofree char *module_name = NULL;
    g_autofree char *module_version = NULL;

    if (!gjs_parse_call_args(cx, "importGIModule", args, "s?s",
                             "moduleName", &module_name,
                             "moduleVersion", &module_version))
        return false;

    JS::RootedObject module_obj(cx);
    if (!gjs_import_gi_module(cx, module_name, module_version, &module_obj))
        return false;

    args.rval().setObject(*module_obj);
    return true;
}

static bool
eval_with_scope(JSContext *cx,
                unsigned   argc,
                jsval     *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject scope(cx);
    JS::RootedObject script_obj(cx);
    guint8 *script;
    size_t script_len;
    g_autofree char *filename = NULL;

    if (!gjs_parse_call_args(cx, "evalWithScope", args, "oos",
                             "scope", &scope,
                             "script", &script_obj,
                             "filename", &filename))
        return false;

    gjs_byte_array_peek_data(cx, script_obj, &script, &script_len);

    return gjs_eval_with_scope(cx, scope, (const char *) script, script_len,
                               filename, args.rval());
}

static bool
get_builtin_search_path(JSContext *cx,
                        unsigned   argc,
                        jsval     *vp)
{
    GjsContext *gjs_context = GJS_CONTEXT(JS_GetContextPrivate(cx));
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    const char **context_search_path;
    int context_search_path_length;
    const char **global_search_path;
    int global_search_path_length;
    JS::AutoValueVector elems(cx);
    int i;

    context_search_path = gjs_context_get_search_path(gjs_context);
    context_search_path_length = context_search_path ? g_strv_length((char **) context_search_path) : 0;
    global_search_path = (const char **) gjs_get_search_path();
    global_search_path_length = global_search_path ? g_strv_length((char **) global_search_path) : 0;

    elems.reserve(context_search_path_length + global_search_path_length);

    JS::RootedValue element(cx);
    for (i = 0; i < context_search_path_length; i++) {
        element.setString(JS_NewStringCopyZ(cx, context_search_path[i]));
        elems.infallibleAppend(element);
    }

    for (i = 0; i < global_search_path_length; i++) {
        element.setString(JS_NewStringCopyZ(cx, global_search_path[i]));
        elems.infallibleAppend(element);
    }

    args.rval().setObject(*JS_NewArrayObject(cx, elems));
    return true;
}

static JSFunctionSpec module_funcs[] = {
    JS_FS("importGIModule", import_gi_module, 2, GJS_MODULE_PROP_FLAGS),
    JS_FS("evalWithScope", eval_with_scope, 3, GJS_MODULE_PROP_FLAGS),
    JS_FS("getBuiltinSearchPath", get_builtin_search_path, 0, GJS_MODULE_PROP_FLAGS),
    JS_FS_END,
};

bool
gjs_js_define_importer_stuff(JSContext              *cx,
                             JS::MutableHandleObject module)
{
    module.set(JS_NewObject(cx, NULL, JS::NullPtr(), JS::NullPtr()));
    return JS_DefineFunctions(cx, module, &module_funcs[0]);
}
