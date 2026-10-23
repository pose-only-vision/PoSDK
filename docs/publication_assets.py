"""Attach public previews and link bulk downloads to immutable ordinary Git."""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
from urllib.parse import quote, unquote, urlsplit

from sphinx.errors import ExtensionError


def _sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(8 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def attach(app, error):
    if error is not None or app.builder.name not in ('html', 'dirhtml', 'singlehtml'):
        return
    release = json.loads((Path(app.confdir).parent / 'public-assets.json').read_text(encoding='utf-8'))
    revision = release.get('revision', '')
    if (release.get('schema') != 'pomvg.public-doc-assets.v1'
            or release.get('repository') != 'pose-only-vision/PoSDK'
            or not re.fullmatch(r'[0-9a-f]{40}', revision)):
        raise ExtensionError('Invalid public documentation asset release')
    source = Path(app.confdir).parent / 'benchmark-assets/eval_data'
    manifest = source / 'manifest.json'
    if _sha256(manifest) != release.get('manifest_sha256'):
        raise ExtensionError('Public benchmark manifest differs from its release')
    data = json.loads(manifest.read_text(encoding='utf-8'))
    if data.get('schema') != 'posdk.benchmark-assets.v1':
        raise ExtensionError('Unrecognized public benchmark asset manifest')
    inventory = data.get('files')
    if not isinstance(inventory, list) or len(inventory) > 20000:
        raise ExtensionError('Invalid public benchmark asset inventory')
    output = Path(app.outdir) / 'eval_data'
    output.mkdir(parents=True, exist_ok=True)
    seen = set()
    external = {}
    base = ('https://raw.githubusercontent.com/pose-only-vision/PoSDK/'
            + revision + '/docs/benchmark-assets/eval_data/')
    for entry in inventory:
        relative = Path(entry['path'])
        if (relative.is_absolute() or '..' in relative.parts or not relative.parts
                or relative.parts[0] != 'public' or str(relative) in seen):
            raise ExtensionError('Invalid public benchmark asset path')
        seen.add(str(relative))
        # Preview videos and figures stay with the documentation. Original
        # recordings and PLY downloads live on the retained assets branch;
        # these URLs use a commit SHA and never use Git LFS.
        if relative.suffix == '.ply' or relative.name == 'video.mp4':
            external[str(relative)] = base + quote(str(relative), safe='/')
            continue
        original = source / relative
        target = output / relative
        if (original.is_symlink() or not original.resolve().is_relative_to(source.resolve())
                or not original.is_file() or original.stat().st_size != entry['bytes']
                or _sha256(original) != entry['sha256']):
            raise ExtensionError('Public benchmark download differs from its manifest: ' + str(relative))
        if not target.resolve().is_relative_to(output.resolve()) or target.is_symlink():
            raise ExtensionError('Public benchmark output path escapes its directory')
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            if target.stat().st_size == entry['bytes'] and _sha256(target) == entry['sha256']:
                continue
            target.unlink()
        try:
            os.link(original, target)
        except OSError:
            shutil.copyfile(original, target)
    shutil.copyfile(manifest, output / 'manifest.json')
    (output / 'external-downloads.json').write_text(
        json.dumps(external, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    references = re.compile(r'(?:\.\./)+eval_data/([^\s\"\'<>]+)')
    def replace(match):
        address = urlsplit(match.group(1))
        target = external.get(unquote(address.path))
        if target is None:
            return match.group(0)
        return (target + ('?' + address.query if address.query else '')
                + ('#' + address.fragment if address.fragment else ''))
    for page in Path(app.outdir).rglob('*.html'):
        before = page.read_text(encoding='utf-8')
        after = references.sub(replace, before)
        if after != before:
            page.write_text(after, encoding='utf-8')


def setup(app):
    app.connect('build-finished', attach)
    return {'version':'1.0', 'parallel_read_safe':True, 'parallel_write_safe':True}
