Test-only independent Z80 implementation by superzazu, under the adjacent MIT LICENSE.
https://github.com/superzazu/z80/tree/d64fe10a2274e5e40019b1086bf7d8990cbc5f23

z80.c and z80.h are unmodified upstream files. They are linked only into the
validation executables, never into the production port. Differential tests do
not assume this reference perfectly models all undocumented Z80 behavior.
