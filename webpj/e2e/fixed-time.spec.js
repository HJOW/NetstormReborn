// 검사 전용 고정 시각 모드(?fixed=25)의 결정론을 실제 Chromium 에서 확인한다: 같은 입력 단계표의 두 실행이 같은 시계·난수·Session 덤프를 낸다(cpppj `--fixed-frame-ms 25` 와 같은 L2·L3 대조의 바탕).
// 같은 90프레임의 웹 논리 화면은 cpppj 기준 화면(`load.png`)과 비교한다(E1278: 7,195픽셀 차이였고 E1280 에서 State 설치의 난수 소비 순서를 맞춰 0픽셀이 됐다). 난수 상태(`random` 행)도 기준선과 같고 Session 덤프의 남은 차이는 시각·주소 의존 행뿐이다.
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
  // cpppj 기준선(`--mission Tutorial1`)은 메인 메뉴를 설치하고 주 루프 1프레임을 돈 뒤 미션을 시작한다(그 1프레임의 주변 바람 난수까지 같아야 난수열이 같다). 주 루프 프레임 번호는 이 1프레임을 포함해 센다. (2026-11-23 WB04-E1280)
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1'));
  await page.evaluate((count) => window.netstorm.stepFixedFrames(count - 1), missionFrames);
  const dump = await page.evaluate(() => window.netstorm.sessionDump());
  const frame = await page.evaluate(() => window.netstorm._gameClock.time().number);
  // 논리 화면의 색 번호 표면과 현재 팔레트(BGRX 256칸)이다.
  const screen = await page.evaluate(() => { const app = window.netstorm, surface = app._surface; return { width: surface.width, height: surface.height, pixels: Array.from(surface.pixels), palette: Array.from(app._clientPaletteScreen.palette()) }; });
  // cpppj 기준선 `later.tsv` 와 같은 130프레임 시점의 덤프도 모은다. (2026-11-23 WB04-E1281)
  await page.evaluate((count) => window.netstorm.stepFixedFrames(count), 130 - missionFrames);
  const later = await page.evaluate(() => window.netstorm.sessionDump());
  await context.close();
  return { dump, later, titleFrames, titleGame, frame, errors, screen };
}

test('고정 시각 모드: 시작 그림은 360프레임·9초이고 같은 입력의 두 실행은 같은 덤프를 낸다', async ({ browser }) => {
  test.setTimeout(300000);
  const first = await run(browser, 90), second = await run(browser, 90);
  for (const result of [first, second]) {
    expect(result.titleFrames, '시작 그림 프레임 수').toBe(360);
    expect(result.titleGame, '시작 그림 뒤 게임 시각(시계 출발 0.1초 + 그림 구간 9초)').toBeCloseTo(9.1, 6);
    expect(result.frame, '주 루프 프레임 번호(시작 그림은 세지 않음)').toBe(90);
    expect(result.errors).toEqual([]);
  }
  expect(second.dump, '두 실행의 Session 덤프').toBe(first.dump);
  expect(second.screen.pixels, '두 실행의 논리 화면').toEqual(first.screen.pixels);
  // 웹 90프레임 논리 화면 대 cpppj 오라클 `load.png`: 다른 픽셀이 0개이다.
  const { readPng, indexedToRgb } = await import('../tools/image-files.mjs'), { compareImages } = await import('../tools/screen-compare.mjs');
  const palette = new Uint8Array(768);
  // BGRX → RGB 로 바꾼다.
  for (let index = 0; index < 256; index += 1) { palette[index * 3] = first.screen.palette[index * 4 + 2]; palette[index * 3 + 1] = first.screen.palette[index * 4 + 1]; palette[index * 3 + 2] = first.screen.palette[index * 4]; }
  const web = indexedToRgb({ width: first.screen.width, height: first.screen.height, pixels: Uint8Array.from(first.screen.pixels) }, palette);
  const oracle = path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', 'tutorial1', 'oracle', 'tutorial1');
  const cpp = readPng(fs.readFileSync(path.join(oracle, 'load.png')));
  const result = compareImages(cpp, web, []);
  expect(result.sizeMatch).toBe(true);
  expect(result.total, '웹 90프레임 화면과 load.png 의 다른 픽셀 수').toBe(0);
  // Session 덤프: 난수 상태·월드·Graph·장부 행은 모두 같고, 남는 차이는 게임 시각·프로세스 기한(`frame`·`regular`)과 그 영향을 받는 애니메이션 프레임(`object`·`slot`)·풀 해시(`pool`)·주소 잔재(`player`)·소유자 0 SP 난수(`sp`)뿐이다.
  const { compareSessionDumps } = await import('../tools/session-dump-compare.mjs');
  const normalize = (text) => text.split(/\r?\n/).join('\n');
  const { differences } = compareSessionDumps(normalize(fs.readFileSync(path.join(oracle, 'load.tsv'), 'utf8')), normalize(first.dump));
  const allowed = new Set(['sp']);
  const unexpected = differences.filter((item) => !(allowed.has(item.key) || item.key.startsWith('player:')));
  expect(unexpected.map((item) => `${item.kind}:${item.key}`), '허용 밖 덤프 차이(난수 상태 random 행 포함)').toEqual([]);
  // 130프레임 시점(`later.tsv`)도 같은 허용 행 밖에 차이가 없다.
  const laterDifferences = compareSessionDumps(normalize(fs.readFileSync(path.join(oracle, 'later.tsv'), 'utf8')), normalize(first.later)).differences;
  expect(laterDifferences.filter((item) => !(allowed.has(item.key) || item.key.startsWith('player:'))).map((item) => `${item.kind}:${item.key}`), '130프레임 허용 밖 덤프 차이').toEqual([]);
});

test('고정 시각 모드가 아니면 프레임을 직접 진행할 수 없다', async ({ page }) => {
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  const message = await page.evaluate(async () => { try { await window.netstorm.stepFixedFrames(1); return ''; } catch (error) { return error.message; } });
  expect(message).toMatch(/고정 시각 모드/);
});
