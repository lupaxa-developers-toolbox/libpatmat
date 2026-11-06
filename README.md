<p align="center">
  <a href="https://github.com/lupaxa-developers-toolbox">
    <img src="https://raw.githubusercontent.com/the-lupaxa-project/brand-assets/master/logos/organisations/developers-toolbox/readme-logo.png" alt="Developers Toolbox" />
  </a>
</p>

<h1 align="center">Libpatmat</h1>

C library for shell-style text patterns.

`patmat_match` compares a whole text with a pattern. It was shaped for name
bans such as `*!*@*`. The match is stateless. The public header is
`include/patmat.h`. It uses C linkage, so a C++ translation unit can include
it.

## Requirements

- GNU make.
- gcc, unless `CC` is set. The C skill uses gcc for syntax checks.
- cppcheck, for `make c-check`.

## Build

`make lib` writes the static archive `build/libpatmat.a`. The archive is
built as C11 with `-Wall -Wextra -Wpedantic -Werror`. The compiler is gcc
unless `CC` is set. `make clean` removes `build/`.

```bash
make lib
cc -std=c11 -Iinclude -o demo demo.c build/libpatmat.a
```

`make lib CC=clang` selects the compiler. `make lib CFLAGS=-g` adds flags to
that C11 build. `make` with no target prints the makefile help.

`make update` refreshes the makefile skills. `make lib` and `make test` do
not need that checkout. `make c-check` and `make version` do.

## Quick Start

Match a ban mask against a name.

```c
#include "patmat.h"

int main(void)
{
    int matched = 0;

    if (patmat_match("Nick!user@host", "*!*@*", &matched) != PATMAT_OK) {
        return 1;
    }
    if (!matched) {
        return 1;
    }
    return 0;
}
```

The text argument is first. `*matched` is `1` when the whole text matches.

## Patterns

The match covers the whole text. Letters `A`–`Z` match either case. Any
other byte matches itself.

| Pattern        | Meaning                                  |                                |
| -------------- | ---------------------------------------- | ------------------------------ |
| any other byte | That byte                                |                                |
| `\`            | The next byte is literal                 |                                |
| `?`            | One byte                                 |                                |
| `*`            | Any run of bytes, including an empty run |                                |
| `[abc]`        | One byte from the class                  |                                |
| `[a-z]`        | One byte in the inclusive range          |                                |
| `[^abc]`       | One byte outside the class               |                                |
| `{foo\         | bar}`                                    | One whole word, `foo` or `bar` |

`[]` is a class that matches no byte. `[^]` matches any one byte. A range
whose ends are out of order, such as `[c-a]`, matches no byte. A final `*`
matches the rest of the text.

`{...}` is tried at the start of the text or after whitespace. Inside the
braces, `*` and `?` stay inside the word. The alternatives can contain `*`,
`?`, and classes. `|` and `}` outside braces are ordinary bytes. After a
word matches, the next pattern byte sees the following whitespace, so
`{foo} {bar}` matches `foo bar`.

Whitespace is space, tab, newline, vertical tab, form feed, or carriage
return.

A pattern that nests more than 64 stars or braces is `PATMAT_BADPAT`.

> **Note:** The matcher is reentrant. It keeps no static state.

## Return Codes

`patmat_match` returns an `int`. It does not allocate.

| Code            | Value | Meaning                                       |
| --------------- | ----- | --------------------------------------------- |
| `PATMAT_OK`     | `0`   | The call succeeded. `*matched` is `1` or `0`. |
| `PATMAT_INVAL`  | `-2`  | The text, pattern, or out-pointer is missing. |
| `PATMAT_BADPAT` | `-3`  | The pattern is not well formed.               |

An unclosed `[` or `{`, a nested `{`, a trailing `\`, or a range that
consumes the closing `]` is `PATMAT_BADPAT`. `*matched` stays unchanged.

## Functions

```c
int patmat_match(const char *text, const char *pattern, int *matched);
```

| Name           | Purpose                                |
| -------------- | -------------------------------------- |
| `patmat_match` | Compare the whole text with a pattern. |

## Empty Text

An empty pattern matches an empty text. `*` matches an empty text. `?` does
not.

## Version

The version text is the macro `PATMAT_VERSION_STRING` in
`include/patmat.h`. It is a dotted semantic version. `.bumpversion.toml`
is the version source, and a bump updates that macro.

After `make init`, `make version` prints the version from
`.bumpversion.toml`.

## Development

```bash
make update
make lib
make check
make clean
```

`make c-check` syntax-checks every `.c` and `.h` file, then runs cppcheck.
`make check` runs `c-check` and then the behaviour tests. `make test` runs
only the behaviour tests.

<a href="https://github.com/the-lupaxa-project">
    <img src="https://raw.githubusercontent.com/the-lupaxa-project/brand-assets/master/logos/components/footer-for-child-orgs.svg" alt="The Lupaxa Project Footer" width="100%" />
</a>
