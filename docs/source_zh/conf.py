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
    'sphinx_rtd_theme', # 如果你安装并想使用 Read the Docs 主题
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
    'custom.css',  # 原有自定义 CSS
    'css/benchmark_tables.css',  # 基准测试表格样式
]
html_js_files = ['debug-task-list.js', 'task-list.js'] # 添加调试和任务列表交互功能

# -- Theme options -----------------------------------------------------------
# sphinx_rtd_theme options: https://sphinx-rtd-theme.readthedocs.io/en/stable/configuring.html
html_logo = '_static/Po_logo.png'
html_favicon = '_static/Po_logo.png' # 可选：设置网站图标

html_theme_options = {
    'logo_only': False, # 如果为 True，导航栏只显示 Logo
    #'display_version': True, # 注释掉不支持的选项
    'prev_next_buttons_location': 'bottom', # bottom, top, both, None
    'style_external_links': False,
    'vcs_pageview_mode': '',
    # 'style_nav_header_background': 'white',
    # Toc options
    'collapse_navigation': False, # 设置为 False 以禁用导航折叠
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

# 配置MathJax支持
extensions.append('sphinx_math_dollar')
extensions.append('sphinx.ext.mathjax')
mathjax_path = 'https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js'

# 配置myst-parser的扩展语法
myst_enable_extensions = [
    "amsmath",
    "colon_fence",
    "deflist",
    "dollarmath",
    "html_image",
    "tasklist",  # 启用任务列表复选框功能
]

# 明确启用复选框功能
myst_enable_checkboxes = True
