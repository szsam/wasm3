//
//  m3_module.c
//
//  Created by Steven Massey on 5/7/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_env.h"
#include "m3_exception.h"



IM3Module  m3_NewModule  (IM3Environment i_environment)
{
    IM3Module module = m3_AllocStruct (M3Module);

    if (module)
    {
        module->name = ".unnamed";
        module->startFunction = -1;
        module->environment = i_environment;

        module->hasWasmCodeCopy = false;
        module->wasmStart = NULL;
        module->wasmEnd = NULL;

#       if d_m3EnableExtensions
        module->numReservedFunctions = 0;
#       endif
    }

    return module;
}


void Module_FreeFunctions (IM3Module i_module)
{
    for (u32 i = 0; i < i_module->numFunctions; ++i)
    {
        IM3Function func = & i_module->functions [i];
        Function_Release (func);
    }
}


void  m3_FreeModule  (IM3Module i_module)
{
    if (i_module)
    {
        m3log (module, "freeing module: %s (funcs: %d; segments: %d)",
               i_module->name, i_module->numFunctions, i_module->numDataSegments);

        Module_FreeFunctions (i_module);

        m3_Free (i_module->functions);
        //m3_Free (i_module->imports);
        m3_Free (i_module->funcTypes);
        m3_Free (i_module->dataSegments);
        m3_Free (i_module->table0);

        for (u32 i = 0; i < i_module->numGlobals; ++i)
        {
            m3_Free (i_module->globals[i].name);
            FreeImportInfo(&(i_module->globals[i].import));
        }
        m3_Free (i_module->globals);

        if (i_module->hasWasmCodeCopy)
        {
            m3_Free (i_module->wasmStart);
        }

        m3_Free (i_module);
    }
}


M3Result  Module_AddGlobal  (IM3Module io_module, IM3Global * o_global, u8 i_type, bool i_mutable, bool i_isImported)
{
    M3Result result = m3Err_none;
_try {
    u32 index = io_module->numGlobals++;
    io_module->globals = m3_ReallocArray (M3Global, io_module->globals, io_module->numGlobals, index);
    _throwifnull(io_module->globals);
    M3Global * global = & io_module->globals [index];

    global->type = i_type;
    global->imported = i_isImported;
    global->isMutable = i_mutable;

    if (o_global)
        * o_global = global;

} _catch:
    return result;
}


M3Result  Module_AddFunction  (IM3Module io_module, u32 i_typeIndex, IM3ImportInfo i_importInfo)
{
    M3Result result = m3Err_none;

_try {

    u32 index = io_module->numFunctions++;

#   if d_m3EnableExtensions
    if (io_module->runtime) // module loaded, so this must be an InjectFunction call
    {
        if (io_module->numReservedFunctions)
        {
            io_module->numReservedFunctions--;
        }
        else _throw ("reserved function capacity exceeded");
    }
    else
#   endif
    io_module->functions = m3_ReallocArray (M3Function, io_module->functions, io_module->numFunctions, index);

    _throwifnull (io_module->functions);
    _throwif ("type sig index out of bounds", i_typeIndex >= io_module->numFuncTypes);

    IM3FuncType ft = io_module->funcTypes [i_typeIndex];

    IM3Function func = Module_GetFunction (io_module, index);
    func->funcType = ft;

# if defined (DEBUG) || d_m3EnableExtensions
    func->index = index;
# endif

    if (i_importInfo and func->numNames == 0)
    {
        func->import = * i_importInfo;
        func->names[0] = i_importInfo->fieldUtf8;
        func->numNames = 1;
    }

    m3log (module, "   added function: %3d; sig: %d", index, i_typeIndex);

} _catch:
    return result;
}


void  Module_GenerateNames  (IM3Module i_module)
{
    for (u32 i = 0; i < i_module->numFunctions; ++i)
    {
        IM3Function func = & i_module->functions [i];

        if (func->numNames == 0)
        {
            char* buff = m3_AllocArray(char, 16);
            snprintf(buff, 16, "$func%d", i);
            func->names[0] = buff;
            func->numNames = 1;
        }
    }
    for (u32 i = 0; i < i_module->numGlobals; ++i)
    {
        IM3Global global = & i_module->globals [i];

        if (global->name == NULL)
        {
            char* buff = m3_AllocArray(char, 16);
            snprintf(buff, 16, "$global%d", i);
            global->name = buff;
        }
    }
}

IM3Function  Module_GetFunction  (IM3Module i_module, u32 i_functionIndex)
{
    IM3Function func = NULL;

    if (i_functionIndex < i_module->numFunctions)
    {
        func = & i_module->functions [i_functionIndex];
        //func->module = i_module;
    }

    return func;
}


const char*  m3_GetModuleName  (IM3Module i_module)
{
    if (!i_module || !i_module->name)
        return ".unnamed";

    return i_module->name;
}

void  m3_SetModuleName  (IM3Module i_module, const char* name)
{
    if (i_module) i_module->name = name;
}

IM3Runtime  m3_GetModuleRuntime  (IM3Module i_module)
{
    return i_module ? i_module->runtime : NULL;
}

