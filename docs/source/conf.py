# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'PoSDK'
copyright = '2025, VINF-SJTU Group and VINF Ltd.'
author = 'Qi Cai'
release = 'v0.10.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'myst_parser',
    'sphinx_rtd_theme', # Read the Docs theme if you have installed it
]

templates_path = ['_templates']
exclude_patterns = []

language = 'C++'

source_suffix = {
    '.rst': 'restructuredtext',
    '.txt': 'markdown',
    '.md': 'markdown',
}

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_css_files = [
    'custom.css',  # Original custom CSS
    'css/benchmark_tables.css',  # Benchmark table styles
]
html_js_files = ['debug-task-list.js', 'task-list.js'] # Add debugging and task list interaction functionality

# -- Theme options -----------------------------------------------------------
# sphinx_rtd_theme options: https://sphinx-rtd-theme.readthedocs.io/en/stable/configuring.html
html_logo = '_static/Po_logo.png'
html_favicon = '_static/Po_logo.png' # Optional: set website favicon

html_theme_options = {
    'logo_only': False, # If True, navigation bar shows only Logo
    #'display_version': True, # Commented out unsupported option
    'prev_next_buttons_location': 'bottom', # bottom, top, both, None
    'style_external_links': False,
    'vcs_pageview_mode': '',
    # 'style_nav_header_background': 'white',
    # Toc options
    'collapse_navigation': False, # Set to False to disable navigation collapse
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

# Configure MathJax support
extensions.append('sphinx_math_dollar')
extensions.append('sphinx.ext.mathjax')
mathjax_path = 'https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js'

# Configure myst-parser extension syntax
myst_enable_extensions = [
    "amsmath",
    "colon_fence",
    "deflist",
    "dollarmath",
    "html_image",
    "tasklist",  # Enable task list checkbox functionality
]

# Explicitly enable checkbox functionality
myst_enable_checkboxes = True
