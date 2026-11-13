/* Overview page, view routing, and the two small 3D scenes (hero and console mark). Display only: it reads nothing
   but /api/ranges and writes nothing. The console's own script (inline in index.html) is untouched by this file. */
(function () {
  "use strict";
  const $ = id => document.getElementById(id);
  const reduced = window.matchMedia && matchMedia("(prefers-reduced-motion: reduce)").matches;
  const TIERCOL = { t868: 0x8ea2c8, t24: 0x38bdf8, uwb: 0xf472b6 };
  const hex = n => "#" + n.toString(16).padStart(6, "0");

  // ---------- view routing: #/console shows the console, anything else the overview ----------
  let inConsole = false;
  let routed = false;
  function route() {
    const want = location.hash === "#/console";
    const apply = () => {
      inConsole = want;
      document.body.classList.toggle("in-console", inConsole);
      $("landing").inert = inConsole; $("app").inert = !inConsole;
      document.title = inConsole ? "CA-TBL Search Console" : "CA-TBL: confidence-aware localization";
      if (inConsole) { stopLanding(); }          // the console is laid out while hidden (visibility, not display), so it needs no resize
      else { startLanding(); $("landing").scrollTo({ top: 0 }); }
    };
    if (!routed || reduced) { routed = true; apply(); return; }
    // warm the incoming view while it is still transparent, then switch two frames later
    const incoming = $(want ? "app" : "landing");
    incoming.classList.add("warm");
    if (!want) startLanding();
    requestAnimationFrame(() => requestAnimationFrame(() => { apply(); setTimeout(() => incoming.classList.remove("warm"), 900); }));
  }
  window.addEventListener("hashchange", route);
  document.querySelectorAll("[data-go]").forEach(b => b.addEventListener("click", e => {
    const t = b.getAttribute("data-go");
    if (t === "console") { location.hash = "#/console"; }
    else if (t === "home") { location.hash = "#/"; }
    else { e.preventDefault(); const el = $(t); if (el) $("landing").scrollTo({ top: el.offsetTop - 70, behavior: reduced ? "auto" : "smooth" }); }
  }));

  // ---------- animated network background (overview only) ----------
  const bg = $("bgCanvas"), bctx = bg.getContext("2d");
  let dots = [], bgRaf = 0, mouse = { x: .5, y: .5 };
  function sizeBg() {
    const r = Math.min(window.devicePixelRatio || 1, 1.25);
    bg.width = innerWidth * r; bg.height = innerHeight * r; bctx.setTransform(r, 0, 0, r, 0, 0);
    const n = Math.round(Math.min(55, innerWidth * innerHeight / 26000));
    dots = Array.from({ length: n }, () => ({ x: Math.random() * innerWidth, y: Math.random() * innerHeight, vx: (Math.random() - .5) * .22, vy: (Math.random() - .5) * .22, r: Math.random() * 1.4 + .6, z: Math.random() * .8 + .2 }));
  }
  let bgLast = 0;
  function bgFrame(now) {
    if (!reduced) bgRaf = requestAnimationFrame(bgFrame);
    if (now && now - bgLast < 33) return;                 // about 30 fps is plenty for slow drift
    const k = now && bgLast ? Math.min((now - bgLast) / 16.7, 3) : 1; bgLast = now || 0;
    bctx.clearRect(0, 0, innerWidth, innerHeight);
    const ox = (mouse.x - .5) * 26, oy = (mouse.y - .5) * 26;
    for (const d of dots) { d.x += d.vx * k; d.y += d.vy * k; if (d.x < -20) d.x = innerWidth + 20; if (d.x > innerWidth + 20) d.x = -20; if (d.y < -20) d.y = innerHeight + 20; if (d.y > innerHeight + 20) d.y = -20; }
    for (let i = 0; i < dots.length; i++) {
      const a = dots[i], ax = a.x + ox * a.z, ay = a.y + oy * a.z;
      for (let j = i + 1; j < dots.length; j++) {
        const b = dots[j], bx = b.x + ox * b.z, by = b.y + oy * b.z, dx = ax - bx, dy = ay - by, d2 = dx * dx + dy * dy;
        if (d2 < 19600) { bctx.strokeStyle = `rgba(129,160,255,${(1 - d2 / 19600) * .2})`; bctx.beginPath(); bctx.moveTo(ax, ay); bctx.lineTo(bx, by); bctx.stroke(); }
      }
      bctx.fillStyle = `rgba(190,215,255,${.25 + a.z * .45})`; bctx.beginPath(); bctx.arc(ax, ay, a.r, 0, 7); bctx.fill();
    }
  }
  addEventListener("pointermove", e => { mouse.x = e.clientX / innerWidth; mouse.y = e.clientY / innerHeight; }, { passive: true });
  addEventListener("resize", () => { if (!inConsole) sizeBg(); });

  // ---------- hero scene: three stacked glass planes, one per radio tier ----------
  let hero = null;
  function hasGL() { try { const c = document.createElement("canvas"); return !!(window.WebGLRenderingContext && (c.getContext("webgl") || c.getContext("experimental-webgl"))); } catch (e) { return false; } }
  function makeHero(canvas, stage) {
    const T = window.THREE; if (!T || !hasGL()) { stage.classList.add("nogl"); return null; }
    let renderer; try { renderer = new T.WebGLRenderer({ canvas, antialias: true, alpha: true }); } catch (e) { stage.classList.add("nogl"); return null; }
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 1.5));
    const scene = new T.Scene(), cam = new T.PerspectiveCamera(38, 1, .1, 100);
    cam.position.set(0, 3.4, 11.6); cam.lookAt(0, 0, 0);
    scene.add(new T.AmbientLight(0x8fa6ff, .55));
    const l1 = new T.PointLight(0x38bdf8, 1.6, 40); l1.position.set(-5, 4, 6); scene.add(l1);
    const l2 = new T.PointLight(0xe879f9, 1.3, 40); l2.position.set(6, -2, 5); scene.add(l2);
    const root = new T.Group(); scene.add(root);
    const tiers = [
      { key: "t868", y: -1.8, r: 3.05, name: "868 MHz LoRa", sub: "coverage floor" },
      { key: "t24", y: 0, r: 2.0, name: "2.4 GHz LoRa", sub: "time-of-flight ranging" },
      { key: "uwb", y: 1.8, r: 1.0, name: "Ultra-wideband", sub: "decimetre precision" }
    ];
    const plates = [];
    for (const t of tiers) {
      const col = TIERCOL[t.key], g = new T.Group(); g.position.y = t.y;
      const disc = new T.Mesh(new T.CylinderGeometry(t.r, t.r, .07, 72), new T.MeshStandardMaterial({ color: col, transparent: true, opacity: .2, roughness: .15, metalness: .35, emissive: col, emissiveIntensity: .12 }));
      const edge = new T.Mesh(new T.TorusGeometry(t.r, .028, 10, 120), new T.MeshStandardMaterial({ color: col, emissive: col, emissiveIntensity: .9, roughness: .3 }));
      edge.rotation.x = Math.PI / 2;
      g.add(disc, edge);
      for (const f of [.34, .67]) { const ring = new T.Mesh(new T.TorusGeometry(t.r * f, .008, 6, 90), new T.MeshBasicMaterial({ color: col, transparent: true, opacity: .28 })); ring.rotation.x = Math.PI / 2; g.add(ring); }
      root.add(g); plates.push({ t, g, disc, edge, base: .2 });
    }
    // gateways: small towers on the coverage plane
    const gwMat = new T.MeshStandardMaterial({ color: 0x60a5fa, emissive: 0x2563eb, emissiveIntensity: .8, roughness: .3 });
    for (let i = 0; i < 7; i++) { const a = i / 7 * Math.PI * 2 + .3, r = i === 0 ? 0 : 2.1 + (i % 2) * .45; const m = new T.Mesh(new T.ConeGeometry(.1, .36, 14), gwMat); m.position.set(Math.cos(a) * r, -1.8 + .22, Math.sin(a) * r); root.add(m); }
    // searchers: green markers orbiting the ranging plane, converging inward over time
    const sMat = new T.MeshStandardMaterial({ color: 0x22c55e, emissive: 0x16a34a, emissiveIntensity: .9, roughness: .35 });
    const seek = Array.from({ length: 4 }, (_, i) => { const m = new T.Mesh(new T.BoxGeometry(.15, .15, .15), sMat); root.add(m); return { m, a: i * 1.57, s: .35 + i * .08 }; });
    // the missing node
    const miss = new T.Mesh(new T.OctahedronGeometry(.2), new T.MeshStandardMaterial({ color: 0xef4444, emissive: 0xdc2626, emissiveIntensity: 1, roughness: .3 }));
    miss.position.y = 1.8 + .22; root.add(miss);
    // escalation pulses travelling up the spine between the planes
    const spine = new T.Mesh(new T.CylinderGeometry(.012, .012, 3.7, 8), new T.MeshBasicMaterial({ color: 0xa5b4fc, transparent: true, opacity: .4 }));
    root.add(spine);
    const pulses = Array.from({ length: 3 }, (_, i) => { const m = new T.Mesh(new T.SphereGeometry(.07, 14, 14), new T.MeshBasicMaterial({ color: 0xffffff })); root.add(m); return { m, p: i / 3 }; });
    // dust
    const pts = new Float32Array(300); for (let i = 0; i < 300; i++) pts[i] = (Math.random() - .5) * 16;
    const dustG = new T.BufferGeometry(); dustG.setAttribute("position", new T.BufferAttribute(pts, 3));
    scene.add(new T.Points(dustG, new T.PointsMaterial({ color: 0x9fb4ff, size: .035, transparent: true, opacity: .6 })));
    // overlay tags
    const tags = plates.map(p => { const d = document.createElement("div"); d.className = "tag3d"; d.style.setProperty("--c", hex(TIERCOL[p.t.key])); d.innerHTML = `<b><i style="background:${hex(TIERCOL[p.t.key])}"></i>${p.t.name}</b><small>${p.t.sub}</small>`; stage.appendChild(d); return d; });
    // interaction: drag to orbit with inertia; hover a plane to light it
    let rx = -.34, ry = .5, vx = 0, vy = 0, drag = false, lx = 0, ly = 0, hover = -1, mx = 0, my = 0, idle = 0, onScreen = true, moved = true;
    const ray = new T.Raycaster(), ndc = new T.Vector2();
    canvas.addEventListener("pointerdown", e => { drag = true; lx = e.clientX; ly = e.clientY; canvas.setPointerCapture(e.pointerId); canvas.classList.add("drag"); });
    canvas.addEventListener("pointerup", e => { drag = false; canvas.classList.remove("drag"); });
    canvas.addEventListener("pointermove", e => {
      const b = canvas.getBoundingClientRect(); ndc.set((e.clientX - b.left) / b.width * 2 - 1, -((e.clientY - b.top) / b.height * 2 - 1)); mx = ndc.x; my = ndc.y; idle = 0; moved = true;
      if (drag) { vy = (e.clientX - lx) * .006; vx = (e.clientY - ly) * .004; ry += vy; rx = Math.max(-1.1, Math.min(.5, rx + vx)); lx = e.clientX; ly = e.clientY; }
    });
    canvas.addEventListener("pointerleave", () => { hover = -1; mx = my = 0; moved = false; });
    function resize() { const w = stage.clientWidth, h = stage.clientHeight; if (!w || !h) return; renderer.setSize(w, h, false); cam.aspect = w / h; cam.updateProjectionMatrix(); }
    new ResizeObserver(resize).observe(stage); resize();
    if ("IntersectionObserver" in window) new IntersectionObserver(es => { onScreen = es[0].isIntersecting; }, { root: $("landing") }).observe(stage);
    const v = new T.Vector3(), clock = new T.Clock();
    // adaptive quality: a slow GPU gets a lower pixel ratio first, then half the frame rate, so motion stays even rather than stuttering
    let ema = 16, seen = 0, tier = 0, skip = false;
    function adapt(raw) {
      if (raw > .25) return;                                 // a tab switch or a stall, not a measure of load
      ema += (raw * 1000 - ema) * .1; if (++seen < 40) return;
      if (tier === 0 && ema > 30) { tier = 1; seen = 0; renderer.setPixelRatio(1); resize(); }
      else if (tier === 1 && ema > 38) { tier = 2; seen = 0; }
    }
    function frame() {
      const raw = clock.getDelta(), dt = Math.min(raw, .05), t = clock.elapsedTime;
      if (!onScreen) return;                               // scrolled past the hero: no drawing at all
      adapt(raw); if (tier === 2) { skip = !skip; if (skip) return; }
      if (!drag) { ry += vy; vy *= .94; rx += vx; vx *= .94; idle += dt; if (!reduced && idle > 1.2) ry += dt * .18; rx += ((-.34 + my * -.12) - rx) * .03; }
      root.rotation.y = ry; root.rotation.x = rx; root.position.y = reduced ? 0 : Math.sin(t * .8) * .06;
      // hover test against the three planes
      if (moved) { moved = false; ray.setFromCamera(ndc, cam); const hit = ray.intersectObjects(plates.map(p => p.disc), false)[0]; hover = hit && !drag ? plates.findIndex(p => p.disc === hit.object) : -1; }
      plates.forEach((p, i) => { const on = i === hover; p.disc.material.opacity += ((on ? .42 : p.base) - p.disc.material.opacity) * .12; p.edge.material.emissiveIntensity += ((on ? 1.8 : .9) - p.edge.material.emissiveIntensity) * .12; const s = on ? 1.035 : 1; p.g.scale.x += (s - p.g.scale.x) * .12; p.g.scale.z = p.g.scale.x; });
      seek.forEach((s, i) => { const k = (Math.sin(t * .35 + i) * .5 + .5), r = 2.0 - k * 1.2; s.a += dt * s.s; s.m.position.set(Math.cos(s.a) * r, 0 + .15, Math.sin(s.a) * r); s.m.rotation.y = s.a; });
      miss.rotation.y += dt * 1.2; miss.rotation.x += dt * .6;
      pulses.forEach(p => { p.p = (p.p + dt * .22) % 1; p.m.position.y = -1.8 + p.p * 3.6; p.m.scale.setScalar(.6 + Math.sin(p.p * Math.PI) * .9); });
      renderer.render(scene, cam);
      plates.forEach((p, i) => { v.set(p.t.r * .72, p.t.y + .1, p.t.r * .72).applyMatrix4(root.matrixWorld).project(cam); const x = (v.x * .5 + .5) * stage.clientWidth, y = (-v.y * .5 + .5) * stage.clientHeight; const tw = tags[i].offsetWidth, th = tags[i].offsetHeight, tx = Math.max(8, Math.min(stage.clientWidth - tw - 8, x - tw * .25)), ty = Math.max(8, Math.min(stage.clientHeight - th - 44, y - th * .5)); const tf = `translate(${Math.round(tx)}px,${Math.round(ty)}px)`; if (tags[i]._tf !== tf) { tags[i]._tf = tf; tags[i].style.transform = tf; } tags[i].classList.toggle("on", i === hover); });
    }
    return { frame };
  }

  // ---------- console mark: three nested rings ----------
  const marks = [];
  function makeMark(canvas) {
    const T = window.THREE; if (!T || !hasGL()) return null;
    let renderer; try { renderer = new T.WebGLRenderer({ canvas, antialias: true, alpha: true }); } catch (e) { return null; }
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2)); renderer.setSize(canvas.clientWidth || 42, canvas.clientHeight || 42, false);
    const scene = new T.Scene(), cam = new T.PerspectiveCamera(40, 1, .1, 20); cam.position.z = 4.4;
    scene.add(new T.AmbientLight(0xffffff, .7)); const pl = new T.PointLight(0xffffff, 1.2, 20); pl.position.set(2, 2, 4); scene.add(pl);
    const g = new T.Group(); scene.add(g);
    const rings = [[1.45, TIERCOL.t868], [1.0, TIERCOL.t24], [.58, TIERCOL.uwb]].map(([r, c], i) => { const m = new T.Mesh(new T.TorusGeometry(r, .075, 12, 64), new T.MeshStandardMaterial({ color: c, emissive: c, emissiveIntensity: .8, roughness: .3 })); m.rotation.x = i * .7; g.add(m); return m; });
    g.add(new T.Mesh(new T.SphereGeometry(.2, 16, 16), new T.MeshBasicMaterial({ color: 0xffffff })));
    canvas.parentElement.classList.add("gl");
    return { frame(t) { rings.forEach((m, i) => { m.rotation.y = t * (.5 + i * .35); m.rotation.z = t * (.2 + i * .15); }); g.rotation.x = Math.sin(t * .4) * .3; renderer.render(scene, cam); } };
  }

  // ---------- reveal on scroll, tilt cards, live reach ----------
  const io = "IntersectionObserver" in window ? new IntersectionObserver(es => es.forEach(e => { if (e.isIntersecting) { e.target.classList.add("in"); io.unobserve(e.target); } }), { root: $("landing"), threshold: .12 }) : null;
  document.querySelectorAll(".reveal").forEach((el, i) => { if (io) io.observe(el); else el.classList.add("in"); });
  document.querySelectorAll(".tilt").forEach(el => {
    if (reduced) return;
    let pend = null;
    el.addEventListener("pointermove", e => { if (pend) { pend = e; return; } pend = e; requestAnimationFrame(() => { const ev = pend; pend = null; const b = el.getBoundingClientRect(), x = (ev.clientX - b.left) / b.width, y = (ev.clientY - b.top) / b.height; el.style.setProperty("--ry", ((x - .5) * 8).toFixed(2) + "deg"); el.style.setProperty("--rx", ((.5 - y) * 6).toFixed(2) + "deg"); el.style.setProperty("--mx", (x * 100) + "%"); el.style.setProperty("--my", (y * 100) + "%"); }); });
    el.addEventListener("pointerleave", () => { el.style.setProperty("--rx", "0deg"); el.style.setProperty("--ry", "0deg"); });
  });
  const fmt = m => m >= 1000 ? (m / 1000).toFixed(2) + " km" : (m >= 100 ? Math.round(m) : m.toFixed(1)) + " m";
  function reach(tries) {
    fetch("/api/ranges").then(r => r.json()).then(d => {
      // r80 can be 0 when even the shortest test distance delivers under 80 %; then the 50 % distance is the honest figure
      if (d.state === "ok") { [["r868", "868"], ["r24", "2p4"], ["rUwb", "uwb"]].forEach(([id, k]) => { const el = $(id), t = d.tiers[k]; if (el && t) { el.textContent = t.r80 > 0 ? fmt(t.r80) : fmt(t.r50) + " at 50 %"; } }); }
      else if (tries > 0 && d.state !== "error") setTimeout(() => reach(tries - 1), 4000);
      else ["r868", "r24", "rUwb"].forEach(id => { $(id).textContent = "n/a"; });
    }).catch(() => ["r868", "r24", "rUwb"].forEach(id => { $(id).textContent = "n/a"; }));
  }

  // ---------- run loop: landing animations stop while the console is open or the tab is hidden ----------
  let raf = 0, running = false;
  function loop() { if (!running) return; if (hero) hero.frame(); raf = requestAnimationFrame(loop); }
  // the small console marks keep turning in both views (a few hundred triangles each)
  let markRaf = 0;
  let markLast = 0;
  function markLoop(now) { markRaf = requestAnimationFrame(markLoop); if (now - markLast < 41) return; markLast = now; const t = now / 1000; marks.forEach(m => m.frame(t)); }
  function startLanding() { if (running) return; running = true; sizeBg(); bgLast = 0; if (!reduced) bgRaf = requestAnimationFrame(bgFrame); else bgFrame(0); loop(); }
  function stopLanding() { running = false; cancelAnimationFrame(raf); cancelAnimationFrame(bgRaf); }
  document.addEventListener("visibilitychange", () => { if (document.hidden) { stopLanding(); cancelAnimationFrame(markRaf); } else { if (!inConsole) startLanding(); if (!reduced && marks.length) markRaf = requestAnimationFrame(markLoop); } });

  hero = makeHero($("heroGl"), $("stage"));
  document.querySelectorAll("canvas.markGl").forEach(c => { const m = makeMark(c); if (m) marks.push(m); });
  if (!reduced && marks.length) markRaf = requestAnimationFrame(markLoop);
  reach(8);
  route();
  if (reduced) { if (hero) hero.frame(); marks.forEach(m => m.frame(0)); }
})();
