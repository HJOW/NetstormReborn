// 한국어 M2의 실제 화면/상태 차이를 기록하고 같은 CPP 본문을 별도 표면에 그려 글꼴 차이와 기존 번역 차이를 구별한다. (WB04-E1323)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');
const path = require('node:path');

// 현재 갈무리11을 사용하는 CPP1339 참조와 기존 영어 시작 기준선이다.
const ROOT = path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-13e74579', 'tutorial1-korean');
const ENGLISH = path.join(__dirname, '..', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', 'tutorial1', 'oracle', 'tutorial1');
const EXPECTED = JSON.parse(fs.readFileSync(path.join(ROOT, 'comparison.json'), 'utf8'));
// 시작 상태와 N프레임 뒤의 명시 캡처 이름이다.
const CHECKPOINTS = [{ frame: 90, image: 'load.png', dump: 'load.tsv' }, { frame: 130, image: 'later.png', dump: 'later.tsv' }];

// 수집 언어·고정 프레임·실제 지도 영역과 실행 파일의 정체성을 대조한다.
function reference(language) {
  const directory = path.join(ROOT, language, 'oracle', 'tutorial1');
  const scenario = JSON.parse(fs.readFileSync(path.join(ROOT, language, 'm1-scenarios.json'), 'utf8'));
  const meta = JSON.parse(fs.readFileSync(path.join(directory, '..', 'oracle.json'), 'utf8'));
  expect(meta.cpppjCommit).toBe(EXPECTED.cpppjCommit); expect(meta.exe.sha256).toBe(EXPECTED.cppExeSha256);
  expect(scenario.language).toBe(language); expect(scenario.settings.currentLanguage).toBe(language); expect(scenario.fixedFrameMs).toBe(25);
  const report = fs.readFileSync(path.join(directory, 'report.txt'), 'utf8');
  expect(report.split(/\r?\n/).find((line) => line.startsWith('audio_view\t')).split('\t').slice(-4).map(Number)).toEqual([85, 0, 1024, 768]);
  return directory;
}

// 환경에 의존하는 미초기화 SP/Player 바이트 외에는 게임 상태 차이를 허용하지 않는다.
async function sameState(expected, actual) {
  const { compareSessionDumps } = await import('../tools/session-dump-compare.mjs');
  expect(compareSessionDumps(expected, actual).differences.filter(({ key }) => key !== 'sp' && !key.startsWith('player:')).map(({ kind, key }) => `${kind}:${key}`)).toEqual([]);
}

// 색 번호/현재 BGRX를 손실 없는 RGB 화면으로 변환한다.
async function rgbScreen(screen) {
  const { indexedToRgb } = await import('../tools/image-files.mjs'), palette = new Uint8Array(768);
  // Windows 팔레트의 예약 바이트를 출력 색에 섞지 않는다.
  for (let index = 0; index < 256; index += 1) { palette[index * 3] = screen.palette[index * 4 + 2]; palette[index * 3 + 1] = screen.palette[index * 4 + 1]; palette[index * 3 + 2] = screen.palette[index * 4]; }
  return indexedToRgb({ width: screen.width, height: screen.height, pixels: Uint8Array.from(screen.pixels) }, palette);
}

// 기록한 영역 안의 차이 수도 고정한다. 영역 밖의 새 차이는 한 픽셀도 허용하지 않는다.
async function compareScreen(cppFile, web, expected) {
  const { readPng } = await import('../tools/image-files.mjs'), { compareImages } = await import('../tools/screen-compare.mjs');
  const compared = compareImages(readPng(fs.readFileSync(cppFile)), web, expected.regions);
  expect(compared.sizeMatch).toBe(true); expect(compared.outside, JSON.stringify(compared.outsideBox)).toBe(0);
  expect(compared.total).toBe(expected.total); expect(compared.regions).toEqual(expected.regions.map(({ pixels }) => pixels));
}

test('한국어 참조의 CPP1339 영어 대조군은 기존225553df와 화면·상태가 같다', async () => {
  const current = reference('english');
  const { readPng } = await import('../tools/image-files.mjs');
  await compareScreen(path.join(ENGLISH, 'load.png'), readPng(fs.readFileSync(path.join(current, 'load.png'))), { total: 0, regions: [] });
  // CPP 글꼴/후속 수정 자체가 게임 상태를 바꾼 차이를 한국어 차이로 오인하지 않는다.
  for (const { dump } of CHECKPOINTS) await sameState(fs.readFileSync(path.join(ENGLISH, dump), 'utf8'), fs.readFileSync(path.join(current, dump), 'utf8'));
});

test('한국어 Tutorial1 M2:90·130프레임 상태 일치·안내 창 밖0픽셀·같은 본문은 글자 영역만 다르다', async ({ browser }) => {
  test.setTimeout(300000);
  const context = await browser.newContext({ viewport: { width: 1024, height: 768 }, locale: 'ko-KR' }), page = await context.newPage(), errors = [];
  try {
    page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
    await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
    await page.goto('/debug.html?fixed=25&aspect=4:3&title=1&lang=ko');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
    await page.evaluate(() => window.netstorm._prepareCampaignAudio());
    // 시작 그림을 같은 시계로 마치고 메인 메뉴 한 프레임 뒤 미션을 시작한다.
    while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
    await page.evaluate(() => window.netstorm.stepFixedFrames(1)); await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1'));
    const directory = reference('korean'); let previous = 1, first;
    // 실제 제품의 본문/게임 진행은 대조를 위해 바꾸지 않는다.
    for (const checkpoint of CHECKPOINTS) {
      await page.evaluate((count) => window.netstorm.stepFixedFrames(count), checkpoint.frame - previous); previous = checkpoint.frame;
      const screen = await page.evaluate(() => { const app = window.netstorm; return { width: app._surface.width, height: app._surface.height, pixels: Array.from(app._surface.pixels), palette: Array.from(app._clientPaletteScreen.palette()), dump: app.sessionDump(), language: app.options.language, topic: app._gameData.currentTell, body: app._mainMenu.dialog.content.styleText }; });
      expect(screen.language).toBe('korean'); expect(screen.topic).toBe('A.'); expect(screen.body).toContain('화면 끝으로 마우스를 움직이거나');
      await sameState(fs.readFileSync(path.join(directory, checkpoint.dump), 'utf8'), screen.dump);
      await compareScreen(path.join(directory, checkpoint.image), await rgbScreen(screen), EXPECTED.production);
      if (!first) first = screen;
    }
    // 같은 CPP 본문은 별도 표면에만 그린다. 실제 Session·정지·현재 창·미션 완료값을 결과에 맞춰 조작하지 않는다.
    const controlled = await page.evaluate(async ({ body, reference }) => {
      const { createTellWindow } = await import('/ui/tell-window.js'), app = window.netstorm, view = app._campaignWorld.mapView();
      const result = createTellWindow({ bodyFor: () => body, gameData: app._gameData, resources: app._resources, name: 'A.', missionSection: (name) => app._clientGuide.current().script.section(name), screen: { left: view.left, top: view.top, width: view.right - view.left, height: view.bottom - view.top } });
      const surface = new app._surface.constructor(app._surface.width, app._surface.height); surface.pixels.set(reference); result.window.draw(surface);
      const dialog = result.window; return { pixels: Array.from(surface.pixels), bounds: [dialog.x, dialog.y, dialog.width, dialog.height] };
    }, { body: fs.readFileSync(path.join(ROOT, 'cpp-topic-A.html'), 'utf8'), reference: first.pixels });
    expect(controlled.bounds).toEqual(EXPECTED.controlled.bounds);
    await compareScreen(path.join(directory, 'load.png'), await rgbScreen({ ...first, pixels: controlled.pixels }), EXPECTED.controlled);
    expect(errors).toEqual([]);
  } finally { await context.close(); }
});
