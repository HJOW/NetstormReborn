// 검사 전용 고정 시각 모드(?fixed=25)의 결정론을 실제 Chromium 에서 확인한다: 같은 입력 단계표의 두 실행이 같은 시계·난수·Session 덤프를 낸다(cpppj `--fixed-frame-ms 25` 와 같은 L2·L3 대조의 바탕).
// 같은 90프레임의 웹 논리 화면은 cpppj 기준 화면(`load.png`)과 비교한다: 차이는 섬 아랫면(난수 모양)·왼쪽 미니맵 표시에만 있다(WB04-E1278). 난수 소비 순서를 맞추면 이 차이가 줄어야 한다.
// 시작 그림 구간(3초씩 세 장)은 시각만 흐르고 프레임으로 세지 않아 정확히 360프레임·게임 시각 9초이며(원본 cpppj 의 고정 프레임 실행과 같음), 이어 Tutorial1 을 시작해 90프레임을 진행한다. (2026-11-23 WB04-E1271)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');

// 새 페이지에서 고정 시각 모드로 시작 그림을 끝내고 Tutorial1 을 시작해 mission 프레임 수만큼 진행한 뒤 덤프와 시작 그림 정보를 돌려준다.
async function run(browser, missionFrames) {
  const context = await browser.newContext({ viewport: { width: 1024, height: 768 }, locale: 'en-US' }), page = await context.newPage();
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click();
  // 효과음 준비는 비동기라 프레임을 세기 전에 마친다(준비가 끝나지 않은 채 프레임을 돌리면 시작 시점이 달라진다).
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  let titleFrames = 0;
  while ((await page.evaluate(() => window.netstorm.state)) !== 'mainMenu' && titleFrames < 1000) { await page.evaluate(() => window.netstorm.stepFixedFrames(1)); titleFrames += 1; }
  const titleGame = await page.evaluate(() => window.netstorm._gameClock.gameSeconds());
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1'));
  await page.evaluate((count) => window.netstorm.stepFixedFrames(count), missionFrames);
  const dump = await page.evaluate(() => window.netstorm.sessionDump());
  const frame = await page.evaluate(() => window.netstorm._gameClock.time().number);
  // 논리 화면의 색 번호 표면과 현재 팔레트(BGRX 256칸)이다.
  const screen = await page.evaluate(() => { const app = window.netstorm, surface = app._surface; return { width: surface.width, height: surface.height, pixels: Array.from(surface.pixels), palette: Array.from(app._clientPaletteScreen.palette()) }; });
  await context.close();
  return { dump, titleFrames, titleGame, frame, errors, screen };
}

test('고정 시각 모드: 시작 그림은 360프레임·9초이고 같은 입력의 두 실행은 같은 덤프를 낸다', async ({ browser }) => {
  test.setTimeout(300000);
  const first = await run(browser, 90), second = await run(browser, 90);
  for (const result of [first, second]) {
    expect(result.titleFrames, '시작 그림 프레임 수').toBe(360);
    expect(result.titleGame, '시작 그림 뒤 게임 시각').toBeCloseTo(9, 6);
    expect(result.frame, '주 루프 프레임 번호(시작 그림은 세지 않음)').toBe(90);
    expect(result.errors).toEqual([]);
  }
  expect(second.dump, '두 실행의 Session 덤프').toBe(first.dump);
  expect(second.screen.pixels, '두 실행의 논리 화면').toEqual(first.screen.pixels);
  // 웹 90프레임 논리 화면 대 cpppj 오라클 `load.png`: 현재 기준 차이 7,195픽셀(0.91%)을 넘지 않고, 차이는 가장자리 상자 [5,32,697,566] 안(섬 아랫면·미니맵)에만 있으며, 안내 창 오른쪽 영역과 HUD 의 나머지는 모두 같다.
  const { readPng, indexedToRgb } = await import('../tools/image-files.mjs'), { compareImages } = await import('../tools/screen-compare.mjs');
  const palette = new Uint8Array(768);
  // BGRX → RGB 로 바꾼다.
  for (let index = 0; index < 256; index += 1) { palette[index * 3] = first.screen.palette[index * 4 + 2]; palette[index * 3 + 1] = first.screen.palette[index * 4 + 1]; palette[index * 3 + 2] = first.screen.palette[index * 4]; }
  const web = indexedToRgb({ width: first.screen.width, height: first.screen.height, pixels: Uint8Array.from(first.screen.pixels) }, palette);
  const cpp = readPng(fs.readFileSync(path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', 'tutorial1', 'oracle', 'tutorial1', 'load.png')));
  const result = compareImages(cpp, web, []);
  expect(result.sizeMatch).toBe(true);
  expect(result.total, '웹 90프레임 화면과 load.png 의 다른 픽셀 수(줄어들면 기준을 낮춘다)').toBeLessThanOrEqual(7195);
  expect(result.outsideBox[0]).toBeGreaterThanOrEqual(5); expect(result.outsideBox[1]).toBeGreaterThanOrEqual(32); expect(result.outsideBox[2]).toBeLessThanOrEqual(697); expect(result.outsideBox[3]).toBeLessThanOrEqual(566);
});

test('고정 시각 모드가 아니면 프레임을 직접 진행할 수 없다', async ({ page }) => {
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  const message = await page.evaluate(async () => { try { await window.netstorm.stepFixedFrames(1); return ''; } catch (error) { return error.message; } });
  expect(message).toMatch(/고정 시각 모드/);
});
