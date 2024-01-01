# libtoml - a tiny TOML parser library

libtoml is a tiny TOML parser library in C that uses only fixed-extent
storage.

libtoml parses a large subset of TOML (Tom's Obvious Minimal Language).
Unlike more general TOML parsers, it doesn't use malloc(3); you need to
give it a set of template structures describing the expected shape of
the incoming TOML, and it will error out if that shape is not matched.
When the parse succeeds, key values will be extracted into static
locations specified in the template structures.

The dialect it parses has some limitations. First, only ASCII encoded
files are considered to be valid TOML documents. Second, all elements
of an array must be of the same type. Third, arrays may not be array
elements.

To be built, libtoml requires [Bazel](https://bazel.build), a C compiler,
and asciidoctor to format the documentation.

Building is straightforward. Simply, run:

```bash
$ bazel build //:toml
```


