//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

grammar Gallium;

parse
    : modularizedDeclaration* EOF
    ;

singleType
    : type EOF
    ;

MODULE_NAME
    : IDENTIFIER
    | IDENTIFIER '::' MODULE_NAME
    ;

IDENTIFIER
    : [\p{XID_Start}] [\p{XID_Continue}]*
    ;

WHITESPACE_WITHOUT_NEWLINES
    : [ \t]
    ;

WHITESPACE
    : WHITESPACE_WITHOUT_NEWLINES
    | NEWLINE
    ;

NEWLINE
    : '\n'
    | '\r\n'
    ;

modularizedDeclaration
    : importDeclaration
    | exportDeclaration
    ;

importDeclaration
    : 'import' WHITESPACE+ imported=MODULE_NAME WHITESPACE* NEWLINE
    | 'import' WHITESPACE+
    ;

identifierList
    : IDENTIFIER
    | IDENTIFIER ','
    ;

exportDeclaration
    : 'export' WHITESPACE+ exported=declaration
    ;

declaration
    : fnDecl=fnDeclaration
    | classDecl=classDeclaration
    | structDecl=structDeclaration
    | typeDecl=typeDeclaration
    ;

fnDeclaration
    : 'fn' WHITESPACE+ name=IDENTIFIER WHITESPACE+ '(' args=fnArgumentList ')' body=blockStatement
    ;

fnArgumentList
    : name=IDENTIFIER WHITESPACE+ ':' WHITESPACE+ realType=type
    | first=fnArgumentList WHITESPACE+ ',' WHITESPACE+ rest=fnArgumentList
    ;

classDeclaration
    : EOF
    ;

structDeclaration
    : EOF
    ;

typeDeclaration
    : 'type' WHITESPACE+ name=IDENTIFIER WHITESPACE+ '=' aliasedType=type WHITESPACE_WITHOUT_NEWLINES+ NEWLINE
    ;

blockStatement
    : EOF
    ;

type
    : ref=(AMPERSTAND_MUT | AMPERSTAND)? referredTo=typeWithoutRef
    ;

typeWithoutRef
    : '[' WHITESPACE+ slicedType=typeWithoutRef WHITESPACE+ ']'
    | ptr=(IMMUTABLE_PTR | MUTABLE_PTR) pointedTo=typeWithoutRef
    | builtin=builtinType
    | userDefinedType=MODULE_NAME (genericArgs=genericTypeList)?
    ;

builtinType
    : 'i8'
    | 'i16'
    | 'i32'
    | 'i64'
    | 'i128'
    | 'isize'
    | 'u8'
    | 'u16'
    | 'u32'
    | 'u64'
    | 'u128'
    | 'usize'
    | 'f32'
    | 'f64'
    | 'f128'
    | 'byte'
    | 'bool'
    | 'char'
    ;

genericTypeList
    : arg=type
    | first=type ',' rest=genericTypeList
    ;

LT
    : '<'
    ;

GT
    : '>'
    ;

AMPERSTAND
    : '&'
    ;

AMPERSTAND_MUT
    : '&mut'
    ;

STAR_CONST
    : '*const'
    ;

STAR_MUT
    : '*mut'
    ;
