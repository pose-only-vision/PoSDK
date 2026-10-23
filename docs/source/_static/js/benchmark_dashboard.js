document.addEventListener('DOMContentLoaded', () => {
  const dashboard = document.querySelector('.benchmark-dashboard');
  if (!dashboard) return;
  document.body.classList.add('benchmark-page');
  let dataset = 'onedsfm';
  const search = dashboard.querySelector('.bench-search');
  const scope = dashboard.querySelector('.bench-scope');
  const scopeControl = scope.closest('label');
  const videoGroups = new Map();
  let closePcaPreview = () => {};
  function update() {
    closePcaPreview();
    if (scopeControl) scopeControl.hidden = dataset === 'onedsfm';
    const query = search.value.trim().toLowerCase();
    dashboard.querySelectorAll('.bench-dataset').forEach(section => {
      section.hidden = section.dataset.dataset !== dataset;
      section.querySelectorAll('.bench-scene, .bench-native-run').forEach(scene => {
        scene.hidden = query !== '' && !scene.dataset.scene.includes(query);
      });
    });
    dashboard.querySelectorAll('[data-bench-dataset]').forEach(button => {
      button.setAttribute('aria-pressed', String(button.dataset.benchDataset === dataset));
    });
    const selected = scope.value;
    dashboard.querySelectorAll('[data-metric-scope]').forEach(chart => {
      chart.hidden = chart.dataset.metricScope !== selected;
    });
    dashboard.querySelectorAll('tr[data-own-r]').forEach(row => {
      row.querySelector('.bench-metric-r').textContent = row.getAttribute(`data-${selected}-r`);
      row.querySelector('.bench-metric-c').textContent = row.getAttribute(`data-${selected}-c`);
      const countCell = row.querySelector('.bench-metric-n');
      if (countCell) countCell.textContent = row.getAttribute(`data-${selected}-n`);
    });
    dashboard.querySelectorAll('img[data-chart]').forEach(img => {
      const folder = img.dataset.chartFolder || 'benchmark_v2';
      const chartScope = img.dataset.chartScope || selected;
      img.src = `../_static/${folder}/${img.dataset.chartDataset}_${img.dataset.chart}_${chartScope}.svg`;
    });
    dashboard.querySelectorAll('source[data-chart-source]').forEach(source => {
      const folder = source.dataset.chartFolder || 'benchmark_v2';
      const chartScope = source.dataset.chartScope || selected;
      source.srcset = `../_static/${folder}/${source.dataset.chartDataset}_${source.dataset.chartSource}_${chartScope}_dark.svg`;
    });
    videoGroups.forEach((stop, group) => {
      if (group.closest('.bench-dataset').hidden || group.closest('.bench-scene').hidden) stop();
    });
  }
  dashboard.querySelectorAll('[data-bench-dataset]').forEach(button => {
    button.addEventListener('click', () => { dataset = button.dataset.benchDataset; update(); });
  });
  search.addEventListener('input', update);
  scope.addEventListener('change', update);
  function revealTarget(hash) {
    let id;
    try { id = decodeURIComponent(hash.slice(1)); } catch { return; }
    const target = document.getElementById(id);
    if (!target || !dashboard.contains(target)) return;
    const section = target.closest('.bench-dataset');
    if (section) dataset = section.dataset.dataset;
    search.value = '';
    update();
    // Deep links must reveal both the cohort and the nested run details.
    for (let parent = target; parent; parent = parent.parentElement) {
      if (parent.tagName === 'DETAILS') parent.open = true;
    }
    target.scrollIntoView({ block: 'start' });
  }
  dashboard.querySelectorAll('a[data-bench-target]').forEach(link => {
    link.addEventListener('click', () => revealTarget(link.hash));
  });
  window.addEventListener('hashchange', () => revealTarget(window.location.hash));
  const pcaOverlay = dashboard.querySelector('.bench-pca-overlay');
  if (pcaOverlay) {
    const images = Array.from(pcaOverlay.querySelectorAll('.bench-pca-image'));
    const title = pcaOverlay.querySelector('#benchmark-pca-title');
    const state = pcaOverlay.querySelector('.bench-pca-state');
    let timer = 0;
    let dismissTimer = 0;
    let fadeTimer = 0;
    let active = null;
    let displayed = null;
    let loading = null;
    let shownSource = '';
    let generation = 0;
    function close() {
      window.clearTimeout(timer);
      window.clearTimeout(dismissTimer);
      window.clearTimeout(fadeTimer);
      timer = 0;
      generation += 1;
      if (active) active.setAttribute('aria-expanded', 'false');
      active = null;
      displayed = null;
      loading = null;
      shownSource = '';
      pcaOverlay.hidden = true;
      images.forEach(image => {
        image.onload = null;
        image.onerror = null;
        image.hidden = true;
        image.classList.remove('is-visible');
        image.removeAttribute('src');
      });
    }
    function show(trigger) {
      window.clearTimeout(timer);
      window.clearTimeout(dismissTimer);
      if (document.hidden || trigger.closest('.bench-dataset').hidden) { close(); return; }
      if (active === trigger && (loading === trigger || shownSource === trigger.dataset.pcaFull)) return;
      window.clearTimeout(fadeTimer);
      if (active) active.setAttribute('aria-expanded', 'false');
      active = trigger;
      loading = trigger;
      const current = ++generation;
      const image = images.find(candidate => candidate !== displayed);
      image.onload = null;
      image.onerror = null;
      image.classList.remove('is-visible');
      image.alt = trigger.dataset.pcaTitle;
      image.hidden = true;
      state.textContent = pcaOverlay.dataset.loading;
      state.hidden = displayed !== null;
      if (!displayed) title.textContent = trigger.dataset.pcaTitle;
      image.onload = () => {
        if (current !== generation) return;
        loading = null;
        const outgoing = displayed;
        image.hidden = false;
        // Commit opacity:0 before fading in, including cached image loads.
        void image.offsetWidth;
        image.classList.add('is-visible');
        if (outgoing) outgoing.classList.remove('is-visible');
        displayed = image;
        shownSource = trigger.dataset.pcaFull;
        title.textContent = trigger.dataset.pcaTitle;
        state.hidden = true;
        fadeTimer = window.setTimeout(() => {
          if (current === generation && outgoing) outgoing.hidden = true;
        }, 220);
      };
      image.onerror = () => {
        if (current !== generation) return;
        loading = null;
        state.textContent = pcaOverlay.dataset.error;
        state.hidden = false;
      };
      image.src = trigger.dataset.pcaFull;
      trigger.setAttribute('aria-expanded', 'true');
      pcaOverlay.hidden = false;
    }
    function schedule(trigger) {
      window.clearTimeout(dismissTimer);
      if (!pcaOverlay.hidden) { show(trigger); return; }
      window.clearTimeout(timer);
      timer = window.setTimeout(() => show(trigger), 500);
    }
    function leave() {
      window.clearTimeout(timer);
      window.clearTimeout(dismissTimer);
      // Bridge the short gap between thumbnails without hiding the window.
      if (pcaOverlay.hidden) close();
      else dismissTimer = window.setTimeout(close, 150);
    }
    dashboard.querySelectorAll('.current-cloud-preview').forEach(trigger => {
      trigger.addEventListener('pointerenter', event => {
        if (event.pointerType !== 'touch') schedule(trigger);
      });
      trigger.addEventListener('pointerleave', leave);
      trigger.addEventListener('pointercancel', close);
      trigger.addEventListener('focus', () => schedule(trigger));
      trigger.addEventListener('blur', leave);
      // Clicking or pressing Enter supports touch and keyboard users in-place.
      trigger.addEventListener('click', event => { event.preventDefault(); show(trigger); });
    });
    document.addEventListener('keydown', event => { if (event.key === 'Escape') close(); });
    document.addEventListener('pointerdown', event => {
      if (active && !active.contains(event.target)) close();
    });
    document.addEventListener('visibilitychange', () => { if (document.hidden) close(); });
    dashboard.addEventListener('toggle', close, true);
    window.addEventListener('scroll', close, { passive: true });
    window.addEventListener('resize', close);
    window.addEventListener('blur', close);
    closePcaPreview = close;
  }
  dashboard.querySelectorAll('.current-videos').forEach(group => {
    const videos = Array.from(group.querySelectorAll('video'));
    const play = group.querySelector('[data-video-play]');
    const pause = group.querySelector('[data-video-pause]');
    const speed = group.querySelector('.bench-video-speed');
    const speedValue = group.querySelector('.bench-video-speed-value');
    const status = group.querySelector('.bench-video-status');
    let action = 0;
    let leader = videos[0];
    let playing = false;
    let manualPause = false;
    let visible = false;
    let frame = 0;
    let lastCheck = 0;
    const expectedPlay = new WeakSet();
    const expectedPause = new WeakSet();
    const expectedSeek = new WeakMap();
    const loading = new WeakMap();
    function hidden() {
      return document.hidden || group.closest('.bench-dataset').hidden || group.closest('.bench-scene').hidden;
    }
    function pauseVideo(video) {
      if (!video.paused) {
        expectedPause.add(video);
        video.pause();
      }
    }
    function seekVideo(video, time) {
      const target = Math.max(0, Math.min(time, Number.isFinite(video.duration) ? video.duration : time));
      if (video.readyState >= 1 && Math.abs(video.currentTime - target) > .04) {
        expectedSeek.set(video, target);
        video.currentTime = target;
      }
    }
    function syncTime(source) {
      videos.forEach(video => { if (video !== source) seekVideo(video, source.currentTime); });
    }
    function setSpeed(rate = Number(speed.value)) {
      rate = Math.max(.25, Math.min(2, rate));
      speed.value = String(rate);
      speedValue.textContent = `${Number(speed.value)}×`;
      speed.setAttribute('aria-valuetext', speedValue.textContent);
      videos.forEach(video => { if (video.playbackRate !== Number(speed.value)) video.playbackRate = Number(speed.value); });
    }
    function tick(now) {
      if (!playing) return;
      if (now - lastCheck >= 250) {
        if (hidden()) { stop(); return; }
        videos.forEach(video => {
          if (video !== leader && !video.seeking && Math.abs(video.currentTime - leader.currentTime) > .12) seekVideo(video, leader.currentTime);
        });
        lastCheck = now;
      }
      frame = requestAnimationFrame(tick);
    }
    function ready(video) {
      if (video.readyState >= 1) return Promise.resolve();
      if (loading.has(video)) return loading.get(video);
      const pending = new Promise((resolve, reject) => {
        const timer = setTimeout(() => finish(new Error('Video metadata timeout')), 15000);
        function finish(error) {
          clearTimeout(timer);
          video.removeEventListener('loadedmetadata', loaded);
          video.removeEventListener('error', failed);
          loading.delete(video);
          error ? reject(error) : resolve();
        }
        const loaded = () => finish();
        const failed = () => finish(new Error('Video load failed'));
        video.addEventListener('loadedmetadata', loaded, { once: true });
        video.addEventListener('error', failed, { once: true });
        if (video.networkState !== 2) video.load();
      });
      loading.set(video, pending);
      return pending;
    }
    async function start(source = leader) {
      if (hidden()) return;
      const current = ++action;
      leader = source;
      playing = true;
      play.disabled = true;
      status.textContent = '';
      try {
        videoGroups.forEach((stop, other) => {
          if (other !== group) stop();
        });
        await Promise.all(videos.map(ready));
        if (current !== action || hidden()) return;
        const time = leader.ended ? 0 : leader.currentTime;
        videos.forEach(video => seekVideo(video, time));
        setSpeed();
        await Promise.all(videos.map(video => {
          if (video.paused) expectedPlay.add(video);
          return video.play();
        }));
        if (current !== action) return;
        cancelAnimationFrame(frame);
        frame = requestAnimationFrame(tick);
      } catch (error) {
        if (current !== action) return;
        stop();
        if (error.name !== 'AbortError') status.textContent = status.dataset.errorText;
      } finally {
        if (current === action) play.disabled = false;
      }
    }
    function stop() {
      ++action;
      playing = false;
      cancelAnimationFrame(frame);
      play.disabled = false;
      videos.forEach(pauseVideo);
      if (hidden() || !visible) videos.forEach(video => { video.controls = false; });
    }
    function userPlay(source = leader) {
      manualPause = false;
      start(source);
    }
    function userPause() {
      manualPause = true;
      stop();
    }
    videoGroups.set(group, stop);
    play.addEventListener('click', () => userPlay());
    pause.addEventListener('click', userPause);
    speed.addEventListener('input', () => setSpeed());
    speed.addEventListener('change', () => setSpeed());
    videos.forEach(video => {
      video.addEventListener('pointermove', event => {
        if (event.pointerType !== 'touch') video.controls = true;
      });
      video.addEventListener('pointerleave', event => {
        if (event.pointerType !== 'touch' && !video.matches(':focus-visible')) video.controls = false;
      });
      video.addEventListener('focus', () => {
        if (video.matches(':focus-visible')) video.controls = true;
      });
      video.addEventListener('blur', () => {
        if (!video.matches(':hover')) video.controls = false;
      });
      video.addEventListener('keydown', () => {
        video.controls = true;
      });
      video.addEventListener('pointerdown', event => {
        if (event.pointerType === 'touch') video.controls = true;
      });
      video.addEventListener('play', () => {
        if (!expectedPlay.delete(video)) userPlay(video);
      });
      video.addEventListener('pause', () => {
        if (!expectedPause.delete(video)) userPause();
      });
      video.addEventListener('seeking', () => {
        const target = expectedSeek.get(video);
        if (target !== undefined && Math.abs(video.currentTime - target) < .15) return;
        // Native loop seeks also align every other player to the new cycle.
        leader = video;
        syncTime(video);
      });
      video.addEventListener('seeked', () => expectedSeek.delete(video));
      video.addEventListener('ratechange', () => {
        if (video.playbackRate !== Number(speed.value)) setSpeed(video.playbackRate);
      });
    });
    document.addEventListener('pointerdown', event => {
      videos.forEach(video => {
        if (event.target !== video && !video.contains(event.target) &&
            (event.pointerType === 'touch' || !video.matches(':hover'))) video.controls = false;
      });
    });
    if ('IntersectionObserver' in window) {
      const observer = new IntersectionObserver(entries => {
        visible = entries[0].isIntersecting;
        if (!visible) stop();
        else if (!manualPause && !playing && !hidden()) start();
      }, { threshold: .15 });
      observer.observe(group.querySelector('.current-video-grid'));
    }
    document.addEventListener('visibilitychange', () => {
      if (document.hidden) stop();
      else if (visible && !manualPause && !playing && !hidden()) start();
    });
    setSpeed();
  });
  update();
  revealTarget(window.location.hash);
});
