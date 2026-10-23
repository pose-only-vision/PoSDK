document.addEventListener('DOMContentLoaded', () => {
  // Preserve links to scenes and datasets from the former single-page report.
  const overview = document.querySelector('.platform-overview');
  const results = overview && overview.querySelector('a[href="results.html"]');
  if (results && /^#bench-/.test(window.location.hash)) {
    const target = new URL(results.href);
    target.search = window.location.search;
    target.hash = window.location.hash;
    window.location.replace(target.href);
  }
});
