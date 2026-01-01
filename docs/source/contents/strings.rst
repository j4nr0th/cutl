String Support
==============

Support is offered for strings. These are represented by a pointer to memory where a string
is stored and its size. The functions for manipulation and formatting do not consider what
the underlying representation actually is, instead they just deal with provided sub-strings.
This makes them suitable to use for arbitrarily encoded data.

It should be noted that none of these strings are "owning", meaning that none of the functions
that operate on them allocate, free, or resize memory buffers. It is the responsibility of
whoever creates these to clean up the memory after it is no longer needed.

Formatting utilities are also provided for these strings, which allow for converting numbers
into their textual representations. While sensible defaults are provided, many parts of the
conversion process can be customized, such as what digits are used, what base the conversion
is, what and how much to pad, what separators to use, and more.


.. toctree::
    :caption: Strings
    :maxdepth: 1

    strings/basic_operations
    strings/string_formatting
    strings/streams

    

