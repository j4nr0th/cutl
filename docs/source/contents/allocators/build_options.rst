Build Options
=============

Every allocation handed out by this library is aligned to ``max_align_t``, and the allocators which
cut blocks out of a larger region place guard bytes on both ends of every block they hand out. Both
are compile-time choices, so a build that does not need them pays nothing for them at all.

The options are CMake variables, and a project pulling this one in with ``add_subdirectory()`` stays
in charge of them: a variable is only given a default here if the including project did not set it
already, either as a normal or as a cache variable beforehand.

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Option
     - Default
     - Meaning
   * - ``CUTL_ALLOCATOR_ALIGNMENT``
     - ``max_align_t``
     - Alignment every allocation is rounded to and handed out with. May be made stricter, but never
       looser, as the allocators round all of their sizes by it.
   * - ``CUTL_ALLOCATOR_GUARD_BYTES``
     - ``16``, or ``0`` for a ``Release`` build
     - Guard bytes placed on both ends of every allocation. Rounded up to the alignment, so the memory
       handed to the caller stays aligned however many guard bytes were asked for. Zero compiles
       guard handling out entirely.
   * - ``CUTL_ASSERTS``
     - on, off for a ``Release`` build
     - The internal consistency checks. Exposed separately from the guards, as they are worth keeping
       in a release build, where the guards are usually off.
   * - ``CUTL_VALIDATE_ALLOCATORS``
     - off
     - Exhaustive, and quadratic, checking of the allocators' own bookkeeping. For pinning down a
       suspected allocator bug only.
   * - ``CUTL_BUILD_TESTS``
     - on
     - Build the test suite.
   * - ``CUTL_TEST_GUARD_BYTES``
     - ``16``
     - Guard bytes the test suite insists on. Since the guards are compiled into the library, and the
       tests are what catch a write outside an allocation, the tests raise
       ``CUTL_ALLOCATOR_GUARD_BYTES`` to this when they are being built. Set it to ``0`` to also
       cover the no-guard build with the test suite.

Guard bytes
-----------

Guard bytes are what turn a write outside of an allocation into a report rather than silent damage to
whatever sits next in memory. When they are enabled, the allocators fill both ends of every block
with a known pattern and check it again when the block is given back, so an out-of-bounds write is
reported at the point where the block is released rather than surfacing somewhere unrelated later.

They are a debugging aid rather than a safety mechanism, and they are off in a ``Release`` build
because every allocation would otherwise carry the extra bytes. The same goes for the guards on the
arena allocator, which keeps its own compact header instead and opts out of them entirely.

An allocation therefore looks like this, where the guard region collapses to nothing when guards are
compiled out::

   block start                                                        block end
   |------------------------------- guards ----------------------------|
   | [ allocator bookkeeping ] [ front guard | usable memory | rear guard ] [ ... ] |
                                 ^
                                 the pointer handed to the caller
