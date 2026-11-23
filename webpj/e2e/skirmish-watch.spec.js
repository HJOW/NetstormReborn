// 스커미시를 지정한 시간 동안 실제 시간으로 지켜보며 30초마다 소유자별 객체 수·종류·사제 생존을 기록한다. AI 의 생산·공격 단계가 실제로 이어지는지, 사제가 죽을 때 승패 안내가 뜨는지 본다. 옵트인 시험이다. (2026-11-23 WB04-E1262)
// 사용: `$env:NETSTORM_WATCH_SECONDS=900; $env:NETSTORM_WATCH_AI=3; $env:NETSTORM_WATCH_DIFF=3; $env:NETSTORM_WATCH_ALLY=100; npx playwright test e2e/skirmish-watch.spec.js`
// 결과는 콘솔에 `WATCH {...}` 줄(JSON)로 남고, 마지막에 `WATCHEND` 요약이 나온다. AI 가 프레임 수에 묶여 있어 가짜 시계 빨리 감기로는 대신할 수 없다(docs/webpj-fast-forward-clock-20261121.md).
const { test, expect } = require('@playwright/test');

const SECONDS = Number(process.env.NETSTORM_WATCH_SECONDS ?? 0);
const AI = Number(process.env.NETSTORM_WATCH_AI ?? 3);
const DIFF = Number(process.env.NETSTORM_WATCH_DIFF ?? 3);
// 동맹 여부를 소유자 2 부터 한 글자씩(1 동맹·0 적)으로 적은 문자열이다.
const ALLY = String(process.env.NETSTORM_WATCH_ALLY ?? '100').padEnd(7, '0');

async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => { const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height }; }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test(`스커미시 관찰: AI ${AI}명 난이도 ${DIFF} 동맹 ${ALLY}`, async ({ page }) => {
  test.skip(!(SECONDS > 0), 'NETSTORM_WATCH_SECONDS 를 설정한 실행에서만 돈다');
  test.setTimeout((SECONDS + 300) * 1000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error' && !message.text().includes('Islandlist.cpp:676 bcr.isValid()')) errors.push(message.text()); });
  const values = { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: AI };
  for (let owner = 2; owner <= 8; owner += 1) { values[`skirmishAi${owner}Diff`] = DIFF; values[`skirmishAi${owner}Ally`] = Number(ALLY[owner - 2]); }
  await page.addInitScript((settings) => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: settings })), values);
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!');
  // 소유자별 객체 수와 TYPE 이름별 개수(상위), 사제 생존, 열린 안내 창 단추를 읽는다.
  const sample = () => page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), types = app._gameAssets.types(); const owners = {};
    for (const { object } of session.world().objects()) { const bucket = owners[object.owner] ??= { total: 0, kinds: {} }; bucket.total += 1; const name = types[object.type - 70]?.assetName ?? `t${object.type}`; bucket.kinds[name] = (bucket.kinds[name] ?? 0) + 1; }
    // 지형·다리·섬 조각은 따로 세고 나머지(건물·유닛·사제 등)의 종류별 개수를 위에서부터 보인다.
    const terrain = new Set(['isle', 'fringe', 'edgeFarm', 'noIsland', 'island', 'islandStalag', 'geyser', 'bridge', 'bridgeConnector', 'player']);
    const compact = {}; for (const [owner, bucket] of Object.entries(owners)) compact[owner] = { bridges: bucket.kinds.bridge ?? 0, islands: (bucket.kinds.island ?? 0) + (bucket.kinds.isle ?? 0) * 0, things: Object.entries(bucket.kinds).filter(([name]) => !terrain.has(name) && name !== 'isle').sort((a, b) => b[1] - a[1]).slice(0, 10) };
    const dead = {}; for (let owner = 1; owner <= 8; owner += 1) { try { dead[owner] = session.missionPriestDead(owner); } catch { dead[owner] = null; } }
    return { game: Math.round(app._gameClock.gameSeconds()), owners: compact, dead, buttons: app._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null }; });
  const samples = []; const startedAt = Date.now();
  for (let tick = 0; tick < SECONDS && errors.length === 0; tick += 1) {
    await page.waitForTimeout(1000);
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (label !== null) { console.log('WATCHDIALOG', tick, JSON.stringify(await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label))), 'game', await page.evaluate(() => Math.round(window.netstorm._gameClock.gameSeconds()))); await clickLabel(page, label); }
    if (tick % 30 === 0) { const row = await sample(); samples.push(row); console.log('WATCH', JSON.stringify({ t: tick, ...row })); }
  }
  const last = samples.at(-1);
  console.log('WATCHEND', JSON.stringify({ seconds: Math.round((Date.now() - startedAt) / 1000), game: last?.game, errors: errors.length }));
  expect(errors).toEqual([]);
});
