/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2008  litl, LLC
 * Copyright (c) 2009 Red Hat, Inc.
 * Copyright (c) 2017  Philip Chimento <philip.chimento@gmail.com>
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

#include <gio/gio.h>

#include "global.h"
#include "importer.h"
#include "jsapi-constructor-proxy.h"
#include "jsapi-util.h"
#include "jsapi-wrapper.h"

static bool
gjs_log(JSContext *cx,
        unsigned   argc,
        JS::Value *vp)
{
    JS::CallArgs argv = JS::CallArgsFromVp(argc, vp);

    if (argc != 1) {
        gjs_throw(cx, "Must pass a single argument to log()");
        return false;
    }

    JSAutoRequest ar(cx);

    /* JS::ToString might throw, in which case we will only log that the value
     * could not be converted to string */
    JS::AutoSaveExceptionState exc_state(cx);
    JS::RootedString jstr(cx, JS::ToString(cx, argv[0]));
    exc_state.restore();

    if (jstr == NULL) {
        g_message("JS LOG: <cannot convert value to string>");
        return true;
    }

    GjsAutoJSChar s(cx);
    if (!gjs_string_to_utf8(cx, JS::StringValue(jstr), &s))
        return false;

    g_message("JS LOG: %s", s.get());

    argv.rval().setUndefined();
    return true;
}

static bool
gjs_log_error(JSContext *cx,
              unsigned   argc,
              JS::Value *vp)
{
    JS::CallArgs argv = JS::CallArgsFromVp(argc, vp);

    if ((argc != 1 && argc != 2) || !argv[0].isObject()) {
        gjs_throw(cx, "Must pass an exception and optionally a message to logError()");
        return false;
    }

    JSAutoRequest ar(cx);

    JS::RootedString jstr(cx);

    if (argc == 2) {
        /* JS::ToString might throw, in which case we will only log that the
         * value could not be converted to string */
        JS::AutoSaveExceptionState exc_state(cx);
        jstr = JS::ToString(cx, argv[1]);
        exc_state.restore();
    }

    gjs_log_exception_full(cx, argv[0], jstr);

    argv.rval().setUndefined();
    return true;
}

static bool
gjs_print_parse_args(JSContext    *cx,
                     JS::CallArgs& argv,
                     GjsAutoChar  *buffer)
{
    GString *str;
    guint n;

    JSAutoRequest ar(cx);

    str = g_string_new("");
    for (n = 0; n < argv.length(); ++n) {
        /* JS::ToString might throw, in which case we will only log that the
         * value could not be converted to string */
        JS::AutoSaveExceptionState exc_state(cx);
        JS::RootedString jstr(cx, JS::ToString(cx, argv[n]));
        exc_state.restore();

        if (jstr != NULL) {
            GjsAutoJSChar s(cx);
            if (!gjs_string_to_utf8(cx, JS::StringValue(jstr), &s)) {
                g_string_free(str, true);
                return false;
            }

            g_string_append(str, s);
            if (n < (argv.length()-1))
                g_string_append_c(str, ' ');
        } else {
            *buffer = g_string_free(str, true);
            if (!*buffer)
                *buffer = g_strdup("<invalid string>");
            return true;
        }

    }
    *buffer = g_string_free(str, false);

    return true;
}

static bool
gjs_print(JSContext *context,
          unsigned   argc,
          JS::Value *vp)
{
    JS::CallArgs argv = JS::CallArgsFromVp (argc, vp);

    GjsAutoChar buffer;
    if (!gjs_print_parse_args(context, argv, &buffer))
        return false;

    g_print("%s\n", buffer.get());

    argv.rval().setUndefined();
    return true;
}

static bool
gjs_printerr(JSContext *context,
             unsigned   argc,
             JS::Value *vp)
{
    JS::CallArgs argv = JS::CallArgsFromVp(argc, vp);

    GjsAutoChar buffer;
    if (!gjs_print_parse_args(context, argv, &buffer))
        return false;

    g_printerr("%s\n", buffer.get());

    argv.rval().setUndefined();
    return true;
}

class GjsGlobal {
    static constexpr JSClassOps class_ops = {
        nullptr,  /* addProperty */
        nullptr,  /* deleteProperty */
        nullptr,  /* getProperty */
        nullptr,  /* setProperty */
        nullptr,  /* enumerate */
        nullptr,  /* resolve */
        nullptr,  /* mayResolve */
        nullptr,  /* finalize */
        nullptr,  /* call */
        nullptr,  /* hasInstance */
        nullptr,  /* construct */
        JS_GlobalObjectTraceHook
    };

    static constexpr JSClass klass = {
        "GjsGlobal",
        JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(GJS_GLOBAL_SLOT_LAST),
        &GjsGlobal::class_ops,
    };

    static constexpr JSFunctionSpec static_funcs[] = {
        JS_FS("log", gjs_log, 1, GJS_MODULE_PROP_FLAGS),
        JS_FS("logError", gjs_log_error, 2, GJS_MODULE_PROP_FLAGS),
        JS_FS("print", gjs_print, 0, GJS_MODULE_PROP_FLAGS),
        JS_FS("printerr", gjs_printerr, 0, GJS_MODULE_PROP_FLAGS),
        JS_FS_END
    };

public:

    static JSObject *
    create(JSContext *cx)
    {
        JS::CompartmentOptions compartment_options;
        compartment_options.behaviors().setVersion(JSVERSION_LATEST);
        JS::RootedObject global(cx,
            JS_NewGlobalObject(cx, &GjsGlobal::klass, nullptr,
                               JS::FireOnNewGlobalHook, compartment_options));
        if (!global)
            return nullptr;

        JSAutoCompartment ac(cx, global);

        if (!JS_InitStandardClasses(cx, global) ||
            !JS_InitReflectParse(cx, global) ||
            !JS_DefineDebuggerObject(cx, global))
            return nullptr;

        return global;
    }

    static bool
    define_properties(JSContext       *cx,
                      JS::HandleObject global)
    {
        if (!JS_DefineProperty(cx, global, "window", global,
                               JSPROP_READONLY | JSPROP_PERMANENT) ||
            !JS_DefineFunctions(cx, global, GjsGlobal::static_funcs) ||
            !gjs_define_constructor_proxy_factory(cx, global))
            return false;

        JS::Value v_importer = gjs_get_global_slot(cx, GJS_GLOBAL_SLOT_IMPORTS);
        g_assert(((void) "importer should be defined before passing null "
                  "importer to GjsGlobal::define_properties",
                  v_importer.isObject()));
        JS::RootedObject root_importer(cx, &v_importer.toObject());

        /* Wrapping is a no-op if the importer is already in the same
         * compartment. */
        return JS_WrapObject(cx, &root_importer) &&
            gjs_object_define_property(cx, global, GJS_STRING_IMPORTS,
                                       root_importer, GJS_MODULE_PROP_FLAGS);
    }
};

/**
 * gjs_create_global_object:
 * @cx: a #JSContext
 *
 * Creates a global object, and initializes it with the default API.
 *
 * Returns: the created global object on success, nullptr otherwise, in which
 * case an exception is pending on @cx
 */
JSObject *
gjs_create_global_object(JSContext *cx)
{
    return GjsGlobal::create(cx);
}

/**
 * gjs_define_global_properties:
 * @cx: a #JSContext
 * @global: a JS global object that has not yet been passed to this function
 *
 * Defines properties on the global object such as 'window' and 'imports'.
 * This function completes the initialization of a new global object, but it
 * is separate from gjs_create_global_object() because all globals share the
 * same root importer.
 * The code creating the main global for the JS context needs to create the
 * root importer in between calling gjs_create_global_object() and
 * gjs_define_global_properties().
 *
 * The caller of this function should be in the compartment for @global.
 * If the root importer object belongs to a different compartment, this
 * function will create a cross-compartment wrapper for it.
 *
 * Returns: true on success, false otherwise, in which case an exception is
 * pending on @cx
 */
bool
gjs_define_global_properties(JSContext       *cx,
                             JS::HandleObject global)
{
    return GjsGlobal::define_properties(cx, global);
}

void
gjs_set_global_slot(JSContext    *cx,
                    GjsGlobalSlot slot,
                    JS::Value     value)
{
    JSObject *global = gjs_get_import_global(cx);
    JS_SetReservedSlot(global, JSCLASS_GLOBAL_SLOT_COUNT + slot, value);
}

JS::Value
gjs_get_global_slot(JSContext    *cx,
                    GjsGlobalSlot slot)
{
    JSObject *global = gjs_get_import_global(cx);
    return JS_GetReservedSlot(global, JSCLASS_GLOBAL_SLOT_COUNT + slot);
}

decltype(GjsGlobal::class_ops) constexpr GjsGlobal::class_ops;
decltype(GjsGlobal::klass) constexpr GjsGlobal::klass;
decltype(GjsGlobal::static_funcs) constexpr GjsGlobal::static_funcs;
