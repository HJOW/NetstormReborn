// 허용 미션에서만 아군 AI를 실제로 만들고 현재 Player 상태 보존·자연 진행을 확인한다.
const { test, expect } = require('@playwright/test');
const { installAllyAi } = require('./ally-ai-test-control');

// 허용 목록 밖 미션은 페이지에 접근하기 전에 거부해 실수로 공격·포획 시험을 실행하지 않게 한다.
test('아군 AI 검사: 허용 목록 밖 미션은 브라우저 상태를 읽기 전에 거부한다', async () => {
  // 이 장치는 호출되면 실패하므로 금지 미션에서 실제 페이지 접근이 없음을 확인한다.
  const page = { evaluate: () => { throw new Error('금지 미션에서 페이지 접근'); } };
  // Tutorial·후반 캠페인·알 수 없는 이름은 모두 같은 제한을 적용한다.
  for (const mission of ['DissolvedAlliance', 'Tutorial1', 'Tutorial6', 'thenoose', 'finalconfrontation', 'unknown']) await expect(installAllyAi(page, mission)).rejects.toThrow('허용 미션');
  // 난이도 보존 선택도 허용 미션 범위를 넓히지 않으며 잘못된 선택값은 페이지 접근 전에 거부한다.
  await expect(installAllyAi(page, 'TheNoose', { preserveDifficulty: true })).rejects.toThrow('허용 미션');
  await expect(installAllyAi(page, 'Skirmish', { preserveDifficulty: 1 })).rejects.toThrow('boolean');
});

// 정량 관찰에서는 공용 활성화기가 스커미시의 적 어려움을 쉬움으로 덮지 않아야 한다.
test('아군 AI 검사: 스커미시의 원래 개별 난이도를 보존한다', async ({ page }) => {
  test.setTimeout(240000);
  // 실제 프레임 오류만 수집하고 현재 자금·기술·동맹을 직접 바꾸지 않는다.
  const errors = [];
  page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.setViewportSize({ width: 1024, height: 768 });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0, difficulty: 2, skirmishAiCount: 1, skirmishAi2Diff: 3, skirmishAi2Ally: 0 } })));
  await page.goto('/debug.html?fixed=25&aspect=4:3&title=1&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
  await page.locator('canvas').click(); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 실제 시작 그림과 동일한 월드 적재 경로를 실행한다.
  while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await installAllyAi(page, 'Skirmish', { preserveDifficulty: true });
  await page.evaluate(async () => { const app = window.netstorm, { buildSkirmishLaunch } = await import('./core/skirmish-setup.js'); await app._beginSkirmish(buildSkirmishLaunch(app._loadSkirmishSettings(), 1)); });
  // 비동기 적재를 원래 프레임으로 기다린다. 미션 안내와 실제 Session 준비가 모두 필요하다.
  for (let frame = 0; frame < 1500; frame += 1) {
    if (await page.evaluate(() => Boolean(window.netstorm._mainMenu.controlPoint('Go!')) && Boolean(window.netstorm._campaignWorld?.session().localConnectionReady()))) break;
    await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  }
  // 시작 안내만 닫고 이후 모든 조작은 실제 아군 AI가 수행한다.
  const point = await page.evaluate(() => { const app = window.netstorm, p = app._mainMenu.controlPoint('Go!'), rect = app.canvas.getBoundingClientRect(); if (!p) throw new Error('스커미시 실제 시작 단추 없음'); return { x: rect.left + p.x * rect.width / 1024, y: rect.top + p.y * rect.height / 768 }; });
  await page.mouse.click(point.x, point.y); await page.evaluate(() => window.netstorm.stepFixedFrames(300));
  const state = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), raw = session.aiMemory().bytes(session.participation().brain(2)), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); return { activated: window.netstormAllyTestState, rates: [0x220, 0x224, 0x228].map((offset) => view.getFloat32(offset, true)), desired: view.getInt32(0x234, true), initialized: view.getUint32(0x258, true) }; });
  expect(state.activated.brain).toBe(state.activated.token);
  // 저장된 적 어려움의 배율0.5와 채취6이 실제 AI 본문까지 보존돼야 한다.
  expect(state.rates).toEqual([0.5, 0.5, 1]); expect(state.desired).toBe(6); expect(state.initialized).toBe(1);
  expect(state.activated.money).toBe(state.activated.before.money); expect(state.activated.allies).toBe(state.activated.before.allies);
  expect(state.activated.name).toEqual(state.activated.before.name); expect(state.activated.technology).toEqual(state.activated.before.technology);
  expect(errors).toEqual([]);
});

// 다시 허용된1-3/1-4에서 현재 Player 값을 유지한 실제 아군 AI로15초를 진행한다.
// 각 미션은 독립 페이지에서 설치해 prototype 감싸기와 저장 상태를 공유하지 않는다.
for (const mission of ['savetheisland', 'fragilefortune']) {
test(`아군 AI 검사: ${mission}의 실제 brain과 현재 Player 값을 보존한다`, async ({ page }) => {
  test.setTimeout(240000);
  const errors = [];
  page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.setViewportSize({ width: 1024, height: 768 });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.goto('/debug.html?fixed=25&aspect=4:3&title=1', { waitUntil: 'domcontentloaded', timeout: 120000 });
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
  await page.locator('canvas').click(); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 실제 시작 그림을 처리하고 원래 미션의 브리핑 단추만 누른다.
  while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await installAllyAi(page, mission);
  await page.evaluate(name => window.netstorm._beginCampaignFromDialog(name), mission);
  await page.evaluate(() => window.netstorm.stepFixedFrames(120));
  const point = await page.evaluate(() => { const app = window.netstorm, p = app._mainMenu.controlPoint('Play Mission'), r = app.canvas.getBoundingClientRect(); if (!p) throw new Error('실제 브리핑 단추 없음'); return { x: r.left + p.x * r.width / 1024, y: r.top + p.y * r.height / 768 }; });
  await page.mouse.click(point.x, point.y); await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  const activated = await page.evaluate(() => window.netstormAllyTestState);
  expect(activated.owner).toBe(1); expect(activated.token).toBeGreaterThan(0); expect(activated.brain).toBe(activated.token);
  expect(activated.money).toBe(activated.before.money); expect(activated.allies).toBe(activated.before.allies);
  expect(activated.name).toEqual(activated.before.name); expect(activated.technology).toEqual(activated.before.technology);
  // 모든 실제 Kernel/Renderer 프레임을 실행하며 수동 건설/공격이나 목표 값 변경은 하지 않는다.
  for (let frame = 0; frame < 600; frame += 100) await page.evaluate(() => window.netstorm.stepFixedFrames(100));
  const after = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), token = session.participation().brain(1), raw = session.aiMemory().bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); return { mission: app._clientGuide.current().script.name, token, initialized: view.getUint32(0x258, true), stopped: view.getUint32(0x254, true), time: app._gameClock.gameSeconds() }; });
  expect(after.mission.toLowerCase()).toBe(mission); expect(after.token).toBe(activated.token);
  expect(after.initialized).toBe(1); expect(after.stopped).toBe(0); expect(after.time).toBeGreaterThan(10); expect(errors).toEqual([]);
  await page.screenshot({ path: test.info().outputPath(mission + '-ally-ai.png') });
  // 기존 AI 재사용 여부도 관측해 기록하지만 승리 완료의 근거로 사용하지 않는다.
  console.log(JSON.stringify({ activated, after }));
});
}

// 아군 AI 활성화만 허용된 단계에서 실제6000프레임의 Kernel·생산·건설을 유지한다.
test('아군 AI 검사: 캠페인1-1의 현재 자금·동맹·이름·기술을 보존하고 자연 진행한다', async ({ page }) => {
  test.setTimeout(1200000);
  // 실제 앱 오류를 수집하며 게임 성공 상태나 적 상태를 직접 바꾸지 않는다.
  const errors = [];
  page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.setViewportSize({ width: 1024, height: 768 });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.goto('/debug.html?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
  await page.locator('canvas').click(); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 원래 시작 그림·메뉴의 모든 프레임을 통과한 뒤 미션을 설치한다.
  while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await installAllyAi(page, 'thewarbegins');
  await expect(installAllyAi(page, 'thewarbegins')).rejects.toThrow('중복 설치');
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('thewarbegins'));
  await page.evaluate(() => window.netstorm.stepFixedFrames(120));
  // 브리핑의 실제 Play Mission 단추를 마우스로 누른다.
  const point = await page.evaluate(() => { const app = window.netstorm, p = app._mainMenu.controlPoint('Play Mission'), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + p.x * rect.width / 1024, y: rect.top + p.y * rect.height / 768 }; });
  await page.mouse.click(point.x, point.y); await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  // 생성 순간의 값은 복사된 관측이며 이후 AI의 정상 비용 지출과 구별한다.
  const activated = await page.evaluate(() => window.netstormAllyTestState);
  expect(errors).toEqual([]); expect(activated).toBeTruthy();
  expect(activated.owner).toBe(1); expect(activated.token).toBeGreaterThan(0); expect(activated.brain).toBe(activated.token);
  expect(activated.money).toBe(activated.before.money); expect(activated.money).toBe(3000);
  expect(activated.allies).toBe(activated.before.allies); expect(activated.name).toEqual(activated.before.name); expect(activated.technology).toEqual(activated.before.technology);
  //100프레임을 한 호출에 묶어도 각 프레임의 실제 Kernel·Renderer는 모두 실행한다.
  for (let frame = 0; frame < 6000; frame += 100) await page.evaluate(() => window.netstorm.stepFixedFrames(100));
  // 현재 할당된 로컬 다리가 늘고 실제 사제가 살아 있음을 조회한다.
  const after = await page.evaluate(() => {
    const session = window.netstorm._campaignWorld.session(), pool = session.core().pool; let bridges = 0, priest = 0;
    // 생성 결과를 셀 뿐 객체·SP·HP·단계를 바꾸지 않는다.
    for (let sid = 1; sid < pool.capacity; sid += 1) { const raw = pool.slot(sid); if (!(raw[11] & 1) && raw[34] === 1) { if (raw[10] === 82) bridges += 1; if (raw[10] === 158 && new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(26, true) > 0) priest += 1; } }
    return { bridges, priest, time: window.netstorm._gameClock.gameSeconds() };
  });
  expect(after.bridges).toBeGreaterThan(0); expect(after.priest).toBe(1); expect(after.time).toBeGreaterThan(140); expect(errors).toEqual([]);
  await page.screenshot({ path: test.info().outputPath('ally-ai6000.png') });
});
