/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>

#include <base/allocate.h>
#include <base/error_or.h>
#include <base/fs.h>
#include <base/options.h>
#include <base/process.h>

#include <arm64.h>

DECLARE_SHARED_ALLOCATOR(arm64)

DA_IMPL(Assembly)

size_t label_reserve_id()
{
    static size_t id = 5000;
    return id++;
}

size_t assembly_add_string(Assembly *assembly, StringView str)
{
    for (StringID *sid = assembly->strings; sid; sid = sid->next) {
        if (sv_eq(sid->string, str)) {
            return sid->id;
        }
    }
    size_t id = label_reserve_id();
    code_add_directive(assembly->data, ".align", "2");
    code_add_label(assembly->data, sv_printf("str_%zu", id));
    // .asciz  may need to be .string
    code_add_directive(assembly->data, ".asciz", sv_cstr(sv_printf("\"%.*s\"", SV_ARG(str)), NULL));
    StringID *sid = allocate_new(StringID);
    sid->string = str;
    sid->id = id;
    sid->next = assembly->strings;
    assembly->strings = sid;
    return id;
}

void assembly_add_data(Assembly *assembly, StringView label, bool global, StringView type, bool is_static, StringView value)
{
    if (code_empty(assembly->data)) {
        code_select_prolog(assembly->data);
        code_add_directive(assembly->data, ".section", "__DATA, __data");
        code_select_code(assembly->data);
    }
    if (global) {
        code_add_directive(assembly->data, ".global", sv_cstr(label, NULL));
    }
    code_add_directive(assembly->data, ".align", "8");
    code_add_label(assembly->data, label);
    code_add_directive(assembly->data, sv_cstr(type, NULL), sv_cstr(value, NULL));
    if (is_static) {
        code_add_directive(assembly->data, ".short", "0");
    }
}

StringView assembly_to_string(Assembly *assembly)
{
    code_select_prolog(assembly->code);
#ifdef IS_APPLE
    code_add_directive(assembly->code, ".section", "__TEXT,__text,regular,pure_instructions");
    code_add_directive(assembly->code, ".align", "2");
#elif defined(IS_LINUX)
    code_add_directive(assembly->code, sv_from(".text"), sv_null());
    code_add_directive(assembly->code, sv_from(".align"), sv_from("2"));
#endif
    code_add_import(assembly->code, sv_from("_resolve_function"));

    code_select_code(assembly->code);
    for (size_t ix = 0; ix < assembly->functions.size; ++ix) {
        ARM64Function *function = assembly->functions.elements + ix;
        Code          *code = NULL;
        switch (function->function->kind) {
        case FK_SCRIBBLE:
            code = function->scribble.code;
            break;
        case FK_NATIVE:
            code = function->native.code;
            break;
        default:
            break;
        }
        if (code && code_has_text(code)) {
            code_append_code(assembly->code, code);
        }
    }
    if (code_empty(assembly->code)) {
        assembly->has_exports = false;
        return sv_null();
    }

    code_select_epilog(assembly->code);
    code_append_code(assembly->code, assembly->data);
    return code_to_string(assembly->code);
}

void assembly_save_and_assemble(Assembly *assembly, StringView bare_file_name)
{
    StringView asm_text = assembly_to_string(assembly);
    if (!assembly_has_exports(assembly)) {
        sv_free(asm_text);
        return;
    }
    StringView asm_file = sv_printf("%.*s.s", SV_ARG(bare_file_name));
    StringView obj_file = sv_printf("%.*s.o", SV_ARG(bare_file_name));
    if (!fs_file_exists(asm_file) || !fs_file_exists(obj_file) || fs_is_newer(obj_file, asm_file)) {
        FILE *s = fopen(sv_cstr(asm_file, NULL), "w+");
        if (!s) {
            fatal("Could not open assembly file %.*s: %s", SV_ARG(asm_file), strerror(errno));
        }
        if (fwrite(sv_cstr(asm_text, NULL), 1, asm_text.length, s) != asm_text.length) {
            fatal("Could not write assembly text to %.*s: %s", SV_ARG(asm_file), strerror(ferror(s)));
        }
        fclose(s);
        sv_free(asm_text);
    }
    MUST(Int, execute(sv_from("as"), sv_cstr(asm_file, NULL), "-o", sv_cstr(obj_file, NULL)));
    sv_free(asm_file);
    sv_free(obj_file);
}

bool assembly_has_exports(Assembly *assembly)
{
    return assembly->has_exports;
}

bool assembly_has_main(Assembly *assembly)
{
    return assembly->has_main;
}

ARM64Function *assembly_function_by_name(Assembly *assembly, StringView name)
{
    for (size_t ix = 0; ix < assembly->functions.size; ++ix) {
        if (sv_eq(assembly->functions.elements[ix].function->name, name)) {
            return assembly->functions.elements + ix;
        }
    }
    return NULL;
}
