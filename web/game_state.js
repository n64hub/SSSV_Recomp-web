// Evaluated in the page by N64Recomp-browser's tools/play_page.mjs (web/play_plan_extended.json's evalFile step), with
// n64r, RDRAM words at N64 addresses. Addresses are the game's data symbols (SSSVRecompSyms' sssv.us.datasyms.toml,
// named as the decompilation names them).
//
//   sssv.level()  gGameState.level (0x803F2D38), the level playing: 1 Smashing Start ... 32 (include/levels.h).
//   sssv.warp(level, ms)  starts that level by the game's own level loader (Module._sssv_web_warp, src/game/recomp_api.cpp)
//       and resolves true once gGameState.level is that level, or false after ms.
//   sssv.clock()  the plan's clock, in ms of the game's time: gGameplayTick (0x803D5544), which the game steps once a game
//       frame, counted at 15 a second (about what the game runs at in Chrome). The tick stops while a screen holds the
//       game for A (a hint, a level's message), and a plan's presses on a clock that stands still would wait for it
//       forever, so once the tick has stood still for 0.5 s this counts the browser's time instead.
//   sssv.state()  [level, the player's animal (gCurrentAnimalIndex), its health], for the console.
//   sssv.protect()  keeps the player alive from here on, as the desktop's test control does (SSSV_TEST_CONTROL's .protect,
//       src/game/recomp_api.cpp): every 100 ms, a player's animal below full health (127) is put back at 127. A warped
//       level can't end: the screens after it read what the menus before it would have set up.
// Evaluating the file defines these once, starts protect() and gives back state().
(() => {
  const ANIMALS = 0x801D9ED8 + 0x3EB0;  // gAnimalState.animals: 8 bytes an animal, the actor's address at +4
  let base = null;
  // RDRAM's offset in the heap, found as page_driver.mjs's n64r finds it.
  const find = () => {
    const h = HEAPU32;
    for (let i = 2; i < h.length - 8; i++) {
      if (h[i] === 0xB0000000 && h[i - 2] <= 2 && (h[i + 4] === 0x400000 || h[i + 4] === 0x800000)) return i * 4 - 0x308;
    }
    throw new Error('RDRAM not found');
  };
  const half = (a) => (n64r(a & ~3) << ((a & 2) * 8)) >> 16;  // a signed halfword
  // A halfword written alone, so the game's own writes to its neighbour are left as they are.
  const setHalf = (a, v) => { base ??= find(); new Uint16Array(HEAPU32.buffer)[(base + ((a - 0x80000000) ^ 2)) >>> 1] = v; };
  const player = () => {
    const i = half(0x803D5534);
    if (i < 0 || i >= 50) return null;
    const actor = n64r(ANIMALS + i * 8 + 4);
    return actor >= 0x80000000 && actor <= 0x807FF000 ? { i, actor } : null;
  };
  const sssv = window.sssv ??= {
    level: () => half(0x803F2D38),
    warp: (level, ms) => {
      _sssv_web_warp(level);
      return new Promise((done) => {
        const t0 = Date.now();
        const t = setInterval(() => {
          if (sssv.level() === level || Date.now() - t0 > ms) { clearInterval(t); done(sssv.level() === level); }
        }, 100);
      });
    },
    clock: () => {
      const t = n64r(0x803D5544) >>> 16, now = performance.now();
      const c = sssv.clocked ??= { t, at: now, last: now, ms: 0 };
      const d = (t - c.t) & 65535;
      if (d > 0 && d < 300) c.ms += d * 1000 / 15;
      if (d !== 0) { c.t = t; c.at = now; }
      else if (now - c.at > 500) c.ms += now - c.last;
      c.last = now;
      return Math.floor(c.ms);
    },
    state: () => {
      const p = player();
      return [sssv.level(), p ? p.i : null, p ? half(p.actor + 0x14C) : null];
    },
    protect: () => {
      sssv.protecting ??= setInterval(() => {
        const p = player();
        if (!p) return;
        const health = half(p.actor + 0x14C);
        // Zero health also marks the body left behind as the player takes another animal: never revive that one.
        if (health > 0 && health < 127 && half(p.actor + 12) < 15000) setHalf(p.actor + 0x14C, 127);
      }, 100);
      return true;
    },
  };
  sssv.protect();
  return sssv.state();
})()
