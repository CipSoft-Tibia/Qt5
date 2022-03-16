/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         _mesa_glsl_parse
#define yylex           _mesa_glsl_lex
#define yyerror         _mesa_glsl_error
#define yydebug         _mesa_glsl_debug
#define yynerrs         _mesa_glsl_nerrs


/* Copy the first part of user declarations.  */
#line 1 "glsl_parser.yy" /* yacc.c:339  */

/*
 * Copyright © 2008, 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
    
#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "main/context.h"

#define YYLEX_PARAM state->scanner

#undef yyerror

static void yyerror(YYLTYPE *loc, _mesa_glsl_parse_state *st, const char *msg)
{
   _mesa_glsl_error(loc, st, "%s", msg);
}

#line 115 "glsl_parser.cc" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "glsl_parser.h".  */
#ifndef YY__MESA_GLSL_GLSL_PARSER_H_INCLUDED
# define YY__MESA_GLSL_GLSL_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int _mesa_glsl_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ATTRIBUTE = 258,
    CONST_TOK = 259,
    BOOL_TOK = 260,
    FLOAT_TOK = 261,
    INT_TOK = 262,
    UINT_TOK = 263,
    BREAK = 264,
    CONTINUE = 265,
    DO = 266,
    ELSE = 267,
    FOR = 268,
    IF = 269,
    DISCARD = 270,
    RETURN = 271,
    SWITCH = 272,
    CASE = 273,
    DEFAULT = 274,
    BVEC2 = 275,
    BVEC3 = 276,
    BVEC4 = 277,
    IVEC2 = 278,
    IVEC3 = 279,
    IVEC4 = 280,
    UVEC2 = 281,
    UVEC3 = 282,
    UVEC4 = 283,
    VEC2 = 284,
    VEC3 = 285,
    VEC4 = 286,
    CENTROID = 287,
    IN_TOK = 288,
    OUT_TOK = 289,
    INOUT_TOK = 290,
    UNIFORM = 291,
    VARYING = 292,
    NOPERSPECTIVE = 293,
    FLAT = 294,
    SMOOTH = 295,
    MAT2X2 = 296,
    MAT2X3 = 297,
    MAT2X4 = 298,
    MAT3X2 = 299,
    MAT3X3 = 300,
    MAT3X4 = 301,
    MAT4X2 = 302,
    MAT4X3 = 303,
    MAT4X4 = 304,
    SAMPLER1D = 305,
    SAMPLER2D = 306,
    SAMPLER3D = 307,
    SAMPLERCUBE = 308,
    SAMPLER1DSHADOW = 309,
    SAMPLER2DSHADOW = 310,
    SAMPLERCUBESHADOW = 311,
    SAMPLER1DARRAY = 312,
    SAMPLER2DARRAY = 313,
    SAMPLER1DARRAYSHADOW = 314,
    SAMPLER2DARRAYSHADOW = 315,
    ISAMPLER1D = 316,
    ISAMPLER2D = 317,
    ISAMPLER3D = 318,
    ISAMPLERCUBE = 319,
    ISAMPLER1DARRAY = 320,
    ISAMPLER2DARRAY = 321,
    USAMPLER1D = 322,
    USAMPLER2D = 323,
    USAMPLER3D = 324,
    USAMPLERCUBE = 325,
    USAMPLER1DARRAY = 326,
    USAMPLER2DARRAY = 327,
    SAMPLER2DRECT = 328,
    ISAMPLER2DRECT = 329,
    USAMPLER2DRECT = 330,
    SAMPLER2DRECTSHADOW = 331,
    SAMPLERBUFFER = 332,
    ISAMPLERBUFFER = 333,
    USAMPLERBUFFER = 334,
    SAMPLEREXTERNALOES = 335,
    STRUCT = 336,
    VOID_TOK = 337,
    WHILE = 338,
    IDENTIFIER = 339,
    TYPE_IDENTIFIER = 340,
    NEW_IDENTIFIER = 341,
    FLOATCONSTANT = 342,
    INTCONSTANT = 343,
    UINTCONSTANT = 344,
    BOOLCONSTANT = 345,
    FIELD_SELECTION = 346,
    LEFT_OP = 347,
    RIGHT_OP = 348,
    INC_OP = 349,
    DEC_OP = 350,
    LE_OP = 351,
    GE_OP = 352,
    EQ_OP = 353,
    NE_OP = 354,
    AND_OP = 355,
    OR_OP = 356,
    XOR_OP = 357,
    MUL_ASSIGN = 358,
    DIV_ASSIGN = 359,
    ADD_ASSIGN = 360,
    MOD_ASSIGN = 361,
    LEFT_ASSIGN = 362,
    RIGHT_ASSIGN = 363,
    AND_ASSIGN = 364,
    XOR_ASSIGN = 365,
    OR_ASSIGN = 366,
    SUB_ASSIGN = 367,
    INVARIANT = 368,
    LOWP = 369,
    MEDIUMP = 370,
    HIGHP = 371,
    SUPERP = 372,
    PRECISION = 373,
    VERSION_TOK = 374,
    EXTENSION = 375,
    LINE = 376,
    COLON = 377,
    EOL = 378,
    INTERFACE = 379,
    OUTPUT = 380,
    PRAGMA_DEBUG_ON = 381,
    PRAGMA_DEBUG_OFF = 382,
    PRAGMA_OPTIMIZE_ON = 383,
    PRAGMA_OPTIMIZE_OFF = 384,
    PRAGMA_INVARIANT_ALL = 385,
    LAYOUT_TOK = 386,
    ASM = 387,
    CLASS = 388,
    UNION = 389,
    ENUM = 390,
    TYPEDEF = 391,
    TEMPLATE = 392,
    THIS = 393,
    PACKED_TOK = 394,
    GOTO = 395,
    INLINE_TOK = 396,
    NOINLINE = 397,
    VOLATILE = 398,
    PUBLIC_TOK = 399,
    STATIC = 400,
    EXTERN = 401,
    EXTERNAL = 402,
    LONG_TOK = 403,
    SHORT_TOK = 404,
    DOUBLE_TOK = 405,
    HALF = 406,
    FIXED_TOK = 407,
    UNSIGNED = 408,
    INPUT_TOK = 409,
    OUPTUT = 410,
    HVEC2 = 411,
    HVEC3 = 412,
    HVEC4 = 413,
    DVEC2 = 414,
    DVEC3 = 415,
    DVEC4 = 416,
    FVEC2 = 417,
    FVEC3 = 418,
    FVEC4 = 419,
    SAMPLER3DRECT = 420,
    SIZEOF = 421,
    CAST = 422,
    NAMESPACE = 423,
    USING = 424,
    ERROR_TOK = 425,
    COMMON = 426,
    PARTITION = 427,
    ACTIVE = 428,
    FILTER = 429,
    IMAGE1D = 430,
    IMAGE2D = 431,
    IMAGE3D = 432,
    IMAGECUBE = 433,
    IMAGE1DARRAY = 434,
    IMAGE2DARRAY = 435,
    IIMAGE1D = 436,
    IIMAGE2D = 437,
    IIMAGE3D = 438,
    IIMAGECUBE = 439,
    IIMAGE1DARRAY = 440,
    IIMAGE2DARRAY = 441,
    UIMAGE1D = 442,
    UIMAGE2D = 443,
    UIMAGE3D = 444,
    UIMAGECUBE = 445,
    UIMAGE1DARRAY = 446,
    UIMAGE2DARRAY = 447,
    IMAGE1DSHADOW = 448,
    IMAGE2DSHADOW = 449,
    IMAGEBUFFER = 450,
    IIMAGEBUFFER = 451,
    UIMAGEBUFFER = 452,
    IMAGE1DARRAYSHADOW = 453,
    IMAGE2DARRAYSHADOW = 454,
    ROW_MAJOR = 455
  };
#endif
/* Tokens.  */
#define ATTRIBUTE 258
#define CONST_TOK 259
#define BOOL_TOK 260
#define FLOAT_TOK 261
#define INT_TOK 262
#define UINT_TOK 263
#define BREAK 264
#define CONTINUE 265
#define DO 266
#define ELSE 267
#define FOR 268
#define IF 269
#define DISCARD 270
#define RETURN 271
#define SWITCH 272
#define CASE 273
#define DEFAULT 274
#define BVEC2 275
#define BVEC3 276
#define BVEC4 277
#define IVEC2 278
#define IVEC3 279
#define IVEC4 280
#define UVEC2 281
#define UVEC3 282
#define UVEC4 283
#define VEC2 284
#define VEC3 285
#define VEC4 286
#define CENTROID 287
#define IN_TOK 288
#define OUT_TOK 289
#define INOUT_TOK 290
#define UNIFORM 291
#define VARYING 292
#define NOPERSPECTIVE 293
#define FLAT 294
#define SMOOTH 295
#define MAT2X2 296
#define MAT2X3 297
#define MAT2X4 298
#define MAT3X2 299
#define MAT3X3 300
#define MAT3X4 301
#define MAT4X2 302
#define MAT4X3 303
#define MAT4X4 304
#define SAMPLER1D 305
#define SAMPLER2D 306
#define SAMPLER3D 307
#define SAMPLERCUBE 308
#define SAMPLER1DSHADOW 309
#define SAMPLER2DSHADOW 310
#define SAMPLERCUBESHADOW 311
#define SAMPLER1DARRAY 312
#define SAMPLER2DARRAY 313
#define SAMPLER1DARRAYSHADOW 314
#define SAMPLER2DARRAYSHADOW 315
#define ISAMPLER1D 316
#define ISAMPLER2D 317
#define ISAMPLER3D 318
#define ISAMPLERCUBE 319
#define ISAMPLER1DARRAY 320
#define ISAMPLER2DARRAY 321
#define USAMPLER1D 322
#define USAMPLER2D 323
#define USAMPLER3D 324
#define USAMPLERCUBE 325
#define USAMPLER1DARRAY 326
#define USAMPLER2DARRAY 327
#define SAMPLER2DRECT 328
#define ISAMPLER2DRECT 329
#define USAMPLER2DRECT 330
#define SAMPLER2DRECTSHADOW 331
#define SAMPLERBUFFER 332
#define ISAMPLERBUFFER 333
#define USAMPLERBUFFER 334
#define SAMPLEREXTERNALOES 335
#define STRUCT 336
#define VOID_TOK 337
#define WHILE 338
#define IDENTIFIER 339
#define TYPE_IDENTIFIER 340
#define NEW_IDENTIFIER 341
#define FLOATCONSTANT 342
#define INTCONSTANT 343
#define UINTCONSTANT 344
#define BOOLCONSTANT 345
#define FIELD_SELECTION 346
#define LEFT_OP 347
#define RIGHT_OP 348
#define INC_OP 349
#define DEC_OP 350
#define LE_OP 351
#define GE_OP 352
#define EQ_OP 353
#define NE_OP 354
#define AND_OP 355
#define OR_OP 356
#define XOR_OP 357
#define MUL_ASSIGN 358
#define DIV_ASSIGN 359
#define ADD_ASSIGN 360
#define MOD_ASSIGN 361
#define LEFT_ASSIGN 362
#define RIGHT_ASSIGN 363
#define AND_ASSIGN 364
#define XOR_ASSIGN 365
#define OR_ASSIGN 366
#define SUB_ASSIGN 367
#define INVARIANT 368
#define LOWP 369
#define MEDIUMP 370
#define HIGHP 371
#define SUPERP 372
#define PRECISION 373
#define VERSION_TOK 374
#define EXTENSION 375
#define LINE 376
#define COLON 377
#define EOL 378
#define INTERFACE 379
#define OUTPUT 380
#define PRAGMA_DEBUG_ON 381
#define PRAGMA_DEBUG_OFF 382
#define PRAGMA_OPTIMIZE_ON 383
#define PRAGMA_OPTIMIZE_OFF 384
#define PRAGMA_INVARIANT_ALL 385
#define LAYOUT_TOK 386
#define ASM 387
#define CLASS 388
#define UNION 389
#define ENUM 390
#define TYPEDEF 391
#define TEMPLATE 392
#define THIS 393
#define PACKED_TOK 394
#define GOTO 395
#define INLINE_TOK 396
#define NOINLINE 397
#define VOLATILE 398
#define PUBLIC_TOK 399
#define STATIC 400
#define EXTERN 401
#define EXTERNAL 402
#define LONG_TOK 403
#define SHORT_TOK 404
#define DOUBLE_TOK 405
#define HALF 406
#define FIXED_TOK 407
#define UNSIGNED 408
#define INPUT_TOK 409
#define OUPTUT 410
#define HVEC2 411
#define HVEC3 412
#define HVEC4 413
#define DVEC2 414
#define DVEC3 415
#define DVEC4 416
#define FVEC2 417
#define FVEC3 418
#define FVEC4 419
#define SAMPLER3DRECT 420
#define SIZEOF 421
#define CAST 422
#define NAMESPACE 423
#define USING 424
#define ERROR_TOK 425
#define COMMON 426
#define PARTITION 427
#define ACTIVE 428
#define FILTER 429
#define IMAGE1D 430
#define IMAGE2D 431
#define IMAGE3D 432
#define IMAGECUBE 433
#define IMAGE1DARRAY 434
#define IMAGE2DARRAY 435
#define IIMAGE1D 436
#define IIMAGE2D 437
#define IIMAGE3D 438
#define IIMAGECUBE 439
#define IIMAGE1DARRAY 440
#define IIMAGE2DARRAY 441
#define UIMAGE1D 442
#define UIMAGE2D 443
#define UIMAGE3D 444
#define UIMAGECUBE 445
#define UIMAGE1DARRAY 446
#define UIMAGE2DARRAY 447
#define IMAGE1DSHADOW 448
#define IMAGE2DSHADOW 449
#define IMAGEBUFFER 450
#define IIMAGEBUFFER 451
#define UIMAGEBUFFER 452
#define IMAGE1DARRAYSHADOW 453
#define IMAGE2DARRAYSHADOW 454
#define ROW_MAJOR 455

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 59 "glsl_parser.yy" /* yacc.c:355  */

   int n;
   float real;
   const char *identifier;

   struct ast_type_qualifier type_qualifier;

   ast_node *node;
   ast_type_specifier *type_specifier;
   ast_fully_specified_type *fully_specified_type;
   ast_function *function;
   ast_parameter_declarator *parameter_declarator;
   ast_function_definition *function_definition;
   ast_compound_statement *compound_statement;
   ast_expression *expression;
   ast_declarator_list *declarator_list;
   ast_struct_specifier *struct_specifier;
   ast_declaration *declaration;
   ast_switch_body *switch_body;
   ast_case_label *case_label;
   ast_case_label_list *case_label_list;
   ast_case_statement *case_statement;
   ast_case_statement_list *case_statement_list;

   struct {
      ast_node *cond;
      ast_expression *rest;
   } for_rest_statement;

   struct {
      ast_node *then_statement;
      ast_node *else_statement;
   } selection_rest_statement;

#line 590 "glsl_parser.cc" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int _mesa_glsl_parse (struct _mesa_glsl_parse_state *state);

#endif /* !YY__MESA_GLSL_GLSL_PARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 620 "glsl_parser.cc" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3101

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  225
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  103
/* YYNRULES -- Number of rules.  */
#define YYNRULES  318
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  483

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   455

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   209,     2,     2,     2,   213,   216,     2,
     201,   202,   211,   207,   206,   208,   205,   212,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   220,   222,
     214,   221,   215,   219,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   203,     2,   204,   217,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   223,   218,   224,   210,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   243,   243,   242,   254,   256,   309,   310,   311,   312,
     313,   325,   327,   331,   332,   333,   337,   346,   354,   365,
     366,   370,   377,   384,   391,   398,   405,   412,   413,   419,
     423,   430,   436,   445,   449,   453,   454,   463,   464,   468,
     469,   473,   479,   491,   495,   501,   508,   518,   519,   523,
     524,   528,   534,   546,   557,   558,   564,   570,   580,   581,
     582,   583,   587,   588,   594,   600,   609,   610,   616,   625,
     626,   632,   641,   642,   648,   654,   660,   669,   670,   676,
     685,   686,   695,   696,   705,   706,   715,   716,   725,   726,
     735,   736,   745,   746,   755,   756,   765,   766,   767,   768,
     769,   770,   771,   772,   773,   774,   775,   779,   783,   799,
     803,   808,   812,   818,   825,   829,   830,   834,   839,   847,
     861,   871,   886,   893,   898,   909,   922,   925,   930,   935,
     944,   948,   949,   959,   969,   979,   989,   999,  1013,  1020,
    1029,  1038,  1047,  1056,  1065,  1074,  1088,  1095,  1106,  1113,
    1114,  1124,  1192,  1238,  1260,  1265,  1273,  1278,  1283,  1291,
    1299,  1300,  1301,  1306,  1307,  1312,  1317,  1323,  1331,  1336,
    1341,  1346,  1352,  1357,  1362,  1367,  1372,  1380,  1384,  1392,
    1393,  1399,  1408,  1414,  1420,  1429,  1430,  1431,  1432,  1433,
    1434,  1435,  1436,  1437,  1438,  1439,  1440,  1441,  1442,  1443,
    1444,  1445,  1446,  1447,  1448,  1449,  1450,  1451,  1452,  1453,
    1454,  1455,  1456,  1457,  1458,  1459,  1460,  1461,  1462,  1463,
    1464,  1465,  1466,  1467,  1468,  1469,  1470,  1471,  1472,  1473,
    1474,  1475,  1476,  1477,  1478,  1479,  1480,  1481,  1482,  1483,
    1484,  1485,  1489,  1499,  1509,  1522,  1529,  1538,  1543,  1551,
    1566,  1571,  1579,  1585,  1594,  1598,  1604,  1605,  1609,  1610,
    1611,  1612,  1613,  1614,  1618,  1625,  1624,  1638,  1639,  1643,
    1649,  1658,  1668,  1680,  1686,  1695,  1704,  1709,  1717,  1721,
    1739,  1747,  1752,  1760,  1765,  1773,  1781,  1789,  1797,  1805,
    1813,  1821,  1828,  1835,  1845,  1846,  1850,  1852,  1858,  1863,
    1872,  1878,  1884,  1890,  1896,  1905,  1906,  1907,  1908,  1912,
    1926,  1942,  1965,  1970,  1978,  1980,  1984,  1999,  2016
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ATTRIBUTE", "CONST_TOK", "BOOL_TOK",
  "FLOAT_TOK", "INT_TOK", "UINT_TOK", "BREAK", "CONTINUE", "DO", "ELSE",
  "FOR", "IF", "DISCARD", "RETURN", "SWITCH", "CASE", "DEFAULT", "BVEC2",
  "BVEC3", "BVEC4", "IVEC2", "IVEC3", "IVEC4", "UVEC2", "UVEC3", "UVEC4",
  "VEC2", "VEC3", "VEC4", "CENTROID", "IN_TOK", "OUT_TOK", "INOUT_TOK",
  "UNIFORM", "VARYING", "NOPERSPECTIVE", "FLAT", "SMOOTH", "MAT2X2",
  "MAT2X3", "MAT2X4", "MAT3X2", "MAT3X3", "MAT3X4", "MAT4X2", "MAT4X3",
  "MAT4X4", "SAMPLER1D", "SAMPLER2D", "SAMPLER3D", "SAMPLERCUBE",
  "SAMPLER1DSHADOW", "SAMPLER2DSHADOW", "SAMPLERCUBESHADOW",
  "SAMPLER1DARRAY", "SAMPLER2DARRAY", "SAMPLER1DARRAYSHADOW",
  "SAMPLER2DARRAYSHADOW", "ISAMPLER1D", "ISAMPLER2D", "ISAMPLER3D",
  "ISAMPLERCUBE", "ISAMPLER1DARRAY", "ISAMPLER2DARRAY", "USAMPLER1D",
  "USAMPLER2D", "USAMPLER3D", "USAMPLERCUBE", "USAMPLER1DARRAY",
  "USAMPLER2DARRAY", "SAMPLER2DRECT", "ISAMPLER2DRECT", "USAMPLER2DRECT",
  "SAMPLER2DRECTSHADOW", "SAMPLERBUFFER", "ISAMPLERBUFFER",
  "USAMPLERBUFFER", "SAMPLEREXTERNALOES", "STRUCT", "VOID_TOK", "WHILE",
  "IDENTIFIER", "TYPE_IDENTIFIER", "NEW_IDENTIFIER", "FLOATCONSTANT",
  "INTCONSTANT", "UINTCONSTANT", "BOOLCONSTANT", "FIELD_SELECTION",
  "LEFT_OP", "RIGHT_OP", "INC_OP", "DEC_OP", "LE_OP", "GE_OP", "EQ_OP",
  "NE_OP", "AND_OP", "OR_OP", "XOR_OP", "MUL_ASSIGN", "DIV_ASSIGN",
  "ADD_ASSIGN", "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN",
  "XOR_ASSIGN", "OR_ASSIGN", "SUB_ASSIGN", "INVARIANT", "LOWP", "MEDIUMP",
  "HIGHP", "SUPERP", "PRECISION", "VERSION_TOK", "EXTENSION", "LINE",
  "COLON", "EOL", "INTERFACE", "OUTPUT", "PRAGMA_DEBUG_ON",
  "PRAGMA_DEBUG_OFF", "PRAGMA_OPTIMIZE_ON", "PRAGMA_OPTIMIZE_OFF",
  "PRAGMA_INVARIANT_ALL", "LAYOUT_TOK", "ASM", "CLASS", "UNION", "ENUM",
  "TYPEDEF", "TEMPLATE", "THIS", "PACKED_TOK", "GOTO", "INLINE_TOK",
  "NOINLINE", "VOLATILE", "PUBLIC_TOK", "STATIC", "EXTERN", "EXTERNAL",
  "LONG_TOK", "SHORT_TOK", "DOUBLE_TOK", "HALF", "FIXED_TOK", "UNSIGNED",
  "INPUT_TOK", "OUPTUT", "HVEC2", "HVEC3", "HVEC4", "DVEC2", "DVEC3",
  "DVEC4", "FVEC2", "FVEC3", "FVEC4", "SAMPLER3DRECT", "SIZEOF", "CAST",
  "NAMESPACE", "USING", "ERROR_TOK", "COMMON", "PARTITION", "ACTIVE",
  "FILTER", "IMAGE1D", "IMAGE2D", "IMAGE3D", "IMAGECUBE", "IMAGE1DARRAY",
  "IMAGE2DARRAY", "IIMAGE1D", "IIMAGE2D", "IIMAGE3D", "IIMAGECUBE",
  "IIMAGE1DARRAY", "IIMAGE2DARRAY", "UIMAGE1D", "UIMAGE2D", "UIMAGE3D",
  "UIMAGECUBE", "UIMAGE1DARRAY", "UIMAGE2DARRAY", "IMAGE1DSHADOW",
  "IMAGE2DSHADOW", "IMAGEBUFFER", "IIMAGEBUFFER", "UIMAGEBUFFER",
  "IMAGE1DARRAYSHADOW", "IMAGE2DARRAYSHADOW", "ROW_MAJOR", "'('", "')'",
  "'['", "']'", "'.'", "','", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'",
  "'%'", "'<'", "'>'", "'&'", "'^'", "'|'", "'?'", "':'", "'='", "';'",
  "'{'", "'}'", "$accept", "translation_unit", "$@1", "version_statement",
  "pragma_statement", "extension_statement_list", "any_identifier",
  "extension_statement", "external_declaration_list",
  "variable_identifier", "primary_expression", "postfix_expression",
  "integer_expression", "function_call", "function_call_or_method",
  "function_call_generic", "function_call_header_no_parameters",
  "function_call_header_with_parameters", "function_call_header",
  "function_identifier", "method_call_generic",
  "method_call_header_no_parameters", "method_call_header_with_parameters",
  "method_call_header", "unary_expression", "unary_operator",
  "multiplicative_expression", "additive_expression", "shift_expression",
  "relational_expression", "equality_expression", "and_expression",
  "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_xor_expression",
  "logical_or_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "constant_expression", "declaration", "function_prototype",
  "function_declarator", "function_header_with_parameters",
  "function_header", "parameter_declarator", "parameter_declaration",
  "parameter_qualifier", "parameter_type_specifier",
  "init_declarator_list", "single_declaration", "fully_specified_type",
  "layout_qualifier", "layout_qualifier_id_list", "layout_qualifier_id",
  "uniform_block_layout_qualifier", "interpolation_qualifier",
  "parameter_type_qualifier", "type_qualifier", "storage_qualifier",
  "type_specifier", "type_specifier_no_prec", "type_specifier_nonarray",
  "basic_type_specifier_nonarray", "precision_qualifier",
  "struct_specifier", "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "declaration_statement", "statement", "simple_statement",
  "compound_statement", "$@2", "statement_no_new_scope",
  "compound_statement_no_new_scope", "statement_list",
  "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "switch_statement",
  "switch_body", "case_label", "case_label_list", "case_statement",
  "case_statement_list", "iteration_statement", "for_init_statement",
  "conditionopt", "for_rest_statement", "jump_statement",
  "external_declaration", "function_definition", "uniform_block",
  "member_list", "uniformopt", "member_declaration", "layout_defaults", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,    40,    41,    91,    93,    46,    44,    43,    45,    33,
     126,    42,    47,    37,    60,    62,    38,    94,   124,    63,
      58,    61,    59,   123,   125
};
# endif

#define YYPACT_NINF -373

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-373)))

#define YYTABLE_NINF -313

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -75,   -37,    53,  -373,   -50,  -373,   -19,  -373,   160,  2970,
    -373,  -373,  -373,  -373,   -36,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,   130,  -373,  -373,    28,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,   -58,  -373,  -373,    43,  -373,  -373,  -373,   147,     0,
      17,    20,    26,    46,   -26,  -373,  2970,  -373,  -166,   -15,
      -8,     2,  -128,  -373,   196,   173,   198,  1003,  -373,  -373,
    -373,     5,  -373,  1225,  -373,  -373,  -373,  -373,  -373,   160,
    -373,  -373,  -373,   -20,  1003,    10,  -373,  -373,  -373,  -373,
     198,  -373,  1225,  -373,  -373,  -373,  -373,  -373,   -54,  -373,
    -373,   460,  -373,  -373,   100,  -373,  -373,  -373,  -373,  -373,
    1003,   181,   160,  -373,    19,    35,  -162,    38,   -78,  -373,
    -373,  -373,  2180,  -373,    95,    -3,   160,   559,  -373,  1003,
    -373,    42,  -373,  -373,    21,  -157,  -373,  -373,    47,    49,
    1347,    40,    64,    50,  1875,    75,    90,  -373,  -373,  -373,
    -373,  -373,  2568,  2568,  2568,  -373,  -373,  -373,  -373,  -373,
      60,  -373,    96,  -373,   -52,  -373,  -373,  -373,    48,  -114,
    2759,    98,   -42,  2568,    83,   -53,   101,   -76,   113,    84,
      85,    86,   201,   204,   -89,  -373,  -373,  -122,  -373,    81,
     222,   106,  -373,  -373,  -373,  -373,   682,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,   160,  1003,  -149,  2280,  2568,
    -373,    87,  -373,  -373,  -373,  -373,   104,  -373,  -373,  -373,
     273,    91,  1003,   -25,   108,  -113,  -373,  -373,  -373,   781,
    -373,   226,  -373,   -54,  -373,  -373,   252,  1776,  2568,  -373,
    -373,  -111,  2568,  2083,  -373,  -373,   -45,  -373,  1347,  -373,
    -373,  2568,   196,  -373,  -373,  2568,   134,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    2568,  -373,  2568,  2568,  2568,  2568,  2568,  2568,  2568,  2568,
    2568,  2568,  2568,  2568,  2568,  2568,  2568,  2568,  2568,  2568,
    2568,  2568,  2568,  -373,   251,  -373,  -373,   135,  -373,  -373,
    2471,  2568,   129,   149,  -373,  -373,    -3,  -373,  1003,   132,
     160,  -373,  2568,   160,  -373,  -373,  -373,  -373,   150,  -373,
    -373,  2083,   -34,  -373,   -24,   309,   151,   160,   198,   156,
    -373,   904,   155,   151,  -373,   162,  -373,   159,   -22,  2856,
    -373,  -373,  -373,  -373,  -373,    83,    83,   -53,   -53,   101,
     101,   101,   101,   -76,   -76,   113,    84,    85,    86,   201,
     204,  -107,  -373,  2568,   143,   161,  -373,  2568,   145,   144,
     160,  -373,   -86,   163,  -373,  2568,  -373,   148,   167,  1347,
     152,   153,  1568,  -373,  -373,  -373,  -373,  -373,  2568,   169,
    -373,  2568,   168,  2568,   157,  -373,  2568,   154,   -80,  -373,
    -373,   -21,  2568,  1568,   361,  -373,    -5,  -373,  2568,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  2568,  -373,  -373,  -373,
     158,   151,  -373,  1347,  2568,   164,  -373,  -373,  1126,  1347,
      -2,  -373,  -373,  -373,  -373,  -101,  -373,  -373,  -373,  -373,
    -373,  1347,  -373
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       4,     0,     0,    11,     0,     1,     2,     5,     0,     0,
      12,    13,    14,    15,     0,   169,   168,   189,   186,   187,
     188,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     190,   191,   192,     0,   172,   173,   176,   170,   158,   157,
     156,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   214,   215,   217,   218,   220,   221,   222,   223,
     224,   226,   227,   229,   230,   231,   232,   234,   235,   237,
     238,   239,   240,   213,   228,   236,   219,   225,   233,   241,
     216,     0,   185,   184,   167,   244,   243,   242,     0,     0,
       0,     0,     0,     0,     0,   307,     3,   306,     0,     0,
     116,   126,     0,   131,   138,   161,   163,     0,   160,   146,
     177,   179,   182,     0,   183,    17,   305,   113,   308,     0,
     174,   175,   171,     0,     0,     0,   176,    19,    20,   145,
       0,   165,     0,     6,     7,     8,     9,    10,     0,    18,
     110,     0,   309,   114,   126,   159,   127,   128,   129,   117,
       0,   126,     0,   111,    13,    15,   139,     0,   176,   162,
     164,   147,     0,   178,     0,   314,     0,     0,   247,     0,
     166,     0,   155,   154,   151,     0,   149,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    24,    22,    23,
      25,    46,     0,     0,     0,    58,    59,    60,    61,   273,
     265,   269,    21,    27,    54,    29,    34,    35,     0,     0,
      40,     0,    62,     0,    66,    69,    72,    77,    80,    82,
      84,    86,    88,    90,    92,    94,   107,     0,   255,     0,
     161,   146,   258,   271,   257,   256,     0,   259,   260,   261,
     262,   263,   118,   123,   125,   130,     0,   132,     0,     0,
     119,     0,   318,   180,    62,   109,     0,    44,    16,   315,
     314,     0,     0,   314,   252,     0,   250,   246,   248,     0,
     112,     0,   148,     0,   301,   300,     0,     0,     0,   304,
     302,     0,     0,     0,    55,    56,     0,   264,     0,    31,
      32,     0,     0,    38,    37,     0,   185,    41,    43,    97,
      98,   100,    99,   102,   103,   104,   105,   106,   101,    96,
       0,    57,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   274,   176,   270,   272,   120,   122,   124,
       0,     0,   140,     0,   254,   144,   314,   181,     0,     0,
       0,   313,     0,     0,   249,   245,   152,   150,     0,   295,
     294,   297,     0,   303,     0,   167,   278,     0,   161,     0,
      26,     0,     0,    33,    30,     0,    36,     0,     0,    50,
      42,    95,    63,    64,    65,    67,    68,    70,    71,    75,
      76,    73,    74,    78,    79,    81,    83,    85,    87,    89,
      91,     0,   108,     0,   133,     0,   137,     0,   141,     0,
       0,   310,     0,     0,   251,     0,   296,     0,     0,     0,
       0,     0,     0,   266,    28,    53,    48,    47,     0,   185,
      51,     0,     0,     0,   134,   142,     0,     0,     0,   317,
     253,     0,   298,     0,   277,   275,     0,   280,     0,   268,
     291,   267,    52,    93,   121,   135,     0,   143,   311,   316,
       0,   299,   293,     0,     0,     0,   281,   285,     0,   289,
       0,   279,   136,   292,   276,     0,   284,   287,   286,   288,
     282,   290,   283
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -373,  -373,  -373,  -373,  -373,  -373,    -7,  -373,  -373,   -79,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,     4,  -373,   -92,   -70,  -130,   -95,
      52,    55,    56,    51,    57,    58,  -373,  -152,  -158,  -373,
    -175,  -230,     6,    29,  -373,  -373,  -373,   136,   241,   236,
     146,  -373,  -373,  -243,    -6,  -373,   116,  -373,   -77,  -373,
    -373,   -82,    -9,   -74,  -373,  -373,   302,  -373,   224,  -145,
    -321,    41,  -286,   114,  -176,  -372,  -373,  -373,   -48,   298,
     109,   121,  -373,  -373,    39,  -373,  -373,   -69,  -373,   -68,
    -373,  -373,  -373,  -373,  -373,  -373,   305,  -373,  -373,  -229,
     165,  -373,  -373
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     9,     3,    95,     6,   264,    10,    96,   202,
     203,   204,   372,   205,   206,   207,   208,   209,   210,   211,
     376,   377,   378,   379,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   310,
     227,   256,   228,   229,    99,   100,   101,   243,   149,   150,
     244,   102,   103,   104,   230,   175,   176,   177,   106,   151,
     107,   108,   257,   110,   111,   112,   113,   114,   167,   168,
     265,   266,   345,   232,   233,   234,   235,   288,   450,   451,
     236,   237,   238,   445,   369,   239,   447,   467,   468,   469,
     470,   240,   361,   417,   418,   241,   115,   116,   117,   261,
     262,   263,   118
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     109,    14,   131,   105,   276,   129,   145,   130,   251,   281,
     255,   259,   330,   464,   465,    97,   464,   465,   343,   286,
     319,   320,   268,   159,   160,   157,    11,    12,    13,   412,
      11,    12,    13,   259,   351,   146,   147,   148,    98,   163,
     367,   248,   289,   290,     1,   272,    15,    16,   170,   273,
     449,     4,   297,     5,   340,   406,   140,   141,   171,   249,
     336,   299,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   449,   341,     7,   125,    33,    34,    35,   152,   126,
      37,    38,    39,    40,   332,   172,   119,   109,   294,   438,
     105,   344,   295,   353,   153,   332,   255,   156,   161,   332,
     333,     8,    97,   362,   145,   332,    94,   364,   366,   354,
     405,   363,   164,   431,   123,   166,   373,   409,   367,   482,
     353,   435,   413,   133,   268,    98,   353,   127,    94,   128,
     331,   174,   231,   146,   147,   148,   439,   380,   321,   322,
     134,   245,   459,   135,   252,   247,   173,   455,   159,   136,
     457,   291,   381,   292,   315,   316,   401,   370,   166,   260,
     166,   332,   471,   120,   121,   124,   254,   122,   419,   137,
     472,   231,   332,   432,   402,   138,    15,    16,   420,   309,
     427,   460,   332,   344,   428,   332,   366,   143,   255,   389,
     390,   391,   392,   317,   318,   336,   284,   285,   144,  -312,
     255,    15,    16,   165,  -115,    33,    34,    35,   162,   158,
      37,   323,   324,   375,   146,   147,   148,   311,   258,   466,
     -19,   430,   480,   385,   386,    15,    16,   231,   393,   394,
      33,    34,    35,   169,   126,    37,   -20,   245,   337,   250,
     441,   277,   271,   444,    11,    12,    13,   387,   388,   344,
     293,   255,   254,   350,    33,    34,    35,   260,   334,    37,
     166,    85,    86,    87,   270,   278,   174,   461,   231,   274,
     452,   275,   279,   453,   231,   344,   282,   368,   344,   231,
     154,    12,   155,   131,   287,   374,   159,   474,   130,   475,
     344,   283,   477,   479,   312,   313,   314,   -45,   344,   298,
     325,   328,   326,   140,   327,   479,   329,   -44,   347,   259,
     346,   352,    15,    16,   356,   349,   382,   383,   384,   254,
     254,   254,   254,   254,   254,   254,   254,   254,   254,   254,
     254,   254,   254,   254,   254,   358,   -39,   251,   403,   410,
     260,    33,    34,    35,   254,   126,    37,    38,    39,    40,
     407,   415,   231,   408,   411,   368,   254,   332,   422,   424,
     421,   426,   231,   425,   433,   434,   436,   440,   437,   443,
     442,   -49,   454,   463,   448,   446,   458,   395,   456,   398,
     473,   396,   338,   397,   476,   242,   399,   246,   400,   357,
     132,   359,   339,   269,   414,   462,   142,   371,   360,   478,
     416,   139,   481,     0,     0,     0,     0,   254,     0,     0,
     231,     0,     0,   231,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   348,     0,     0,     0,     0,
       0,     0,     0,     0,   231,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   231,     0,     0,     0,     0,   231,
     231,     0,     0,    15,    16,    17,    18,    19,    20,   178,
     179,   180,   231,   181,   182,   183,   184,   185,     0,     0,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,     0,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,   186,   127,    83,   128,   187,   188,   189,
     190,   191,     0,     0,   192,   193,     0,     0,     0,     0,
       0,     0,     0,     0,    17,    18,    19,    20,     0,     0,
       0,     0,     0,    84,    85,    86,    87,     0,    88,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    94,     0,     0,     0,     0,     0,     0,     0,     0,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,     0,     0,    83,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   194,     0,     0,     0,     0,     0,   195,   196,   197,
     198,     0,     0,    85,    86,    87,     0,     0,     0,     0,
       0,     0,   199,   200,   201,    15,    16,    17,    18,    19,
      20,   178,   179,   180,     0,   181,   182,   183,   184,   185,
       0,     0,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,     0,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,   186,   127,    83,   128,   187,
     188,   189,   190,   191,     0,     0,   192,   193,     0,     0,
       0,     0,     0,   267,     0,     0,    17,    18,    19,    20,
       0,     0,     0,     0,     0,    84,    85,    86,    87,     0,
      88,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    94,     0,     0,     0,     0,     0,     0,
       0,     0,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,     0,     0,    83,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   194,     0,     0,     0,     0,     0,   195,
     196,   197,   198,     0,     0,    85,    86,    87,     0,     0,
       0,     0,     0,     0,   199,   200,   335,    15,    16,    17,
      18,    19,    20,   178,   179,   180,     0,   181,   182,   183,
     184,   185,     0,     0,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,     0,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,   186,   127,    83,
     128,   187,   188,   189,   190,   191,     0,     0,   192,   193,
       0,     0,     0,     0,     0,   355,     0,     0,    17,    18,
      19,    20,     0,     0,     0,     0,     0,    84,    85,    86,
      87,     0,    88,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    94,     0,     0,     0,     0,
       0,     0,     0,     0,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,     0,     0,    83,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   194,     0,     0,     0,     0,
       0,   195,   196,   197,   198,     0,     0,    85,    86,    87,
       0,     0,     0,     0,     0,     0,   199,   200,   423,    15,
      16,    17,    18,    19,    20,   178,   179,   180,     0,   181,
     182,   183,   184,   185,   464,   465,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,     0,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,   186,
     127,    83,   128,   187,   188,   189,   190,   191,     0,     0,
     192,   193,     0,     0,     0,     0,     0,     0,     0,     0,
      17,    18,    19,    20,     0,     0,     0,     0,     0,    84,
      85,    86,    87,     0,    88,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    94,     0,     0,
       0,     0,     0,     0,     0,     0,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,     0,     0,
      83,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   194,     0,     0,
       0,     0,     0,   195,   196,   197,   198,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   199,   200,
      15,    16,    17,    18,    19,    20,   178,   179,   180,     0,
     181,   182,   183,   184,   185,     0,     0,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,     0,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
     186,   127,    83,   128,   187,   188,   189,   190,   191,     0,
       0,   192,   193,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      84,    85,    86,    87,     0,    88,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    94,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   194,     0,
       0,     0,     0,     0,   195,   196,   197,   198,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   199,
     200,    15,    16,    17,    18,    19,    20,   178,   179,   180,
       0,   181,   182,   183,   184,   185,     0,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,     0,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,   186,   127,    83,   128,   187,   188,   189,   190,   191,
       0,     0,   192,   193,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    84,    85,    86,    87,     0,    88,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    94,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   194,
       0,     0,     0,     0,     0,   195,   196,   197,   198,    15,
      16,    17,    18,    19,    20,     0,     0,     0,     0,     0,
     199,   141,     0,     0,     0,     0,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,     0,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,     0,
     127,    83,   128,   187,   188,   189,   190,   191,     0,     0,
     192,   193,     0,     0,     0,     0,     0,     0,     0,     0,
      17,    18,    19,    20,     0,     0,     0,     0,     0,    84,
      85,    86,    87,     0,    88,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    94,     0,     0,
       0,     0,     0,     0,     0,     0,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,     0,   127,
      83,   128,   187,   188,   189,   190,   191,     0,     0,   192,
     193,     0,     0,     0,     0,     0,     0,   194,     0,     0,
       0,     0,     0,   195,   196,   197,   198,     0,     0,    85,
      86,    87,     0,     0,     0,     0,     0,     0,   199,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   194,     0,     0,     0,
       0,     0,   195,   196,   197,   198,    15,    16,    17,    18,
      19,    20,     0,     0,     0,     0,     0,   280,     0,     0,
       0,     0,     0,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,     0,   126,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,     0,   127,    83,   128,
     187,   188,   189,   190,   191,     0,     0,   192,   193,     0,
       0,     0,     0,     0,     0,    17,    18,    19,    20,     0,
       0,     0,     0,     0,     0,     0,   365,    85,    86,    87,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,     0,     0,    94,     0,     0,     0,     0,     0,
       0,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,     0,   127,    83,   128,   187,   188,   189,
     190,   191,     0,     0,   192,   193,     0,     0,     0,     0,
       0,     0,     0,     0,   194,    17,    18,    19,    20,     0,
     195,   196,   197,   198,    85,    86,    87,     0,     0,     0,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,     0,   127,    83,   128,   187,   188,   189,
     190,   191,     0,     0,   192,   193,     0,     0,     0,     0,
       0,   194,     0,     0,   253,     0,     0,   195,   196,   197,
     198,     0,     0,     0,    85,    86,    87,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    17,    18,    19,    20,
       0,   194,     0,     0,   342,     0,     0,   195,   196,   197,
     198,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,     0,   127,    83,   128,   187,   188,
     189,   190,   191,     0,     0,   192,   193,     0,     0,     0,
       0,     0,     0,    17,    18,    19,    20,     0,     0,     0,
       0,     0,     0,     0,     0,    85,    86,    87,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,     0,   127,    83,   128,   187,   188,   189,   190,   191,
       0,     0,   192,   193,     0,     0,     0,     0,     0,     0,
       0,     0,   194,     0,     0,   404,     0,     0,   195,   196,
     197,   198,    85,    86,    87,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    17,    18,    19,    20,     0,   194,
       0,     0,     0,     0,     0,   195,   196,   197,   198,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,   296,     0,   127,    83,   128,   187,   188,   189,   190,
     191,     0,     0,   192,   193,     0,     0,     0,     0,     0,
       0,    17,    18,    19,    20,     0,     0,     0,     0,     0,
       0,     0,     0,    85,    86,    87,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,   429,     0,
     127,    83,   128,   187,   188,   189,   190,   191,     0,     0,
     192,   193,     0,     0,     0,     0,     0,     0,     0,     0,
     194,     0,     0,     0,     0,     0,   195,   196,   197,   198,
      85,    86,    87,    15,    16,    17,    18,    19,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,     0,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,     0,     0,    83,     0,   194,     0,     0,
       0,     0,     0,   195,   196,   197,   198,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    84,    85,    86,    87,     0,    88,     0,
       0,     0,     0,     0,     0,     0,    89,    90,    91,    92,
      93,    94
};

static const yytype_int16 yycheck[] =
{
       9,     8,    84,     9,   180,    84,     4,    84,    86,   184,
     162,    36,   101,    18,    19,     9,    18,    19,   248,   194,
      96,    97,   167,   105,   106,   104,    84,    85,    86,   350,
      84,    85,    86,    36,   263,    33,    34,    35,     9,   113,
     283,   203,    94,    95,   119,   202,     3,     4,   130,   206,
     422,    88,   210,     0,   203,   341,   222,   223,   132,   221,
     236,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   443,   221,   123,    81,    32,    33,    34,   206,    36,
      37,    38,    39,    40,   206,   139,   122,    96,   202,   410,
      96,   249,   206,   206,   222,   206,   248,   104,   107,   206,
     222,   120,    96,   278,     4,   206,   131,   282,   283,   222,
     340,   222,   119,   220,    86,   124,   291,   346,   361,   220,
     206,   407,   352,   123,   269,    96,   206,    84,   131,    86,
     219,   138,   141,    33,    34,    35,   222,   295,   214,   215,
     123,   150,   222,   123,   222,   152,   200,   433,   230,   123,
     436,   203,   310,   205,   207,   208,   331,   202,   167,   165,
     169,   206,   448,    33,    34,   223,   162,    37,   202,   123,
     456,   180,   206,   403,   332,   201,     3,     4,   202,   221,
     202,   202,   206,   341,   206,   206,   361,   202,   340,   319,
     320,   321,   322,    92,    93,   371,   192,   193,   206,   224,
     352,     3,     4,   223,   202,    32,    33,    34,   203,    36,
      37,    98,    99,   292,    33,    34,    35,   213,   123,   224,
     201,   379,   224,   315,   316,     3,     4,   236,   323,   324,
      32,    33,    34,   223,    36,    37,   201,   246,   245,   201,
     415,   201,   221,   419,    84,    85,    86,   317,   318,   407,
     202,   403,   248,   262,    32,    33,    34,   263,    36,    37,
     269,   114,   115,   116,   222,   201,   273,   442,   277,   222,
     428,   222,   222,   431,   283,   433,   201,   283,   436,   288,
      84,    85,    86,   365,   224,   292,   368,   463,   365,   464,
     448,   201,   468,   469,   211,   212,   213,   201,   456,   201,
     216,   100,   217,   222,   218,   481,   102,   201,   204,    36,
     223,   203,     3,     4,    88,   224,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,    83,   202,    86,   203,   348,
     346,    32,    33,    34,   340,    36,    37,    38,    39,    40,
     221,   201,   361,   204,   222,   361,   352,   206,   202,   204,
     367,   202,   371,   201,   221,   204,   221,   204,   224,   202,
     222,   202,   204,    12,   221,   223,   222,   325,   221,   328,
     222,   326,   246,   327,   220,   144,   329,   151,   330,   273,
      88,   277,   246,   169,   353,   443,    98,   288,   277,   468,
     361,    96,   470,    -1,    -1,    -1,    -1,   403,    -1,    -1,
     419,    -1,    -1,   422,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   260,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   443,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   463,    -1,    -1,    -1,    -1,   468,
     469,    -1,    -1,     3,     4,     5,     6,     7,     8,     9,
      10,    11,   481,    13,    14,    15,    16,    17,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,
      -1,    -1,    -1,   113,   114,   115,   116,    -1,   118,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,   131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   201,    -1,    -1,    -1,    -1,    -1,   207,   208,   209,
     210,    -1,    -1,   114,   115,   116,    -1,    -1,    -1,    -1,
      -1,    -1,   222,   223,   224,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    13,    14,    15,    16,    17,
      -1,    -1,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    -1,    94,    95,    -1,    -1,
      -1,    -1,    -1,   224,    -1,    -1,     5,     6,     7,     8,
      -1,    -1,    -1,    -1,    -1,   113,   114,   115,   116,    -1,
     118,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,   131,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    85,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   201,    -1,    -1,    -1,    -1,    -1,   207,
     208,   209,   210,    -1,    -1,   114,   115,   116,    -1,    -1,
      -1,    -1,    -1,    -1,   222,   223,   224,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    13,    14,    15,
      16,    17,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    -1,    94,    95,
      -1,    -1,    -1,    -1,    -1,   224,    -1,    -1,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,   113,   114,   115,
     116,    -1,   118,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,   131,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    85,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,    -1,    -1,
      -1,   207,   208,   209,   210,    -1,    -1,   114,   115,   116,
      -1,    -1,    -1,    -1,    -1,    -1,   222,   223,   224,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    -1,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,   113,
     114,   115,   116,    -1,   118,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    -1,
      85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,
      -1,    -1,    -1,   207,   208,   209,   210,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   222,   223,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      13,    14,    15,    16,    17,    -1,    -1,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      -1,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     113,   114,   115,   116,    -1,   118,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   131,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,
      -1,    -1,    -1,    -1,   207,   208,   209,   210,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   222,
     223,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      -1,    13,    14,    15,    16,    17,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    -1,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   113,   114,   115,   116,    -1,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   131,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   201,
      -1,    -1,    -1,    -1,    -1,   207,   208,   209,   210,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
     222,   223,    -1,    -1,    -1,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    -1,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    -1,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,   113,
     114,   115,   116,    -1,   118,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    -1,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,
      -1,    -1,    -1,   207,   208,   209,   210,    -1,    -1,   114,
     115,   116,    -1,    -1,    -1,    -1,    -1,    -1,   222,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,    -1,
      -1,    -1,   207,   208,   209,   210,     3,     4,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,   222,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    -1,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   113,   114,   115,   116,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,   131,    -1,    -1,    -1,    -1,    -1,
      -1,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   201,     5,     6,     7,     8,    -1,
     207,   208,   209,   210,   114,   115,   116,    -1,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,
      -1,   201,    -1,    -1,   204,    -1,    -1,   207,   208,   209,
     210,    -1,    -1,    -1,   114,   115,   116,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
      -1,   201,    -1,    -1,   204,    -1,    -1,   207,   208,   209,
     210,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    -1,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,   115,   116,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    -1,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    -1,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   201,    -1,    -1,   204,    -1,    -1,   207,   208,
     209,   210,   114,   115,   116,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,   201,
      -1,    -1,    -1,    -1,    -1,   207,   208,   209,   210,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,   115,   116,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    -1,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    -1,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     201,    -1,    -1,    -1,    -1,    -1,   207,   208,   209,   210,
     114,   115,   116,     3,     4,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    -1,    85,    -1,   201,    -1,    -1,
      -1,    -1,    -1,   207,   208,   209,   210,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   113,   114,   115,   116,    -1,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   126,   127,   128,   129,
     130,   131
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   119,   226,   228,    88,     0,   230,   123,   120,   227,
     232,    84,    85,    86,   231,     3,     4,     5,     6,     7,
       8,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    85,   113,   114,   115,   116,   118,   126,
     127,   128,   129,   130,   131,   229,   233,   267,   268,   269,
     270,   271,   276,   277,   278,   279,   283,   285,   286,   287,
     288,   289,   290,   291,   292,   321,   322,   323,   327,   122,
      33,    34,    37,    86,   223,   231,    36,    84,    86,   234,
     283,   286,   291,   123,   123,   123,   123,   123,   201,   321,
     222,   223,   304,   202,   206,     4,    33,    34,    35,   273,
     274,   284,   206,   222,    84,    86,   231,   234,    36,   286,
     286,   287,   203,   288,   231,   223,   287,   293,   294,   223,
     286,   288,   139,   200,   231,   280,   281,   282,     9,    10,
      11,    13,    14,    15,    16,    17,    83,    87,    88,    89,
      90,    91,    94,    95,   201,   207,   208,   209,   210,   222,
     223,   224,   234,   235,   236,   238,   239,   240,   241,   242,
     243,   244,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   265,   267,   268,
     279,   287,   298,   299,   300,   301,   305,   306,   307,   310,
     316,   320,   273,   272,   275,   287,   274,   231,   203,   221,
     201,    86,   222,   204,   249,   262,   266,   287,   123,    36,
     279,   324,   325,   326,   231,   295,   296,   224,   294,   293,
     222,   221,   202,   206,   222,   222,   299,   201,   201,   222,
     222,   265,   201,   201,   249,   249,   265,   224,   302,    94,
      95,   203,   205,   202,   202,   206,    82,   263,   201,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   221,
     264,   249,   211,   212,   213,   207,   208,    92,    93,    96,
      97,   214,   215,    98,    99,   216,   217,   218,   100,   102,
     101,   219,   206,   222,    36,   224,   299,   231,   272,   275,
     203,   221,   204,   266,   263,   297,   223,   204,   325,   224,
     287,   324,   203,   206,   222,   224,    88,   281,    83,   298,
     306,   317,   265,   222,   265,   113,   265,   278,   279,   309,
     202,   305,   237,   265,   231,   234,   245,   246,   247,   248,
     263,   263,   249,   249,   249,   251,   251,   252,   252,   253,
     253,   253,   253,   254,   254,   255,   256,   257,   258,   259,
     260,   265,   263,   203,   204,   266,   297,   221,   204,   324,
     287,   222,   295,   266,   296,   201,   309,   318,   319,   202,
     202,   231,   202,   224,   204,   201,   202,   202,   206,    82,
     263,   220,   266,   221,   204,   297,   221,   224,   295,   222,
     204,   265,   222,   202,   299,   308,   223,   311,   221,   300,
     303,   304,   263,   263,   204,   297,   221,   297,   222,   222,
     202,   265,   303,    12,    18,    19,   224,   312,   313,   314,
     315,   297,   297,   222,   299,   265,   220,   299,   312,   299,
     224,   314,   220
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   225,   227,   226,   228,   228,   229,   229,   229,   229,
     229,   230,   230,   231,   231,   231,   232,   233,   233,   234,
     234,   235,   235,   235,   235,   235,   235,   236,   236,   236,
     236,   236,   236,   237,   238,   239,   239,   240,   240,   241,
     241,   242,   242,   243,   244,   244,   244,   245,   245,   246,
     246,   247,   247,   248,   249,   249,   249,   249,   250,   250,
     250,   250,   251,   251,   251,   251,   252,   252,   252,   253,
     253,   253,   254,   254,   254,   254,   254,   255,   255,   255,
     256,   256,   257,   257,   258,   258,   259,   259,   260,   260,
     261,   261,   262,   262,   263,   263,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   265,   265,   266,
     267,   267,   267,   267,   268,   269,   269,   270,   270,   271,
     272,   272,   273,   273,   273,   273,   274,   274,   274,   274,
     275,   276,   276,   276,   276,   276,   276,   276,   277,   277,
     277,   277,   277,   277,   277,   277,   278,   278,   279,   280,
     280,   281,   281,   281,   282,   282,   283,   283,   283,   284,
     285,   285,   285,   285,   285,   285,   285,   285,   286,   286,
     286,   286,   286,   286,   286,   286,   286,   287,   287,   288,
     288,   288,   289,   289,   289,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   291,   291,   291,   292,   292,   293,   293,   294,
     295,   295,   296,   296,   297,   298,   299,   299,   300,   300,
     300,   300,   300,   300,   301,   302,   301,   303,   303,   304,
     304,   305,   305,   306,   306,   307,   308,   308,   309,   309,
     310,   311,   311,   312,   312,   313,   313,   314,   314,   315,
     315,   316,   316,   316,   317,   317,   318,   318,   319,   319,
     320,   320,   320,   320,   320,   321,   321,   321,   321,   322,
     323,   323,   324,   324,   325,   325,   326,   326,   327
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     3,     2,     2,     2,     2,
       2,     0,     2,     1,     1,     1,     5,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     4,     1,
       3,     2,     2,     1,     1,     1,     3,     2,     2,     2,
       1,     2,     3,     2,     1,     1,     1,     2,     2,     2,
       1,     2,     3,     2,     1,     2,     2,     2,     1,     1,
       1,     1,     1,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     1,     3,     3,     3,     3,     1,     3,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     5,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     1,
       2,     2,     4,     1,     2,     1,     1,     2,     3,     3,
       2,     5,     3,     2,     3,     2,     0,     1,     1,     1,
       1,     1,     3,     5,     6,     7,     8,     5,     1,     2,
       4,     5,     6,     7,     4,     2,     1,     2,     4,     1,
       3,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     2,     2,     3,     1,     1,     1,
       1,     2,     1,     1,     2,     2,     1,     1,     2,     1,
       3,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     5,     4,     1,     2,     3,
       1,     3,     1,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     0,     4,     1,     1,     2,
       3,     1,     2,     1,     2,     5,     3,     1,     1,     4,
       5,     2,     3,     3,     2,     1,     2,     2,     2,     1,
       2,     5,     7,     6,     1,     1,     1,     0,     2,     3,
       2,     2,     2,     3,     2,     1,     1,     1,     1,     2,
       6,     7,     1,     2,     0,     1,     5,     4,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (&yylloc, state, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location, state); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct _mesa_glsl_parse_state *state)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  YYUSE (state);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct _mesa_glsl_parse_state *state)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, state);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, struct _mesa_glsl_parse_state *state)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       , state);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, state); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, struct _mesa_glsl_parse_state *state)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (state);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct _mesa_glsl_parse_state *state)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

/* User initialization code.  */
#line 48 "glsl_parser.yy" /* yacc.c:1429  */
{
   yylloc.first_line = 1;
   yylloc.first_column = 1;
   yylloc.last_line = 1;
   yylloc.last_column = 1;
   yylloc.source = 0;
}

#line 2606 "glsl_parser.cc" /* yacc.c:1429  */
  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);

        yyls = yyls1;
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, &yylloc, YYLEX_PARAM);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 243 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   _mesa_glsl_initialize_types(state);
	}
#line 2797 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 3:
#line 247 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   delete state->symbols;
	   state->symbols = new(ralloc_parent(state)) glsl_symbol_table;
	   _mesa_glsl_initialize_types(state);
	}
#line 2807 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 5:
#line 257 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   bool supported = false;

	   switch ((yyvsp[-1].n)) {
	   case 100:
	      state->es_shader = true;
	      supported = state->ctx->API == API_OPENGLES2 ||
		          state->ctx->Extensions.ARB_ES2_compatibility;
	      break;
	   case 110:
	   case 120:
	      /* FINISHME: Once the OpenGL 3.0 'forward compatible' context or
	       * the OpenGL 3.2 Core context is supported, this logic will need
	       * change.  Older versions of GLSL are no longer supported
	       * outside the compatibility contexts of 3.x.
	       */
	   case 130:
	   case 140:
	   case 150:
	   case 330:
	   case 400:
	   case 410:
	   case 420:
	      supported = _mesa_is_desktop_gl(state->ctx) &&
			  ((unsigned) (yyvsp[-1].n)) <= state->ctx->Const.GLSLVersion;
	      break;
	   default:
	      supported = false;
	      break;
	   }

	   state->language_version = (yyvsp[-1].n);
	   state->version_string =
	      ralloc_asprintf(state, "GLSL%s %d.%02d",
			      state->es_shader ? " ES" : "",
			      state->language_version / 100,
			      state->language_version % 100);

	   if (!supported) {
	      _mesa_glsl_error(& (yylsp[-1]), state, "%s is not supported. "
			       "Supported versions are: %s\n",
			       state->version_string,
			       state->supported_version_string);
	   }

	   if (state->language_version >= 140) {
	      state->ARB_uniform_buffer_object_enable = true;
	   }
	}
#line 2861 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 10:
#line 314 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   if (state->language_version == 110) {
	      _mesa_glsl_warning(& (yylsp[-1]), state,
				 "pragma `invariant(all)' not supported in %s",
				 state->version_string);
	   } else {
	      state->all_invariant = true;
	   }
	}
#line 2875 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 16:
#line 338 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   if (!_mesa_glsl_process_extension((yyvsp[-3].identifier), & (yylsp[-3]), (yyvsp[-1].identifier), & (yylsp[-1]), state)) {
	      YYERROR;
	   }
	}
#line 2885 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 17:
#line 347 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   /* FINISHME: The NULL test is required because pragmas are set to
	    * FINISHME: NULL. (See production rule for external_declaration.)
	    */
	   if ((yyvsp[0].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[0].node)->link);
	}
#line 2897 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 18:
#line 355 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   /* FINISHME: The NULL test is required because pragmas are set to
	    * FINISHME: NULL. (See production rule for external_declaration.)
	    */
	   if ((yyvsp[0].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[0].node)->link);
	}
#line 2909 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 21:
#line 371 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[0].identifier);
	}
#line 2920 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 22:
#line 378 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.int_constant = (yyvsp[0].n);
	}
#line 2931 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 23:
#line 385 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.uint_constant = (yyvsp[0].n);
	}
#line 2942 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 24:
#line 392 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.float_constant = (yyvsp[0].real);
	}
#line 2953 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 25:
#line 399 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.bool_constant = (yyvsp[0].n);
	}
#line 2964 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 26:
#line 406 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.expression) = (yyvsp[-1].expression);
	}
#line 2972 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 28:
#line 414 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_array_index, (yyvsp[-3].expression), (yyvsp[-1].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	}
#line 2982 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 29:
#line 420 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.expression) = (yyvsp[0].expression);
	}
#line 2990 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 30:
#line 424 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[-2].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[0].identifier);
	}
#line 3001 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 31:
#line 431 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_inc, (yyvsp[-1].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
#line 3011 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 32:
#line 437 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_dec, (yyvsp[-1].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
#line 3021 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 36:
#line 455 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[-2].expression), (yyvsp[0].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	}
#line 3031 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 41:
#line 474 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.expression) = (yyvsp[-1].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
	}
#line 3041 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 42:
#line 480 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.expression) = (yyvsp[-2].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
	}
#line 3051 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 44:
#line 496 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_function_expression((yyvsp[0].type_specifier));
	   (yyval.expression)->set_location(yylloc);
   	}
#line 3061 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 45:
#line 502 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[0].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	}
#line 3072 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 46:
#line 509 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[0].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	}
#line 3083 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 51:
#line 529 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.expression) = (yyvsp[-1].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
	}
#line 3093 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 52:
#line 535 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.expression) = (yyvsp[-2].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
	}
#line 3103 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 53:
#line 547 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[-1].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	}
#line 3114 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 55:
#line 559 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_inc, (yyvsp[0].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
#line 3124 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 56:
#line 565 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_dec, (yyvsp[0].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
#line 3134 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 57:
#line 571 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[-1].n), (yyvsp[0].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
#line 3144 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 58:
#line 580 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_plus; }
#line 3150 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 59:
#line 581 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_neg; }
#line 3156 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 60:
#line 582 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_logic_not; }
#line 3162 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 61:
#line 583 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_bit_not; }
#line 3168 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 63:
#line 589 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mul, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3178 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 64:
#line 595 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_div, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3188 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 65:
#line 601 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mod, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3198 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 67:
#line 611 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_add, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3208 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 68:
#line 617 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_sub, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3218 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 70:
#line 627 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lshift, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3228 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 71:
#line 633 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_rshift, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3238 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 73:
#line 643 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_less, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3248 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 74:
#line 649 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_greater, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3258 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 75:
#line 655 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lequal, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3268 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 76:
#line 661 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_gequal, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3278 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 78:
#line 671 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_equal, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3288 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 79:
#line 677 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_nequal, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3298 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 81:
#line 687 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_and, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3308 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 83:
#line 697 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_xor, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3318 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 85:
#line 707 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3328 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 87:
#line 717 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_and, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3338 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 89:
#line 727 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_xor, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3348 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 91:
#line 737 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_or, (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3358 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 93:
#line 747 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_conditional, (yyvsp[-4].expression), (yyvsp[-2].expression), (yyvsp[0].expression));
	   (yyval.expression)->set_location(yylloc);
	}
#line 3368 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 95:
#line 757 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[-1].n), (yyvsp[-2].expression), (yyvsp[0].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	}
#line 3378 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 96:
#line 765 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_assign; }
#line 3384 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 97:
#line 766 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_mul_assign; }
#line 3390 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 98:
#line 767 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_div_assign; }
#line 3396 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 99:
#line 768 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_mod_assign; }
#line 3402 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 100:
#line 769 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_add_assign; }
#line 3408 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 101:
#line 770 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_sub_assign; }
#line 3414 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 102:
#line 771 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_ls_assign; }
#line 3420 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 103:
#line 772 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_rs_assign; }
#line 3426 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 104:
#line 773 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_and_assign; }
#line 3432 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 105:
#line 774 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_xor_assign; }
#line 3438 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 106:
#line 775 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.n) = ast_or_assign; }
#line 3444 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 107:
#line 780 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.expression) = (yyvsp[0].expression);
	}
#line 3452 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 108:
#line 784 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   if ((yyvsp[-2].expression)->oper != ast_sequence) {
	      (yyval.expression) = new(ctx) ast_expression(ast_sequence, NULL, NULL, NULL);
	      (yyval.expression)->set_location(yylloc);
	      (yyval.expression)->expressions.push_tail(& (yyvsp[-2].expression)->link);
	   } else {
	      (yyval.expression) = (yyvsp[-2].expression);
	   }

	   (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
	}
#line 3469 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 110:
#line 804 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   state->symbols->pop_scope();
	   (yyval.node) = (yyvsp[-1].function);
	}
#line 3478 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 111:
#line 809 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.node) = (yyvsp[-1].declarator_list);
	}
#line 3486 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 112:
#line 813 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyvsp[-1].type_specifier)->precision = (yyvsp[-2].n);
	   (yyvsp[-1].type_specifier)->is_precision_statement = true;
	   (yyval.node) = (yyvsp[-1].type_specifier);
	}
#line 3496 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 113:
#line 819 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.node) = (yyvsp[0].node);
	}
#line 3504 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 117:
#line 835 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.function) = (yyvsp[-1].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[0].parameter_declarator)->link);
	}
#line 3513 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 118:
#line 840 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.function) = (yyvsp[-2].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[0].parameter_declarator)->link);
	}
#line 3522 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 119:
#line 848 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.function) = new(ctx) ast_function();
	   (yyval.function)->set_location(yylloc);
	   (yyval.function)->return_type = (yyvsp[-2].fully_specified_type);
	   (yyval.function)->identifier = (yyvsp[-1].identifier);

	   state->symbols->add_function(new(state) ir_function((yyvsp[-1].identifier)));
	   state->symbols->push_scope();
	}
#line 3537 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 120:
#line 862 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->set_location(yylloc);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[-1].type_specifier);
	   (yyval.parameter_declarator)->identifier = (yyvsp[0].identifier);
	}
#line 3551 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 121:
#line 872 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->set_location(yylloc);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[-4].type_specifier);
	   (yyval.parameter_declarator)->identifier = (yyvsp[-3].identifier);
	   (yyval.parameter_declarator)->is_array = true;
	   (yyval.parameter_declarator)->array_size = (yyvsp[-1].expression);
	}
#line 3567 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 122:
#line 887 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyvsp[-2].type_qualifier).flags.i |= (yyvsp[-1].type_qualifier).flags.i;

	   (yyval.parameter_declarator) = (yyvsp[0].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[-2].type_qualifier);
	}
#line 3578 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 123:
#line 894 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.parameter_declarator) = (yyvsp[0].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[-1].type_qualifier);
	}
#line 3587 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 124:
#line 899 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyvsp[-2].type_qualifier).flags.i |= (yyvsp[-1].type_qualifier).flags.i;

	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[-2].type_qualifier);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[0].type_specifier);
	}
#line 3602 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 125:
#line 910 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[-1].type_qualifier);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[0].type_specifier);
	}
#line 3615 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 126:
#line 922 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	}
#line 3623 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 127:
#line 926 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	}
#line 3632 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 128:
#line 931 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.out = 1;
	}
#line 3641 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 129:
#line 936 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	   (yyval.type_qualifier).flags.q.out = 1;
	}
#line 3651 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 132:
#line 950 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[0].identifier), false, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[-2].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[0].identifier), ir_var_auto));
	}
#line 3665 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 133:
#line 960 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-2].identifier), true, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[-4].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[-2].identifier), ir_var_auto));
	}
#line 3679 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 134:
#line 970 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-3].identifier), true, (yyvsp[-1].expression), NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[-5].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[-3].identifier), ir_var_auto));
	}
#line 3693 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 135:
#line 980 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-4].identifier), true, NULL, (yyvsp[0].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[-6].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[-4].identifier), ir_var_auto));
	}
#line 3707 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 136:
#line 990 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-5].identifier), true, (yyvsp[-3].expression), (yyvsp[0].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[-7].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[-5].identifier), ir_var_auto));
	}
#line 3721 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 137:
#line 1000 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-2].identifier), false, NULL, (yyvsp[0].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[-4].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[-2].identifier), ir_var_auto));
	}
#line 3735 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 138:
#line 1014 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   /* Empty declaration list is valid. */
	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[0].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	}
#line 3746 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 139:
#line 1021 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[0].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-1].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
#line 3759 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 140:
#line 1030 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-2].identifier), true, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-3].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
#line 3772 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 141:
#line 1039 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-3].identifier), true, (yyvsp[-1].expression), NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-4].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
#line 3785 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 142:
#line 1048 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-4].identifier), true, NULL, (yyvsp[0].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-5].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
#line 3798 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 143:
#line 1057 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-5].identifier), true, (yyvsp[-3].expression), (yyvsp[0].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-6].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
#line 3811 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 144:
#line 1066 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-2].identifier), false, NULL, (yyvsp[0].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-3].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
#line 3824 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 145:
#line 1075 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[0].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->invariant = true;

	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
#line 3839 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 146:
#line 1089 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->specifier = (yyvsp[0].type_specifier);
	}
#line 3850 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 147:
#line 1096 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->qualifier = (yyvsp[-1].type_qualifier);
	   (yyval.fully_specified_type)->specifier = (yyvsp[0].type_specifier);
	}
#line 3862 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 148:
#line 1107 "glsl_parser.yy" /* yacc.c:1646  */
    {
	  (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
	}
#line 3870 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 150:
#line 1115 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_qualifier) = (yyvsp[-2].type_qualifier);
	   if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[0]), state, (yyvsp[0].type_qualifier))) {
	      YYERROR;
	   }
	}
#line 3881 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 151:
#line 1125 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));

	   /* Layout qualifiers for ARB_fragment_coord_conventions. */
	   if (!(yyval.type_qualifier).flags.i && state->ARB_fragment_coord_conventions_enable) {
	      if (strcmp((yyvsp[0].identifier), "origin_upper_left") == 0) {
		 (yyval.type_qualifier).flags.q.origin_upper_left = 1;
	      } else if (strcmp((yyvsp[0].identifier), "pixel_center_integer") == 0) {
		 (yyval.type_qualifier).flags.q.pixel_center_integer = 1;
	      }

	      if ((yyval.type_qualifier).flags.i && state->ARB_fragment_coord_conventions_warn) {
		 _mesa_glsl_warning(& (yylsp[0]), state,
				    "GL_ARB_fragment_coord_conventions layout "
				    "identifier `%s' used\n", (yyvsp[0].identifier));
	      }
	   }

	   /* Layout qualifiers for AMD/ARB_conservative_depth. */
	   if (!(yyval.type_qualifier).flags.i &&
	       (state->AMD_conservative_depth_enable ||
	        state->ARB_conservative_depth_enable)) {
	      if (strcmp((yyvsp[0].identifier), "depth_any") == 0) {
	         (yyval.type_qualifier).flags.q.depth_any = 1;
	      } else if (strcmp((yyvsp[0].identifier), "depth_greater") == 0) {
	         (yyval.type_qualifier).flags.q.depth_greater = 1;
	      } else if (strcmp((yyvsp[0].identifier), "depth_less") == 0) {
	         (yyval.type_qualifier).flags.q.depth_less = 1;
	      } else if (strcmp((yyvsp[0].identifier), "depth_unchanged") == 0) {
	         (yyval.type_qualifier).flags.q.depth_unchanged = 1;
	      }
	
	      if ((yyval.type_qualifier).flags.i && state->AMD_conservative_depth_warn) {
	         _mesa_glsl_warning(& (yylsp[0]), state,
	                            "GL_AMD_conservative_depth "
	                            "layout qualifier `%s' is used\n", (yyvsp[0].identifier));
	      }
	      if ((yyval.type_qualifier).flags.i && state->ARB_conservative_depth_warn) {
	         _mesa_glsl_warning(& (yylsp[0]), state,
	                            "GL_ARB_conservative_depth "
	                            "layout qualifier `%s' is used\n", (yyvsp[0].identifier));
	      }
	   }

	   /* See also uniform_block_layout_qualifier. */
	   if (!(yyval.type_qualifier).flags.i && state->ARB_uniform_buffer_object_enable) {
	      if (strcmp((yyvsp[0].identifier), "std140") == 0) {
	         (yyval.type_qualifier).flags.q.std140 = 1;
	      } else if (strcmp((yyvsp[0].identifier), "shared") == 0) {
	         (yyval.type_qualifier).flags.q.shared = 1;
	      } else if (strcmp((yyvsp[0].identifier), "column_major") == 0) {
	         (yyval.type_qualifier).flags.q.column_major = 1;
	      }

	      if ((yyval.type_qualifier).flags.i && state->ARB_uniform_buffer_object_warn) {
	         _mesa_glsl_warning(& (yylsp[0]), state,
	                            "#version 140 / GL_ARB_uniform_buffer_object "
	                            "layout qualifier `%s' is used\n", (yyvsp[0].identifier));
	      }
	   }

	   if (!(yyval.type_qualifier).flags.i) {
	      _mesa_glsl_error(& (yylsp[0]), state, "unrecognized layout identifier "
			       "`%s'\n", (yyvsp[0].identifier));
	      YYERROR;
	   }
	}
#line 3953 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 152:
#line 1193 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));

	   if (state->ARB_explicit_attrib_location_enable) {
	      /* FINISHME: Handle 'index' once GL_ARB_blend_func_exteneded and
	       * FINISHME: GLSL 1.30 (or later) are supported.
	       */
	      if (strcmp("location", (yyvsp[-2].identifier)) == 0) {
		 (yyval.type_qualifier).flags.q.explicit_location = 1;

		 if ((yyvsp[0].n) >= 0) {
		    (yyval.type_qualifier).location = (yyvsp[0].n);
		 } else {
		    _mesa_glsl_error(& (yylsp[0]), state,
				     "invalid location %d specified\n", (yyvsp[0].n));
		    YYERROR;
		 }
	      }

	      if (strcmp("index", (yyvsp[-2].identifier)) == 0) {
		 (yyval.type_qualifier).flags.q.explicit_index = 1;

		 if ((yyvsp[0].n) >= 0) {
		    (yyval.type_qualifier).index = (yyvsp[0].n);
		 } else {
		    _mesa_glsl_error(& (yylsp[0]), state,
		                     "invalid index %d specified\n", (yyvsp[0].n));
                    YYERROR;
                 }
              }
	   }

	   /* If the identifier didn't match any known layout identifiers,
	    * emit an error.
	    */
	   if (!(yyval.type_qualifier).flags.i) {
	      _mesa_glsl_error(& (yylsp[-2]), state, "unrecognized layout identifier "
			       "`%s'\n", (yyvsp[-2].identifier));
	      YYERROR;
	   } else if (state->ARB_explicit_attrib_location_warn) {
	      _mesa_glsl_warning(& (yylsp[-2]), state,
				 "GL_ARB_explicit_attrib_location layout "
				 "identifier `%s' used\n", (yyvsp[-2].identifier));
	   }
	}
#line 4003 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 153:
#line 1239 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
	   /* Layout qualifiers for ARB_uniform_buffer_object. */
	   if (!state->ARB_uniform_buffer_object_enable) {
	      _mesa_glsl_error(& (yylsp[0]), state,
			       "#version 140 / GL_ARB_uniform_buffer_object "
			       "layout qualifier `%s' is used\n", (yyvsp[0].type_qualifier));
	   } else if (state->ARB_uniform_buffer_object_warn) {
	      _mesa_glsl_warning(& (yylsp[0]), state,
				 "#version 140 / GL_ARB_uniform_buffer_object "
				 "layout qualifier `%s' is used\n", (yyvsp[0].type_qualifier));
	   }
	}
#line 4021 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 154:
#line 1261 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.row_major = 1;
	}
#line 4030 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 155:
#line 1266 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.packed = 1;
	}
#line 4039 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 156:
#line 1274 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.smooth = 1;
	}
#line 4048 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 157:
#line 1279 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.flat = 1;
	}
#line 4057 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 158:
#line 1284 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.noperspective = 1;
	}
#line 4066 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 159:
#line 1292 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.constant = 1;
	}
#line 4075 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 162:
#line 1302 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[0].type_qualifier).flags.i;
	}
#line 4084 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 164:
#line 1308 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[0].type_qualifier).flags.i;
	}
#line 4093 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 165:
#line 1313 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
	   (yyval.type_qualifier).flags.q.invariant = 1;
	}
#line 4102 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 166:
#line 1318 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[0].type_qualifier).flags.i;
	   (yyval.type_qualifier).flags.q.invariant = 1;
	}
#line 4112 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 167:
#line 1324 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.invariant = 1;
	}
#line 4121 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 168:
#line 1332 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.constant = 1;
	}
#line 4130 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 169:
#line 1337 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.attribute = 1;
	}
#line 4139 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 170:
#line 1342 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.varying = 1;
	}
#line 4148 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 171:
#line 1347 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1;
	   (yyval.type_qualifier).flags.q.varying = 1;
	}
#line 4158 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 172:
#line 1353 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	}
#line 4167 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 173:
#line 1358 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.out = 1;
	}
#line 4176 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 174:
#line 1363 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1; (yyval.type_qualifier).flags.q.in = 1;
	}
#line 4185 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 175:
#line 1368 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1; (yyval.type_qualifier).flags.q.out = 1;
	}
#line 4194 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 176:
#line 1373 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.uniform = 1;
	}
#line 4203 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 177:
#line 1381 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_specifier) = (yyvsp[0].type_specifier);
	}
#line 4211 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 178:
#line 1385 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_specifier) = (yyvsp[0].type_specifier);
	   (yyval.type_specifier)->precision = (yyvsp[-1].n);
	}
#line 4220 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 180:
#line 1394 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_specifier) = (yyvsp[-2].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = NULL;
	}
#line 4230 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 181:
#line 1400 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.type_specifier) = (yyvsp[-3].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = (yyvsp[-1].expression);
	}
#line 4240 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 182:
#line 1409 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[0].identifier));
	   (yyval.type_specifier)->set_location(yylloc);
	}
#line 4250 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 183:
#line 1415 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[0].struct_specifier));
	   (yyval.type_specifier)->set_location(yylloc);
	}
#line 4260 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 184:
#line 1421 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[0].identifier));
	   (yyval.type_specifier)->set_location(yylloc);
	}
#line 4270 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 185:
#line 1429 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "void"; }
#line 4276 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 186:
#line 1430 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "float"; }
#line 4282 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 187:
#line 1431 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "int"; }
#line 4288 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 188:
#line 1432 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "uint"; }
#line 4294 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 189:
#line 1433 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "bool"; }
#line 4300 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 190:
#line 1434 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "vec2"; }
#line 4306 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 191:
#line 1435 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "vec3"; }
#line 4312 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 192:
#line 1436 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "vec4"; }
#line 4318 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 193:
#line 1437 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "bvec2"; }
#line 4324 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 194:
#line 1438 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "bvec3"; }
#line 4330 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 195:
#line 1439 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "bvec4"; }
#line 4336 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 196:
#line 1440 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "ivec2"; }
#line 4342 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 197:
#line 1441 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "ivec3"; }
#line 4348 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 198:
#line 1442 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "ivec4"; }
#line 4354 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 199:
#line 1443 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "uvec2"; }
#line 4360 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 200:
#line 1444 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "uvec3"; }
#line 4366 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 201:
#line 1445 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "uvec4"; }
#line 4372 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 202:
#line 1446 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat2"; }
#line 4378 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 203:
#line 1447 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat2x3"; }
#line 4384 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 204:
#line 1448 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat2x4"; }
#line 4390 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 205:
#line 1449 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat3x2"; }
#line 4396 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 206:
#line 1450 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat3"; }
#line 4402 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 207:
#line 1451 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat3x4"; }
#line 4408 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 208:
#line 1452 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat4x2"; }
#line 4414 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 209:
#line 1453 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat4x3"; }
#line 4420 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 210:
#line 1454 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "mat4"; }
#line 4426 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 211:
#line 1455 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler1D"; }
#line 4432 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 212:
#line 1456 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler2D"; }
#line 4438 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 213:
#line 1457 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler2DRect"; }
#line 4444 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 214:
#line 1458 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler3D"; }
#line 4450 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 215:
#line 1459 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "samplerCube"; }
#line 4456 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 216:
#line 1460 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "samplerExternalOES"; }
#line 4462 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 217:
#line 1461 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler1DShadow"; }
#line 4468 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 218:
#line 1462 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler2DShadow"; }
#line 4474 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 219:
#line 1463 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler2DRectShadow"; }
#line 4480 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 220:
#line 1464 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "samplerCubeShadow"; }
#line 4486 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 221:
#line 1465 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler1DArray"; }
#line 4492 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 222:
#line 1466 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler2DArray"; }
#line 4498 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 223:
#line 1467 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler1DArrayShadow"; }
#line 4504 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 224:
#line 1468 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "sampler2DArrayShadow"; }
#line 4510 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 225:
#line 1469 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "samplerBuffer"; }
#line 4516 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 226:
#line 1470 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "isampler1D"; }
#line 4522 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 227:
#line 1471 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "isampler2D"; }
#line 4528 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 228:
#line 1472 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "isampler2DRect"; }
#line 4534 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 229:
#line 1473 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "isampler3D"; }
#line 4540 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 230:
#line 1474 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "isamplerCube"; }
#line 4546 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 231:
#line 1475 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "isampler1DArray"; }
#line 4552 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 232:
#line 1476 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "isampler2DArray"; }
#line 4558 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 233:
#line 1477 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "isamplerBuffer"; }
#line 4564 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 234:
#line 1478 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "usampler1D"; }
#line 4570 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 235:
#line 1479 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "usampler2D"; }
#line 4576 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 236:
#line 1480 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "usampler2DRect"; }
#line 4582 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 237:
#line 1481 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "usampler3D"; }
#line 4588 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 238:
#line 1482 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "usamplerCube"; }
#line 4594 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 239:
#line 1483 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "usampler1DArray"; }
#line 4600 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 240:
#line 1484 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "usampler2DArray"; }
#line 4606 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 241:
#line 1485 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.identifier) = "usamplerBuffer"; }
#line 4612 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 242:
#line 1489 "glsl_parser.yy" /* yacc.c:1646  */
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[0]), state,
				         "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     (yyval.n) = ast_precision_high;
		  }
#line 4627 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 243:
#line 1499 "glsl_parser.yy" /* yacc.c:1646  */
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[0]), state,
					 "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     (yyval.n) = ast_precision_medium;
		  }
#line 4642 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 244:
#line 1509 "glsl_parser.yy" /* yacc.c:1646  */
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[0]), state,
					 "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     (yyval.n) = ast_precision_low;
		  }
#line 4657 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 245:
#line 1523 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier((yyvsp[-3].identifier), (yyvsp[-1].declarator_list));
	   (yyval.struct_specifier)->set_location(yylloc);
	   state->symbols->add_type((yyvsp[-3].identifier), glsl_type::void_type);
	}
#line 4668 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 246:
#line 1530 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier(NULL, (yyvsp[-1].declarator_list));
	   (yyval.struct_specifier)->set_location(yylloc);
	}
#line 4678 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 247:
#line 1539 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.declarator_list) = (yyvsp[0].declarator_list);
	   (yyvsp[0].declarator_list)->link.self_link();
	}
#line 4687 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 248:
#line 1544 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.declarator_list) = (yyvsp[-1].declarator_list);
	   (yyval.declarator_list)->link.insert_before(& (yyvsp[0].declarator_list)->link);
	}
#line 4696 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 249:
#line 1552 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_fully_specified_type *type = new(ctx) ast_fully_specified_type();
	   type->set_location(yylloc);

	   type->specifier = (yyvsp[-2].type_specifier);
	   (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
	   (yyval.declarator_list)->set_location(yylloc);

	   (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[-1].declaration)->link);
	}
#line 4712 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 250:
#line 1567 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.declaration) = (yyvsp[0].declaration);
	   (yyvsp[0].declaration)->link.self_link();
	}
#line 4721 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 251:
#line 1572 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.declaration) = (yyvsp[-2].declaration);
	   (yyval.declaration)->link.insert_before(& (yyvsp[0].declaration)->link);
	}
#line 4730 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 252:
#line 1580 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[0].identifier), false, NULL, NULL);
	   (yyval.declaration)->set_location(yylloc);
	}
#line 4740 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 253:
#line 1586 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[-3].identifier), true, (yyvsp[-1].expression), NULL);
	   (yyval.declaration)->set_location(yylloc);
	}
#line 4750 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 256:
#line 1604 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.node) = (ast_node *) (yyvsp[0].compound_statement); }
#line 4756 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 264:
#line 1619 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	}
#line 4766 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 265:
#line 1625 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   state->symbols->push_scope();
	}
#line 4774 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 266:
#line 1629 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, (yyvsp[-1].node));
	   (yyval.compound_statement)->set_location(yylloc);
	   state->symbols->pop_scope();
	}
#line 4785 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 267:
#line 1638 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.node) = (ast_node *) (yyvsp[0].compound_statement); }
#line 4791 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 269:
#line 1644 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	}
#line 4801 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 270:
#line 1650 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, (yyvsp[-1].node));
	   (yyval.compound_statement)->set_location(yylloc);
	}
#line 4811 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 271:
#line 1659 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   if ((yyvsp[0].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[0]), state, "<nil> statement\n");
	      assert((yyvsp[0].node) != NULL);
	   }

	   (yyval.node) = (yyvsp[0].node);
	   (yyval.node)->link.self_link();
	}
#line 4825 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 272:
#line 1669 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   if ((yyvsp[0].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[0]), state, "<nil> statement\n");
	      assert((yyvsp[0].node) != NULL);
	   }
	   (yyval.node) = (yyvsp[-1].node);
	   (yyval.node)->link.insert_before(& (yyvsp[0].node)->link);
	}
#line 4838 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 273:
#line 1681 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement(NULL);
	   (yyval.node)->set_location(yylloc);
	}
#line 4848 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 274:
#line 1687 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement((yyvsp[-1].expression));
	   (yyval.node)->set_location(yylloc);
	}
#line 4858 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 275:
#line 1696 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.node) = new(state) ast_selection_statement((yyvsp[-2].expression), (yyvsp[0].selection_rest_statement).then_statement,
						   (yyvsp[0].selection_rest_statement).else_statement);
	   (yyval.node)->set_location(yylloc);
	}
#line 4868 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 276:
#line 1705 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.selection_rest_statement).then_statement = (yyvsp[-2].node);
	   (yyval.selection_rest_statement).else_statement = (yyvsp[0].node);
	}
#line 4877 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 277:
#line 1710 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.selection_rest_statement).then_statement = (yyvsp[0].node);
	   (yyval.selection_rest_statement).else_statement = NULL;
	}
#line 4886 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 278:
#line 1718 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.node) = (ast_node *) (yyvsp[0].expression);
	}
#line 4894 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 279:
#line 1722 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-2].identifier), false, NULL, (yyvsp[0].expression));
	   ast_declarator_list *declarator = new(ctx) ast_declarator_list((yyvsp[-3].fully_specified_type));
	   decl->set_location(yylloc);
	   declarator->set_location(yylloc);

	   declarator->declarations.push_tail(&decl->link);
	   (yyval.node) = declarator;
	}
#line 4909 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 280:
#line 1740 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.node) = new(state) ast_switch_statement((yyvsp[-2].expression), (yyvsp[0].switch_body));
	   (yyval.node)->set_location(yylloc);
	}
#line 4918 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 281:
#line 1748 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.switch_body) = new(state) ast_switch_body(NULL);
	   (yyval.switch_body)->set_location(yylloc);
	}
#line 4927 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 282:
#line 1753 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.switch_body) = new(state) ast_switch_body((yyvsp[-1].case_statement_list));
	   (yyval.switch_body)->set_location(yylloc);
	}
#line 4936 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 283:
#line 1761 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.case_label) = new(state) ast_case_label((yyvsp[-1].expression));
	   (yyval.case_label)->set_location(yylloc);
	}
#line 4945 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 284:
#line 1766 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.case_label) = new(state) ast_case_label(NULL);
	   (yyval.case_label)->set_location(yylloc);
	}
#line 4954 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 285:
#line 1774 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   ast_case_label_list *labels = new(state) ast_case_label_list();

	   labels->labels.push_tail(& (yyvsp[0].case_label)->link);
	   (yyval.case_label_list) = labels;
	   (yyval.case_label_list)->set_location(yylloc);
	}
#line 4966 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 286:
#line 1782 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.case_label_list) = (yyvsp[-1].case_label_list);
	   (yyval.case_label_list)->labels.push_tail(& (yyvsp[0].case_label)->link);
	}
#line 4975 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 287:
#line 1790 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   ast_case_statement *stmts = new(state) ast_case_statement((yyvsp[-1].case_label_list));
	   stmts->set_location(yylloc);

	   stmts->stmts.push_tail(& (yyvsp[0].node)->link);
	   (yyval.case_statement) = stmts;
	}
#line 4987 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 288:
#line 1798 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.case_statement) = (yyvsp[-1].case_statement);
	   (yyval.case_statement)->stmts.push_tail(& (yyvsp[0].node)->link);
	}
#line 4996 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 289:
#line 1806 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   ast_case_statement_list *cases= new(state) ast_case_statement_list();
	   cases->set_location(yylloc);

	   cases->cases.push_tail(& (yyvsp[0].case_statement)->link);
	   (yyval.case_statement_list) = cases;
	}
#line 5008 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 290:
#line 1814 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.case_statement_list) = (yyvsp[-1].case_statement_list);
	   (yyval.case_statement_list)->cases.push_tail(& (yyvsp[0].case_statement)->link);
	}
#line 5017 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 291:
#line 1822 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
	   					    NULL, (yyvsp[-2].node), NULL, (yyvsp[0].node));
	   (yyval.node)->set_location(yylloc);
	}
#line 5028 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 292:
#line 1829 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
						    NULL, (yyvsp[-2].expression), NULL, (yyvsp[-5].node));
	   (yyval.node)->set_location(yylloc);
	}
#line 5039 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 293:
#line 1836 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
						    (yyvsp[-3].node), (yyvsp[-2].for_rest_statement).cond, (yyvsp[-2].for_rest_statement).rest, (yyvsp[0].node));
	   (yyval.node)->set_location(yylloc);
	}
#line 5050 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 297:
#line 1852 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.node) = NULL;
	}
#line 5058 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 298:
#line 1859 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.for_rest_statement).cond = (yyvsp[-1].node);
	   (yyval.for_rest_statement).rest = NULL;
	}
#line 5067 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 299:
#line 1864 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.for_rest_statement).cond = (yyvsp[-2].node);
	   (yyval.for_rest_statement).rest = (yyvsp[0].expression);
	}
#line 5076 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 300:
#line 1873 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
	   (yyval.node)->set_location(yylloc);
	}
#line 5086 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 301:
#line 1879 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
	   (yyval.node)->set_location(yylloc);
	}
#line 5096 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 302:
#line 1885 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
	   (yyval.node)->set_location(yylloc);
	}
#line 5106 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 303:
#line 1891 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, (yyvsp[-1].expression));
	   (yyval.node)->set_location(yylloc);
	}
#line 5116 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 304:
#line 1897 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
	   (yyval.node)->set_location(yylloc);
	}
#line 5126 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 305:
#line 1905 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].function_definition); }
#line 5132 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 306:
#line 1906 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5138 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 307:
#line 1907 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 5144 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 308:
#line 1908 "glsl_parser.yy" /* yacc.c:1646  */
    { (yyval.node) = NULL; }
#line 5150 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 309:
#line 1913 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.function_definition) = new(ctx) ast_function_definition();
	   (yyval.function_definition)->set_location(yylloc);
	   (yyval.function_definition)->prototype = (yyvsp[-1].function);
	   (yyval.function_definition)->body = (yyvsp[0].compound_statement);

	   state->symbols->pop_scope();
	}
#line 5164 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 310:
#line 1927 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_uniform_block(*state->default_uniform_qualifier,
					   (yyvsp[-4].identifier), (yyvsp[-2].declarator_list));

	   if (!state->ARB_uniform_buffer_object_enable) {
	      _mesa_glsl_error(& (yylsp[-5]), state,
			       "#version 140 / GL_ARB_uniform_buffer_object "
			       "required for defining uniform blocks\n");
	   } else if (state->ARB_uniform_buffer_object_warn) {
	      _mesa_glsl_warning(& (yylsp[-5]), state,
				 "#version 140 / GL_ARB_uniform_buffer_object "
				 "required for defining uniform blocks\n");
	   }
	}
#line 5184 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 311:
#line 1943 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;

	   ast_type_qualifier qual = *state->default_uniform_qualifier;
	   if (!qual.merge_qualifier(& (yylsp[-6]), state, (yyvsp[-6].type_qualifier))) {
	      YYERROR;
	   }
	   (yyval.node) = new(ctx) ast_uniform_block(qual, (yyvsp[-4].identifier), (yyvsp[-2].declarator_list));

	   if (!state->ARB_uniform_buffer_object_enable) {
	      _mesa_glsl_error(& (yylsp[-6]), state,
			       "#version 140 / GL_ARB_uniform_buffer_object "
			       "required for defining uniform blocks\n");
	   } else if (state->ARB_uniform_buffer_object_warn) {
	      _mesa_glsl_warning(& (yylsp[-6]), state,
				 "#version 140 / GL_ARB_uniform_buffer_object "
				 "required for defining uniform blocks\n");
	   }
	}
#line 5208 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 312:
#line 1966 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.declarator_list) = (yyvsp[0].declarator_list);
	   (yyvsp[0].declarator_list)->link.self_link();
	}
#line 5217 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 313:
#line 1971 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   (yyval.declarator_list) = (yyvsp[-1].declarator_list);
	   (yyvsp[0].declarator_list)->link.insert_before(& (yyval.declarator_list)->link);
	}
#line 5226 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 316:
#line 1985 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_fully_specified_type *type = new(ctx) ast_fully_specified_type();
	   type->set_location(yylloc);

	   type->qualifier = (yyvsp[-4].type_qualifier);
	   type->qualifier.flags.q.uniform = true;
	   type->specifier = (yyvsp[-2].type_specifier);
	   (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->ubo_qualifiers_valid = true;

	   (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[-1].declaration)->link);
	}
#line 5245 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 317:
#line 2000 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   void *ctx = state;
	   ast_fully_specified_type *type = new(ctx) ast_fully_specified_type();
	   type->set_location(yylloc);

	   type->qualifier.flags.q.uniform = true;
	   type->specifier = (yyvsp[-2].type_specifier);
	   (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->ubo_qualifiers_valid = true;

	   (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[-1].declaration)->link);
	}
#line 5263 "glsl_parser.cc" /* yacc.c:1646  */
    break;

  case 318:
#line 2017 "glsl_parser.yy" /* yacc.c:1646  */
    {
	   if (!state->default_uniform_qualifier->merge_qualifier(& (yylsp[-2]), state,
								  (yyvsp[-2].type_qualifier))) {
	      YYERROR;
	   }
	}
#line 5274 "glsl_parser.cc" /* yacc.c:1646  */
    break;


#line 5278 "glsl_parser.cc" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, state, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (&yylloc, state, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, state);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp, state);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, state, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, state);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp, state);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
