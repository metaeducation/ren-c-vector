//
//  File: %sys-vector.c
//  Summary: "Vector Datatype header file"
//  Section: datatypes
//  Project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  Homepage: https://github.com/metaeducation/ren-c/
//
//=////////////////////////////////////////////////////////////////////////=//
//
// Copyright 2012 REBOL Technologies
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
// The cell for a REB_VECTOR points to a "pairing"--which is two value cells
// stored in an optimized format that fits inside one REBSER node.  This is
// a relatively light allocation, which allows the vector's properties
// (bit width, signedness, integral-ness) to be stored in addition to a
// BINARY! of the vector's bytes.
//
//=//// NOTES /////////////////////////////////////////////////////////////=//
//
// * See %extensions/vector/README.md
//

extern REBTYP *EG_Vector_Type;

#define VAL_VECTOR_BINARY(v) \
    VAL(VAL_NODE1(v))  // pairing[0]

#define VAL_VECTOR_SIGN_INTEGRAL_WIDE(v) \
    PAIRING_KEY(VAL(VAL_NODE1(v)))  // pairing[1]

#define VAL_VECTOR_SIGN(v) \
    PAYLOAD(Any, VAL_VECTOR_SIGN_INTEGRAL_WIDE(v)).first.flag

inline static bool VAL_VECTOR_INTEGRAL(noquote(Cell(const*)) v) {
    assert(CELL_CUSTOM_TYPE(v) == EG_Vector_Type);
    REBVAL *siw = VAL_VECTOR_SIGN_INTEGRAL_WIDE(v);
    if (PAYLOAD(Any, siw).second.flag != 0)
        return true;

    assert(VAL_VECTOR_SIGN(v));
    return false;
}

inline static Byte VAL_VECTOR_WIDE(noquote(Cell(const*)) v) {  // "wide" REBSER term
    int32_t wide = EXTRA(Any, VAL_VECTOR_SIGN_INTEGRAL_WIDE(v)).i32;
    assert(wide == 1 or wide == 2 or wide == 3 or wide == 4);
    return wide;
}

#define VAL_VECTOR_BITSIZE(v) \
    (VAL_VECTOR_WIDE(v) * 8)

inline static Byte* VAL_VECTOR_HEAD(noquote(Cell(const*)) v) {
    assert(CELL_CUSTOM_TYPE(v) == EG_Vector_Type);
    REBVAL *binary = VAL(VAL_NODE1(v));
    return BIN_HEAD(VAL_BINARY_ENSURE_MUTABLE(binary));
}

inline static REBLEN VAL_VECTOR_LEN_AT(noquote(Cell(const*)) v) {
    assert(CELL_CUSTOM_TYPE(v) == EG_Vector_Type);
    return VAL_LEN_HEAD(VAL_VECTOR_BINARY(v)) / VAL_VECTOR_WIDE(v);
}

#define VAL_VECTOR_INDEX(v) 0  // !!! Index not currently supported
#define VAL_VECTOR_LEN_HEAD(v) VAL_VECTOR_LEN_AT(v)

inline static REBVAL *Init_Vector(
    Cell(*) out,
    Binary(*) bin,
    bool sign,
    bool integral,
    Byte bitsize
){
    RESET_CUSTOM_CELL(out, EG_Vector_Type, CELL_FLAG_FIRST_IS_NODE);

    REBVAL *paired = Alloc_Pairing();

    Init_Binary(paired, bin);
    assert(BIN_LEN(bin) % (bitsize / 8) == 0);

    REBVAL *siw = PAIRING_KEY(paired);
    Reset_Unquoted_Header_Untracked(TRACK(siw), CELL_MASK_BYTES);
    assert(bitsize == 8 or bitsize == 16 or bitsize == 32 or bitsize == 64);
    PAYLOAD(Any, siw).first.flag = sign;
    PAYLOAD(Any, siw).second.flag = integral;
    EXTRA(Any, siw).i32 = bitsize / 8;  // e.g. VAL_VECTOR_WIDE()

    Manage_Pairing(paired);
    INIT_VAL_NODE1(out, paired);
    return cast(REBVAL*, out);
}


// !!! These hooks allow the REB_VECTOR cell type to dispatch to code in the
// VECTOR! extension if it is loaded.
//
extern REBINT CT_Vector(noquote(Cell(const*)) a, noquote(Cell(const*)) b, bool strict);
extern Bounce MAKE_Vector(Frame(*) frame_, enum Reb_Kind kind, option(const REBVAL*) parent, const REBVAL *arg);
extern Bounce TO_Vector(Frame(*) frame_, enum Reb_Kind kind, const REBVAL *arg);
extern void MF_Vector(REB_MOLD *mo, noquote(Cell(const*)) v, bool form);
extern REBTYPE(Vector);
