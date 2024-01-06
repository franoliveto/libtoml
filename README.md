# libtoml - a TOML parser library. Work-in-progress.

**DO NOT use it in production. It won't work anyway.**

libtoml is a small TOML parsing library in C that uses only fixed extent
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

## Building using Bazel

Make sure that [Bazel](https://bazel.build) is installed on your system.

Then build and run the example:

```bash
$ bazel run //:example
```

To run the tests:

```bash
$ bazel test //:toml_test
```

A common way to consume libtoml is to use Bazel's external
dependencies feature. To do this, create a WORKSPACE file in the root
directory of your Bazel workspace with the following content:

```starlark
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "libtoml",
    sha256 = "1e8d53fa4ae5a97f9676a0adbf5272a8eb508cc57bd3cf0bcdd595a41a39d5f2",
    strip_prefix = "libtoml-227b72a0db73b57ee241e90ef1233a0387fed09e",
    urls = ["https://github.com/franoliveto/libtoml/archive/227b72a0db73b57ee241e90ef1233a0387fed09e.zip"],
)
```

In this example, a ZIP archive of the libtoml code is downloaded from
GitHub. **1e8d53fa4ae5a97f9676a0adbf5272a8eb508cc57bd3cf0bcdd595a41a39d5f2**
is the git commit hash of the libtoml version being used. See the
[Bazel reference](https://docs.bazel.build/versions/master/repo/http.html#http_archive-sha256)
for more information.

Now, create a BUILD file with a `cc_binary` rule in the same directory
as your code:

```starlark
cc_binary(
    ...
    deps = ["@libtoml//:toml"],
)
```

We declare a dependency on the toml library (//:toml) using the prefix
we declared in our WORKSPACE file (@libtoml).