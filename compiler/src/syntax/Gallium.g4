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

BUILTIN_TYPE
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
    | 'void'
    ;

fragment DECIMAL_DIGIT
    : [0-9]
    ;

fragment HEX_DIGIT
    : [0-9a-fA-F]
    ;

fragment OCTAL_DIGIT
    : [0-7]+
    ;

fragment STRING_LITERAL_CHARACTER
    : '\\0'   // null character
    | '\\n'  // newline
    | '\\r'  // carriage return
    | '\\t'  // tab
    | '\\v'  // vertical tab
    | '\\\'' // single quote
    | '\\"'  // double quote
    | '\\\\' // backslash
    | '\\a'  // bell
    | '\\b'  // backspace
    | '\\f'  // form feed
    | '\\o' OCTAL_DIGIT OCTAL_DIGIT OCTAL_DIGIT // 3-digit octal number, 000-377. maps to a single byte
    | '\\x' HEX_DIGIT HEX_DIGIT // 2-digit hex number, 00-FF. maps to a single byte
    | '\\' DECIMAL_DIGIT DECIMAL_DIGIT DECIMAL_DIGIT // 3 digit decimal number, 000-255. maps to a single byte
   // | '\\u' DECIMAL_DIGIT DECIMAL_DIGIT DECIMAL_DIGIT DECIMAL_DIGIT
   //      (DECIMAL_DIGIT DECIMAL_DIGIT DECIMAL_DIGIT DECIMAL_DIGIT)? // U+nnnn (or U+nnnnnnnn if all 8)
    | ~[\\"\n\r] // any character besides `\`
    ;

fragment STRING_CHAR_SEQ
    : STRING_LITERAL_CHARACTER*
    ;

STRING_LITERAL
    : '"' STRING_CHAR_SEQ? '"'
    ;

CHAR_LITERAL
    : '\'' STRING_LITERAL_CHARACTER '\''
    ;

OCTAL_LITERAL
    : '0o' OCTAL_DIGIT+
    ;

HEX_LITERAL
    : '0x' HEX_DIGIT+
    ;

BINARY_LITERAL
    : '0b' BINARY_LITERAL+
    ;

DECIMAL_LITERAL
    : DECIMAL_DIGIT+
    ;

BOOL_LITERAL
    : 'true'
    | 'false'
    ;

NIL_LITERAL
    : 'nil'
    ;

LT
    : '<'
    ;

GT
    : '>'
    ;

TILDE
    : '~'
    ;

STAR
    : '*'
    ;

AMPERSTAND
    : '&'
    ;

AMPERSTAND_MUT
    : '&mut'
    ;

CARET
    : '^'
    ;

PIPE
    : '|'
    ;

LTLT
    : '<<'
    ;

LTEQ
    : '<='
    ;

GTEQ
    : '>='
    ;

EQEQ
    : '=='
    ;

BANGEQ
    : '!='
    ;

AND_KEYWORD
    : 'and'
    ;

OR_KEYWORD
    : 'or'
    ;

XOR_KEYWORD
    : 'xor'
    ;

NOT_KEYWORD
    : 'not'
    ;

FORWARD_SLASH
    : '/'
    ;

PERCENT
    : '%'
    ;

PLUS
    : '+'
    ;

HYPHEN
    : '-'
    ;

STAR_CONST
    : '*const'
    ;

STAR_MUT
    : '*mut'
    ;

WHITESPACE
    : ' '
    | '\t'
    ;

NEWLINE
    : '\n'
    | '\r\n'
    ;

PUB
    : 'pub'
    ;

PROT
    : 'prot'
    ;

PRIV
    : 'priv'
    ;

TO
    : 'to'
    ;

DOWNTO
    : 'downto'
    ;

fragment IDENTIFIER_START
    : [\p{XID_START}_] // why does XID_Start not include '_'?
    ;

fragment IDENTIFIER_CONT
    : [\p{XID_Continue}]
    ;

IDENTIFIER
    : IDENTIFIER_START IDENTIFIER_CONT*
    ;

WALRUS
    : ':='
    ;

PLUSEQ
    : '+='
    ;

HYPHENEQ
    : '-='
    ;

STAREQ
    : '*='
    ;

SLASHEQ
    : '/='
    ;

PERCENTEQ
    : '%='
    ;

LTLTEQ
    : '<<='
    ;

GTGTEQ
    : '>>='
    ;

AMPERSTANDEQ
    : '&='
    ;

CARETEQ
    : '^='
    ;

PIPEEQ
    : '|='
    ;

BLOCK_COMMENT
    :   '/*' .*? '*/'
        -> skip
    ;

LINE_COMMENT
    :   '//' ~[\r\n]*
        -> skip
    ;

ws
    : (WHITESPACE | NEWLINE)+
    ;
    
parse
    : ws? modularizedDeclaration+ ws? EOF
    ;

modularIdentifier
    : (isRoot='::')? IDENTIFIER ('::' IDENTIFIER)*
    ;

modularizedDeclaration
    : importDeclaration WHITESPACE* (NEWLINE+ | EOF)
    | exportDeclaration WHITESPACE* (NEWLINE+ | EOF)
    ;

importDeclaration
    : 'import' WHITESPACE+ modularIdentifier (WHITESPACE+ 'as' WHITESPACE+ alias=IDENTIFIER)?
    | 'import' WHITESPACE+ importList WHITESPACE+ 'from' WHITESPACE+ modularIdentifier
    ;

importList
    : '{' NEWLINE+ ws identifierList ws '}'
    | '{' ws? identifierList ws? '}'
    ;

identifierList
    : IDENTIFIER (',' ws? IDENTIFIER ws?)*
    ;

exportDeclaration
    : (exportKeyword='export' WHITESPACE+)? declaration
    ;

declaration
    : fnDeclaration
    | classDeclaration
    | structDeclaration
    | typeDeclaration
    | externalDeclaration
    | constDeclaration
    ;

constDeclaration
    : 'const' ws IDENTIFIER ws? ':' ws? type ws? '=' ws? constantExpr
    ;

externalDeclaration
    : 'external' ws '{' ws? (fnPrototype ws)+ ws? '}'
    ;

fnDeclaration
    : (isExtern='extern' ws)? fnPrototype ws? blockExpression
    ;

fnPrototype
    : 'fn' ws IDENTIFIER ws? typeParamList? ws? '(' fnArgumentList? ')'
           (ws fnAttributeList)?
           (ws '->' ws type)
    ;

fnAttributeList
    : fnAttribute (ws fnAttribute)*
    ;

fnAttribute
    : '__pure'
    | '__throws'
    | '__alwaysinline'
    | '__inline'
    | '__noinline'
    | '__malloc'
    | '__hot'
    | '__cold'
    | '__arch(' STRING_LITERAL ')'
    | '__noreturn'
    | '__stdlib'
    ;

fnArgumentList
    : singleFnArgument ws? (',' ws? singleFnArgument)*
    ;

selfArgument
    : '&self'
    | '&mut' ws 'self'
    | 'self'
    | 'mut' ws 'self'
    ;

singleFnArgument
    : IDENTIFIER ws? ':' ws? type
    | selfArgument
    ;

typeParamList
    : LT ws? typeParam (ws? ',' ws? typeParam)* ws? GT
    ;

typeParam
    : IDENTIFIER (':' ws? interface=maybeGenericIdentifier)? (ws? '=' ws? defaultType=maybeGenericIdentifier)?
    ;

classDeclaration
    : 'class' ws IDENTIFIER typeParamList? (ws? classInheritance)? ws? '{'
      (ws? classMember WHITESPACE* NEWLINE+)*
      ws?
      '}'
    ;

classInheritance
    : ':' ws? classInheritedType (ws? ',' ws? classInheritedType)*
    ;

classInheritedType
    : (visibility=(PUB | PROT | PRIV) ws)? modularIdentifier (LT genericTypeList GT)?
    ;

classMember
    : (visibility=(PUB | PROT | PRIV) ws)? classMemberWithoutPub
    ;

classMemberWithoutPub
    : var='mut' classVariableBody
    | let='let' classVariableBody
    | fnPrototype ws? blockExpression
    | typeDeclaration
    ;

classVariableBody
    : WHITESPACE+ IDENTIFIER WHITESPACE* ':' WHITESPACE+ type
    ;

structDeclaration
    : 'struct' ws IDENTIFIER (ws? typeParamList?) ws? '{'
      (ws structMember WHITESPACE* NEWLINE+)*
      ws?
      '}'
    ;

structMember
    : IDENTIFIER ':' WHITESPACE+ type
    ;

typeDeclaration
    : 'type' ws? typeParamList? ws? IDENTIFIER ws? '=' ws? type
    ;

statement
    : exprStatement
    | assertStatement
    | bindingStatement
    ;

assertStatement
    : 'assert' ws expr (ws? ',' ws STRING_LITERAL)?
    ;

bindingStatement
    : let='let' ws IDENTIFIER (ws? ':' ws? type)? ws? '=' ws? expr
    | var='mut' ws IDENTIFIER (ws? ':' ws? type)? ws? '=' ws? expr
    ;

exprStatement
    : expr
    ;

callArgList
    : expr ws? (',' ws? expr)*
    ;

restOfCall
    : paren='(' callArgList? ')'
    | bracket='[' callArgList? ']'
    | '.' IDENTIFIER
    ;

blockExpression
    : '{' ((ws? statement WHITESPACE* NEWLINE) (ws? statement WHITESPACE* NEWLINE)*)? ws? '}'
    ;

returnExpr
    : 'return' (WHITESPACE* expr)?
    ;

breakExpr
    : 'break' (WHITESPACE* expr)?
    ;

continueExpr
    : 'continue'
    ;

ifExpr
    : 'if' ws expr ws 'then' ws expr ws 'else' ws expr
    | 'if' ws expr ws blockExpression (ws elifBlock)* (ws elseBlock)?
    ;

elifBlock
    : 'elif' ws expr ws blockExpression
    ;

elseBlock
    : 'else' ws blockExpression
    ;

loopExpr
    : 'while' ws whileCond=expr ws blockExpression
    | 'loop' ws? blockExpression
    | 'for' ws loopVariable=IDENTIFIER ws ':=' ws expr ws direction=(TO | DOWNTO) ws expr ws? blockExpression
    ;

expr
    : arrayExpr
    | primaryExpr
    | blockExpression
    | expr restOfCall
    | op=NOT_KEYWORD ws expr
    | op=(TILDE | AMPERSTAND | HYPHEN | STAR) expr
    | op=AMPERSTAND_MUT ws expr
    | expr ws as='as' ws type
    | expr ws asUnsafe='as!' ws type
    | expr ws op=(STAR | FORWARD_SLASH | PERCENT) ws expr
    | expr ws op=(PLUS | HYPHEN) ws expr
    | expr ws ((op=LTLT) | (gtgtHack=GT GT)) ws expr // dirty hack to make >> parse in generic contexts
    | expr ws op=AMPERSTAND ws expr
    | expr ws op=CARET ws expr
    | expr ws op=PIPE ws expr
    | expr ws op=AND_KEYWORD ws expr
    | expr ws op=XOR_KEYWORD ws expr
    | expr ws op=OR_KEYWORD ws expr
    | expr ws op=(LT | GT | LTEQ | GTEQ) ws expr
    | expr ws op=(EQEQ | BANGEQ) ws expr
    | expr ws op=(WALRUS | PLUSEQ | HYPHENEQ | STAREQ | SLASHEQ | PERCENTEQ | LTLTEQ | GTGTEQ | AMPERSTANDEQ | CARETEQ | PIPEEQ) ws expr
    | ifExpr
    | loopExpr
    | (returnExpr | breakExpr | continueExpr)
    ;

primaryExpr
    : groupExpr
    | maybeGenericIdentifier
    | digitLiteral
    | floatLiteral
    | structInitExpr
    | STRING_LITERAL
    | CHAR_LITERAL
    | BOOL_LITERAL
    | NIL_LITERAL
    ;

arrayExpr
    : '[' ws? expr ws? (',' ws? expr)* ws? ']'
    ;

structInitExpr
    : typeWithoutRef ws? '{' ws? structInitMemberList ws? '}'
    ;

structInitMember
    : IDENTIFIER ws? ':' ws? expr ws?
    ;

structInitMemberList
    : structInitMember (',' ws? structInitMember)*
    ;

constantExpr
    : digitLiteral
    | floatLiteral
    | STRING_LITERAL
    | CHAR_LITERAL
    | BOOL_LITERAL
    | NIL_LITERAL
    ;

maybeGenericIdentifier
    : modularIdentifier typeParamList? ('::' memberGenericIdentifier)*
    ;

memberGenericIdentifier
    : IDENTIFIER typeParamList?
    ;

groupExpr
    : '(' ws? expr ws? ')'
    ;

digitLiteral
    : hex=HEX_LITERAL
    | octal=OCTAL_LITERAL
    | binary=BINARY_LITERAL
    | decimal=DECIMAL_LITERAL
    ;

floatLiteral
    : DECIMAL_LITERAL? '.' DECIMAL_LITERAL
    ;

type
    : ref=(AMPERSTAND_MUT | AMPERSTAND)? WHITESPACE* typeWithoutRef
    ;

typeWithoutRef
    : squareBracket='[' WHITESPACE* (mut='mut' WHITESPACE+)? typeWithoutRef WHITESPACE* (';' WHITESPACE* DECIMAL_LITERAL)? ']'
    | ptr=(STAR_CONST | STAR_MUT) (WHITESPACE+) typeWithoutRef
    | BUILTIN_TYPE
    | userDefinedType=maybeGenericIdentifier
    | fnType='fn' WHITESPACE* '(' (WHITESPACE* genericTypeList WHITESPACE*)? ')' WHITESPACE+ '->' WHITESPACE+ type
    | 'dyn' ws dynType=maybeGenericIdentifier
    ;

genericTypeList
    : type WHITESPACE* (',' WHITESPACE* type)*
    ;
