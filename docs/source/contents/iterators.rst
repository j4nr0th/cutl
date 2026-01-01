Iterators
=========

A few different somewhat more advanced iterators are provided by ``cutl``.
These are currently three different iterators, each suitable for a different purpose:

- :c:type:`combination_iterator_t` used to iterate over :math:`C^n_r` combinations,
- :c:type:`permutation_iterator_t` used to iterate over :math:`P^n_r` permutations,
- :c:type:`multidim_iterator_t` used to iterate over multidimensional square arrays.

.. toctree::
    :caption: Iterators
    :maxdepth: 1

    iterators/combinations
    iterators/permutations
    iterators/multidim_iterator
