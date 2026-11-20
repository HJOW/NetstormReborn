// 스커미시를 AI 수·난이도·동맹을 바꿔 가며 NETSTORM_LONG_SECONDS 초 동안 진행해 AI 의 건설·공격이 오류 없이 도는지 실제 Chromium 에서 확인한다. 환경 변수가 없으면 건너뛴다(조합 4개 × 최대 5분).
// 캠페인 장시간 진행(campaign-long-run)과 같은 방식으로 AI 가 활동을 시작한 뒤에야 나오는 미연결 서비스·좌표 오류를 잡는다. 7명 AI 스커미시에서 섬 목록 진단 미연결 오류를 이 방식으로 찾았다. (2026-11-20 WB04-E1124)
// 시작 섬이 지도 가장자리(256)에 닿으면 원본과 같은 `bcr.isValid()` 표시 진단이 나오지만 게임은 계속되므로(원본 AssertDisplay) 오류로 세지 않는다.
// 사용: `$env:NETSTORM_LONG_SECONDS=300; npx playwright test e2e/skirmish-long-run.spec.js`
const { test, expect } = require('@playwright/test');

// 진행 시간(초)이다. 설정하지 않으면 시험을 건너뛴다.
const SECONDS = Number(process.env.NETSTORM_LONG_SECONDS ?? 0);
// 실제 돌 버튼·메뉴 항목의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 시험할 조합이다. count = AI 수, diff = 소유자 2~8 AI 의 난이도(1 쉬움·2 보통·3 어려움), ally = 소유자 2~8 AI 의 동맹 여부(0/1)이다.
const CASES = [
  { count: 7, diff: 1, ally: [1, 1, 0, 0, 0, 0, 0] }, { count: 7, diff: 3, ally: [1, 1, 1, 1, 0, 0, 0] },
  { count: 6, diff: 2, ally: [1, 0, 1, 0, 1, 0, 0] }, { count: 7, diff: 3, ally: [0, 0, 0, 0, 0, 0, 0] },
];

for (const [index, item] of CASES.entries()) {
  test(`스커미시 장시간: AI ${item.count}명 난이도 ${item.diff} 동맹 ${item.ally.join('')}`, async ({ page }) => {
    test.skip(!(SECONDS > 0), 'NETSTORM_LONG_SECONDS 를 설정한 실행에서만 돈다');
    test.setTimeout((SECONDS + 200) * 1000);
    // 오류는 스택까지 모으되 알려진 원본 표시 진단은 뺀다.
    const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
    page.on('console', (message) => { if (message.type() === 'error' && !message.text().includes('Islandlist.cpp:676 bcr.isValid()')) errors.push(message.text()); });
    const values = { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, skirmishAiCount: item.count };
    for (let owner = 2; owner <= 8; owner += 1) { values[`skirmishAi${owner}Diff`] = item.diff; values[`skirmishAi${owner}Ally`] = item.ally[owner - 2]; }
    await page.addInitScript((settings) => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: settings })), values);
    await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
    await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
    await clickLabel(page, 'Go!');
    const before = await page.evaluate(() => window.netstorm._gameClock.gameSeconds());
    // 진행하는 동안 1초마다 안내 창이 떠 있으면 닫고, 오류가 나면 바로 멈춰 원인을 알린다.
    for (let tick = 0; tick < SECONDS && errors.length === 0; tick += 1) {
      await page.waitForTimeout(1000);
      const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
      if (label !== null) await clickLabel(page, label);
    }
    const info = await page.evaluate(() => ({ seconds: window.netstorm._gameClock.gameSeconds(), objects: window.netstorm._campaignWorld.session().world().objects().length }));
    expect(errors, `조합 ${index}`).toEqual([]); expect(info.seconds - before, `조합 ${index}`).toBeGreaterThan(SECONDS * 0.5);
    // AI 가 월드를 키운다(시작 상태보다 객체가 훨씬 많다).
    expect(info.objects, `조합 ${index}`).toBeGreaterThan(1000);
  });
}
