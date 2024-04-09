#!/usr/bin/env python3
#
# Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
#
# SPDX-License-Identifier: MIT
#

from jinja2 import Environment, FileSystemLoader, select_autoescape
import json
import os
import subprocess
import sys

eddy_build_dir = os.path.join("..", "..", "..", "build")
eddy_bin_dir = os.path.join(eddy_build_dir, "bin")
ts_path = os.path.join(eddy_bin_dir, "ts")

templates = {}

def parse_typescript(name):
    os.path.exists("stdout") and os.remove("stdout")
    os.path.exists("stderr") and os.remove("stderr")
    with open("stdout", "w+") as out, open("stderr", "w+") as err:
        ex = subprocess.call([ts_path, name + ".ts"], stdout=out, stderr=err)
        if ex != 0:
            print(f"Parse of '{name}' failed: {ex}")
            subprocess.call(["cat", "stdout"])
            subprocess.call(["cat", "stderr"])
            return False
    os.path.exists("stdout") and os.remove("stdout")
    os.path.exists("stderr") and os.remove("stderr")
    return True


def generate(ctx, ext):
    if ctx["kind"] not in ctx:
        print(f"Huh?? '{ctx['name']}' has no '{ctx['kind']}' property...")
        return False
    if ctx["kind"] == "enumeration":
        if "values" not in ctx["enumeration"]:
            print(f"Huh?? Enum '{ctx['name']}' has no values...")
            return False

    file = ctx["name"] + ext
    tpl = ctx["kind"] + ext;
    if tpl not in templates:
        template = env.get_template(tpl + ".in")
        templates[tpl] = template
    else:
        template = templates[tpl]
    with open(file, "w") as f:
        print(template.render(ctx), file=f)
    subprocess.call(["clang-format", "-i", file])
    return True


def generate_code(name):
    print("Generating", name)
    with open(name + ".json") as f:
        ctx = json.load(f)
    generate(ctx, ".h")
    generate(ctx, ".c")


def process_module(name):
    if not parse_typescript("base"):
        return        
    with open(name + ".json") as f:
        mod = json.load(f)
    for type in mod:
        generate_code(type["name"])
    template_cmake = env.get_template("CMakeLists.txt.in")
    with open("CMakeLists.txt", "w") as f:
        print(template_cmake.render({"types": mod}), file=f)


env = Environment(
        loader=FileSystemLoader("."),
        autoescape=select_autoescape())
process_module("base")
