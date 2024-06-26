/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include <base/allocate.h>
#include <base/error_or.h>
#include <base/fn.h>
#include <base/options.h>
#include <base/process.h>

#include <arm64.h>

DECLARE_SHARED_ALLOCATOR(arm64)
SHARED_ALLOCATOR_IMPL(arm64)

OpcodeMap get_opcode_map(type_id type)
{
    OpcodeMap ret = { 0 };
    ret.type = type;
    size_t sz = typeid_sizeof(type);
    bool   un_signed = false;
    if (typeid_kind(type) == TK_PRIMITIVE) {
        BuiltinType builtin_type = typeid_builtin_type(type);
        uint16_t    type_meta = type >> 16;
        un_signed = type_meta & 0x0100;
    }
    switch (sz) {
    case 0x01:
        ret.load_opcode = (un_signed) ? "ldrb" : "ldrsb";
        ret.store_opcode = "strb";
        ret.reg_width = RW_32;
        break;
    case 0x02:
        ret.load_opcode = (un_signed) ? "ldrh" : "ldrsh";
        ret.store_opcode = "strh";
        ret.reg_width = RW_32;
        break;
    case 0x04:
        ret.load_opcode = "ldr";
        ret.store_opcode = "str";
        ret.reg_width = RW_32;
        break;
    default:
        ret.load_opcode = "ldr";
        ret.store_opcode = "str";
        ret.reg_width = RW_64;
        break;
    }
    return ret;
}

StringView value_location_to_string(ValueLocation loc)
{
    StringBuilder sb = sb_create();
    sb_append_cstr(&sb, ValueLocationKind_name(loc.kind));
    sb_append_cstr(&sb, " ");
    switch (loc.kind) {
    case VLK_POINTER:
        sb_printf(&sb, "[%s, #0x%llx]", x_reg(loc.pointer.reg), loc.pointer.offset);
        break;
    case VLK_REGISTER:
        sb_printf(&sb, "%s",
            reg_with_width(loc.reg, (typeid_sizeof(loc.type) == 8) ? RW_64 : RW_32));
        break;
    case VLK_REGISTER_RANGE:
        sb_printf(&sb, "%s-%s",
            reg_with_width(loc.range.start, (typeid_sizeof(loc.type) == 8) ? RW_64 : RW_32),
            reg_with_width(loc.range.end - 1, (typeid_sizeof(loc.type) == 8) ? RW_64 : RW_32));
        break;
    case VLK_LABEL:
    case VLK_DATA:
        sb_printf(&sb, "%.*s", SV_ARG(loc.static_data.symbol));
        if (loc.static_data.offset > 0) {
             sb_printf(&sb, "+0x%0llx", loc.static_data.offset);
        }
        break;
    case VLK_IMMEDIATE:
        if ((int) loc.integer.type > 0) {
            uint64_t v = MUST_OPTIONAL(UInt64, integer_unsigned_value(loc.integer));
            sb_printf(&sb, "#%llu", v);
        } else {
            int64_t v = MUST_OPTIONAL(Int64, integer_signed_value(loc.integer));
            sb_printf(&sb, "#%lld", v);
        }
        break;
    case VLK_FLOAT:
        sb_printf(&sb, "#%f", loc.float_value);
        break;
    case VLK_DISCARD:
    case VLK_STACK:
        break;
    default:
        UNREACHABLE();
    }
    sb_printf(&sb, "%s (%.*s, %zu)", (loc.dont_release) ? "!" : "", SV_ARG(typeid_name(loc.type)), typeid_sizeof(loc.type));
    return sb.view;
}

ErrorOrInt output_arm64(BackendConnection *conn, IRProgram *program)
{
    // if (OPT_DEBUG) {
    //     register_execution_observer(arm64_inspect);
    // }

    JSONValue config = conn->config;
    JSONValue stages = json_get_default(&config, "stages", json_array());
    JSONValue stage = {0};
    for (size_t ix = 0; ix < json_len(&stages); ++ix) {
        stage = MUST_OPTIONAL(JSONValue, json_at(&stages, ix));
        StringView name = json_get_string(&stage, "name", sv_null());
        if (sv_eq_cstr(name, "generate")) {
            break;
        }
    }
    assert(stage.type == JSON_TYPE_OBJECT);

    ARM64Context ctx = {0};
    ctx.conn = conn;
    ctx.program = program;
    ctx.scope.kind = SK_GLOBAL;
    ctx.scope.up = NULL;
    if (stage.type != JSON_TYPE_NULL) {
        ctx.debug = json_get_bool(&stage, "debug", false);
    }
    generate_arm64(conn, program, &ctx);
    Assembly     *main = NULL;

#ifdef IS_APPLE
    StringView sdk_path = { 0 }; // "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk";
    if (sv_empty(sdk_path)) {
        Process *p = process_create(sv_from("xcrun"), "-sdk", "macosx", "--show-sdk-path");
        MUST(Int, process_execute(p));
        sdk_path = sv_strip(p->out.buffer.view);
    }
#endif

    for (size_t ix = 0; ix < ctx.assemblies.size; ++ix) {
        Assembly *assembly = ctx.assemblies.elements + ix;
        if (assembly_has_main(assembly)) {
            main = assembly;
            break;
        }
    }
    if (!main) {
        fatal("No main() function found");
    }

    if (mkdir(".scribble", 0755) && (errno != EEXIST)) {
        fatal("Could not create .scribble build directory");
    }

#if 0
    assembly_new_function(main);
    for (size_t ix = 0; ix < ctx->assemblies.size; ++ix) {
        Assembly *assembly = ctx->assemblies.elements + ix;
        size_t      fnc_ix = da_append_ARM64Function(
            &main->functions,
            (ARM64Function) {
                     .assembly = assembly,
                     .function = function,
                     .scope.kind = SK_FUNCTION,
                     .scope.up = &assembly->scope,
            });
        ARM64Function *arm_function = assembly->functions.elements + fnc_ix;
        if (!assembly_has_static(assembly) || !assembly_has_exports(assembly)) {
            continue;
        }
        assembly_add_instruction(main, "bl", "static_%.*s", SV_ARG(assembly->name));
    }
    code_close_function(main->active, sv_from("static_initializer"), 0);
#endif

    StringList modules = sl_create();
    for (size_t ix = 0; ix < ctx.assemblies.size; ++ix) {
        Assembly  *assembly = ctx.assemblies.elements + ix;
        StringView bare_file_name = fn_barename(assembly->module->name);
        bare_file_name = sv_printf(".scribble/%.*s", SV_ARG(bare_file_name));
        assembly_save_and_assemble(assembly, bare_file_name);
        if (assembly_has_exports(assembly)) {
            if (!json_get_bool(&stage, "keep-assembly", false)) {
                StringView asm_file = sv_printf("%.*s.s", SV_ARG(bare_file_name));
                char buf[asm_file.length + 1];
                unlink(sv_cstr(asm_file, buf));
                sv_free(asm_file);
            }
            StringView obj_file = sv_printf("%.*s.o", SV_ARG(bare_file_name));
            sl_push(&modules, obj_file);
        }
    }

    int result = 0;
    if (modules.size > 0) {
        StringView scribble_dir = get_option(sv_from("scribble-dir"));
        StringView bin_name = program->name;
        int        slash = sv_last(bin_name, '/');
        if (slash >= 0) {
            bin_name = sv_lchop(bin_name, slash + 1);
        }
        int dot = sv_last(bin_name, '.');
        if (dot > 0) {
            bin_name = sv_rchop(bin_name, bin_name.length - dot);
        }

#ifdef IS_APPLE
        StringList ld_args = sl_create();
        sl_push(&ld_args, sv_from("-o"));
        sl_push(&ld_args, bin_name);
        sl_push(&ld_args, SV("-lbase"));
        sl_push(&ld_args, SV("-lscribblert"));
        sl_push(&ld_args, SV("-lscribblestart"));
        sl_push(&ld_args, SV("-lSystem"));
        sl_push(&ld_args, SV("-syslibroot"));
        sl_push(&ld_args, sdk_path);
        sl_push(&ld_args, SV("-e"));
        sl_push(&ld_args, SV("_start"));
        sl_push(&ld_args, SV("-arch"));
        sl_push(&ld_args, SV("arm64"));
        sl_push(&ld_args, sv_printf("-L%.*s/lib", SV_ARG(scribble_dir)));
        sl_extend(&ld_args, &modules);

        //        std::vector<std::string> ld_args = { "-o", config.main(), "-loblrt",
        //            "-lSystem", "-syslibroot", sdk_path, "-e", "_start", "-arch", "arm64",
        //            format("-L{}/lib", obl_dir) };
        //        for (auto& m : modules)
        //            ld_args.push_back(m);

        int ld_result = MUST(Int, execute_sl(SV("ld"), &ld_args));
        if (ld_result) {
            fatal("ld failed with exit code %d", ld_result);
        }
#elif defined(IS_LINUX)
        StringList ld_args = sl_create();
        sl_push(&ld_args, sv_from("-o"));
        sl_push(&ld_args, bin_name);
        sl_push(&ld_args, sv_from("-lscribblert"));
        sl_push(&ld_args, sv_from("-e"));
        sl_push(&ld_args, sv_from("_start"));
        sl_push(&ld_args, sv_from("-A"));
        sl_push(&ld_args, sv_from("aarch64"));
        sl_push(&ld_args, sv_printf("-L%.*s/lib", SV_ARG(scribble_dir)));
        sl_extend(&ld_args, &modules);
        MUST(Int, int, ld_result, execute_sl(sv_from("ld"), &ld_args))
        if (ld_result) {
            fatal("ld failed with exit code %d", ld_result);
        }
#endif
        if (OPT_RUN) {
            fflush(stderr);
            fflush(stdout);
            StringView run_cmd = sv_printf("./%.*s", SV_ARG(bin_name));
            Process   *p = process_create(run_cmd);
            ErrorOrInt exit_code_or_error = process_execute(p);
            if (ErrorOrInt_is_error(exit_code_or_error)) {
                Error e = exit_code_or_error.error;
                ERROR(Int, ProcessError, e.code, "Program execution failed: %s", Error_to_string(e));
            } else {
                printf("%.*s", SV_ARG(p->out.buffer.view));
                result = exit_code_or_error.value;
                if (has_option("exit-code")) {
                    printf("%d\n", result);
                }
            }
        }
    }
    RETURN(Int, result);
}
