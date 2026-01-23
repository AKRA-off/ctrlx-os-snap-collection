# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'KORD API'
copyright = '2025, Kassow Robots'
author = 'Martin Straka'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ["breathe"]

breathe_projects = {"kord-api": "./_doxy_out/xml/"}
breathe_default_project = "kord-api"

breathe_projects_source = {
    "kord-public" : ( "../include/kord/api", ["kord.h", "api_commands.h"] )
}

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

suppress_warnings = ['duplicate']

DOCS_BASE_URL = os.environ.get("DOCS_BASE_URL", "")

if DOCS_BASE_URL == "":
    import warnings
    warnings.warn("DOCS_BASE_URL is not set, using empty string")

# Update these values when new version is released
current_language = "en"
current_version = "2.0.2"

versions = [
    ["2.0.2", f"{DOCS_BASE_URL}/versions/2.0.2"],
    ["2.0.1", f"{DOCS_BASE_URL}/versions/2.0.1"],
    ["2.0.0", f"{DOCS_BASE_URL}/versions/2.0.0"],
    ["1.4.5", f"{DOCS_BASE_URL}/versions/1.4.5"], 
    ["1.4.4", f"{DOCS_BASE_URL}/versions/1.4.4"],
    ["1.4.1", f"{DOCS_BASE_URL}/versions/1.4.1"],
    ["1.4.0", f"{DOCS_BASE_URL}/versions/1.4.0"],
    ["1.3.0", f"{DOCS_BASE_URL}/versions/1.3.0"],
    ["1.2.0", f"{DOCS_BASE_URL}/versions/1.2.0"]
]

# we set the html_context wit current language and version 
# and empty languages and versions for now
html_context = {
    'current_language' : current_language,
    'languages' : [["en", "/index.html"]],
    'current_version' : current_version,
    'versions' : versions,
}
