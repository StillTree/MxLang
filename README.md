# MxLang

A matrix only language, for a uni project.

## Examples

```julia
# Comments start with a hash

# A variable declaration and assignment, with inferred type
let mat1 = [1 2][3 4]
# Equivalent to
let mat1: 2x2 = [1 2][3 4]

# A constant
const mat2 = [1 2][3 4]
# Equivalent to
const mat2: 2x2 = [1 2][3 4]

# Shape changes are not allowed
let mat3 = [1 2][3 4]
mat3 = [1 2 3][4 5 6][7 8 9] # Error

# Value changes or reasignments are, indexes are 1-based
mat3[1 2] = 5 # Allowed
mat3 = [5 6][7 8] # Allowed
# Entire rows can also be swapped
mat3[2] = [9 10] # Allowed

# By default everything is zeroed out, unless defined
let mat4: 2x2 = [1 2] # Silently becomes: [1 2][0 0]
# This only works with explicit type declarations
let mat5 = [1 2] # Becomes a 1x2, nothing gets zeroed out

# Scalars are matrices too (1x1)
let scalar = 123 # Inferred as 1x1
# Equivalent to
let scalar = [123] # Also inferred as 1x1
# Equivalent to
let scalar = <123> # Also inferred as 1x1

# Vectors are just matrices with a single column
let vec = [1][2] # 2D vector (2x1 matrix)
# Equivalent to
let vec = <1 2> # 2D vector (2x1 matrix)

# Basic operations (follow math rules)
# Addition (+) is allowed only between the same vector shapes
# Subtraction (-) is also allowed only between the same vector shapes
# Matrix multiplication (*) is chosen when none of the operands is a 1x1 matrix,
# it follows the formula: AxB * BxC = AxC
# Scalar multiplication (*) is chosen when one of the operands is a 1x1 matrix
# Vector multiplication (*) is chosen when both operands have a single column and the same row count,
# it results in a dot product
# Vector and matrix multiplication (*) behaves like normal matrix multiplication
# Powering (^) is allowed only between scalars

# Adjacency does not means multiplication
let mat6 = [1 2][3 4]
let mat7 = [5 6][7 8]
let mat8 = mat6mat7 # Error

# Transpose
let mat9 = [1 2][3 4]
let transposed = mat6' # Results in a 2x2

# Determinant
let mat10 = [1 2][3 4]
let determinant = det(mat7) # Results in a scalar (1x1)

# Identity matrix
let mat11: 3x3 = ident() # Requires an explicit type declaration

# Matrix inverse
let mat12: 2x3 = [1 2 3][4 5 6]
let mat13 = inv(mat12)

# Printing
let mat13 = [1 2][3 4]
print(mat13) # Prints the matrix
```

## EBNF

```ebnf
program      -> statement* EOF

statement    -> var_decl | assignment | expression

var_decl     -> ("let" | "const") IDENTIFIER type_hint? "=" expression
type_hint    -> ":" INTEGER "x" INTEGER

assignment   -> IDENTIFIER index_suffix? "=" expression
index_suffix -> "[" expression (expression)? "]"

expression   -> term ( ( "+" | "-") term )*
term         -> unary ( ( "*" | "/" ) unary )*
unary        -> "-" unary | factor
factor       -> postfix ( "^" postfix )*
postfix      -> call_or_atom ( "'" )*

call_or_atom -> IDENTIFIER ( "(" args? ")" | index_suffix )? | NUMBER | matrix_lit | vector_lit | "(" expression ")"
args         -> expression ( "," expression )*
```
