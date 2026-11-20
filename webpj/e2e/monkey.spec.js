// 임의 클릭·키·메뉴 조작(몽키)으로 미션 화면의 오류와 입력 큐 먹통을 찾는다. NETSTORM_MONKEY_SECONDS 를 정한 실행에서만 돈다(조합 3개 × 그 시간).
// 시드가 고정이라 같은 조작열이 재현되며, 오류나 큐 머리가 0.75초 넘게 남는 먹통이 나오면 바로 멈추고 마지막 조작 8개를 알린다. 안내 창은 가끔 마지막 단추로 닫는다. (2026-11-20 WB04-E1128)
// 사용: `$env:NETSTORM_MONKEY_SECONDS=150; npx playwright test e2e/monkey.spec.js --workers=1`
const { test, expect } = require('@playwright/test');
// 조작 시간(초)이다. 설정하지 않으면 건너뛴다.
const SECONDS = Number(process.env.NETSTORM_MONKEY_SECONDS ?? 0);
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}
// 재현 가능한 난수(시드 고정)이다.
function rng(seed) { let s = seed >>> 0; return () => { s = (Math.imul(s, 1664525) + 1013904223) >>> 0; return s / 4294967296; }; }
// 미션 화면에서 쓰는 단축키와 임의 키이다.
const KEYS = ['p', 'r', 'n', 'q', 'w', 'a', 's', 'z', 'y', 'x', 'e', 'Escape', 'F2', 'F3', 'F4', 'F5', 'F6', 'F8', 'F9', 'Shift+F9', 't', 'Shift+F3', 'c', 'Tab', 'Enter', 'Space', 'h', 'ArrowUp', 'ArrowLeft'];
for (const scenario of [{ name: 'campaign-1-1', seed: 11 }, { name: 'skirmish-3', seed: 22 }, { name: 'campaign-1-1-b', seed: 33 }]) {
  test(`monkey ${scenario.name}`, async ({ page }) => {
    test.skip(!(SECONDS > 0), 'NETSTORM_MONKEY_SECONDS 를 설정한 실행에서만 돈다');
    test.setTimeout((SECONDS + 150) * 1000);
    const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error' && !message.text().includes('Islandlist.cpp:676')) errors.push(message.text()); });
    const settings = { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, skirmishAiCount: 3 };
    await page.addInitScript((v) => { localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: v })); const done = {}; for (const k of ['Tutorial1','Tutorial2','Tutorial3','Tutorial4','Tutorial5','Tutorial6']) done['Done' + k] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done })); }, settings);
    await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
    await clickLabel(page, 'Campaign');
    if (scenario.name.startsWith('skirmish')) { await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start'); }
    else { await clickLabel(page, 'Struggle For Freedom'); const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label); await clickLabel(page, label); }
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
    for (let i = 0; i < 6; i += 1) { const l = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null); if (l === null) break; await clickLabel(page, l); await page.waitForTimeout(400); }
    const next = rng(scenario.seed), actions = []; let stuck = null;
    const started = Date.now();
    while (Date.now() - started < SECONDS * 1000 && errors.length === 0 && !stuck) {
      const r = next(); let action;
      if (r < 0.35) { const x = 90 + Math.floor(next() * 930), y = 20 + Math.floor(next() * 740); action = ['left', x, y]; await page.mouse.click(x, y, { delay: 30 }); }
      else if (r < 0.6) { const x = 90 + Math.floor(next() * 930), y = 20 + Math.floor(next() * 740); action = ['right', x, y]; await page.mouse.click(x, y, { button: 'right', delay: 30 }); }
      else if (r < 0.85) { const key = KEYS[Math.floor(next() * KEYS.length)]; action = ['key', key]; await page.keyboard.press(key); }
      else if (r < 0.92) { const x = 2 + Math.floor(next() * 78), y = 20 + Math.floor(next() * 740); action = ['left-hud', x, y]; await page.mouse.click(x, y, { delay: 30 }); }
      else { const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null); action = ['dialog', label]; if (label !== null) await clickLabel(page, label); }
      actions.push(action); if (actions.length > 12) actions.shift();
      await page.waitForTimeout(150);
      const head = await page.evaluate(() => { const q = window.netstorm._clientInputQueue; const h = q?.peek(); return h ? h.code.toString(16) : null; });
      if (head) { await page.waitForTimeout(600); const again = await page.evaluate(() => { const q = window.netstorm._clientInputQueue; const h = q?.peek(); return h ? h.code.toString(16) : null; }); if (again === head) stuck = head; }
      if (!(await page.evaluate(() => Boolean(window.netstorm?._campaignWorld)))) { actions.push(['LEFT-MISSION']); break; }
    }
    expect(errors, `${scenario.name} 마지막 조작 ${JSON.stringify(actions.slice(-8))}`).toEqual([]); expect(stuck, `${scenario.name} 입력 큐 머리 ${stuck}, 마지막 조작 ${JSON.stringify(actions.slice(-8))}`).toBeNull();
  });
}
