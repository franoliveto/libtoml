Work in progress.

libtoml is a small TOML parsing library in C that uses only fixed extent storage.

libtoml parses a large subset of TOML (Tom's Obvious Minimal Language). Unlike
more general TOML parsers, it doesn't use malloc(3); you need to give it a set of
template structures describing the expected shape of the incoming TOML, and it
will error out if that shape is not matched. When the parse succeeds, key values
will be extracted into static locations specified in the template structures.

All elements of an array must be of the same type. Arrays may not be array elements.

