// 미션 월드가 없는 메인 메뉴의 Help 에서도 TYPE 주제(priestType·sunCannonType 등)에 Hits/Range/Cost 정보 패널과 유닛 그림이 나오는지 확인한다. (2026-11-23 WB04-E1240)
// 원본은 월드가 없으면 ReadAssetQuickHelpType 으로 장기 TYPE 표를 읽어 같은 정보를 만든다(UberGump help.type). 이전 웹은 미션 중에만 이 정보를 만들었다.
const { test, expect } = require('@playwright/test');

test('메인 메뉴 Help: 월드 없이 TYPE 주제로 이동하면 정보 패널과 그림이 나온다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  expect(await page.evaluate(() => window.netstorm._campaignWorld === null)).toBe(true);
  // 실제 F1 키로 도움말 창을 연다(Help 단추의 General Help - F1 항목과 같다). 첫 주제(F1Help)에는 TYPE 정보가 없다.
  await page.keyboard.press('F1');
  await page.waitForFunction(() => Array.isArray(window.netstorm._mainMenu.dialog?.history), null, { timeout: 15000 });
  expect(await page.evaluate(() => ({ info: window.netstorm._mainMenu.dialog.informationFactory === null, picture: window.netstorm._mainMenu.dialog.picture === null }))).toEqual({ info: true, picture: true });
  // 링크 이동과 같은 경로(open)로 TYPE 주제를 열면 월드가 없어도 정보 생성기와 그림이 채워진다.
  const priest = await page.evaluate(() => {
    const dialog = window.netstorm._mainMenu.dialog; dialog.open('priestType');
    return { html: dialog.informationFactory?.() ?? null, picture: dialog.picture ? { type: dialog.picture.type, frame: dialog.picture.frameNumber } : null, history: dialog.history.slice(-1)[0] };
  });
  expect(priest.history).toBe('priestType'); expect(priest.picture?.type).toBe(158);
  expect(priest.html).toContain('Class: <c>High Priest</c>'); expect(priest.html).toContain('Hits: <c>100</c>'); expect(priest.html).toContain('Range: <c>30</c>');
  // 이름 규칙(sun%s)으로 찾는 TYPE 과 Back 으로 돌아온 주제의 정보도 월드 없이 조회된다.
  const cannon = await page.evaluate(() => { const dialog = window.netstorm._mainMenu.dialog; dialog.open('sunCannonType'); return { html: dialog.informationFactory?.() ?? null, picture: dialog.picture?.type ?? null }; });
  expect(cannon.picture).toBe(128); expect(cannon.html).toContain('Class: <c>Shooter</c>'); expect(cannon.html).toContain('Hits: <c>600</c>'); expect(cannon.html).toContain('Damage: <c>16</c>');
  // TYPE 이 아닌 주제는 정보 없이 본문만 보인다.
  expect(await page.evaluate(() => { const dialog = window.netstorm._mainMenu.dialog; dialog.open('F1Help'); return [dialog.informationFactory, dialog.picture]; })).toEqual([null, null]);
  expect(errors).toEqual([]);
});
