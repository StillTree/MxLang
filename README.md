# MxLang

A matrix only language, kind of similar to MATLAB but with **static type checking**.

## Running

Running these command should correctly build the interpreter binary with the default CMake toolchain.

```sh
git clone https://github.com/StillTree/MxLang
cd MxLang
mkdir Build
cd Build
cmake ..
cmake --build .
```

> [!NOTE]  
> This interpreter has been compiled with Clang and GCC, as well as tested on Linux and MacOS. Getting this up and running on Windows
> using MSVC might require some tweaks.

## Examples

Here are some example programs written in Mx.

> [!NOTE]  
> These examples are fully written by AI, I gave it Mx's EBNF grammar definitions and it spat out these. I did not check them personally!

### Row normalization of a 3x3 matrix

```julia
let M: 3x3 = [1 2 3][4 5 6][7 8 9]
let i = 1

while i <= 3 {
    let s = M[i 1] + M[i 2] + M[i 3]

    if s != 0 {
        M[i] = M[i] / s
    }

    i = i + 1
}

display(M)
```

### Power iteration–style vector normalization

```julia
let A: 2x2 = [3 1][2 4]
let v = <<1 1>>
let k = 0

while k < 5 {
    let x1 = A[1 1] * v[1] + A[1 2] * v[2]
    let x2 = A[2 1] * v[1] + A[2 2] * v[2]

    v = <<x1 x2>>

    let n = (v[1] * v[1] + v[2] * v[2]) ^ 0.5

    if n > 0 {
        v[1] = v[1] / n
        v[2] = v[2] / n
    }

    k = k + 1
}

display(v)
```

### Diagonal extraction, sum, and transpose

```julia
let A: 3x3 = [5 1 2][3 9 4][6 7 8]
let diag = <<0 0 0>>
let i = 1

while i <= 3 {
    diag[i] = A[i i]
    i = i + 1
}

let s = diag[1] + diag[2] + diag[3]

if s > 15 {
    let v = <<s 1 1>>
    display(v')
} else {
    display(diag')
}
```

### Normalized power iteration (power method)

```julia
let A: 2x2 = [2 1][1 3]
let B: 2x2 = [1 0][0 1]
let v = <<1 0>>
let sum = 0
let i = 1

while i <= 4 {
	v = A * v

    let mag2 = v[1]*v[1] + v[2]*v[2]
    sum = sum + mag2

    let n = mag2 ^ 0.5
    if n > 0 {
        v[1] = v[1] / n
        v[2] = v[2] / n
    }

	B = B * A

    i = i + 1
}

display(v)
display(B)
display(sum)
```

## EBNF

The formal context-free grammar for Mx.

```ebnf
program      ::= statement* EOF

statement    ::= var_decl
               | assignment
               | if_stmt
               | while_stmt
               | block
               | expression

block        ::= "{" statement* "}"

if_stmt      ::= "if" expression block ( "else" ( if_stmt | block ) )?
while_stmt   ::= "while" expression block

var_decl     ::= ( "let" | "const" ) IDENTIFIER TYPE_DECL? "=" expression

assignment   ::= IDENTIFIER index_suffix? "=" expression
index_suffix ::= "[" expression ( expression )? "]"

matrix_lit   ::= row_lit ( row_lit )*
row_lit      ::= "[" expression+ "]"
vector_lit   ::= "<<" expression+ ">>"

expression   ::= logic_and ( "or" logic_and )*
logic_and    ::= equality ( "and" equality )*
equality     ::= comparison ( ( "==" | "!=" ) comparison )*
comparison   ::= term ( ( "<" | "<=" | ">" | ">=" ) term )*
term         ::= factor ( ( "+" | "-") factor )*
factor       ::= exponent ( ( "*" | "/" ) exponent )*
unary        ::= ( "-" unary ) | exponent
exponent     ::= postfix ( "^" postfix )*
postfix      ::= primary ( "'" )?

primary      ::= IDENTIFIER ( "(" call_args? ")" | index_suffix )? 
               | NUMBER 
               | matrix_lit 
               | vector_lit 
               | "(" expression ")"

call_args    ::= expression ( "," expression )*
```
