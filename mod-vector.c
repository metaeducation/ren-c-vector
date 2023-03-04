//
//  File: %mod-vector.c
//  Summary: "VECTOR! extension main C file"
//  Section: Extension
//  Project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  Homepage: https://github.com/metaeducation/ren-c/
//
//=////////////////////////////////////////////////////////////////////////=//
//
// Copyright 2012 Atronix Engineering
// Copyright 2012-2019 Ren-C Open Source Contributors
// REBOL is a trademark of REBOL Technologies
//
// See README.md and CREDITS.md for more information.
//
// Licensed under the Lesser GPL, Version 3.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.gnu.org/licenses/lgpl-3.0.html
//
//=////////////////////////////////////////////////////////////////////////=//
//
// See notes in %extensions/vector/README.md

#include "sys-core.h"

#include "tmp-mod-vector.h"

#include "sys-vector.h"


REBTYP *EG_Vector_Type;  // (E)xtension (G)lobal

Symbol(const*) S_Vector(void) {
    return Canon(VECTOR_X);
}

//
//  startup*: native [
//
//  {Make the VECTOR! datatype work with GENERIC actions, comparison ops, etc}
//
//      return: <none>
//  ]
//
DECLARE_NATIVE(startup_p)
{
    VECTOR_INCLUDE_PARAMS_OF_STARTUP_P;

    // !!! See notes on Hook_Datatype for this poor-man's substitute for a
    // coherent design of an extensible object system (as per Lisp's CLOS)
    //
   EG_Vector_Type = Hook_Datatype(
        "http://datatypes.rebol.info/vector",
        "compact scalar array",
        &S_Vector,
        &T_Vector,
        &CT_Vector,
        &MAKE_Vector,
        &TO_Vector,
        &MF_Vector
    );

    return NONE;
}


//
//  shutdown*: native [
//
//  {Remove behaviors for VECTOR! added by REGISTER-VECTOR-HOOKS}
//
//      return: <none>
//  ]
//
DECLARE_NATIVE(shutdown_p)
{
    VECTOR_INCLUDE_PARAMS_OF_SHUTDOWN_P;

    Unhook_Datatype(EG_Vector_Type);

    return NONE;
}
