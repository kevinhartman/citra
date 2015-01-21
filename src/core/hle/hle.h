// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>

#include "common/common_types.h"
#include "core/core.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace HLE {

typedef u32 Addr;
typedef void (*Func)();

struct FunctionDef {
    u32                 id;
    Func                func;
    std::string         name;
};

struct ModuleDef {
    std::string         name;
    int                 num_funcs;
    const FunctionDef*  func_table;
};

void RegisterModule(std::string name, int num_functions, const FunctionDef *func_table);

void CallSVC(u32 opcode);

void Init();

void Shutdown();

} // namespace
