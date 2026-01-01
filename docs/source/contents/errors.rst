Results and Error Codes
=======================

If a function can fail, it will almost always return :c:enum:`cutl_result_t` value.
In the few cases where this is not the case, it is always clearly stated in the function's documentation.
When a function successfully returns, the value will be :c:enumerator:`CUTL_SUCCESS`, with other
values indicating why the failure occurred.

.. c:autoenum:: cutl_result_t
    :file: error.h
    :members:
