# AGENTS Guidelines

- Every request from the developer must be quoted verbatim in the related git commit message using Markdown blockquote style (a leading `>`). This makes it easy to track what was asked versus what was done.

- Indent code with 4 spaces and keep lines under 100 columns when possible. Prefer extracting helper expressions rather than wrapping lines early.
- Place the pointer `*` next to the variable name and express const-ness on pointed-to values, e.g. `static void const *foo`.
- Name arrays of foo values `foo` and the number of entries `foos`. Count bytes with `size_t` and other quantities with `int`.
- Use vertical and horizontal spacing to align related code so that similar things look similar.
- Sort `#include` directives alphabetically within their groups.
- Prefer descriptive variable names like `window` instead of abbreviations.
- Put one statement on each line rather than chaining them together.
- Write constants on the left side of comparisons, e.g. `0 != foo()`.
