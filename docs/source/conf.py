# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

from pathlib import Path
import os
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
pomvg_docs_audience = os.environ.get('POMVG_DOCS_AUDIENCE', 'public')

project = 'PoMVG'
copyright = '2026, VINF-SJTU Group and VINF Ltd.'
author = 'Qi Cai'
release = 'v' + (Path(__file__).resolve().parents[1] / 'VERSION.txt').read_text().strip()
version = release

# Collaborators
collaborators = 'Yuanxin Wu, Xinrui Li'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'documentation_policy',
    'publication_assets',
    'myst_parser',
    'sphinx_rtd_theme', # Read the Docs theme if you have installed it
]

templates_path = ['_templates']
exclude_patterns = [
    'benchmark_comparison/processed/*.md',  # Exclude markdown tables (included via {include} directive)
]

language = 'en'  # English language setting

source_suffix = {
    '.rst': 'restructuredtext',
    '.txt': 'markdown',
    '.md': 'markdown',
}

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
# Shared tutorial media, followed by language-specific resources.
html_static_path = ['../_static', '_static']
html_css_files = [
    'css/user_guide.css',
    'custom.css',  # Original custom CSS
    'css/benchmark_tables.css',  # Benchmark table styles
    'css/benchmark_dashboard.css',
    'css/platform_comparison.css',
]
html_js_files = ['debug-task-list.js', 'task-list.js', 'js/benchmark_dashboard.js', 'js/platform_comparison.js'] # Add debugging and task list interaction functionality

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

# Allow raw LaTeX in markdown
myst_dmath_double_inline = True

# Generate heading anchors for better navigation
# This also ensures proper heading level conversion in LaTeX
myst_heading_anchors = 3

# -- Options for LaTeX/PDF output --------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-latex-output

# Use XeLaTeX for Unicode support (required for Chinese characters)
latex_engine = 'xelatex'

# LaTeX document settings
# The 6th parameter (default True) controls whether to use the first heading
# Setting to True uses the first heading as the chapter title
latex_documents = [
    ('index', 'PoMVG_Documentation.tex', 'PoMVG Documentation',
     'Qi Cai', 'manual', True),
]

# LaTeX theme options
latex_theme = 'manual'
latex_logo = '_static/Po_logo.png'

# Additional files to copy to LaTeX output directory
latex_additional_files = ['_static/PoIG.png']

# Set the top-level sectioning for LaTeX output
# 'chapter' means the first heading (#) in each toctree document becomes a \chapter
# and subsequent headings (##, ###) become \section, \subsection, etc.
latex_toplevel_sectioning = 'chapter'

# Ensure correct sectioning depth
latex_use_parts = False

# LaTeX preamble for Unicode and Chinese font support
latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    'papersize': 'a4paper',
    
    # The font size ('10pt', '11pt' or '12pt').
    'pointsize': '11pt',
    
    # Custom page header format to remove comma
    'fncychap': r'\usepackage[Bjornstrup]{fncychap}',
    'printindex': r'\footnotesize\raggedright\printindex',
    
    # Release info format - remove comma from header
    'releasename': 'Release',
    
    # Additional stuff for the LaTeX preamble
    'preamble': r'''
\usepackage{xeCJK}  % For Chinese/Japanese/Korean support
\usepackage{fontspec}  % For custom fonts

% Set main font (for English and numbers) - use system fonts
% Using Times New Roman which is available on macOS
\IfFontExistsTF{Times New Roman}{
  \setmainfont{Times New Roman}[BoldFont=Times New Roman Bold]
  \newfontfamily\PoMVGHeadingFont{Times New Roman}
}{
  \setmainfont{texgyretermes-regular.otf}[
    BoldFont=texgyretermes-bold.otf,
    ItalicFont=texgyretermes-italic.otf,
    BoldItalicFont=texgyretermes-bolditalic.otf,
  ]
  \newfontfamily\PoMVGHeadingFont{texgyretermes-regular.otf}[
    BoldFont=texgyretermes-bold.otf,
  ]
}

% Set Chinese fonts (using system fonts available on macOS)
% STSong is available via fontspec
\IfFontExistsTF{STSong}{
  \setCJKmainfont{STSong}[BoldFont=STHeiti, ItalicFont=Kaiti SC]
  \setCJKsansfont{STHeiti}
  \setCJKmonofont{STSong}
}{
  \setCJKmainfont{FandolSong-Regular.otf}[
    BoldFont=FandolSong-Bold.otf,
    ItalicFont=FandolKai-Regular.otf,
  ]
  \setCJKsansfont{FandolHei-Regular.otf}[BoldFont=FandolHei-Bold.otf]
  \setCJKmonofont{FandolFang-Regular.otf}
}

% Use Times New Roman for all titles (章节标题)
\usepackage{titlesec}
\titleformat{\chapter}[display]
  {\normalfont\huge\bfseries\PoMVGHeadingFont}{\chaptertitlename\ \thechapter}{20pt}{\Huge}
\titleformat{\section}
  {\normalfont\Large\bfseries\PoMVGHeadingFont}{\thesection}{1em}{}
\titleformat{\subsection}
  {\normalfont\large\bfseries\PoMVGHeadingFont}{\thesubsection}{1em}{}
\titleformat{\subsubsection}
  {\normalfont\normalsize\bfseries\PoMVGHeadingFont}{\thesubsubsection}{1em}{}

% Set CJK sans-serif font

% Set CJK monospace font  

% Additional packages for better formatting
\usepackage{booktabs}  % Better tables
\usepackage{longtable}  % Multi-page tables
\usepackage{graphicx}  % Enhanced graphics support
\usepackage{hyperref}  % Hyperlinks (loaded automatically, but can configure)
\usepackage{amsmath}  % Math support
\usepackage{amssymb}  % Math symbols
\usepackage{array}  % Enhanced array and table support
\usepackage{tabularx}  % Auto-width tables
\usepackage{etoolbox}  % For patching environments

% Configure hyperref for PDF
\hypersetup{
    colorlinks=true,
    linkcolor=blue,
    filecolor=magenta,
    urlcolor=cyan,
    pdftitle={PoMVG Documentation},
    pdfauthor={Qi Cai; Collaborators: Yuanxin Wu, Xinrui Li},
    pdfsubject={PoMVG Documentation},
}

% Page layout
\usepackage{geometry}
\geometry{
    a4paper,
    left=2.5cm,
    right=2.5cm,
    top=3cm,
    bottom=3cm,
    headheight=14pt,  % Fix fancyhdr warning
}

% Table of contents depth
\setcounter{tocdepth}{3}
\setcounter{secnumdepth}{3}

% Better code block formatting
\usepackage{listings}
\usepackage{xcolor}
\lstset{
    basicstyle=\ttfamily\small,
    breaklines=true,
    frame=single,
    backgroundcolor=\color{gray!10},
}

% Define colors for table highlighting
\definecolor{bestcolor}{RGB}{178,34,34}  % Red color for best values
\definecolor{secondbestcolor}{RGB}{0,0,0}  % Black color for second-best values

% Custom commands for highlighting table values
% Best value: red + bold
\newcommand{\bestvalue}[1]{\textcolor{bestcolor}{\textbf{#1}}}
% Second-best value: black + bold  
\newcommand{\secondbestvalue}[1]{\textbf{#1}}

% Table settings for better display
% Reduce column separation globally for tables to help them fit better.
\preto\tabulary{\setlength{\tabcolsep}{4pt}}

% Allow tables to break across pages
\usepackage{ltablex}  % Combines longtable and tabularx
\keepXColumns  % Keep X column width consistent

% Better table cell padding
\renewcommand{\arraystretch}{1.3}

% Allow text wrapping in table cells
\newcolumntype{L}[1]{>{\raggedright\arraybackslash}p{#1}}
\newcolumntype{C}[1]{>{\centering\arraybackslash}p{#1}}
\newcolumntype{R}[1]{>{\raggedleft\arraybackslash}p{#1}}

% Custom table environment for better overflow handling
\makeatletter
\def\maxwidth#1{\ifdim\wd#1>\linewidth\linewidth\else\wd#1\fi}
\makeatother

% Custom header/footer with Times New Roman font
\usepackage{fancyhdr}
\pagestyle{fancy}
\fancyhf{}
\fancyhead[LE,RO]{\PoMVGHeadingFont\thepage}
\fancyhead[LO]{\PoMVGHeadingFont\nouppercase{\rightmark}}
\fancyhead[RE]{\PoMVGHeadingFontPoMVG Documentation -- Release @POMVG_DOCS_RELEASE@}
\renewcommand{\headrulewidth}{0.4pt}
\renewcommand{\footrulewidth}{0pt}

% Apply Times New Roman to plain page style (used in chapter start pages)
\fancypagestyle{plain}{%
  \fancyhf{}
  \fancyfoot[C]{\PoMVGHeadingFont\thepage}
  \renewcommand{\headrulewidth}{0pt}
  \renewcommand{\footrulewidth}{0pt}
}
''',
    
    # Latex figure (float) alignment
    'figure_align': 'htbp',
    
    # Extra blank page before chapters
    'extraclassoptions': 'openany,oneside',
    
    # Font package inclusion
    'fontpkg': '',
    
    # Table style - use tabularcolumns with better wrapping
    'tableofcontents': r'\sphinxtableofcontents',
}

# Configure table column wrapping
# Use 'X' for auto-width columns that wrap text
latex_table_style = ['booktabs', 'colorrows']

# Custom title page with logo and collaborators
latex_elements['maketitle'] = r'''
\begin{titlepage}
\centering
\vspace*{1cm}

{\Huge\bfseries PoMVG Documentation\par}
\vspace{1.5cm}

{\Large Qi Cai\par}
\vspace{0.5cm}
{\large Collaborators: Yuanxin Wu, Xinrui Li\par}
\vspace{2cm}
{\large Release @POMVG_DOCS_RELEASE@\par}
\vspace{1cm}
{\large Nov 11, 2025\par}

\vspace{1.5cm}
% Add logo
\includegraphics[width=0.3\textwidth]{Po_logo.png}

\vfill

{\large VINF-SJTU Group and VINF Ltd.\par}
\end{titlepage}

\clearpage

% Preface
\cleardoublepage
\phantomsection
\addcontentsline{toc}{chapter}{Preface}
\chapter*{Preface}
\markboth{Preface}{Preface}

PoMVG is a C++ software platform for \textbf{Spatial Visual Computing}, designed for 3D multi-view geometry processing, including camera pose estimation, feature point cloud generation, and more.

This guide introduces the PoMVG architecture, core library usage, and custom plugin development (through plugins, you can integrate and benchmark mainstream 3D reconstruction platforms such as OpenMVG, COLMAP, GLOMAP, etc. \emph{Platform Comparison}).

\section*{Pose-only Imaging Geometry}

Traditional visual geometry methods face challenges such as the ``curse of dimensionality'', low computational efficiency, and insufficient robustness when processing large-scale scenes.

\textbf{Pose-only Imaging Geometry} theory provides a novel approach, offering a new visual representation and computational paradigm. It expresses high-dimensional 3D scene information losslessly on a low-dimensional pose manifold, achieving complete decoupling between camera pose estimation and 3D scene reconstruction, without requiring joint optimization of poses and scene features in ultra-high-dimensional space.

PoMVG core library is built based on the Pose-only Imaging Geometry theory, providing an efficient and robust toolkit designed to serve as an engine foundation for lightweight, embedded visual computing applications (such as robot navigation, spatial computing, mixed reality, etc.).

\begin{center}
\includegraphics[width=0.8\textwidth]{PoIG}
\end{center}

\section*{Cite Us}

If your research benefits from the Pose-only theory or PoMVG, please cite the following publications:

\begin{verbatim}
@article{cai2019equivalent,
  title={Equivalent Constraints for Two-View Geometry: Pose Solution/Pure
         Rotation Identification and 3D Reconstruction},
  journal={International Journal of Computer Vision},
  volume={127},
  pages={163--180},
  year={2019},
  publisher={Springer},
  doi={10.1007/s11263-018-1136-9}
}

@article{cai2023pose,
  title={A Pose-only Solution to Visual Reconstruction and Navigation},
  journal={IEEE Transactions on Pattern Analysis and Machine Intelligence},
  volume={45},
  number={1},
  pages={73--86},
  year={2023},
  publisher={IEEE},
  doi={10.1109/TPAMI.2021.3139681}
}

@article{cai2024linear,
  title={Linear Relative Pose Estimation Founded on Pose-only Imaging Geometry},
  journal={arXiv preprint arXiv:2401.13357},
  year={2024}
}
\end{verbatim}

\section*{License Compliance}

If you identify any open-source license compliance issues with PoMVG, please notify us immediately through GitHub Issues. We are committed to maintaining compliance with all open-source licenses and will address any concerns promptly.

\clearpage
'''

# Keep PDF headers and the title page on the same release as HTML builds.
for _element in ('preamble', 'maketitle'):
    latex_elements[_element] = latex_elements[_element].replace('@POMVG_DOCS_RELEASE@', release)
