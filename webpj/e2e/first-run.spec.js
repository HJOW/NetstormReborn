// 독립 브라우저 저장소에서 최초 안내·잠금/체크·음량과 재실행 수명을 확인한다. 실제 미션 완료 입력은 만들지 않는다. (WB04-E616)
const { test, expect } = require('@playwright/test');

// 그려진 조작부의 논리 좌표를 Canvas CSS 좌표로 바꿔 실제 마우스로 누른다. 숨겨진 메인 메뉴는 controlPoint가 거부한다.
async function clickLabel(page, label) {
  // 원본 라벨 측정 규칙으로 얻은 현재 화면상의 점이다.
  const point = await page.evaluate((text) => {
    const app = window.netstorm; // 현재 앱 수명이다.
    const logical = app._mainMenu.controlPoint(text); // 입력을 받는 조작부의 논리 좌표다.
    if (!logical) throw new Error(`조작부 없음: ${text}`);
    const rect = app.canvas.getBoundingClientRect(); // 현재 Canvas의 CSS 표시 경계다.
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width,
      y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 클릭 대기 화면을 통과한다. 각 reload도 사용자 입력을 다시 요구해야 한다.
async function start(page) {
  await page.goto('/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
}

// 현재 캠페인의 완료/비활성 표시를 읽는다. 렌더링·입력에 실제로 쓰이는 목록을 관찰한다.
async function campaignStates(page) {
  return page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items
    .filter((item) => item.kind === 'checked').map((item) => ({ checked: item.checked, disabled: item.disabled })));
}

// 빈 저장소 → 안내 → 메뉴 → 두 음량 → Struggle 최초 잠금 → 진행 입력 저장 → 재시작을 같은 브라우저 수명으로 확인한다.
test('최초 실행과 기존 사용자 설정·진행의 재실행', async ({ page }) => {
  // 처리되지 않은 브라우저 예외 목록이다.
  const errors = [];
  page.on('pageerror', (error) => errors.push(String(error)));
  await start(page);
  expect(await page.evaluate(() => [localStorage.getItem('netstorm.settings'), localStorage.getItem('netstorm.progress')])).toEqual([null, null]);
  expect(await page.evaluate(() => window.netstorm._gameData.currentTell)).toBe('Welcome');
  await clickLabel(page, 'OK');
  await expect.poll(() => page.evaluate(() => window.netstorm._gameData.currentTell)).toBe('Welcome2');
  expect(await page.evaluate(() => window.netstorm._mainMenu.dialog.buttons.map((button) => button.label))).toEqual(['Play Early Missions', 'Main Menu']);
  expect(await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.settings')).values.welcomeGiven)).toBe(1);
  await clickLabel(page, 'Main Menu');
  expect(await page.evaluate(() => window.netstorm._mainMenu.dialog)).toBeNull();
  expect(await page.evaluate(() => window.netstorm._settings.snapshot())).toEqual({ sound: 1, music: 1, playWind: 1, swapLeftRightSpeakers: 0,
    soundVolume: 5, musicVolume: 5, edgeScroll: 1, autoDemo: 1, tellTips: 1, useBanners: 0, maxFPS: 0, language: 0, welcomeGiven: 1, tipNumber: 0 });
  // 효과음·음악 하위 메뉴의 실제 체크를 읽고 기존 사용자 값 3·2를 UI로 저장한다.
  for (const [label, selected] of [['Sound Effect Volume', 'Volume 3'], ['Music Volume', 'Volume 2']]) {
    await clickLabel(page, 'Options');
    await clickLabel(page, label);
    expect(await page.evaluate(() => window.netstorm._mainMenu.popup.levels.at(-1).list.items.filter((item) => item.checked).map((item) => item.label))).toEqual(['Volume 5']);
    await clickLabel(page, selected);
  }
  await clickLabel(page, 'Campaign');
  await clickLabel(page, 'Struggle For Freedom');
  expect(await campaignStates(page)).toEqual([{ checked: false, disabled: false }, ...Array(5).fill({ checked: false, disabled: true })]);
  // 잠긴 행은 controlPoint 대상이 아니므로 실제 행 경계를 읽어 눌러도 창이 유지되는지 확인한다.
  const lockedPoint = await page.evaluate(() => {
    const app = window.netstorm; // 현재 앱 수명이다.
    const menu = app._mainMenu.dialog.menu; // 실제 Struggle 선택 목록이다.
    const rect = app.canvas.getBoundingClientRect(); // 논리 좌표 변환에 쓰는 CSS 경계다.
    const bounds = menu.bounds(); // 잠긴 행을 포함하는 논리 경계다.
    return { x: rect.left + (bounds.left + 40) * rect.width / app._mainMenu.width,
      y: rect.top + (bounds.top + menu.offsets[1] + 7) * rect.height / app._mainMenu.height };
  });
  await page.mouse.click(lockedPoint.x, lockedPoint.y, { delay: 60 });
  expect(await page.evaluate(() => window.netstorm._gameData.currentTell)).toBe('offical3.overview');
  expect(await campaignStates(page)).toEqual([{ checked: false, disabled: false }, ...Array(5).fill({ checked: false, disabled: true })]);
  // 자연 승리 대신 명시적인 검사 입력으로 Done 값을 저장한다. 자기 체크와 다음 행 해금의 차이를 관찰한다.
  await page.evaluate(() => window.netstorm._gameData.set('DoneTheWarBegins', 1));
  await start(page);
  expect(await page.evaluate(() => window.netstorm._gameData.currentTell)).toBe('Tips');
  await clickLabel(page, 'OK');
  expect(await page.evaluate(() => [window.netstorm._settings.get('soundVolume'), window.netstorm._settings.get('musicVolume')])).toEqual([3, 2]);
  await clickLabel(page, 'Campaign');
  await clickLabel(page, 'Struggle For Freedom');
  expect(await campaignStates(page)).toEqual([{ checked: true, disabled: false }, { checked: false, disabled: false }, ...Array(4).fill({ checked: false, disabled: true })]);
  expect(errors).toEqual([]);
});
