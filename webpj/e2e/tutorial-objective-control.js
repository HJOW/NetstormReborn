// 새 안내 검사의 공용 조작이다. 기존 실제 미션/프레임/DOM 입력만 사용하고 gate·SP·승패 상태를 주입하지 않는다.
const { expect } = require('@playwright/test');

// 고정 시각의 모든 원래 프레임을 실행한다. 호출을 묶어도 게임 프레임은 생략하지 않는다.
async function step(page, count = 1) { await page.evaluate((frames) => window.netstorm.stepFixedFrames(frames), count); }
// 현재 실제 Tell 창의 마지막 돌 버튼을 화면 좌표로 눌러 안내를 닫는다.
async function dismissTell(page) {
  for (let index = 0; index < 8; index += 1) {
    const point = await page.evaluate(() => {
      const app = window.netstorm, label = app._mainMenu.dialog?.buttons?.at(-1)?.label;
      if (!label) return null;
      const logical = app._mainMenu.controlPoint(label), rect = app.canvas.getBoundingClientRect();
      if (!logical) throw new Error('현재 안내의 실제 돌 버튼 좌표 없음');
      return { x: rect.left + logical.x * rect.width / app._layout.logicalWidth, y: rect.top + logical.y * rect.height / app._layout.logicalHeight };
    });
    if (!point) break;
    await page.mouse.click(point.x, point.y); await step(page, 25);
  }
}
// 기존 도움말은 ESC 대신 자기 OK 돌 버튼으로 닫는다. 실제 현재 언어 라벨과 좌표를 사용한다.
async function closeHelp(page) {
  const point = await page.evaluate(() => {
    const app = window.netstorm, label = app._mainMenu.dialog?.okButton?.label;
    if (!label) throw new Error('현재 실제 도움말 OK 버튼 없음');
    const logical = app._mainMenu.controlPoint(label), rect = app.canvas.getBoundingClientRect();
    if (!logical) throw new Error('현재 도움말 OK 버튼 좌표 없음');
    return { x: rect.left + logical.x * rect.width / app._layout.logicalWidth, y: rect.top + logical.y * rect.height / app._layout.logicalHeight };
  });
  await page.mouse.click(point.x, point.y); await step(page, 2);
}
// 현재 실제 제출 문구와 gate를 읽는다. 기대 문구는 현재 번역표에서 직접 조회해 언어별로 대조한다.
async function objective(page, english = '') {
  return page.evaluate((key) => {
    const app = window.netstorm, runtime = app._clientGuide?.current()?.runtime;
    return { text: app._campaignTutorialObjective?.text() ?? '', expected: app._resources.translate(key), gate: runtime ? { ...runtime.gate } : null,
      mission: app._clientGuide?.current()?.script.name ?? null, dialog: Boolean(app._mainMenu.dialog), original: app.options.originalTutorialPresentation };
  }, english);
}
// 기본 새 표시를 켠 고정 시각의 실제 Tutorial1을 시작한다. 시작 그림은 이 기능의 범위가 아니므로 명시적으로 건너뛴다.
async function startTutorial(page, { aspect = '4:3', language = 'english', viewport = { width: 1024, height: 768 } } = {}) {
  await page.setViewportSize(viewport);
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.goto(`/debug.html?fixed=25&aspect=${aspect}&title=0&lang=${language}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click(); await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await page.evaluate(() => window.netstorm._prepareCampaignAudio()); await step(page);
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1')); await step(page, 90);
  expect((await objective(page)).dialog, '실제 첫 안내').toBe(true);
  expect((await objective(page)).text, '안내를 읽는 동안 숨김').toBe('');
  await dismissTell(page); await step(page, 30);
  const first = await objective(page, 'Press F4 to return to your island, or place a bridge.');
  expect(first.original).toBe(false); expect(first.gate.stage).toBe(65); expect(first.text).toBe(first.expected); expect(first.text).not.toBe('');
}
module.exports = { step, dismissTell, closeHelp, objective, startTutorial };
