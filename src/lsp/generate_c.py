#!/usr/bin/python3

import json
import sys

emitted_structures = set()
emitted_types = []
depends = []
structures = {}
variants = {}
enums = {}
current = set()

builtin = {
    "string": { "name": "string", "cname": "StringView", "calias": "StringView", "plural": "StringList" },
    "URI": { "name": "URI", "cname": "StringView", "calias": "StringView", "plural": "StringList" },
    "DocumentUri": { "name": "DocumentUri", "cname": "StringView", "calias": "StringView", "plural": "StringList" },
    "bool": { "name": "bool", "cname": "bool", "calias": "Bool" },
    "int": { "name": "int", "cname": "int", "calias": "Int" },
    "uint": { "name": "uint", "cname": "unsigned int", "calias": "UInt" },
    "null": { "name": "null", "cname": "Null", "calias": "Null" },
    "empty": { "name": "empty", "cname": "Empty", "calias": "Empty" },
    "any": { "name": "any", "cname": "JSONValue", "calias": "JSONValue" },
    "BoolOrNull": { "name": "BoolOrNull", "cname": "BoolOrNull", "calias": "BoolOrNull" },
    "BoolOrEmpty": { "name": "BoolOrEmpty", "cname": "BoolOrEmpty", "calias": "BoolOrEmpty" },
    "IntOrNull": { "name": "IntOrNull", "cname": "IntOrNull", "calias": "IntOrNull" },
}

def p(i, f, line, eol=True):
    end = '\n' if eol else ''
    print(f'{" "*i}{line}', end=end, file=f)


def render_type(field):
    t = field["type"]
    typedesc = {
        "name": t,
        "cname": t,
        "calias": t,
        "plural": t + "s"
    }
    if t in builtin:
        typedesc = builtin[t]
    t = typedesc["cname"] if "cname" in typedesc else t
    if "array" in field and field["array"]:
        if "plural" not in typedesc:
            typedesc["plural"] = typedesc["calias"] + "s" if "calias" in typedesc else t + "s"
        t = typedesc["plural"]
    if "optional" in field and field["optional"]:
        t = "Optional" + typedesc["calias"]
    return t


def render_prefix(field):
    t = field["type"]
    typedesc = {
        "name": t,
        "cname": t,
        "calias": t,
        "plural": t + "s"
    }
    if t in builtin:
        typedesc = builtin[t]
    t = typedesc["calias"] if "calias" in typedesc else typedesc["cname"] if "cname" in typedesc else t
    if "array" in field and field["array"]:
        if "plural" not in typedesc:
            typedesc["plural"] = typedesc["calias"] + "s" if "calias" in typedesc else t + "s"
        t = typedesc["plural"]
    if "optional" in field and field["optional"]:
        t = "Optional" + (typedesc["calias"] if "calias" in typedesc else t)
    return t


def emit_enum_h(name, h):
    enum = enums[name]
    print("typedef enum {", file=h)
    for value in enum["values"]:
        print(f"    {name}{value['name']}", file=h, end="")
        if enum['value_type'] == "int":
            print(f" = {value['value']}", file=h, end="")
        print(",", file=h)
    print(f"}} {name};", file=h)
    print(file=h)

    if "optional" in enum and enum['optional']:
        print(f"OPTIONAL({name});", file=h)
        print(f"OPTIONAL_JSON({name});", file=h)
        print(file=h)
    if "array" in enum and enum['array']:
        plural = enum['plural'] if "plural" in enum else name + "s"
        print(f"DA_WITH_NAME({name}, {plural});", file=h)
        print(f"JSON({plural}, {plural});", file=h)
        if "optional-array" in enum and enum['optional-array']:
            print(f"OPTIONAL({plural});", file=h)
            print(f"OPTIONAL_JSON({plural});", file=h)
        print(file=h)

    if enum['value_type'] == "string":
        print(f"extern StringView {name}_to_string({name} value);", file=h)
        print(f"extern {name} {name}_parse(StringView s);", file=h)

    if "decode" not in enum or enum["decode"]:
        print(f"extern {name} {name}_decode(OptionalJSONValue value);", file=h)
    if "encode" not in enum or enum["encode"]:
        print(f"extern OptionalJSONValue {name}_encode({name} value);", file=h)
    print(file=h)


def emit_enum_c(name, c):
    enum = enums[name]
    if "optional" in enum and enum['optional']:
        print(f"OPTIONAL_JSON_IMPL({name});", file=c)
        print(file=c)
    if "array" in enum and enum['array']:
        plural = enum['plural'] if "plural" in enum else name + "s"
        print(f"DA_IMPL({name});", file=c)
        print(f"DA_JSON_IMPL({name}, {plural}, elements);", file=c)
        if "optional-array" in enum and enum['optional-array']:
            print(f"OPTIONAL_JSON_IMPL({plural});", file=c)
        print(file=c)

    if enum['value_type'] == "string":
        print(f"""
StringView {name}_to_string({name} value)
{{
    switch(value) {{""", file=c)
        for value in enum["values"]:
            print(f'    case {name}{value["name"]}: return sv_from("{value["value"]}");', file=c)
        print(f"""    default: UNREACHABLE();
    }}
}}

{name} {name}_parse(StringView s)
{{""", file=c)
        for value in enum["values"]:
            print(f'    if (sv_eq_cstr(s, "{value["value"]}")) return {name}{value["name"]};', file=c)
        print(f"""    UNREACHABLE();
}}
""", file=c)
        if "decode" not in enum or enum["decode"]:
            print(f"""{name} {name}_decode(OptionalJSONValue json)
{{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return {name}_parse(json.value.string);
}}
""", file=c)
        if "encode" not in enum or enum["encode"]:
            print(f"""OptionalJSONValue {name}_encode({name} value)
{{
    RETURN_VALUE(JSONValue, json_string({name}_to_string(value)));
}}
""", file=c)

    if enum['value_type'] == "int":
        if "decode" not in enum or enum["decode"]:
            print(f"""{name} {name}_decode(OptionalJSONValue json)
{{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_INT);
    return ({name}) json_int_value(json.value);
}}
""", file=c)
        if "encode" not in enum or enum["encode"]:
            print(f"""OptionalJSONValue {name}_encode({name} value)
{{
    RETURN_VALUE(JSONValue, json_int((int) value));
}}
""", file=c)


def emit_enum(name, h, c):
    if name in emitted_structures:
        return
    emit_enum_h(name, h)
    emit_enum_c(name, c)
    emitted_structures.add(name)
    emitted_types.append(('E', name))


def emit_enums(h, c):
    for e in enums:
        emit_enum(e, h, c)


def variant_def(typedef, optional, name, variant, h, indent):
    if typedef:
        print("typedef ", end="", file=h)
    print(f"{' '*indent}struct {{", file=h)
    indent += 4
    if not typedef:
        print(f'{" "*indent}bool has_value;', file=h)
    print(f'{" "*indent}int tag;', file=h)
    print(f"{' '*indent}union {{", file=h)
    indent += 4
    tag = 0;
    for v in variant:
        if "type" in v:
            print(f'{" "*indent}{render_type(v)} _{tag};', file=h)
        if "struct" in v:
            inline_struct_def('_' + str(tag), v["struct"], h, indent)
        tag += 1
    indent -= 4
    print(f"{' '*indent}}};", file=h)
    indent -= 4
    print(f"{' '*indent}}}", end="", file=h)
    if name is not None:
        print(f' {name}', end='', file=h)
    print(";", file=h)


def emit_variant(v, h, c):
    pass


def define_field(name, field, h, indent):
    if "type" in field:
        p(indent, h, f'{render_type(field)} {name};')
    if "struct" in field:
        inline_struct_def(name, field["struct"], h, indent)
    if "variant" in field:
        variant_def(False, "optional" in field and field["optional"],
                   name, field["variant"], h, indent)


def struct_def_fields(fields, h, indent):
    for field in fields:
        define_field(field["name"], field, h, indent)


def typedef_struct_def(struct, h):
    p(0, h, 'typedef struct {')
    indent = 4
    struct_def_fields(struct["all_fields"], h, indent)
    indent -= 4
    p(indent, h, f'}} {struct["name"]};')


def inline_struct_def(name, fields, h, indent):
    p(indent, h, 'struct {')
    indent += 4
    p(indent, h,'bool has_value;')
    p(indent, h, "struct {")
    indent += 4
    struct_def_fields(fields, h, indent)
    indent -= 4
    p(indent, h, "};")
    indent -= 4
    p(indent, h, f'}} {name};')


def collect_fields(struct):
    if "all_fields" in struct:
        return
    fields = []
    if "extends" in struct:
        bases_reversed = []
        bases = struct["extends"]
        while len(bases) > 0:
            s = structures[bases.pop()]
            bases_reversed.insert(0, s)
            if "extends" in s:
                bases.append(s["extends"])
        for base in bases_reversed:
            fields.extend(base["fields"])
    fields.extend(struct["fields"])
    struct["all_fields"] = fields


def emit_struct_h(name, h):
    struct = structures[name]
    typedef_struct_def(struct, h)
    print(file=h)

    decode = "decode" not in struct or struct["decode"]
    encode = "encode" not in struct or struct["encode"]

    if "optional" in struct and struct['optional']:
        print(f"OPTIONAL({name});", file=h)
        if encode:
            print(f"OPTIONAL_JSON_ENCODE({name});", file=h)
        if decode:
            print(f"OPTIONAL_JSON_DECODE({name});", file=h)
        print(file=h)
    if "array" in struct and struct['array']:
        plural = struct['plural'] if "plural" in struct else name + "s"
        print(f"DA_WITH_NAME({name}, {plural});", file=h)
        if encode:
            print(f"JSON_ENCODE({plural}, {plural});", file=h)
        if decode:
            print(f"JSON_DECODE({plural}, {plural});", file=h)
        if "optional-array" in struct and struct['optional-array']:
            print(f"OPTIONAL({plural});", file=h)
            if encode:
                print(f"OPTIONAL_JSON_ENCODE({plural});", file=h)
            if decode:
                print(f"OPTIONAL_JSON_DECODE({plural});", file=h)
        print(file=h)
    if decode:
        print(f"extern {name} {name}_decode(OptionalJSONValue value);", file=h)
    if encode:
        print(f"extern OptionalJSONValue {name}_encode({name} value);", file=h)
    print(file=h)


def decode_variant_variant(variant, var, value, tag, c, indent):
    if "type" in variant:
        if variant["type"] == "bool":
            p(indent, c, f'if ({var}.value.type == JSON_TYPE_BOOLEAN) {{')
            p(indent, c, f'    {value}.tag = {tag};')
            p(indent, c, f'    {value}._{tag} = {var}.value.boolean;')
            p(indent, c, f'}}')
        elif variant["type"] == "int":
            p(indent, c, f'if ({var}.value.type == JSON_TYPE_INT) {{')
            p(indent, c, f'    {value}.tag = {tag};')
            p(indent, c, f'    {value}._{tag} = json_int_value({var}.value);')
            p(indent, c, f'}}')
        elif variant["type"] == "null":
            p(indent, c, f'if ({var}.value.type == JSON_TYPE_NULL) {{')
            p(indent, c, f'    {value}.tag = {tag};')
            p(indent, c, f'    {value}._{tag} = (Null) {{}};')
            p(indent, c, f'}}')
        elif variant["type"] == "empty":
            p(indent, c, f'if ({var}.value.type == JSON_TYPE_OBJECT && {var}.value.object.size == 0) {{')
            p(indent, c, f'    {value}.tag = {tag};')
            p(indent, c, f'    {value}._{tag} = (Empty) {{}};')
            p(indent, c, f'}}')
        elif variant["type"] == "string":
            p(indent, c, f'if ({var}.value.type == JSON_TYPE_STRING) {{')
            p(indent, c, f'    {value}.tag = {tag};')
            p(indent, c, f'    {value}._{tag} = {var}.value.string;')
            p(indent, c, f'}}')
        else:
            p(indent, c, f'if ({var}.value.type == JSON_TYPE_OBJECT) {{')
            p(indent, c, f'    {value}.tag = {tag};')
            p(indent, c, f'    {value}._{tag} = {render_prefix(variant)}_decode({var});')
            p(indent, c, f'}}')
    if "struct" in variant:
        p(indent, c, f'if ({var}.value.type == JSON_TYPE_OBJECT) {{')
        indent += 4
        print(f'{" "*indent}{value}.tag = {tag};', file=c)
        for struct_field in variant["struct"]:
            decode_field(struct_field, var, f'{value}._{tag}.{struct_field["name"]}', c, indent)
            # print(f'{" "*indent}    {value}._{tag}.{struct_variant["name"]} = {render_prefix(struct_variant)}_decode({var});', file=c)
        indent -= 4
        print(f'{" "*indent}}}', file=c)


def decode_variant(variant, var, value, c, indent=4):
    if "optional" in variant and variant["optional"]:
        p(indent, c, f'if ({var}.has_value) {{')
        indent += 4
        p(indent, c, f'{value}.has_value = true;')
    else:
        p(indent, c, f'assert({var}.has_value);')
    tag = 0
    for v in variant["variant"]:
        decode_variant_variant(v, var, value, tag, c, indent)
        tag += 1
    if "optional" in variant and variant["optional"]:
        indent -= 4
        p(indent, c, '}')


def decode_field(field, json_var, value, c, indent=4):
    print(f'{" "*indent}{{', file=c)
    indent += 4
    var = "v"+str(indent)
    print(f'{" "*indent}OptionalJSONValue {var} = json_get(&{json_var}.value, "{field["name"]}");', file=c)
    if "type" in field:
        print(f'{" "*indent}{value} = {render_prefix(field)}_decode({var});', file=c)
    if "struct" in field:
        if "optional" in field and field["optional"]:
            print(f'{" "*indent}if ({var}.has_value) {{', file=c)
            indent += 4
        else:
            print(f'{" "*indent}assert({var}.has_value);', file=c)
        p(indent, c, f"{value}.has_value = true;")
        for struct_field in field["struct"]:
            decode_field(struct_field, var, f'{value}.{struct_field["name"]}', c, indent)
        if "optional" in field and field["optional"]:
            indent -= 4
            print(f'{" "*indent}}}', file=c)
    if "variant" in field:
        decode_variant(field, var, f'{value}', c, indent)
    indent -= 4
    p(indent, c, '}')


def encode_field(field, value, c, indent=4):
    enclosing = "v" + str(indent)
    if "type" in field:
        p(indent, c, f'json_optional_set(&{enclosing}, "{field["name"]}", {render_prefix(field)}_encode({value}));')
    if "struct" in field:
        if "optional" in field and field["optional"]:
            p(indent, c, f'if ({value}.has_value) {{')
        else:
            p(indent, c, '{')
        indent += 4
        var = "v"+str(indent)
        p(indent, c, f'JSONValue {var} = json_object();')
        for struct_field in field["struct"]:
            encode_field(struct_field, f'{value}.{struct_field["name"]}', c, indent)
        p(indent, c, f'json_set(&{enclosing}, "{field["name"]}", {var});')
        indent -= 4
        p(indent, c, '}')
    if "variant" in field:
        encode_variant(field, enclosing, value, c, indent)


def encode_variant_variant(variant, var, value, tag, c, indent=4):
    p(indent, c, f'case {tag}:')
    indent += 4
    if "type" in variant:
        if variant["type"] == "bool":
            p(indent, c, f'{var} = json_bool({value}._{tag});')
        elif variant["type"] == "int":
            p(indent, c, f'{var} = json_int({value}._{tag});')
        elif variant["type"] == "string":
            p(indent, c, f'{var} = json_string({value}._{tag});')
        elif variant["type"] == "null":
            p(indent, c, f'{var} = json_null();')
        elif variant["type"] == "empty":
            p(indent, c, f'{var} = json_object();')
        else:
            p(indent, c, f'{var} = {render_prefix(variant)}_encode({value}._{tag}).value;')
    if "struct" in variant:
        p(indent, c, f'{var} = json_object();')
        for struct_field in variant["struct"]:
            p(indent, c, f'json_optional_set(&{var}, "{struct_field["name"]}", {render_prefix(struct_field)}_encode({value}._{tag}.{struct_field["name"]}));')
    p(indent, c, "break;")
    indent -= 4


def encode_variant(variant, enclosing, value, c, indent=4):
    if "optional" in variant and variant["optional"]:
        p(indent, c, f'if ({value}.has_value) {{')
    else:
        p(indent, c, f'assert({value}.has_value);')
        p(indent, c, '{')
    indent += 4
    var = "v"+str(indent)
    p(indent, c, f'JSONValue {var} = {{0}};')
    tag = 0
    p(indent, c, f'switch ({value}.tag) {{')
    for v in variant["variant"]:
        encode_variant_variant(v, var, value, tag, c, indent)
        tag += 1
    p(indent, c, 'default:');
    indent += 4
    p(indent, c, 'UNREACHABLE();');
    indent -= 4
    p(indent, c, '}')
    p(indent, c, f'json_set(&{enclosing}, "{variant["name"]}", {var});')
    indent -= 4
    p(indent, c, "}")


def emit_struct_c(name, c):
    struct = structures[name]
    decode = "decode" not in struct or struct["decode"]
    encode = "encode" not in struct or struct["encode"]

    if "optional" in struct and struct['optional']:
        if encode:
            print(f"OPTIONAL_JSON_ENCODE_IMPL({name});", file=c)
        if decode:
            print(f"OPTIONAL_JSON_DECODE_IMPL({name});", file=c)
        print(file=c)
    if "array" in struct and struct['array']:
        plural = struct['plural'] if "plural" in struct else name + "s"
        print(f"DA_IMPL({name});", file=c)
        if encode:
            print(f"DA_JSON_ENCODE_IMPL({name}, {plural}, elements);", file=c)
        if decode:
            print(f"DA_JSON_DECODE_IMPL({name}, {plural}, elements);", file=c)
        if "optional-array" in struct and struct['optional-array']:
            if encode:
                print(f"OPTIONAL_JSON_ENCODE_IMPL({plural});", file=c)
            if decode:
                print(f"OPTIONAL_JSON_DECODE_IMPL({plural});", file=c)
        print(file=c)
    if decode:
        print(f"""{name} {name}_decode(OptionalJSONValue v4)
{{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    {name} value = {{0}};""", file=c)
        for field in struct["all_fields"]:
            decode_field(field, "v4", f'value.{field["name"]}', c)
        print(f"""    return value;
}}
""", file=c)
    if encode:
        print(f"""OptionalJSONValue {name}_encode({name} value)
{{
    JSONValue v4 = json_object();""", file=c)
        for field in struct["all_fields"]:
            encode_field(field, f'value.{field["name"]}', c)
        print(f"""    RETURN_VALUE(JSONValue, v4);
}}
""", file=c)


def emit_struct(name, h, c):
    if name in emitted_structures:
        return
    struct = structures[name]
    if "extends" in struct:
        for e in struct["extends"]:
            emit_struct(e, h, c)
    collect_fields(struct)

    for field in struct["fields"]:
        if "type" in field:
            if field["type"] in structures:
                emit_struct(field["type"], h, c)
            if field["type"] in variants:
                emit_variant(field["type"], h, c)
            if field["type"] in enums:
                emit_enum(field["type"], h, c)
    emit_struct_h(name, h)
    emit_struct_c(name, c)
    emitted_structures.add(name)


def emit_structs(h, c):
    for s in structures:
        emit_struct(s, h, c)


def emit_variant_h(name, h):
    variant = variants[name]
    p(0, h, 'typedef struct {')
    indent = 4
    p(indent, h, 'int tag;')
    p(indent, h, 'union {')
    indent += 4
    tag = 0
    for elem in variant["variant"]:
        define_field(f'_{tag}', elem, h, indent)
        tag += 1
    indent -= 4
    p(indent, h, '};')
    indent -= 4
    p(indent, h, f'}} {name};')
    print(file=h)

    if "optional" in variant and variant['optional']:
        print(f"OPTIONAL({name});", file=h)
        print(f"OPTIONAL_JSON({name});", file=h)
        print(file=h)
    if "array" in variant and variant['array']:
        plural = variant['plural'] if "plural" in variant else name + "s"
        print(f"DA_WITH_NAME({name}, {plural});", file=h)
        if "optional-array" in variant and variant['optional-array']:
            print(f"OPTIONAL({plural});", file=h)
            print(f"OPTIONAL_JSON({plural});", file=h)
        print(file=h)
    if "decode" not in variant or variant["decode"]:
        print(f"extern {name} {name}_decode(OptionalJSONValue value);", file=h)
    if "encode" not in variant or variant["decode"]:
        print(f"extern OptionalJSONValue {name}_encode({name} value);", file=h)
    print(file=h)


def emit_variant_c(name, c):
    variant = variants[name]
    decode = "decode" not in variant or variant["decode"]
    encode = "encode" not in variant or variant["encode"]

    if "optional" in variant and variant['optional']:
        if encode:
            print(f"OPTIONAL_JSON_ENCODE_IMPL({name});", file=c)
        if decode:
            print(f"OPTIONAL_JSON_DECODE_IMPL({name});", file=c)
        print(file=c)
    if "array" in variant and variant['array']:
        plural = variant['plural'] if "plural" in variant else name + "s"
        print(f"DA_IMPL({name});", file=c)
        if encode:
            print(f"DA_JSON_ENCODE_IMPL({name}, {plural}, elements);", file=c)
        if decode:
            print(f"DA_JSON_DECODE_IMPL({name}, {plural}, elements);", file=c)
        if "optional-array" in variant and variant['optional-array']:
            if encode:
                print(f"OPTIONAL_JSON_ENCODE_IMPL({plural});", file=c)
            if decode:
                print(f"OPTIONAL_JSON_DECODE_IMPL({plural});", file=c)
        print(file=c)
    if decode:
        p(0, c, f'{name} {name}_decode(OptionalJSONValue v4)')
        p(0, c, '{')
        indent = 4
        p(indent, c, 'assert(v4.has_value);')
        p(indent, c, f'{name} value = {{0}};')
        decode_variant(variant, "v4", "value", c)
        print(f"""    return value;
}}
""", file=c)
    if encode:
        p(0, c, f'OptionalJSONValue {name}_encode({name} value)')
        p(0, c, '{')
        indent = 4
        p(indent, c, f'JSONValue v4 = json_object();')
        encode_variant(variant, "v4", "value", c)
        p(indent, c, f'RETURN_VALUE(JSONValue, v4);')
        p(0, c, '}')
        print(file=c)


def emit_variant(name, h, c):
    if name in emitted_structures:
        return
    variant = variants[name]

    for elem in variant["variant"]:
        if "type" in elem:
            t = elem["type"]
            if t in structures:
                emit_struct(t, h, c)
            if t in variants:
                emit_variant(t, h, c)
            if t in enums:
                emit_enum(t, h, c)
    emit_variant_h(name, h)
    emit_variant_c(name, c)
    emitted_structures.add(name)


def emit_variants(h, c):
    for v in variants:
        emit_variant(v, h, c)


def header(f):
    print("""/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 
""", file=f)


def c_header(f):
    header(f)
    print(f"#include <lsp/{module}.h>", file=f)
    print(file=f)

def h_header(f):
    header(f)
    print(f"#ifndef __LSP_{module.upper()}_H__", file=f)
    print(f"#define __LSP_{module.upper()}_H__", file=f)
    print(file=f)
    print(f"#include <lsp/lsp_base.h>", file=f)
    for dep in depends:
        p(0, h, f'#include <lsp/{dep}.h>')
    print(file=h)


def c_footer(f):
    print("// clang-format on", file=f)
    print(file=f)

def h_footer(f):
    print(file=f)
    print("// clang-format on", file=f)
    print(f"#endif /* __LSP_{module.upper()}_H__ */", file=f)
    print(file=f)


# emit_forwards()

module = sys.argv[1]
with open(module + ".json") as fd:
    model = json.load(fd)

if "depends" in model:
    depends = model["depends"];
for enum in model['enums']:
    enums[enum['name']] = enum
for structure in model["structs"]:
    structures[structure["name"]] = structure
for variant in model["variants"]:
    variants[variant["name"]] = variant

with open(module+'.h', mode="w") as h:
    with open(module+'.c', mode="w") as c:
        h_header(h)
        c_header(c)
        emit_enums(h, c)
        emit_variants(h, c)
        emit_structs(h, c)
        h_footer(h)
        c_footer(c)
