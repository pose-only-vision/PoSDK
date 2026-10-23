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
author = '蔡奇'
release = 'v' + (Path(__file__).resolve().parents[1] / 'VERSION.txt').read_text().strip()
version = release

# 协作者
collaborators = '武元新, 李欣锐'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'documentation_policy',
    'publication_assets',
    'myst_parser',
    'sphinx_rtd_theme', # 如果你安装并想使用 Read the Docs 主题
]

templates_path = ['_templates']
exclude_patterns = [
    'benchmark_comparison/processed/*.md',  # 排除 Markdown 表格（通过 {include} 指令引入）
]

language = 'zh_CN'  # 中文语言设置

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
    'custom.css',  # 原有自定义 CSS
    'css/benchmark_tables.css',  # 基准测试表格样式
    'css/benchmark_dashboard.css',
    'css/platform_comparison.css',
]
html_js_files = ['debug-task-list.js', 'task-list.js', 'js/benchmark_dashboard.js', 'js/platform_comparison.js'] # 添加调试和任务列表交互功能

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

# 允许在markdown中使用原始LaTeX
myst_dmath_double_inline = True

# 为标题生成锚点，便于导航
# 这也确保了LaTeX中标题级别的正确转换
myst_heading_anchors = 3

# -- Options for LaTeX/PDF output --------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-latex-output

# 使用 XeLaTeX 支持 Unicode（中文必需）
latex_engine = 'xelatex'

# LaTeX 文档设置
# 第6个参数（默认True）控制是否使用第一个标题
# 设置为True将使用第一个标题作为章节标题
latex_documents = [
    ('index', 'PoMVG_文档.tex', 'PoMVG 文档',
     'Qi Cai', 'manual', True),
]

# LaTeX 主题选项
latex_theme = 'manual'
latex_logo = '_static/Po_logo.png'

# 复制额外文件到LaTeX输出目录
latex_additional_files = ['_static/PoIG.png']

# 设置LaTeX输出的顶级章节类型
# 'chapter' 表示toctree中每个文档的第一个标题(#)转为\chapter
# 后续标题(##, ###)依次转为\section, \subsection等
latex_toplevel_sectioning = 'chapter'

# 确保正确的章节深度
latex_use_parts = False

# LaTeX 前导码，用于 Unicode 和中文字体支持
latex_elements = {
    # 纸张大小 ('letterpaper' 或 'a4paper')
    'papersize': 'a4paper',
    
    # 字体大小 ('10pt', '11pt' 或 '12pt')
    'pointsize': '11pt',
    
    # 自定义页眉格式，移除逗号
    'fncychap': r'\usepackage[Bjornstrup]{fncychap}',
    'printindex': r'\footnotesize\raggedright\printindex',
    
    # 版本信息格式 - 从页眉移除逗号
    'releasename': '版本',
    
    # LaTeX 前导码的额外内容
    'preamble': r'''
\usepackage{xeCJK}  % 支持中文/日文/韩文
\usepackage{fontspec}  % 自定义字体

% 设置主字体（英文和数字）- 使用系统字体
% 使用 macOS 上可用的 Times New Roman
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

% 设置中文字体（使用 macOS 上可用的系统字体）
% STSong 可通过 fontspec 使用
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

% 标题使用 Times New Roman 字体（英文部分）
\usepackage{titlesec}
\titleformat{\chapter}[display]
  {\normalfont\huge\bfseries\PoMVGHeadingFont}{\chaptertitlename\ \thechapter}{20pt}{\Huge}
\titleformat{\section}
  {\normalfont\Large\bfseries\PoMVGHeadingFont}{\thesection}{1em}{}
\titleformat{\subsection}
  {\normalfont\large\bfseries\PoMVGHeadingFont}{\thesubsection}{1em}{}
\titleformat{\subsubsection}
  {\normalfont\normalsize\bfseries\PoMVGHeadingFont}{\thesubsubsection}{1em}{}

% 设置 CJK 无衬线字体

% 设置 CJK 等宽字体

% 额外包用于更好的格式
\usepackage{booktabs}  % 更好的表格
\usepackage{longtable}  % 多页表格
\usepackage{graphicx}  % 增强的图形支持
\usepackage{hyperref}  % 超链接（自动加载，但可以配置）
\usepackage{amsmath}  % 数学支持
\usepackage{amssymb}  % 数学符号
\usepackage{array}  % 增强的数组和表格支持
\usepackage{tabularx}  % 自动宽度表格
\usepackage{etoolbox}  % 用于修补环境

% 配置 PDF 的 hyperref
\hypersetup{
    colorlinks=true,
    linkcolor=blue,
    filecolor=magenta,
    urlcolor=cyan,
    pdftitle={PoMVG 文档},
    pdfauthor={蔡奇; 协作者: 武元新, 李欣锐},
    pdfsubject={PoMVG 文档},
}

% 页面布局
\usepackage{geometry}
\geometry{
    a4paper,
    left=2.5cm,
    right=2.5cm,
    top=3cm,
    bottom=3cm,
    headheight=14pt,  % 修复fancyhdr警告
}

% 目录深度
\setcounter{tocdepth}{3}
\setcounter{secnumdepth}{3}

% 更好的代码块格式
\usepackage{listings}
\usepackage{xcolor}
\lstset{
    basicstyle=\ttfamily\small,
    breaklines=true,
    frame=single,
    backgroundcolor=\color{gray!10},
}

% 为表格高亮定义颜色
\definecolor{bestcolor}{RGB}{178,34,34}  % 红色用于最佳值
\definecolor{secondbestcolor}{RGB}{0,0,0}  % 黑色用于次优值

% 自定义命令用于高亮表格数值
% 最佳值：红色 + 粗体
\newcommand{\bestvalue}[1]{\textcolor{bestcolor}{\textbf{#1}}}
% 次优值：黑色 + 粗体  
\newcommand{\secondbestvalue}[1]{\textbf{#1}}

% 表格设置以改善显示
% 全局减小表格的列间距，以更好地适应页面
\preto\tabulary{\setlength{\tabcolsep}{4pt}}

% 允许表格跨页
\usepackage{ltablex}  % 结合 longtable 和 tabularx
\keepXColumns  % 保持 X 列宽度一致

% 更好的表格单元格内边距
\renewcommand{\arraystretch}{1.3}

% 允许表格单元格中的文本换行
\newcolumntype{L}[1]{>{\raggedright\arraybackslash}p{#1}}
\newcolumntype{C}[1]{>{\centering\arraybackslash}p{#1}}
\newcolumntype{R}[1]{>{\raggedleft\arraybackslash}p{#1}}

% 自定义表格环境，更好地处理溢出
\makeatletter
\def\maxwidth#1{\ifdim\wd#1>\linewidth\linewidth\else\wd#1\fi}
\makeatother

% 自定义页眉/页脚，使用Times New Roman字体
\usepackage{fancyhdr}
\pagestyle{fancy}
\fancyhf{}
\fancyhead[LE,RO]{\PoMVGHeadingFont\thepage}
\fancyhead[LO]{\PoMVGHeadingFont\nouppercase{\rightmark}}
\fancyhead[RE]{\PoMVGHeadingFontPoMVG 文档 -- 版本 @POMVG_DOCS_RELEASE@}
\renewcommand{\headrulewidth}{0.4pt}
\renewcommand{\footrulewidth}{0pt}

% 对plain页面样式应用Times New Roman字体（用于章节起始页）
\fancypagestyle{plain}{%
  \fancyhf{}
  \fancyfoot[C]{\PoMVGHeadingFont\thepage}
  \renewcommand{\headrulewidth}{0pt}
  \renewcommand{\footrulewidth}{0pt}
}
''',
    
    # LaTeX 图形（浮动）对齐
    'figure_align': 'htbp',
    
    # 章节前额外空白页
    'extraclassoptions': 'openany,oneside',
    
    # 字体包包含
    'fontpkg': '',
    
    # 表格样式 - 使用带更好换行的 tabularcolumns
    'tableofcontents': r'\sphinxtableofcontents',
}

# 配置表格列换行
# 使用 'X' 表示自动宽度的可换行列
latex_table_style = ['booktabs', 'colorrows']

# 自定义标题页，包含logo和协作者信息
latex_elements['maketitle'] = r'''
\begin{titlepage}
\centering
\vspace*{1cm}

{\Huge\bfseries PoMVG 文档\par}
\vspace{1.5cm}

{\Large 蔡奇\par}
\vspace{0.5cm}
{\large 协作者: 武元新, 李欣锐\par}
\vspace{2cm}
{\large 版本 @POMVG_DOCS_RELEASE@\par}
\vspace{1cm}
{\large 2025年11月11日\par}

\vspace{1.5cm}
% 添加logo
\includegraphics[width=0.3\textwidth]{Po_logo.png}

\vfill

{\large VINF-SJTU Group and VINF Ltd.\par}
\end{titlepage}

\clearpage

% 序言
\cleardoublepage
\phantomsection
\addcontentsline{toc}{chapter}{序言}
\chapter*{序言}
\markboth{序言}{序言}

PoMVG是一个面向\textbf{空间视觉计算 (Spatial Visual Computing)}的C++软件平台，可用于3D多视图几何处理，包括相机位姿估计、特征点云生成等。

本指南将介绍 PoMVG 架构、核心库的使用以及自定义插件的开发（通过插件可以集成及对比评测OpenMVG、COLMAP、GLOMAP等主流三维重建平台，\emph{平台对比评测}）。

\section*{纯位姿成像几何（Pose-only Imaging Geometry）}

经典视觉几何方法在处理大规模场景时，常常面临"维度灾难"、计算效率低以及鲁棒性不足等挑战。

\textbf{纯位姿成像几何 (Pose-only Imaging Geometry)}理论方法另辟蹊径，给出了一种新的视觉表达及计算范式。它将高维 3D 场景信息无损地表达在低维位姿流形上，实现了相机位姿估计与三维场景重建的完全解耦，不需要进行位姿与场景特征的超高维空间联合优化。

PoMVG 核心库基于纯位姿成像几何理论方法构建，提供了一套高效、稳健的工具箱，希望为轻量化、嵌入式视觉计算应用（如机器人导航、空间计算、混合现实等）提供底座引擎支撑。

\begin{center}
\includegraphics[width=0.8\textwidth]{PoIG}
\end{center}

\section*{Cite Us}

如果您的研究工作受益于纯位姿理论方法或 PoMVG，请引用以下文献：

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
