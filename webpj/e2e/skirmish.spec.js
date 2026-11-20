// 12차 스커미시 설정 화면을 실제 Chromium 에서 확인한다: Campaign 목록의 Skirmish 항목(켠 판본만), 설정 화면 흐름, 설정 저장, 한국어 화면. 월드 생성은 아직 연결 전이라 시작 호출까지만 본다. (2026-11-20 WB04-E1102)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼·메뉴 항목의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 현재 대화상자의 목록 항목·버튼 라벨이다.
const menuLabels = (page) => page.evaluate(() => window.netstorm._mainMenu.dialog?.menu?.items.map((item) => item.label) ?? []);
const buttonLabels = (page) => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.map((button) => button.label) ?? []);

async function openMain(page, url) {
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto(url);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
}

test.beforeEach(async ({ page }) => {
  await page.addInitScript(() => { if (!localStorage.getItem('netstorm.settings')) localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })); });
});

test('주소에 ?skirmish=0 이 있으면 Campaign 목록에 Skirmish 항목이 없다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await openMain(page, '/?skirmish=0&aspect=4:3'); await clickLabel(page, 'Campaign');
  expect(await menuLabels(page)).not.toContain('Skirmish'); expect(await menuLabels(page)).toContain('Early Missions');
  expect(errors).toEqual([]);
});

test('기본 판본에서 Campaign 의 Skirmish 설정 화면 흐름과 저장이 동작한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await openMain(page, '/?aspect=4:3'); await clickLabel(page, 'Campaign');
  expect(await menuLabels(page)).toContain('Skirmish');
  await clickLabel(page, 'Skirmish');
  expect(await menuLabels(page)).toEqual(['AI players: 1', 'AI 1: Normal / Enemy', 'Banned units: 0/5']); expect(await buttonLabels(page)).toEqual(['Start', 'Cancel']);
  // AI 수를 3 으로, AI 2 를 어려움·동맹으로 바꾼다.
  await clickLabel(page, 'AI players: 1'); await clickLabel(page, '3');
  expect(await menuLabels(page)).toEqual(['AI players: 3', 'AI 1: Normal / Enemy', 'AI 2: Normal / Enemy', 'AI 3: Normal / Enemy', 'Banned units: 0/5']);
  await clickLabel(page, 'AI 2: Normal / Enemy'); await clickLabel(page, 'Hard'); await clickLabel(page, 'Ally'); await clickLabel(page, 'Back');
  expect(await menuLabels(page)).toContain('AI 2: Hard / Ally');
  // 금지 유닛 두 개를 고르고 되돌아온다.
  await clickLabel(page, 'Banned units: 0/5');
  await clickLabel(page, '[ ] Wind Generator - windBattery'); await clickLabel(page, '[ ] Sun Cannon - sunCannon'); await clickLabel(page, 'Back');
  expect(await menuLabels(page)).toContain('Banned units: 2/5');
  // 설정은 로컬 저장소에 쓰인다(화면의 'AI 2' 는 소유자 3 이다).
  const saved = await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.settings')).values);
  expect(saved.skirmishAiCount).toBe(3); expect(saved.skirmishAi3Diff).toBe(3); expect(saved.skirmishAi3Ally).toBe(1); expect(saved.skirmishBans).toBe('windBattery,sunCannon');
  // 시작은 skirmish-start.spec.js 가 확인한다. 취소는 Campaign 목록으로 돌아간다. 다시 열면 저장된 설정이 보인다.
  await clickLabel(page, 'Cancel'); expect(await menuLabels(page)).toContain('Early Missions');
  await clickLabel(page, 'Skirmish'); expect(await menuLabels(page)).toContain('AI 2: Hard / Ally');
  expect(errors).toEqual([]);
});

test('한국어 화면의 스커미시 설정 화면이 번역되고 금지 유닛 이름이 한국어 번역표를 따른다', async ({ page }) => {
  test.setTimeout(90000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await openMain(page, '/?lang=ko&aspect=4:3'); await clickLabel(page, '캠페인');
  expect(await menuLabels(page)).toContain('스커미시'); await clickLabel(page, '스커미시');
  expect(await menuLabels(page)).toContain('인공지능 수: 1'); expect(await buttonLabels(page)).toEqual(['시작', '취소']);
  await clickLabel(page, '금지 유닛: 0/5');
  const labels = await menuLabels(page);
  expect(labels.length).toBeGreaterThan(5); expect(labels.some((label) => label.includes('windBattery') && !label.includes('Wind Generator'))).toBe(true);
  expect(errors).toEqual([]);
});
