#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (c) 2010 - 2022, Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V.
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# We kindly request you to use one or more of the following phrases to refer to
# foxBMS in your hardware, software, documentation or advertising materials:
#
# - "This product uses parts of foxBMS®"
# - "This product includes parts of foxBMS®"
# - "This product is derived from foxBMS®"

"""Implements a waf tool to configure a VS Code workspace to foxBMS specific
needs.

For information on VS Code see https://code.visualstudio.com/.
"""

import os
from pathlib import Path
import re

import jinja2
from waflib import Context, Utils

# This tool uses slash as path separator for the sake of simplicity as it
# - works on both, Windows and unix-like systems (see
#   https://docs.microsoft.com/en-us/archive/blogs/larryosterman/why-is-the-dos-path-character)
# - it make the json configuration file more readable


def fix_jinja(txt):
    """appends empty line to file, if missing"""
    return (
        os.linesep.join([s for s in txt.splitlines() if not s.strip() == ""])
        + os.linesep
    )


def configure(conf):  # pylint: disable=too-many-statements,too-many-branches
    """configuration step of the VS Code waf tool:

    - Find code
    - configure a project if code was found"""
    # create a VS Code workspace if code is installed on this platform
    is_remote_session = False
    if Utils.is_win32:
        conf.find_program("code", mandatory=False)
        if not conf.env.CODE:
            code_dir = "Microsoft VS Code"
            path_list = [
                os.path.join(os.environ["LOCALAPPDATA"], "Programs", code_dir),
                os.path.join(os.environ["PROGRAMFILES"], code_dir),
            ]
            conf.find_program(
                "code",
                path_list=path_list,
                mandatory=False,
            )
    else:
        conf.find_program("code", mandatory=False)
        if not conf.env.CODE:
            # we might be in a remote environment, scan for this
            code_server_dir = os.path.join(os.path.expanduser("~"), ".vscode-server")
            is_remote_session = os.path.isdir(code_server_dir)
            conf.msg("Found 'vscode-server' (remote session)", code_server_dir)

    if not (conf.env.CODE or is_remote_session):
        return
    conf.start_msg("Creating workspace")
    vscode_dir = conf.path.make_node(".vscode")
    vscode_dir.mkdir()
    vscode_config_dir = conf.path.find_dir(os.path.join("tools", "ide", "vscode"))
    template_loader = jinja2.FileSystemLoader(searchpath=vscode_config_dir.relpath())
    template_env = jinja2.Environment(loader=template_loader)

    if Utils.is_win32:
        waf_wrapper_script = Path(conf.path.abspath()).as_posix() + "/waf.bat"
    else:
        waf_wrapper_script = Path(conf.path.abspath()) / "waf.sh"
    axivion_base_path = Path(
        os.path.join(conf.path.abspath(), "tests", "axivion")
    ).as_posix()
    axivion_start_analysis = None
    axivion_start_dashboard = None
    axivion_config_exe = None
    if Utils.is_win32:
        axivion_start_analysis = axivion_base_path + "/scripts/start_local_analysis.bat"
        axivion_start_dashboard = (
            axivion_base_path + "/scripts/start_local_dashserver.bat"
        )
        if conf.env.AXIVION_CONFIG:
            axivion_config_exe = Path(conf.env.AXIVION_CONFIG[0]).as_posix()

    template = template_env.get_template("tasks.json.jinja2")
    tasks = template.render(
        WAF_WRAPPER_SCRIPT=waf_wrapper_script,
        AXIVION_CONFIG_EXE=axivion_config_exe,
        AXIVION_START_ANALYSIS=axivion_start_analysis,
        AXIVION_START_DASHBOARD=axivion_start_dashboard,
        JFLASH=conf.env.JFLASH,
    )
    vsc_tasks_file = os.path.join(vscode_dir.relpath(), "tasks.json")
    conf.path.make_node(vsc_tasks_file).write(fix_jinja(tasks))

    template = template_env.get_template("extensions.json.jinja2")
    extensions = template.render()
    vsc_extensions_file = os.path.join(vscode_dir.relpath(), "extensions.json")
    conf.path.make_node(vsc_extensions_file).write(fix_jinja(extensions))

    template = template_env.get_template("cspell.json.jinja2")
    cspell = template.render()
    vsc_cspell_file = os.path.join(vscode_dir.relpath(), "cspell.json")
    conf.path.make_node(vsc_cspell_file).write(fix_jinja(cspell))

    template = template_env.get_template("settings.json.jinja2")
    # Python and friends: Python, conda, pylint, black
    py_exe = "python"
    if conf.env.PYTHON:
        py_exe = Path(conf.env.PYTHON[0]).as_posix()
    pylint_exe = "pylint"
    if conf.env.PYLINT:
        pylint_exe = Path(conf.env.PYLINT[0]).as_posix()
    pylint_cfg = ""
    if conf.env.PYLINT_CONFIG:
        pylint_cfg = Path(conf.env.PYLINT_CONFIG[0]).as_posix()
    black_exe = "black"
    if conf.env.BLACK:
        black_exe = Path(conf.env.BLACK[0]).as_posix()
    black_cfg = ""
    if conf.env.BLACK_CONFIG:
        black_cfg = Path(conf.env.BLACK_CONFIG[0]).as_posix()
    # directory of waf and waf-tools
    waf_dir = Path(Context.waf_dir).as_posix()
    waf_tools_dir = Path(
        os.path.join(conf.path.abspath(), "tools", "waf-tools")
    ).as_posix()
    # Clang-format
    clang_format_executable = ""
    if conf.env.CLANG_FORMAT:
        clang_format_executable = Path(conf.env.CLANG_FORMAT[0]).as_posix()

    ax_modules_rel = Path(os.path.join("lib", "scripts"))

    if Utils.is_win32:
        axivion_modules = (
            Path(os.environ.get("ProgramFiles(x86)", "C:\\Program Files(x86)"))
            / "Bauhaus"
            / ax_modules_rel
        ).as_posix()
        userprofile = os.environ.get(
            "USERPROFILE", os.environ.get("HOMEDRIVE", "C:") + os.sep
        )
    else:
        axivion_modules = (
            Path(os.environ.get("HOME", "/")) / "bauhaus-suite" / ax_modules_rel
        )
        userprofile = os.environ.get("HOME", "~")

    axivion_vs_config = {
        "analysisProject": "foxbms-2",
        "localPath": Path(conf.path.abspath()).as_posix(),
        "analysisPath": Path(userprofile).as_posix(),
    }
    if conf.env.AXIVION_CC:
        projects_path = os.path.join(userprofile, ".bauhaus", "localbuild", "projects")
        axivion_vs_config["analysisPath"] = Path(projects_path).as_posix()

        try:
            axivion_modules = (
                Path(conf.env.AXIVION_CC[0]).parent.parent / ax_modules_rel
            ).as_posix()
        except IndexError:
            pass
    settings = template.render(
        PYTHONPATH=py_exe,
        PYTHON_ANALYSIS_EXTRA_PATHS=[waf_dir, waf_tools_dir, axivion_modules],
        PYLINT_PATH=pylint_exe,
        PYLINT_CONFIG=pylint_cfg,
        BLACK_PATH=black_exe,
        BLACK_CONFIG=black_cfg,
        CLANG_FORMAT_EXECUTABLE=clang_format_executable,
        AXIVION_VS_CONFIG=axivion_vs_config,
    )

    vsc_settings_file = os.path.join(vscode_dir.relpath(), "settings.json")
    conf.path.make_node(vsc_settings_file).write(fix_jinja(settings))

    template = template_env.get_template("c_cpp_properties.json.jinja2")
    defines_read = (
        conf.root.find_node(conf.env.COMPILER_BUILTIN_DEFINES_FILE[0])
        .read()
        .splitlines()
    )
    vscode_defines = [i.split("=") for i in conf.env.DEFINES]
    reg = re.compile(r"(#define)([ ])([a-zA-Z0-9_]{1,})([ ])([a-zA-Z0-9_\":. ]{1,})")
    for d in defines_read:
        define = d.split("/*")[0]
        _def = reg.search(define)
        if _def:
            def_name, val = _def.group(3), _def.group(5)
            if def_name in ("__DATE__", "__TIME__"):
                continue
            if '"' in val:
                val = val.replace('"', '\\"')
            vscode_defines.append((def_name, val))
    rtos_includes = [
        Path(str(conf.root.find_node(i).path_from(conf.path))).as_posix()
        for i in conf.env.INCLUDES_RTOS
    ]
    c_cpp_properties = template.render(
        ARMCL=Path(conf.env.CC[0]).as_posix(),
        RTOS=conf.env.RTOS_NAME[0],
        RTOS_INCLUDES=rtos_includes,
        BALANCING_STRATEGY=conf.env.balancing_strategy,
        AFE_MANUFACTURER=conf.env.afe_manufacturer,
        AFE_IC=conf.env.afe_ic,
        TEMPERATURE_SENSOR_MANUFACTURER=conf.env.temperature_sensor_manuf,
        TEMPERATURE_SENSOR_MODEL=conf.env.temperature_sensor_model,
        TEMPERATURE_SENSOR_METHOD=conf.env.temperature_sensor_meth,
        STATE_ESTIMATOR_SOC=conf.env.state_estimator_soc,
        STATE_ESTIMATOR_SOE=conf.env.state_estimator_soe,
        STATE_ESTIMATOR_SOF=conf.env.state_estimator_sof,
        STATE_ESTIMATOR_SOH=conf.env.state_estimator_soh,
        IMD_MANUFACTURER=conf.env.imd_manufacturer,
        IMD_MODEL=conf.env.imd_model,
        INCLUDES=[Path(x).as_posix() for x in conf.env.INCLUDES],
        C_STANDARD="c11",
        DEFINES=vscode_defines,
    )
    vsc_c_cpp_properties_file = os.path.join(
        vscode_dir.relpath(), "c_cpp_properties.json"
    )
    for i in conf.env.VSCODE_MK_DIRS:
        conf.path.make_node(i).mkdir()
    conf.path.make_node(vsc_c_cpp_properties_file).write(fix_jinja(c_cpp_properties))

    template = template_env.get_template("launch.json.jinja2")
    gdb_exe = "gdb"
    if conf.env.GDB:
        gdb_exe = Path(conf.env.GDB[0]).as_posix()
    launch = template.render(GDB=gdb_exe)

    vsc_launch_file = os.path.join(vscode_dir.relpath(), "launch.json")
    conf.path.make_node(vsc_launch_file).write(fix_jinja(launch))

    conf.end_msg("ok")
