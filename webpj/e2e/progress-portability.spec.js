// 실제1-1 신규 승리에서 생성한 저장 사본으로 제품 번들의 읽기·새로고침·다음 미션 브리핑을 확인한다.
// 저장 사본 주입은 호환 검사 입력이며 새로운 자연 승리나 목표 수행의 근거로 세지 않는다.
const { test, expect } = require('@playwright/test');
const { readFileSync, writeFileSync } = require('node:fs');
const path = require('node:path');
// 장치가 없는 GUI 환경에서도 실제 오디오 엔진을 유지하는 선택적 검사 설정이다.
const { installSilentAudioSink } = require('./silent-audio-sink');

// E1394 실제 관찰 원문의 SHA-256과 원래 저장 문자열을 함께 보존한 검사 자료다.
const progress = JSON.parse(readFileSync(path.join(__dirname, 'fixtures/natural-campaign1-1-progress.json'), 'utf8'));

// 원래 시작 화면과 모든 고정 프레임을 거쳐 메뉴를 준비한다.
async function mainMenu(page) {
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
  await page.locator('canvas').click();
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 시작 그림은 원래360프레임이며 임의 상태 변경 없이 처리한다.
  for (let frame = 0; frame < 500 && await page.evaluate(() => window.netstorm.state !== 'mainMenu'); frame++) {
    await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  }
  expect(await page.evaluate(() => window.netstorm.state)).toBe('mainMenu');
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
}

// 현재 표시된 실제 돌 단추의 논리 좌표로 마우스 입력을 전달한다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    if (!logical) throw new Error('저장 호환 검사의 실제 단추 없음: ' + text);
    return { x: rect.left + logical.x * rect.width / app._layout.logicalWidth, y: rect.top + logical.y * rect.height / app._layout.logicalHeight };
  }, label);
  await page.mouse.click(point.x, point.y);
  await page.evaluate(() => window.netstorm.stepFixedFrames(2));
}

// 완료 자료를 읽은 실제 메뉴에서 해제된1-2를 골라 World와 Play Mission 브리핑을 끝까지 준비한다.
async function unlockedBriefing(page) {
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument.toLowerCase() === 'masterofwhirligigs')?.label);
  expect(label).toBeTruthy(); await clickLabel(page, label);
  // 요청 이름만 바뀐 상태를 실제 미션 진입으로 오인하지 않는다.
  for (let frame = 0; frame < 1000; frame++) {
    const ready = await page.evaluate(() => {
      const app = window.netstorm;
      return app._clientGuide.current()?.script.name.toLowerCase() === 'masterofwhirligigs' && app._campaignDisplayActive() && Boolean(app._campaignWorld?.session().localConnectionReady()) && Boolean(app._mainMenu.controlPoint('Play Mission'));
    });
    if (ready) return;
    await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  }
  throw new Error('저장 복원 뒤1-2 실제 브리핑 준비 시간 초과');
}

// 제품 번들에서 실제 저장 사본을 읽고 미션 진입 중 새로고침 뒤에도 같은 해제/브리핑을 확인한다.
test('실제1-1 저장 사본: 제품 번들의 새로고침과 해제된1-2 실제 브리핑', async ({ page }, info) => {
  test.setTimeout(240000);
  const errors = [];
  page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await installSilentAudioSink(page);
  await page.addInitScript((stored) => {
    if (!localStorage.getItem('netstorm.settings')) localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } }));
    if (!localStorage.getItem('netstorm.progress')) localStorage.setItem('netstorm.progress', stored);
  }, progress.storage);
  // 오디오 장치가 없는 GUI 검사 환경에서는 제품의 기존mute 옵션을 명시한다. 소리 검증 결과로 사용하지 않는다.
  const mute = process.env.NETSTORM_E2E_MUTE === '1' ? '&mute=1' : '';
  await page.goto('/?fixed=25&aspect=4:3&title=1&lang=english' + mute, { waitUntil: 'domcontentloaded', timeout: 120000 });
  await mainMenu(page); await unlockedBriefing(page);
  expect(await page.evaluate(() => localStorage.getItem('netstorm.progress'))).toBe(progress.storage);
  // 실제 World/브리핑 진입 뒤 새로고침하므로 앱의 저장 읽기와 세션 정리 경계를 함께 지난다.
  await page.reload({ waitUntil: 'domcontentloaded', timeout: 120000 });
  await mainMenu(page); await unlockedBriefing(page);
  expect(await page.evaluate(() => localStorage.getItem('netstorm.progress'))).toBe(progress.storage);
  const observed = await page.evaluate(() => ({ userAgent: navigator.userAgent, audioEnabled: window.netstorm.options.audio, audio: (window.netstormAudioTestContexts ?? []).map((context) => ({ sink: context.sinkId, state: context.state, currentTime: context.currentTime, sampleRate: context.sampleRate })), mission: window.netstorm._clientGuide.current().script.name, tell: window.netstorm._gameData.currentTell, buttons: window.netstorm._mainMenu.dialog.buttons.map((button) => button.label) }));
  await page.screenshot({ path: info.outputPath('restored-next-briefing.png') });
  // 실제 검사 OS/브라우저/음향 모드를 별도 파일에 남겨 목록 reporter만 사용해도 증빙을 보존한다.
  const evidence = info.outputPath('progress-reader.json');
  writeFileSync(evidence, JSON.stringify({ source: progress, project: info.project.name, node: process.version, platform: process.platform, observed }, null, 2));
  await info.attach('progress-reader.json', { path: evidence, contentType: 'application/json' });
  expect(errors).toEqual([]);
});
