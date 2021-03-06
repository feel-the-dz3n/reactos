/*
 * Copyright 2008 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* GlobalObj */
#define DISPID_GLOBAL_NAN           0x0000
#define DISPID_GLOBAL_INFINITY      0x0001
#define DISPID_GLOBAL_ARRAY         0x0002
#define DISPID_GLOBAL_BOOLEAN       0x0003
#define DISPID_GLOBAL_DATE          0x0004
#define DISPID_GLOBAL_FUNCTION      0x0005
#define DISPID_GLOBAL_NUMBER        0x0006
#define DISPID_GLOBAL_OBJECT        0x0007
#define DISPID_GLOBAL_STRING        0x0008
#define DISPID_GLOBAL_REGEXP        0x0009
#define DISPID_GLOBAL_ACTIVEXOBJ    0x000a
#define DISPID_GLOBAL_VBARRAY       0x000b
#define DISPID_GLOBAL_ENUMERATOR    0x000c
#define DISPID_GLOBAL_ESCAPE        0x000d
#define DISPID_GLOBAL_EVAL          0x000e
#define DISPID_GLOBAL_ISNAN         0x000f
#define DISPID_GLOBAL_ISFINITE      0x0010
#define DISPID_GLOBAL_PARSEINT      0x0011
#define DISPID_GLOBAL_PARSEFLOAT    0x0012
#define DISPID_GLOBAL_UNESCAPE      0x0013
#define DISPID_GLOBAL_GETOBJECT     0x0014
#define DISPID_GLOBAL_SCRIPTENGINE  0x0015
#define DISPID_GLOBAL_MAJORVER      0x0016
#define DISPID_GLOBAL_MINORVER      0x0017
#define DISPID_GLOBAL_BUILDVER      0x0018
#define DISPID_GLOBAL_COLLECT       0x0019
#define DISPID_GLOBAL_MATH          0x001a


/* DateObj */
#define DISPID_DATEOBJ_PARSE  0x0064
#define DISPID_DATEOBJ_UTC    0x0065


/* MathObj */
#define DISPID_MATHOBJ_E        0x00c8
#define DISPID_MATHOBJ_LOG2E    0x00c9
#define DISPID_MATHOBJ_LOG10E   0x00ca
#define DISPID_MATHOBJ_LN2      0x00cb
#define DISPID_MATHOBJ_LN10     0x00cc
#define DISPID_MATHOBJ_PI       0x00cd
#define DISPID_MATHOBJ_SQRT2    0x00ce
#define DISPID_MATHOBJ_SQRT1_2  0x00cf
#define DISPID_MATHOBJ_ABS      0x00d0
#define DISPID_MATHOBJ_ACOS     0x00d1
#define DISPID_MATHOBJ_ASIN     0x00d2
#define DISPID_MATHOBJ_ATAN     0x00d3
#define DISPID_MATHOBJ_ATAN2    0x00d4
#define DISPID_MATHOBJ_CEIL     0x00d5
#define DISPID_MATHOBJ_COS      0x00d6
#define DISPID_MATHOBJ_EXP      0x00d7
#define DISPID_MATHOBJ_FLOOR    0x00d8
#define DISPID_MATHOBJ_LOG      0x00d9
#define DISPID_MATHOBJ_MAX      0x00da
#define DISPID_MATHOBJ_MIN      0x00db
#define DISPID_MATHOBJ_POW      0x00dc
#define DISPID_MATHOBJ_RANDOM   0x00dd
#define DISPID_MATHOBJ_ROUND    0x00de
#define DISPID_MATHOBJ_SIN      0x00df
#define DISPID_MATHOBJ_SQRT     0x00e0
#define DISPID_MATHOBJ_TAN      0x00e1


/* NumberObj */
#define DISPID_NUMBEROBJ_MAX_VALUE          0x012c
#define DISPID_NUMBEROBJ_MIN_VALUE          0x012d
#define DISPID_NUMBEROBJ_NAN                0x012e
#define DISPID_NUMBEROBJ_NEGATIVE_INFINITY  0x012f
#define DISPID_NUMBEROBJ_POSITIVE_INFINITY  0x0130


/* RegExpObj */
#define DISPID_REGEXPOBJ_INDEX      0x0190
#define DISPID_REGEXPOBJ_INPUT      0x0191
#define DISPID_REGEXPOBJ_LASTINDEX  0x0192


/* StringObj */
#define DISPID_STRINGOBJ_FROMCHARCODE  0x01f4


/* ArrayInstance */
#define DISPID_ARRAY_LENGTH       0x0258
#define DISPID_ARRAY_CONCAT       0x0259
#define DISPID_ARRAY_JOIN         0x025a
#define DISPID_ARRAY_POP          0x025b
#define DISPID_ARRAY_PUSH         0x025c
#define DISPID_ARRAY_REVERSE      0x025d
#define DISPID_ARRAY_SHIFT        0x025e
#define DISPID_ARRAY_SLICE        0x025f
#define DISPID_ARRAY_SORT         0x0260
#define DISPID_ARRAY_SPLICE       0x0261
#define DISPID_ARRAY_TOSTRING     0x0262
#define DISPID_ARRAY_TOLOCSTRING  0x0263
#define DISPID_ARRAY_VALUEOF      0x0264
#define DISPID_ARRAY_UNSHIFT      0x0265
#define DISPID_ARRAY_HASOWNPROP   0x0266
#define DISPID_ARRAY_PROPISENUM   0x0267
#define DISPID_ARRAY_ISPROTOF     0x0268


/* FunctionInstance */
#define DISPID_FUNCTION_LENGTH       0x02bc
#define DISPID_FUNCTION_TOSTRING     0x02bd
#define DISPID_FUNCTION_TOLOCSTRING  0x02be
#define DISPID_FUNCTION_VALUEOF      0x02bf
#define DISPID_FUNCTION_APPLY        0x02c0
#define DISPID_FUNCTION_CALL         0x02c1
#define DISPID_FUNCTION_HASOWNPROP   0x02c2
#define DISPID_FUNCTION_PROPISENUM   0x02c3
#define DISPID_FUNCTION_ISPROTOF     0x02c4


/* StringInstance */
#define DISPID_STRING_LENGTH          0x0320
#define DISPID_STRING_TOSTRING        0x0321
#define DISPID_STRING_VALUEOF         0x0322
#define DISPID_STRING_ANCHOR          0x0323
#define DISPID_STRING_BIG             0x0324
#define DISPID_STRING_BLINK           0x0325
#define DISPID_STRING_BOLD            0x0326
#define DISPID_STRING_CHARAT          0x0327
#define DISPID_STRING_CHARCODEAT      0x0328
#define DISPID_STRING_CONCAT          0x0329
#define DISPID_STRING_FIXED           0x032a
#define DISPID_STRING_FONTCOLOR       0x032b
#define DISPID_STRING_FONTSIZE        0x032c
#define DISPID_STRING_INDEXOF         0x032d
#define DISPID_STRING_ITALICS         0x032e
#define DISPID_STRING_LASTINDEXOF     0x032f
#define DISPID_STRING_LINK            0x0330
#define DISPID_STRING_MATCH           0x0331
#define DISPID_STRING_REPLACE         0x0332
#define DISPID_STRING_SEARCH          0x0333
#define DISPID_STRING_SLICE           0x0334
#define DISPID_STRING_SMALL           0x0335
#define DISPID_STRING_SPLIT           0x0336
#define DISPID_STRING_STRIKE          0x0337
#define DISPID_STRING_SUB             0x0338
#define DISPID_STRING_SUBSTRING       0x0339
#define DISPID_STRING_SUBSTR          0x033a
#define DISPID_STRING_SUP             0x033b
#define DISPID_STRING_TOLOWERCASE     0x033c
#define DISPID_STRING_TOUPPERCASE     0x033d
#define DISPID_STRING_TOLOCLOWERCASE  0x033e
#define DISPID_STRING_TOLOCUPPERCASE  0x033f
#define DISPID_STRING_LOCCOMPARE      0x0340
#define DISPID_STRING_HASOWNPROP      0x0341
#define DISPID_STRING_PROPISENUM      0x0342
#define DISPID_STRING_ISPROTOF        0x0343


/* BoolInstance */
#define DISPID_BOOL_TOSTRING     0x0384
#define DISPID_BOOL_TOLOCSTRING  0x0385
#define DISPID_BOOL_VALUEOF      0x0386
#define DISPID_BOOL_HASOWNPROP   0x0387
#define DISPID_BOOL_PROPISENUM   0x0388
#define DISPID_BOOL_ISPROTOF     0x0389


/* NumberInstance */
#define DISPID_NUMBER_TOSTRING       0x03e8
#define DISPID_NUMBER_TOLOCSTRING    0x03e9
#define DISPID_NUMBER_TOFIXED        0x03ea
#define DISPID_NUMBER_TOEXPONENTIAL  0x03eb
#define DISPID_NUMBER_TOPRECISION    0x03ec
#define DISPID_NUMBER_VALUEOF        0x03ed
#define DISPID_NUMBER_HASOWNPROP     0x03ee
#define DISPID_NUMBER_PROPISENUM     0x03ef
#define DISPID_NUMBER_ISPROTOF       0x03f0


/* ObjectInstance */
#define DISPID_OBJECT_TOSTRING     0x044c
#define DISPID_OBJECT_TOLOCSTRING  0x044d
#define DISPID_OBJECT_HASOWNPROP   0x044e
#define DISPID_OBJECT_PROPISENUM   0x044f
#define DISPID_OBJECT_ISPROTOF     0x0450
#define DISPID_OBJECT_VALUEOF      0x0451


/* DateInstance */
#define DISPID_DATE_TOSTRING            0x04b0
#define DISPID_DATE_TOLOCSTRING         0x04b1
#define DISPID_DATE_HASOWNPROP          0x04b2
#define DISPID_DATE_PROPISENUM          0x04b3
#define DISPID_DATE_ISPROTOF            0x04b4
#define DISPID_DATE_VALUEOF             0x04b5
#define DISPID_DATE_TOUTCSTRING         0x04b6
#define DISPID_DATE_TODATESTRING        0x04b7
#define DISPID_DATE_TOTIMESTRING        0x04b8
#define DISPID_DATE_TOLOCDATESTRING     0x04b9
#define DISPID_DATE_TOLOCTIMESTRING     0x04ba
#define DISPID_DATE_GETTIME             0x04bb
#define DISPID_DATE_GETFULLYEAR         0x04bc
#define DISPID_DATE_GETUTCFULLYEAR      0x04bd
#define DISPID_DATE_GETMONTH            0x04be
#define DISPID_DATE_GETUTCMONTH         0x04bf
#define DISPID_DATE_GETDATE             0x04c0
#define DISPID_DATE_GETUTCDATE          0x04c1
#define DISPID_DATE_GETDAY              0x04c2
#define DISPID_DATE_GETUTCDAY           0x04c3
#define DISPID_DATE_GETHOURS            0x04c4
#define DISPID_DATE_GETUTCHOURS         0x04c5
#define DISPID_DATE_GETMINUTES          0x04c6
#define DISPID_DATE_GETUTCMINUTES       0x04c7
#define DISPID_DATE_GETSECONDS          0x04c8
#define DISPID_DATE_GETUTCSECONDS       0x04c9
#define DISPID_DATE_GETMILLISECONDS     0x04ca
#define DISPID_DATE_GETUTCMILLISECONDS  0x04cb
#define DISPID_DATE_GETTIMEZONEOFFSET   0x04cc
#define DISPID_DATE_SETTIME             0x04cd
#define DISPID_DATE_SETMILLISECONDS     0x04ce
#define DISPID_DATE_SETUTCMILLISECONDS  0x04cf
#define DISPID_DATE_SETSECONDS          0x04d0
#define DISPID_DATE_SETUTCSECONDS       0x04d1
#define DISPID_DATE_SETMINUTES          0x04d2
#define DISPID_DATE_SETUTCMINUTES       0x04d3
#define DISPID_DATE_SETHOURS            0x04d4
#define DISPID_DATE_SETUTCHOURS         0x04d5
#define DISPID_DATE_SETDATE             0x04d6
#define DISPID_DATE_SETUTCDATE          0x04d7
#define DISPID_DATE_SETMONTH            0x04d8
#define DISPID_DATE_SETUTCMONTH         0x04d9
#define DISPID_DATE_SETFULLYEAR         0x04da
#define DISPID_DATE_SETUTCFULLYEAR      0x04db


/* RegExpInstance */
#define DISPID_REGEXP_SOURCE       0x0514
#define DISPID_REGEXP_GLOBAL       0x0515
#define DISPID_REGEXP_IGNORECASE   0x0516
#define DISPID_REGEXP_MULTILINE    0x0517
#define DISPID_REGEXP_LASTINDEX    0x0518
#define DISPID_REGEXP_TOSTRING     0x0519
#define DISPID_REGEXP_TOLOCSTRING  0x051a
#define DISPID_REGEXP_HASOWNPROP   0x051b
#define DISPID_REGEXP_PROPISENUM   0x051c
#define DISPID_REGEXP_ISPROTOF     0x051d
#define DISPID_REGEXP_EXEC         0x051e


/* ErrorInstance */
#define DISPID_ERROR_NAME         0x0578
#define DISPID_ERROR_MESSAGE      0x0579
#define DISPID_ERROR_IGNORECASE   0x057a
#define DISPID_ERROR_MULTILINE    0x057b
#define DISPID_ERROR_LASTINDEX    0x057c
#define DISPID_ERROR_TOSTRING     0x057d
#define DISPID_ERROR_TOLOCSTRING  0x057e
#define DISPID_ERROR_HASOWNPROP   0x057f
#define DISPID_ERROR_PROPISENUM   0x0580
#define DISPID_ERROR_ISPROTOF     0x0581
