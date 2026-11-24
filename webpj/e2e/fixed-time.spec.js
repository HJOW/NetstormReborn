// 검사 전용 고정 시각 모드(?fixed=25)의 결정론을 실제 Chromium 에서 확인한다: 같은 입력 단계표의 두 실행이 같은 시계·난수·Session 덤프를 낸다(cpppj `--fixed-frame-ms 25` 와 같은 L2·L3 대조의 바탕).
// 같은 90프레임의 웹 논리 화면은 cpppj 기준 화면(`load.png`)과 비교한다(E1278: 7,195픽셀 차이였고 E1280 에서 State 설치의 난수 소비 순서를 맞춰 0픽셀이 됐다). 난수 상태(`random` 행)도 기준선과 같고 Session 덤프의 남은 차이는 `sp`·`player` 행뿐이다(E1281 에서 게임 시각도 맞춤).
// 영어 4:3(1024×768)·16:9(1366×768)·16:10(1280×800)을 각각 같은 크기의 cpppj 오라클과 대조한다.
// 시작 그림 구간(3초씩 세 장)은 시각만 흐르고 프레임으로 세지 않아 정확히 360프레임·게임 시각 9초(+시계 출발 0.1초)이며(원본 cpppj 의 고정 프레임 실행과 같음), 이어 Tutorial1 을 시작해 90프레임을 진행한다. (2026-11-23 WB04-E1271)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');

// 화면 모드별 브라우저 크기·가로세로비 이름·cpppj 오라클 폴더이다. 4:3 은 1024×768, 16:9 는 논리 1366×768 이다. (2026-11-23 WB04-E1282)
const ORACLE_ROOT = path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', 'tutorial1');
const MODES = {
  '4:3': { viewport: { width: 1024, height: 768 }, oracle: path.join(ORACLE_ROOT, 'oracle', 'tutorial1'), scenarios: path.join(ORACLE_ROOT, 'm1-scenarios.json') },
  '16:9': { viewport: { width: 1366, height: 768 }, oracle: path.join(ORACLE_ROOT, 'oracle-1366x768', 'tutorial1'), scenarios: path.join(ORACLE_ROOT, 'm1-scenarios-1366x768.json') },
  '16:10': { viewport: { width: 1280, height: 800 }, oracle: path.join(ORACLE_ROOT, '16x10', 'oracle', 'tutorial1'), scenarios: path.join(ORACLE_ROOT, '16x10', 'm1-scenarios.json'), report: true },
};

// 새 페이지에서 고정 시각 모드로 시작 그림을 끝내고 Tutorial1 을 시작해 mission 프레임 수만큼 진행한 뒤 덤프와 시작 그림 정보를 돌려준다. aspect 는 MODES 의 키이다.
async function run(browser, missionFrames, aspect = '4:3') {
  const context = await browser.newContext({ viewport: MODES[aspect].viewport, locale: 'en-US' }), page = await context.newPage();
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto(`/debug.html?fixed=25&aspect=${aspect}&title=1`);
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
  const patchMode = await page.evaluate(() => window.netstorm._campaignWorld.session().core().destroy.hasPatchDiagnostics);
  await context.close();
  return { dump, later, titleFrames, titleGame, frame, errors, screen, patchMode };
}

// 웹 결과를 cpppj 오라클 폴더(load.png·load.tsv·later.tsv)와 대조한다: 논리 화면은 다른 픽셀이 0개, Session 덤프는 `sp`(소유자 0 의 미초기화 난수)와 `player:*`(+1·+2 바이트 주소 잔재, Player 1 은 동적 Deck 필드)만 다르다. (2026-11-23 WB04-E1282)
async function expectMatchesOracle(result, mode) {
  expect(result.patchMode, '실제 앱의 진단 정상 복귀 삭제 경로').toBe(true);
  const oracle = mode.oracle;
  // 오라클 수집 조건: 수집 시나리오의 해상도 설정과 오라클 메타(oracle.json)의 기준선 커밋·화면 크기가 웹 화면 모드와 같다.
  const scenarios = JSON.parse(fs.readFileSync(mode.scenarios, 'utf8')), meta = JSON.parse(fs.readFileSync(path.join(oracle, '..', 'oracle.json'), 'utf8'));
  expect([scenarios.settings.SCREENW, scenarios.settings.SCREENH], '수집 시나리오의 해상도').toEqual([result.screen.width, result.screen.height]);
  expect(meta.cpppjCommit, '오라클 기준선 커밋').toBe('225553df');
  // 새16:10 보고서의 실제 지도 영역도 수집 설정/웹 논리 크기와 일치해야 한다.
  if (mode.report) {
    const report = fs.readFileSync(path.join(oracle, 'report.txt'), 'utf8');
    const view = report.split(/\r?\n/).find((line) => line.startsWith('audio_view\t'));
    expect(view, 'C++ 실제 지도 영역 보고').toBeTruthy();
    expect(view.split('\t').slice(-4).map(Number)).toEqual([85, 0, result.screen.width, result.screen.height]);
  }
  const { readPng, indexedToRgb } = await import('../tools/image-files.mjs'), { compareImages } = await import('../tools/screen-compare.mjs');
  const palette = new Uint8Array(768);
  // BGRX → RGB 로 바꾼다.
  for (let index = 0; index < 256; index += 1) { palette[index * 3] = result.screen.palette[index * 4 + 2]; palette[index * 3 + 1] = result.screen.palette[index * 4 + 1]; palette[index * 3 + 2] = result.screen.palette[index * 4]; }
  const web = indexedToRgb({ width: result.screen.width, height: result.screen.height, pixels: Uint8Array.from(result.screen.pixels) }, palette);
  const cpp = readPng(fs.readFileSync(path.join(oracle, 'load.png')));
  const compared = compareImages(cpp, web, []);
  expect(compared.sizeMatch, '화면 크기').toBe(true);
  expect(compared.total, '웹 90프레임 화면과 load.png 의 다른 픽셀 수').toBe(0);
  const { compareSessionDumps } = await import('../tools/session-dump-compare.mjs');
  const normalize = (text) => text.split(/\r?\n/).join('\n');
  const allowed = new Set(['sp']);
  const unexpected = (differences) => differences.filter((item) => !(allowed.has(item.key) || item.key.startsWith('player:'))).map((item) => `${item.kind}:${item.key}`);
  expect(unexpected(compareSessionDumps(normalize(fs.readFileSync(path.join(oracle, 'load.tsv'), 'utf8')), normalize(result.dump)).differences), '90프레임 허용 밖 덤프 차이(난수 상태 random 행 포함)').toEqual([]);
  // 130프레임 시점(`later.tsv`)도 같은 허용 행 밖에 차이가 없다.
  expect(unexpected(compareSessionDumps(normalize(fs.readFileSync(path.join(oracle, 'later.tsv'), 'utf8')), normalize(result.later)).differences), '130프레임 허용 밖 덤프 차이').toEqual([]);
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
  await expectMatchesOracle(first, MODES['4:3']);
});

test('고정 시각 모드 16:9(1366×768): 90·130프레임 화면과 덤프가 cpppj 오라클과 같다', async ({ browser }) => {
  test.setTimeout(300000);
  const result = await run(browser, 90, '16:9');
  expect(result.screen.width).toBe(1366); expect(result.screen.height).toBe(768);
  expect(result.errors).toEqual([]);
  await expectMatchesOracle(result, MODES['16:9']);
});

test('고정 시각 모드가 아니면 프레임을 직접 진행할 수 없다', async ({ page }) => {
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/debug.html?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  const message = await page.evaluate(async () => { try { await window.netstorm.stepFixedFrames(1); return ''; } catch (error) { return error.message; } });
  expect(message).toMatch(/고정 시각 모드/);
});

// 사용자 결정의 16:10 논리 크기로 처음으로 같은 해상도의 C++ 화면/상태를 직접 대조한다. (WB04-E1322)
test('고정 시각 모드 16:10(1280×800): 화면과90·130프레임 덤프가 cpppj 오라클과 같다', async ({ browser }) => {
  test.setTimeout(300000);
  const result = await run(browser, 90, '16:10');
  expect([result.screen.width, result.screen.height]).toEqual([1280, 800]); expect(result.errors).toEqual([]);
  await expectMatchesOracle(result, MODES['16:10']);
});
