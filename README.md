# Truthtable Project

A simple reworking of a digital circuit truth table printout I did a year ago
Mostly an exercise in doing some C before my OS Class. 

# Folders
- build contains all cmake-related files and the project proper
- include
  - circuit.h
  - gate.h
  - variable.h
- src
   - circuit.c
   - gate.c
   - variable.c

# Specifications
Prints out a binary truth table of a circuit read in via a file.

## Standard Circuit Declaration

INPUT must be the first word followed by n which determines the number of input variables.
Following that is the declaration of the names of each input variable.

Example:
`INPUT 3 a b c`

OUTPUT is declared in the same fashion and must follow after the last input variable was read.

After that, a number of gates and variables may be declared separated by whitespace following the format.
GATE parameters

Where GATE is a valid gate and parameters is a matching set of valid variable declarations.
Example Program which computes z = ab + ac.
```
INPUT 3 a b c
OUTPUT 1 z
AND a b x
AND a c y
OR x y z
```


## Variable Types
There are five types of variables. Input, Output, Temporary Variables, Constants, and Discards

- Input variables may only be used as inputs of gates.
- Constant Variables may only be used as inputs of gates and their values cannot be changed. 
  - Two names are set aside for constant variables "1" and "0" representing their binary equivalents.
- Output variables may only be used as outputs of gates
- Temporary Variables may be used as outputs, or if declared beforehand, may be used as inputs
- Discard Variables may only be used as output and their result is effectively ignored
  - The name "_" is set aside to be used for discard variables. 

The maximum size of a variable name is determined by NAME_SIZE defined in variable.h
It is set to 64 as a default.

## Gate Types

Each gate will be represented via i's and o's, where they represent input and output parameters respectively. Ellipses (...) are used to indicate a variable number of parameters whenever n is specified. Descriptions may refer to previous gates declared as a function with each parameter represented as an argument.

- PASS i o
  - Computes the identity gate where o = i

`Necessary for converting temporary variables into output parameters`

- NOT i o
  - Computes the inverse gate where o = ̅i

- AND i<sub>1</sub> i<sub>2</sub> o
  - Computes the logical *and* where o = i<sub>1</sub>i<sub>2</sub>.

- NAND i<sub>1</sub> i<sub>2</sub> o
  - Computes the logical *nand* where o = NOT(i<sub>1</sub>i<sub>2</sub>).

- OR i<sub>1</sub> i<sub>2</sub> o
  - Computes the logical *or* where o = i<sub>1</sub> + i<sub>2</sub>.

- NOR i<sub>1</sub> i<sub>2</sub> o
  - Computes the logical *nor* where o = NOT(i<sub>1</sub> + i<sub>2</sub>)

- XOR i<sub>1</sub> i<sub>2</sub> o
  - Computes the logical *xor* where o = i<sub>1</sub> ⊕ i<sub>2</sub>, , where ⊕ indicates exclusive or

- DECODER n i<sub>1</sub> · · ·i~n~ o<sub>0</sub> · · · o<sub>2<sup>(n-1)</sup></sub>
  - Computes a n : 2<sup>n</sup> decoder gate in logic design. 
    The first argument gives the number of inputs, n. 
    The next n parameters are the inputs, followed by 2^n^ parameters indicating the outputs. 
    The inputs are interpreted as an n-bit binary number b in the range 0, · · · , 2<sup>(n − 1)</sup>, where i~1~ is the most significant bit and i~n~ is the least significant bit. 
    The output o<sub>b</sub> will be 1 and all others will be 0.

- MULTIPLEXER n i~0~ · · ·i<sub>2<sup>(n − 1)</sup></sub> s<sub>1</sub> · · ·s<sub>n</sub> o
  - Computes a 2<sup>n</sup> : 1 multiplexer gate in logic design. 
    The inputs to a multiplexer are either regular inputs or selectors, indicated by i and s, respectively. 
    The first parameter, n, gives the number of selectors. 
    The next 2<sup>n</sup> parameters give the regular inputs, followed by n selector inputs, and finally the output. 
    The selector inputs are interpreted as an n-bit binary number s in the range 0, · · · , 2<sup>(n − 1)</sup>. 
    The output is o = i<sub>s</sub>.


The maximum number for n is limited by the program to prevent overflow is defined by MAX_VAR_COUNT in truthtable.c. 
It is set to 32 by default.

# Running the program
To run the program is fairly simple just add the file you want to enter as the first argument such that running it looks like this:

`.\truthtable.exe test.txt`

It will print out into the console a truth table consisting of ones and zeros.
The example program above would have printed
```
0 0 0 | 0
1 0 0 | 0
0 1 0 | 0
1 1 0 | 1
0 0 1 | 0
1 0 1 | 1 
0 1 1 | 0 
1 1 1 | 1
```
Where the first numbers represent the binary representation of inputs followed by a pipe which then prints the resulting outputs.
