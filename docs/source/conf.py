"""Configuration file for the Sphinx documentation builder."""

from pathlib import Path

# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "cutl"
copyright = "2026, Jan Roth"
author = "Jan Roth"
release = "0.0.1v"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions: list[str] = [
    "sphinx.ext.napoleon",
    "sphinx.ext.autodoc",
    "sphinx.ext.doctest",
    "sphinx.ext.viewcode",
    "hawkmoth",
    "hawkmoth.ext.javadoc",
]

templates_path = ["_templates"]
exclude_patterns: list[str] = []


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_book_theme"
html_static_path = ["_static"]


# -- Options for autodoc extension -------------------------------------------
autodoc_default_options = {
    "members": True,
    "undoc-members": True,
    "inherited-members": True,
    "show-inheritence": True,
}


# -- Options for Napoleon ----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/extensions/napoleon.html

napoleon_google_docstring = False
napoleon_numpy_docstring = True
napoleon_include_special_with_doc = True
napoleon_use_param = True
napoleon_use_rtype = True
napoleon_attr_annotations = True

# -- Options for C hawkmoth --------------------------------------------------
# https://hawkmoth.readthedocs.io/en/stable/extension.html#configuration
hawkmoth_root = (Path(__file__).parent.parent.parent / "include" / "cutl").absolute()
hawkmoth_transform_default = "javadoc"
hawkmoth_clang = ["--std=c23"]
