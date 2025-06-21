# AGENTS Guidelines

- Every request from the developer must be quoted verbatim in the related git commit message using Markdown blockquote style (a leading `>`). This makes it easy to track what was asked versus what was done.

- Indent code with 4 spaces and keep lines under 100 columns when possible. Prefer extracting helper expressions rather than wrapping lines early.
- Place the pointer `*` next to the variable name, e.g. `int *ptr`, and express const-ness on pointed-to values like `static void const *foo`.
- Name arrays of foo values `foo` and the number of entries `foos`. Count bytes with `size_t` and other quantities with `int`.
- Use vertical and horizontal spacing to align related code so that similar things look similar.
- Sort `#include` directives alphabetically within their groups.
- Prefer descriptive variable names like `window` instead of abbreviations.
- Put one statement on each line rather than chaining them together.
- Write constants on the left side of comparisons, e.g. `0 != foo()`.
- Fix warnings rather than disabling them. Cast explicitly to resolve
  implicit cast warnings and insert padding fields to resolve implicit
  struct padding warnings.
- When initializing multiple values of the same type, put the first on
  the line with the type and align the rest after the `=` sign:

    Room const pod      = {0},
               corridor = {0};
- Prefer to mark constant stack values `const` so changing values stand
  out, but avoid marking pointers themselves `const` unless necessary.

- Omit defensive bounds or null checks for internal code; sanitizers will catch these bugs.
- Track open work with TODO comments directly in the code.
- Do not use `__attribute__((unused))`; remove unused code instead.
