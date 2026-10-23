"""Owner page selection, public allowlist, and Sphinx exclusion enforcement."""
from __future__ import annotations

import hashlib
import fnmatch
import json
import os
from pathlib import Path
import posixpath
import re
import tempfile

PUBLIC_FILE = 'public_pages.json'
OWNER_FILE = '.owner-pages.json'
SCHEMA = 'pomvg.documentation-pages.v1'
LANGUAGES = {'zh': 'source_zh', 'en': 'source'}


def _read(path, default=None):
    if not path.exists():
        return default
    if path.is_symlink() or not path.is_file() or path.stat().st_size > 1024 * 1024:
        raise ValueError('文档控制文件无效：' + str(path))
    value = json.loads(path.read_text(encoding='utf-8'))
    if not isinstance(value, dict) or value.get('schema') != SCHEMA or not isinstance(value.get('pages'), dict):
        raise ValueError('文档控制文件格式无效：' + str(path))
    for key, row in value['pages'].items():
        _key(key)
        if not isinstance(row, dict) or set(row) - {'build', 'publish'} or type(row.get('build')) is not bool:
            raise ValueError('文档选项必须为布尔值')
        if 'publish' in row and type(row['publish']) is not bool:
            raise ValueError('文档发布选项必须为布尔值')
    return value


def _key(value):
    if not isinstance(value, str) or not re.fullmatch(r'(source|source_zh)/[A-Za-z0-9_./-]+\.(rst|md|txt)', value):
        raise ValueError('无效文档路径')
    if any(p in ('', '.', '..') for p in value.split('/')):
        raise ValueError('文档路径不能越界')
    return value


def _navigation_text(text, markdown=False):
    """Read navigation directives without treating code/comments as navigation."""
    lines = re.sub(r'<!--.*?-->', '', text, flags=re.S).splitlines()
    visible, blocks = [], []
    i = 0
    while i < len(lines):
        line = lines[i]
        fence = re.match(r'^\s*(`{3,}|~{3,}|:{3,})(.*)$', line)
        directive = re.match(r'^(\s*)\.\.\s+([\w-]+)::', line)
        if markdown and fence:
            marker, kind = fence.groups()
            content = []
            i += 1
            while i < len(lines) and not re.fullmatch(
                    r'\s*' + re.escape(marker[0]) + '{' + str(len(marker)) + r',}\s*', lines[i]):
                content.append(lines[i])
                i += 1
            if kind.strip() == '{toctree}':
                blocks.append(content)
            i += 1
        elif directive and directive[2] in ('toctree', 'code', 'code-block', 'sourcecode', 'raw'):
            indent = len(directive[1])
            content = []
            i += 1
            while i < len(lines) and (not lines[i].strip()
                    or len(lines[i]) - len(lines[i].lstrip()) > indent):
                content.append(lines[i])
                i += 1
            if directive[2] == 'toctree':
                blocks.append(content)
        elif re.match(r'^\s*\.\.(?:\s|$)', line) and not directive:
            indent = len(line) - len(line.lstrip())
            i += 1
            while i < len(lines) and (not lines[i].strip()
                    or len(lines[i]) - len(lines[i].lstrip()) > indent):
                i += 1
        else:
            visible.append(line)
            i += 1
    return visible, blocks


def _document_title(text, fallback, markdown=False):
    lines, _ = _navigation_text(text, markdown)
    for i, line in enumerate(lines):
        heading = re.match(r'^\s*#{1,6}\s+(.+?)\s*#*\s*$', line)
        if heading:
            return heading[1]
        value = line.strip()
        if value and i + 1 < len(lines) and re.fullmatch(
                r'[=~`^"*+#-]{3,}', lines[i + 1].strip()):
            return value
    return fallback


def _navigation_glob(name, pattern):
    names, patterns = name.split('/'), pattern.split('/')
    def match(i, j):
        if j == len(patterns):
            return i == len(names)
        if patterns[j] == '**':
            return match(i, j + 1) or (i < len(names) and match(i + 1, j))
        return i < len(names) and fnmatch.fnmatchcase(names[i], patterns[j]) and match(i + 1, j + 1)
    return match(0, 0)


def navigation(docs, pages):
    """Mirror source toctrees, including pages currently excluded from builds."""
    docs = Path(docs)
    result = {}
    for language, folder in LANGUAGES.items():
        known = {str(Path(key).relative_to(folder).with_suffix('')): row
                 for key, row in pages.items() if row['language'] == language}
        referenced = set()
        outlines = {}

        def outline(docname):
            if docname in outlines:
                return outlines[docname]
            entries = []
            text = (docs / known[docname]['path']).read_text(encoding='utf-8')
            for block in _navigation_text(text, known[docname]['path'].endswith('.md'))[1]:
                glob = any(line.strip() == ':glob:' for line in block)
                targets = []
                for line in block:
                    value = line.strip()
                    if not value or value.startswith((':', '#', '.. ')) or value == '..':
                        continue
                    match = re.fullmatch(r'(.*?)\s*<([^<>]+)>', value)
                    label, target = (match[1].strip(), match[2]) if match else ('', value)
                    target = target.split('#', 1)[0]
                    if not target or target == 'self' or re.match(r'^[\w+.-]+:', target):
                        continue
                    target = posixpath.normpath(target.lstrip('/') if target.startswith('/')
                                               else posixpath.join(posixpath.dirname(docname), target))
                    if target.startswith('../') or target == '..':
                        continue
                    if Path(target).suffix in ('.md', '.rst', '.txt'):
                        target = str(Path(target).with_suffix(''))
                    matches = sorted(key for key in known if _navigation_glob(key, target)) if glob else [target]
                    targets.extend((key, label) for key in matches if key in known)
                if any(line.strip() == ':reversed:' for line in block):
                    targets.reverse()
                entries.extend(targets)
            outlines[docname] = entries
            return entries

        def node(docname, label, identity, ancestors):
            referenced.add(docname)
            children = []
            if docname not in ancestors:
                for index, (target, title) in enumerate(outline(docname)):
                    if target in ancestors or target == docname:
                        continue
                    children.append(node(target, title, identity + '/' + str(index), ancestors | {docname}))
            return {'id': identity, 'path': known[docname]['path'],
                    'title': label or known[docname]['title'], 'children': children}

        tree = []
        if 'index' in known:
            root = node('index', '', language + '/home', set())
            tree = [dict(root, children=[]), *root['children']]
        unlisted = []
        for docname in sorted(known):
            if docname not in referenced:
                unlisted.append(node(docname, '', language + '/unlisted/' + docname, set()))
        if unlisted:
            tree.append({'id': language + '/unlisted', 'path': None,
                         'title': '未列入导航的页面' if language == 'zh' else 'Pages outside navigation',
                         'children': unlisted})
        result[language] = tree
    return result


def discover(docs):
    docs = Path(docs)
    result = {}
    for language, folder in LANGUAGES.items():
        source = docs / folder
        if source.is_symlink():
            raise ValueError('文档源码目录不能为符号链接')
        for path in source.rglob('*'):
            relative = path.relative_to(source)
            if path.suffix not in ('.rst', '.md', '.txt') or any(
                    p in ('_static', '_templates', 'processed', 'scripts') for p in relative.parts):
                continue
            if path.is_symlink() or not path.resolve().is_relative_to(source.resolve()):
                raise ValueError('文档源码不能为符号链接：' + str(relative))
            key = _key(str(path.relative_to(docs)))
            title = _document_title(path.read_text(encoding='utf-8'), relative.stem, path.suffix == '.md')
            result[key] = dict(path=key, language=language, title=title,
                               section=relative.parts[0] if len(relative.parts) > 1 else 'overview')
    return result


def catalog(docs):
    docs = Path(docs)
    public = _read(docs / PUBLIC_FILE, {'schema': SCHEMA, 'pages': {}})['pages']
    owner = _read(docs / OWNER_FILE, {'schema': SCHEMA, 'pages': {}})['pages']
    rows = []
    pages = discover(docs)
    tree = navigation(docs, pages)
    for key, row in sorted(pages.items()):
        approved = key in public
        local = owner.get(key, {})
        row.update(build=local.get('build', public.get(key, {}).get('build', False)),
                   publish=approved, locked=key in ('source/index.rst', 'source_zh/index.rst'))
        rows.append(row)
    digest = hashlib.sha256(json.dumps({'pages': rows, 'navigation': tree}, sort_keys=True, ensure_ascii=False).encode()).hexdigest()
    return {'schema': SCHEMA, 'revision': digest, 'pages': rows, 'navigation': tree}


def selection(docs, language, audience):
    if language not in LANGUAGES or audience not in ('owner', 'public'):
        raise ValueError('文档语言或构建范围无效')
    if audience == 'public':
        # An ignored, host-local preference must never widen a public build.
        public = _read(Path(docs) / PUBLIC_FILE, {'schema': SCHEMA, 'pages': {}})['pages']
        known = discover(docs)
        return {key for key, row in public.items()
                if key in known and known[key]['language'] == language and row['build']}
    return {r['path'] for r in catalog(docs)['pages'] if r['language'] == language and r['build']}


def _atomic(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_symlink():
        raise ValueError('拒绝写入符号链接：' + str(path))
    fd, name = tempfile.mkstemp(prefix='.docs-policy-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(content)
        os.replace(name, path)
    finally:
        Path(name).unlink(missing_ok=True)


def save(docs, revision, changes):
    """Save compilation/export choices without changing local Git tracking."""
    docs = Path(docs).resolve()
    current = catalog(docs)
    if revision != current['revision']:
        raise ValueError('文档列表已变化，请刷新后重新勾选')
    if not isinstance(changes, list) or len(changes) > 1000:
        raise ValueError('无效文档选择')
    rows = {r['path']: dict(r) for r in current['pages']}
    seen = set()
    for change in changes:
        if not isinstance(change, dict) or set(change) != {'path', 'build', 'publish'}:
            raise ValueError('文档选择只接受 path、build、publish')
        key = _key(change['path'])
        if key not in rows or key in seen or type(change['build']) is not bool or type(change['publish']) is not bool:
            raise ValueError('文档不存在、重复或选项无效')
        if rows[key]['locked'] and not (change['build'] and change['publish']):
            raise ValueError('首页必须保留编译和发布')
        rows[key].update(change)
        seen.add(key)
    public = {'schema': SCHEMA, 'pages': {k: {'build': r['build']} for k, r in rows.items() if r['publish']}}
    local = {'schema': SCHEMA, 'pages': {k: {'build': r['build'], 'publish': r['publish']} for k, r in rows.items()}}
    paths = [docs / PUBLIC_FILE, docs / OWNER_FILE]
    for path in paths:
        if path.is_symlink():
            raise ValueError('文档配置文件不能为符号链接')
    before = {p: p.read_bytes() if p.exists() else None for p in paths}
    try:
        _atomic(paths[0], (json.dumps(public, ensure_ascii=False, indent=2) + '\n').encode())
        _atomic(paths[1], (json.dumps(local, ensure_ascii=False, indent=2) + '\n').encode())
    except Exception:
        for path, value in before.items():
            if value is None:
                path.unlink(missing_ok=True)
            else:
                _atomic(path, value)
        raise
    return catalog(docs)


def _target(source, value, strict=False):
    value = value.strip()
    if '<' in value and value.endswith('>'):
        value = value.rsplit('<', 1)[1][:-1]
    value = value.split('#', 1)[0]
    if not value or '://' in value or value == 'self':
        return None
    path = Path(value.lstrip('/')) if value.startswith('/') else Path(source).parent / value
    parts = []
    for part in path.parts:
        if part == '..':
            if not parts:
                if strict:
                    raise ValueError('文档引入文件越界')
                return None
            parts.pop()
        elif part != '.':
            parts.append(part)
    return str(Path(*parts).with_suffix('')) if parts else None


def filter_source(text, docname, allowed, known):
    """Prune excluded navigation; reject includes that bypass page selection."""
    lines = text.splitlines(True)
    output = []
    rst_indent = None
    myst_tree = False
    for line in lines:
        stripped = line.strip()
        indent = len(line) - len(line.lstrip())
        if re.match(r'```+\{toctree\}', stripped):
            myst_tree = True
            output.append(line)
            continue
        if myst_tree and re.fullmatch(r'```+', stripped):
            myst_tree = False
            output.append(line)
            continue
        if re.match(r'\.\.\s+toctree::', stripped):
            rst_indent = indent
            output.append(line)
            continue
        if rst_indent is not None and stripped and indent <= rst_indent:
            rst_indent = None
        if (myst_tree or rst_indent is not None) and stripped and not stripped.startswith((':', '#', '..')):
            target = _target(docname, stripped)
            if target in known and target not in allowed:
                continue
        include = re.search(r'(?:\.\.\s+(?:include|literalinclude)::\s*|(?:```+|:::+)\{(?:include|literalinclude)\}\s*|:file:\s*)(\S+)', line)
        if include:
            target = _target(docname, include.group(1), strict=True)
            if target in known and target not in allowed:
                raise ValueError('已禁止编译的页面不能作为 include 引入：' + target)
        def role(match):
            target = _target(docname, match.group(1))
            if target in known and target not in allowed:
                return match.group(1).split('<', 1)[0].strip()
            return match.group()
        line = re.sub(r':doc:`([^`]+)`', role, line)
        def link(match):
            target = _target(docname, match.group(2))
            return match.group(1) if target in known and target not in allowed else match.group()
        line = re.sub(r'\[([^\]]+)\]\(([^)]+\.(?:md|rst)(?:#[^)]*)?)\)', link, line)
        output.append(line)
    return ''.join(output)


def remove_search_documents(data, docnames):
    """Drop compiled-out documents from a Sphinx search index.

    Sphinx can leave a removed page in ``docnames`` after an incremental build.
    Term lists that only pointed at that page then still return it from search.
    """
    names = data.get('docnames')
    if not isinstance(names, list):
        return data
    blocked = set(docnames)
    drop = {index for index, name in enumerate(names) if name in blocked}
    if not drop:
        for kind in ('terms', 'titleterms'):
            data[kind] = {key: value for key, value in data.get(kind, {}).items() if value != []}
        return data
    mapping = {}
    next_index = 0
    for index in range(len(names)):
        if index not in drop:
            mapping[index] = next_index
            next_index += 1

    def remap_ids(value):
        if isinstance(value, bool):
            return value
        if isinstance(value, int):
            return mapping.get(value)
        if isinstance(value, list) and all(isinstance(item, int) and not isinstance(item, bool) for item in value):
            kept = [mapping[item] for item in value if item in mapping]
            if not kept:
                return None
            return kept[0] if len(kept) == 1 else kept
        return value

    for kind in ('terms', 'titleterms'):
        updated = {}
        for key, value in data.get(kind, {}).items():
            mapped = remap_ids(value)
            if mapped is not None and mapped != []:
                updated[key] = mapped
        data[kind] = updated
    for key in ('docnames', 'filenames', 'titles'):
        rows = data.get(key)
        if isinstance(rows, list) and len(rows) == len(names):
            data[key] = [item for index, item in enumerate(rows) if index not in drop]
    titles = {}
    for title, entries in data.get('alltitles', {}).items():
        if not isinstance(entries, list):
            continue
        kept = []
        for entry in entries:
            if (isinstance(entry, list) and entry and isinstance(entry[0], int)
                    and not isinstance(entry[0], bool) and entry[0] in mapping):
                kept.append([mapping[entry[0]], *entry[1:]])
        if kept:
            titles[title] = kept
    data['alltitles'] = titles
    return data


def setup(app):
    app.add_config_value('pomvg_docs_audience', 'public', 'env')

    def configure(app, config):
        docs = Path(app.confdir).parent
        folder = Path(app.confdir).name
        language = next(k for k, v in LANGUAGES.items() if v == folder)
        # Only the official owner builder explicitly opts in to local pages.
        audience = config.pomvg_docs_audience
        selected = selection(docs, language, audience)
        known = {k: v for k, v in discover(docs).items() if k.startswith(folder + '/')}
        excluded = [str(Path(k).relative_to(folder)) for k in known if k not in selected]
        config.exclude_patterns = list(config.exclude_patterns) + excluded
        if audience == 'public':
            config.html_copy_source = False
            config.html_show_sourcelink = False
        app._pomvg_page_selection = (docs, folder, selected, known)

    def read(app, docname, source):
        docs, folder, selected, known = app._pomvg_page_selection
        allowed_names = {str(Path(k).relative_to(folder).with_suffix('')) for k in selected}
        known_names = {str(Path(k).relative_to(folder).with_suffix('')) for k in known}
        source[0] = filter_source(source[0], docname, allowed_names, known_names)

    def finished(app, error):
        if error is not None:
            return
        docs, folder, selected, known = app._pomvg_page_selection
        for key in set(known) - selected:
            relative = Path(key).relative_to(folder)
            # An incremental Sphinx build otherwise leaves private HTML and
            # downloadable source from an earlier owner build on disk.
            for rel in (relative.with_suffix('.html'), Path('_sources') / (str(relative) + '.txt')):
                path = Path(app.outdir) / rel
                if path.is_file() and not path.is_symlink():
                    path.unlink()
            cached = Path(app.doctreedir) / relative.with_suffix('.doctree')
            if cached.is_file() and not cached.is_symlink():
                cached.unlink()
        search = Path(app.outdir) / 'searchindex.js'
        if search.is_file() and not search.is_symlink():
            # Sphinx can retain a page after it is removed from the build.
            # Drop that document and any terms that only pointed at it.
            from sphinx.search import js_index
            data = js_index.loads(search.read_text(encoding='utf-8'))
            excluded = {str(Path(key).relative_to(folder).with_suffix(''))
                        for key in set(known) - selected}
            remove_search_documents(data, excluded)
            _atomic(search, js_index.dumps(data).encode('utf-8'))
    app.connect('config-inited', configure)
    app.connect('source-read', read)
    app.connect('build-finished', finished)
    return {'version': '1.0', 'parallel_read_safe': False, 'parallel_write_safe': True}
