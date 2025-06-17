//
//  file: %mod-vector.c
//  summary: "VECTOR! datatype"
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
// See %extensions/vector/README.md
//

#include "sys-core.h"
#include "tmp-mod-vector.h"

#include "sys-vector.h"


// Ren-C vectors are built on type of BLOB!.  This means that the memory
// must be read via memcpy() in order to avoid strict aliasing violations.
//
static Element* Get_Vector_At(Sink(Element) out, const Cell* vec, REBLEN n)
{
    Byte* data = VAL_VECTOR_HEAD(vec);

    bool integral = VAL_VECTOR_INTEGRAL(vec);
    bool sign = VAL_VECTOR_SIGN(vec);
    Byte bitsize = VAL_VECTOR_BITSIZE(vec);

    if (not integral) {
        switch (bitsize) {
          case 32: {
            float f;
            memcpy(&f, cast(float*, data) + n, sizeof(f));
            return Init_Decimal(out, f); }

          case 64: {
            double d;
            memcpy(&d, cast(double*, data) + n, sizeof(d));
            return Init_Decimal(out, d); }
        }
    }
    else {
        if (sign) {
            switch (bitsize) {
              case 8: {
                int8_t i;
                memcpy(&i, cast(int8_t*, data) + n, sizeof(i));
                return Init_Integer(out, i); }

              case 16: {
                int16_t i;
                memcpy(&i, cast(int16_t*, data) + n, sizeof(i));
                return Init_Integer(out, i); }

              case 32: {
                int32_t i;
                memcpy(&i, cast(int32_t*, data) + n, sizeof(i));
                return Init_Integer(out, i); }

              case 64: {
                int64_t i;
                memcpy(&i, cast(int64_t*, data) + n, sizeof(i));
                return Init_Integer(out, i); }
            }
        }
        else {
            switch (bitsize) {
              case 8: {
                uint8_t i;
                memcpy(&i, cast(uint8_t*, data) + n, sizeof(i));
                return Init_Integer(out, i); }

              case 16: {
                uint16_t i;
                memcpy(&i, cast(uint16_t*, data) + n, sizeof(i));
                return Init_Integer(out, i); }

              case 32: {
                uint32_t i;
                memcpy(&i, cast(uint32_t*, data) + n, sizeof(i));
                return Init_Integer(out, i); }

              case 64: {
                int64_t i;
                memcpy(&i, cast(int64_t*, data) + n, sizeof(i));
                assert(i >= 0);  // integers don't support

                return Init_Integer(out, i); }
            }
        }
    }

    panic ("Unsupported vector element sign/type/size combination");
}


static Option(Error*) Trap_Set_Vector_At(
    Cell* vec,
    REBLEN n,
    const Element* set
){
    assert(Is_Integer(set) or Is_Decimal(set));  // caller should error

    Byte* data = VAL_VECTOR_HEAD(vec);

    bool integral = VAL_VECTOR_INTEGRAL(vec);
    bool sign = VAL_VECTOR_SIGN(vec);
    Byte bitsize = VAL_VECTOR_BITSIZE(vec);

    if (not integral) {
        REBDEC d64;
        if (Is_Integer(set))
            d64 = cast(double, VAL_INT64(set));
        else {
            assert(Is_Decimal(set));
            d64 = VAL_DECIMAL(set);
        }

        switch (bitsize) {
          case 32: {
            // Can't be "out of range", just loses precision
            REBD32 d = cast(REBD32, d64);
            memcpy(cast(REBD32*, data) + n, &d, sizeof(d));
            return nullptr; }

          case 64: {
            memcpy(cast(REBDEC*, data) + n, &d64, sizeof(d64));
            return nullptr; }
        }
    }
    else {
        int64_t i64;
        if (Is_Integer(set))
            i64 = VAL_INT64(set);
        else {
            assert(Is_Decimal(set));
            i64 = cast(int32_t, VAL_DECIMAL(set));
        }

        if (sign) {
            switch (bitsize) {
              case 8: {
                if (i64 < INT8_MIN or i64 > INT8_MAX)
                    goto out_of_range;
                int8_t i = cast(int8_t, i64);
                memcpy(cast(int8_t*, data) + n, &i, sizeof(i));
                return nullptr; }

              case 16: {
                if (i64 < INT16_MIN or i64 > INT16_MAX)
                    goto out_of_range;
                int16_t i = cast(int16_t, i64);
                memcpy(cast(int16_t*, data) + n, &i, sizeof(i));
                return nullptr; }

              case 32: {
                if (i64 < INT32_MIN or i64 > INT32_MAX)
                    goto out_of_range;
                int32_t i = cast(int32_t, i64);
                memcpy(cast(int32_t*, data) + n, &i, sizeof(i));
                return nullptr; }

              case 64: {
                // type uses full range
                memcpy(cast(int64_t*, data) + n, &i64, sizeof(i64));
                return nullptr; }
            }
        }
        else {  // unsigned
            if (i64 < 0)
                goto out_of_range;

            switch (bitsize) {
              case 8: {
                if (i64 > UINT8_MAX)
                    goto out_of_range;
                uint8_t u = cast(uint8_t, i64);
                memcpy(cast(uint8_t*, data) + n, &u, sizeof(u));
                return nullptr; }

              case 16: {
                if (i64 > UINT16_MAX)
                    goto out_of_range;
                uint16_t u = cast(uint16_t, i64);
                memcpy(cast(uint16_t*, data) + n, &u, sizeof(u));
                return nullptr; }

              case 32: {
                if (i64 > UINT32_MAX)
                    goto out_of_range;
                uint32_t u = cast(uint32_t, i64);
                memcpy(cast(uint32_t*, data) + n, &u, sizeof(u));
                return nullptr; }

              case 64: {
                uint32_t u = cast(uint32_t, i64);
                memcpy(cast(uint64_t*, data) + n, &u, sizeof(u));
                return nullptr; }
            }
        }
    }

  out_of_range:

    return Cell_Error(rebValue("make error! [",
        set, "-{out of range for}- unspaced [", rebI(bitsize), "{-bit}]",
            rebT(sign ? "signed" : "unsigned"), "-{VECTOR! type}-",
    "]"));
}


static Option(Error*) Trap_Set_Vector_Row(
    Cell* vec,
    const Element* block_or_blob
){
    if (Is_Block(block_or_blob)) {
        const Element* tail;
        const Element* at = Cell_List_At(&tail, block_or_blob);

        REBLEN n = 0;
        for (; at != tail; ++at, ++n) {
            Option(Error*) e = Trap_Set_Vector_At(vec, n, at);
            if (e)
                return e;
        }
    }
    else { // !!! This would just interpet the data as int64_t pointers (???)
        assert(Is_Blob(block_or_blob));

        Size size;
        const Byte* data = Cell_Blob_Size_At(&size, block_or_blob);

        DECLARE_ELEMENT (temp);

        REBLEN n = 0;
        for (; size > 0; --size, ++n) {
            Init_Integer(temp, data[n]);

            Option(Error*) e = Trap_Set_Vector_At(vec, n, temp);
            if (e)
                return e;
        }
    }

    return nullptr;
}



// Convert a vector to a block (no calls at present)
//
static Array* Vector_To_Array(const Element* vec)
{
    USED(&Vector_To_Array);

    REBLEN len = Cell_Series_Len_At(vec);
    assert(len >= 0);

    Array* arr = Make_Source(len);
    Element* dest = Array_Head(arr);
    REBLEN n;
    for (n = VAL_INDEX(vec); n < Cell_Series_Len_Head(vec); ++n, ++dest)
        Get_Vector_At(dest, vec, n);

    Set_Flex_Len(arr, len);
    assert(dest == Array_Tail(arr));

    return arr;
}


// !!! Comparison in R3-Alpha was an area that was not well developed.  Ren-C
// has EQUAL? and LESSER? and builds on that (like Ord and Eq in Haskell, or
// sorting only on operator< and operator== in C++)
//
// For now just define EQUAL?
//
IMPLEMENT_GENERIC(EQUAL_Q, Is_Vector)
{
    INCLUDE_PARAMS_OF_EQUAL_Q;

    Element* v1 = Element_ARG(VALUE1);
    Element* v2 = Element_ARG(VALUE2);
    UNUSED(ARG(STRICT));

    bool non_integer1 = not VAL_VECTOR_INTEGRAL(v1);
    bool non_integer2 = not VAL_VECTOR_INTEGRAL(v2);
    if (non_integer1 != non_integer2)
        return RAISE(Error_Not_Same_Type_Raw());  // !!! is thisnecessary?

    REBLEN l1 = VAL_VECTOR_LEN_AT(v1);
    REBLEN l2 = VAL_VECTOR_LEN_AT(v2);
    REBLEN len = MIN(l1, l2);

    DECLARE_ELEMENT (temp1);
    DECLARE_ELEMENT (temp2);

    REBLEN n;
    for (n = 0; n < len; n++) {
        Get_Vector_At(temp1, v1, n + VAL_VECTOR_INDEX(v1));
        Get_Vector_At(temp2, v2, n + VAL_VECTOR_INDEX(v2));
        if (not rebUnboxLogic(CANON(EQUAL_Q), temp1, temp2))
            return LOGIC(false);
    }

    return LOGIC(true);
}


// !!! R3-Alpha code did this shuffle via the bits in the vector, not by
// extracting into values.  This could use Byte* access to get a similar
// effect if it were a priority.  Extract and reinsert Cells for now.
//
IMPLEMENT_GENERIC(SHUFFLE, Is_Vector)
{
    INCLUDE_PARAMS_OF_SHUFFLE;

    Element* vec = Element_ARG(SERIES);
    bool secure = Bool_ARG(SECURE);

    REBLEN idx = VAL_VECTOR_INDEX(vec);

    DECLARE_ELEMENT (temp1);
    DECLARE_ELEMENT (temp2);

    REBLEN n;
    for (n = VAL_VECTOR_LEN_AT(vec); n > 1;) {
        REBLEN k = idx + cast(REBLEN, Random_Int(secure)) % n;
        n--;

        Get_Vector_At(temp1, vec, k);
        Get_Vector_At(temp2, vec, n + idx);

        Option(Error*) e;

        e = Trap_Set_Vector_At(vec, k, temp2);
        assert(not e);
        UNUSED(e);

        e = Trap_Set_Vector_At(vec, n + idx, temp1);
        assert(not e);
        UNUSED(e);
    }

    return COPY(vec);
}


IMPLEMENT_GENERIC(MAKE, Is_Vector)
{
    INCLUDE_PARAMS_OF_MAKE;
    UNUSED(ARG(TYPE));

    Element* spec = Element_ARG(DEF);

    if (Is_Integer(spec) and not Is_Decimal(spec)) {
        REBINT len = Int32s(spec, 0);
        if (len < 0)
            return FAIL(PARAM(DEF));

        Byte bitsize = 32;
        REBLEN num_bytes = (len * bitsize) / 8;
        Binary* bin = Make_Binary(num_bytes);
        memset(Binary_Head(bin), 0, num_bytes);
        Term_Binary_Len(bin, num_bytes);

        const bool sign = true;
        const bool integral = true;
        return Init_Vector(OUT, bin, sign, integral, bitsize);
    }

    if (not Is_Block(spec))
        return FAIL(PARAM(DEF));

  //=//// MAKE VECTOR FROM A BLOCK! SPEC //////////////////////////////////=//

    // Make a vector from a block spec.  Binding isn't technically required
    // if we're only examining the symbols literally.
    //
    //    make vector! [integer! 32 100]
    //    make vector! [decimal! 64 100]
    //    make vector! [unsigned integer! 32]
    //    Fields:
    //         signed:     signed, unsigned
    //         datatypes:  integer, decimal
    //         dimensions: 1 - N
    //         bitsize:    1, 8, 16, 32, 64
    //         size:       integer units
    //         init:        block of values
    //
    // 1. !!! Note: VECTOR! was an ANY-SERIES!.  But as a user-defined type,
    //    it is being separated from being the kind of thing that knows how
    //    series internals are implemented.  It's not clear that user-defined
    //    types like vectors will be positional.  VAL_VECTOR_INDEX() is always
    //    0 for now.

    const Element* tail;
    const Element* item = Cell_List_At(&tail, spec);

    bool sign = true;  // default to signed, not unsigned
    if (
        item != tail
        and Is_Word(item) and Cell_Word_Id(item) == EXT_SYM_UNSIGNED
    ){
        sign = false;
        ++item;
    }

    bool integral = false;  // default to integer, not floating point
    if (item == tail or not Is_Word(item))
        return FAIL(item);

    if (Cell_Word_Id(item) == SYM_INTEGER_X)  // e_X_clamation (INTEGER!)
        integral = true;
    else if (Cell_Word_Id(item) == SYM_DECIMAL_X) {  // (DECIMAL!)
        integral = false;
        if (not sign)
            return FAIL("VECTOR!: C doesn't have unsigned floating points");
    }
    else
        return FAIL("VECTOR!: integer! or decimal! required");

    ++item;

    Byte bitsize;
    if (item == tail or not Is_Integer(item))
        return FAIL("VECTOR!: bit size required, no defaulting");

    REBLEN i = Int32(item);
    if (i == 8 or i == 16) {
        if (not integral)
            return FAIL("VECTOR!: C doesn't have 8 or 16 bit floating points");
    }
    else if (i != 32 and i != 64)
        return FAIL("VECTOR!: C floating points only 32 or 64 bit");

    bitsize = i;
    ++item;

    Byte len = 1;  // !!! default len to 1...why?
    if (item != tail and Is_Integer(item)) {
        if (Int32(item) < 0)
            return FAIL("VECTOR!: length must be positive");
        len = Int32(item);
        ++item;
    }
    else
        len = 1;

    const Element* iblk;
    if (item != tail and (Is_Block(item) or Is_Blob(item))) {
        REBLEN init_len = Cell_Series_Len_At(item);
        if (Is_Blob(item) and integral)  // !!! What was this about?
            return FAIL("VECTOR!: BLOB! can't be integral (?)");
        if (init_len > len)  // !!! Expands without error, is this good?
            len = init_len;
        iblk = item;
        ++item;
    }
    else
        iblk = nullptr;

    REBLEN index = 0;  // index offset inside returned VECTOR! is 0, see [1]
    if (item != tail and Is_Integer(item)) {
        index = (Int32s(item, 1) - 1);
        ++item;
    }

    if (item != tail)
        return FAIL("Too many arguments in MAKE VECTOR! block");

    REBLEN num_bytes = len * (bitsize / 8);
    Binary* bin = Make_Binary(num_bytes);
    memset(Binary_Head(bin), 0, num_bytes);  // !!! 0 bytes -> 0 int/float?
    Term_Binary_Len(bin, num_bytes);

    Init_Vector(OUT, bin, sign, integral, bitsize);
    UNUSED(index);  // !!! Not currently used, may (?) be added later

    if (iblk != nullptr) {
        Option(Error*) e = Trap_Set_Vector_Row(OUT, iblk);
        if (e)
            return FAIL(unwrap e);
    }

    return OUT;
}


IMPLEMENT_GENERIC(PICK, Is_Vector) {
    INCLUDE_PARAMS_OF_PICK;

    Element* vec = Element_ARG(LOCATION);
    Element* picker = Element_ARG(PICKER);

    REBINT n;
    if (Is_Integer(picker) or Is_Decimal(picker))  // #2312
        n = Int32(picker);
    else
        return FAIL(PARAM(PICKER));

    if (n == 0)  // Rebol2/Red convention, 0 is bad pick
        return RAISE(Error_Out_Of_Range(picker));

    if (n < 0)
        ++n;  // Rebol/Red convention, picking -1 from tail gives last item

    n += VAL_VECTOR_INDEX(vec);

    if (n <= 0 or cast(REBLEN, n) > VAL_VECTOR_LEN_AT(vec))
        return nullptr;  // out of range of vector data

    Get_Vector_At(OUT, vec, n - 1);
    return OUT;
}


// Because the vector uses Alloc_Pairing() for its 2-cells-of value, it has
// to defer to the binary itself for locked status (also since it can co-opt
// a BINARY! as its backing store, it has to honor the protection status of
// the binary)
//
// !!! How does this tie into CONST-ness?  How should aggregate types handle
// their overall constness vs. that of their components?
//
IMPLEMENT_GENERIC(POKE, Is_Vector) {
    INCLUDE_PARAMS_OF_POKE;

    Element* vec = Element_ARG(LOCATION);
    Element* picker = Element_ARG(PICKER);
    Value* poke = ARG(VALUE);

    Ensure_Mutable(VAL_VECTOR_BLOB(vec));

    REBINT n;
    if (Is_Integer(picker) or Is_Decimal(picker)) // #2312
        n = Int32(picker);
    else
        return FAIL(PARAM(PICKER));

    if (n == 0)
        return RAISE(Error_Out_Of_Range(picker));  // Rebol2/Red convention

    if (n < 0)
        ++n;  // Rebol2/Red convention, poking -1 from tail sets last item

    n += VAL_VECTOR_INDEX(value);

    if (n <= 0 or cast(REBLEN, n) > VAL_VECTOR_LEN_AT(vec))
        return RAISE(Error_Out_Of_Range(picker));

    Option(Error*) e = Trap_Set_Vector_At(vec, n - 1, cast(Element*, poke));
    if (e)
        return FAIL(unwrap e);

    return nullptr;  // all data modified through stub, no writeback needed
}


IMPLEMENT_GENERIC(LENGTH_OF, Is_Vector)
{
    INCLUDE_PARAMS_OF_LENGTH_OF;

    Element* vec = Element_ARG(ELEMENT);
    return Init_Integer(OUT, VAL_VECTOR_LEN_AT(vec));
}


IMPLEMENT_GENERIC(COPY, Is_Vector) {
    INCLUDE_PARAMS_OF_COPY;

    Element* vec = Element_ARG(VALUE);

    if (Bool_ARG(PART) or Bool_ARG(DEEP))
        return FAIL(Error_Bad_Refines_Raw());

    Binary* bin = u_cast(Binary*, Copy_Flex_Core(
        NODE_FLAG_MANAGED,
        Cell_Binary(VAL_VECTOR_BLOB(vec))
    ));

    return Init_Vector(
        OUT,
        bin,
        VAL_VECTOR_SIGN(vec),
        VAL_VECTOR_INTEGRAL(vec),
        VAL_VECTOR_BITSIZE(vec)
    );
}


IMPLEMENT_GENERIC(MOLDIFY, Is_Vector)
{
    INCLUDE_PARAMS_OF_MOLDIFY;

    Element* vec = Element_ARG(ELEMENT);
    Molder* mo = Cell_Handle_Pointer(Molder, ARG(MOLDER));
    bool form = Bool_ARG(FORM);

    REBLEN len = VAL_VECTOR_LEN_AT(vec);
    REBLEN n = VAL_VECTOR_INDEX(vec);

    bool integral = VAL_VECTOR_INTEGRAL(vec);
    bool sign = VAL_VECTOR_SIGN(vec);
    REBLEN bits = VAL_VECTOR_BITSIZE(vec);

    if (not form) {
        Type type = integral ? TYPE_INTEGER : TYPE_DECIMAL;
        Append_Ascii(mo->string, "#[vector! [");

        // `<(opt) unsigned> kind bits len [`
        //
        if (not sign)
            Append_Ascii(mo->string, "unsigned ");
        Append_Spelling(mo->string, Canon_Symbol(Symbol_Id_From_Type(type)));
        Append_Codepoint(mo->string, ' ');
        Append_Int(mo->string, bits);
        Append_Codepoint(mo->string, ' ');
        Append_Int(mo->string, len);
        Append_Ascii(mo->string, " [");
        if (len != 0)
            New_Indented_Line(mo);
    }

    DECLARE_ELEMENT (temp);

    REBLEN c = 0;
    for (; n < VAL_VECTOR_LEN_AT(vec); n++) {
        Get_Vector_At(temp, vec, n);

        Byte buf[32];
        Byte l;
        if (integral)
            l = Emit_Integer(buf, VAL_INT64(temp));
        else
            l = Emit_Decimal(buf, VAL_DECIMAL(temp), 0, '.', mo->digits);
        Append_Ascii_Len(mo->string, s_cast(buf), l);

        if ((++c > 7) && (n + 1 < VAL_VECTOR_LEN_AT(vec))) {
            New_Indented_Line(mo);
            c = 0;
        }
        else
            Append_Codepoint(mo->string, ' ');
    }

    // !!! There was some handling here for trimming spaces, should be done
    // another way for UTF-8 everywhere if it's important.

    if (not form) {
        if (len)
            New_Indented_Line(mo);

        Append_Codepoint(mo->string, ']');

        Append_Codepoint(mo->string, ']');
    }

    return TRIPWIRE;
}


//
//  startup*: native [
//
//  "Startup VECTOR! Extension"
//
//      return: []
//  ]
//
DECLARE_NATIVE(STARTUP_P)
{
    INCLUDE_PARAMS_OF_STARTUP_P;

    return TRIPWIRE;
}


//
//  shutdown*: native [
//
//  "Shutdown VECTOR! Extension"
//
//      return: []
//  ]
//
DECLARE_NATIVE(SHUTDOWN_P)
{
    INCLUDE_PARAMS_OF_SHUTDOWN_P;

    return TRIPWIRE;
}
