//
//  file: %sys-vector.c
//  summary: "Vector Datatype header file"
//  section: datatypes
//  project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  homepage: https://github.com/metaeducation/ren-c/
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
// The cell for a VECTOR! points to a "Pairing"--which is two value cells
// stored in an optimized format that fits inside one Stub-Sized slot.  This is
// a relatively light allocation, which allows the vector's properties
// (bit width, signedness, integral-ness) to be stored in addition to a
// BLOB! of the vector's bytes.
//
// The reason that the Stub.link and Stub.misc fields etc. aren't used on a
// FLAVOR_BINARY Stub to store the extra information is because you're able
// to alias BLOB! data as VECTOR!.  That arbitrary data may already use the
// Stub.link and Stub.misc for other things.
//
//=//// NOTES /////////////////////////////////////////////////////////////=//
//
// * See %extensions/vector/README.md
//

typedef Pairing Vector;

INLINE Vector* VAL_VECTOR(const Cell* v) {
    assert(Is_Vector(v));
    return cast(Pairing*, CELL_PAYLOAD_1(v));
}

#define VAL_VECTOR_BLOB(v) \
    Pairing_First(VAL_VECTOR(v))

#define VAL_VECTOR_SIGN_INTEGRAL_WIDE(v) \
    Pairing_Second(VAL_VECTOR(v))

#define VAL_VECTOR_SIGN(v) \
    VAL_VECTOR_SIGN_INTEGRAL_WIDE(v)->payload.split.one.bit

INLINE bool VAL_VECTOR_INTEGRAL(const Cell* v) {
    Element* siw = VAL_VECTOR_SIGN_INTEGRAL_WIDE(v);
    if (siw->payload.split.two.bit != 0)
        return true;

    assert(VAL_VECTOR_SIGN(v));
    return false;
}

INLINE Byte VAL_VECTOR_WIDE(const Cell* v) {  // "wide" Flex term
    int32_t wide = VAL_VECTOR_SIGN_INTEGRAL_WIDE(v)->extra.i32;
    assert(wide == 1 or wide == 2 or wide == 3 or wide == 4);
    return wide;
}

#define VAL_VECTOR_BITSIZE(v) \
    (VAL_VECTOR_WIDE(v) * 8)

inline static Byte* VAL_VECTOR_HEAD(const Cell* v) {
    Element* blob = VAL_VECTOR_BLOB(v);
    return Binary_Head(Cell_Binary_Ensure_Mutable(blob));
}

inline static REBLEN VAL_VECTOR_LEN_AT(const Cell* v) {
    return Series_Len_Head(VAL_VECTOR_BLOB(v)) / VAL_VECTOR_WIDE(v);
}

#define VAL_VECTOR_INDEX(v) 0  // !!! Index not currently supported
#define VAL_VECTOR_LEN_HEAD(v) VAL_VECTOR_LEN_AT(v)

inline static Element* Init_Vector(
    Sink(Element) out,
    Binary* bin,
    bool sign,
    bool integral,
    Byte bitsize
){
    Pairing* paired = Alloc_Pairing(BASE_FLAG_MANAGED);

    assert(Binary_Len(bin) % (bitsize / 8) == 0);
    Init_Blob(Pairing_First(paired), bin);

    Element* siw = Pairing_Second(paired);
    Reset_Cell_Header_Noquote(
        siw,
        FLAG_HEART(TYPE_HANDLE)
            | CELL_FLAG_DONT_MARK_PAYLOAD_1  // data just a flag, no GC marking
            | CELL_FLAG_DONT_MARK_PAYLOAD_2  // also a flag, no GC marking
    );
    siw->payload.split.one.bit = sign;
    siw->payload.split.two.bit = integral;
    assert(bitsize == 8 or bitsize == 16 or bitsize == 32 or bitsize == 64);
    siw->extra.i32 = bitsize / 8;  // e.g. VAL_VECTOR_WIDE()

    Reset_Extended_Cell_Header_Noquote(
        out,
        EXTRA_HEART_VECTOR,
        (not CELL_FLAG_DONT_MARK_PAYLOAD_1)  // vector pairing needs mark
            | CELL_FLAG_DONT_MARK_PAYLOAD_2  // index shouldn't be marked
    );
    CELL_PAYLOAD_1(out) = paired;

    return out;
}
