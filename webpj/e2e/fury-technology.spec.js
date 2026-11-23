// Priest Training 미션 Fury War(bc8fury76)의 원소 선택(Wind/Rain/Thunder/Sun)이 `<$CheatAllTechnology,원소>` 스크립트 명령으로 그 원소의 기술을 모두 지급하는지 실제 Chromium 에서 확인한다. (2026-11-22 WB04-E1189)
// 이 명령은 웹에 연결돼 있지 않아 원소를 골라도 기술이 하나도 생기지 않았다. 시험은 선택 전에는 지급 대상 원소 기술이 없고, 고른 뒤에는 그 원소의 기술이 현재 기술 부모에 생기며 다른 원소는 그대로임을 본다.
const { test, expect } = require('@playwright/test');

const ASPECT = process.env.NETSTORM_E2E_ASPECT ?? '4:3';
const VIEWPORTS = { '4:3': { width: 1024, height: 768 }, '16:9': { width: 1280, height: 720 }, '16:10': { width: 1280, height: 800 } };

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 현재 안내 창의 단추 이름 목록이다(창이 없으면 null).
// 대본의 `$Button= Wind` 처럼 앞 공백이 붙은 이름도 있어 공백을 뺀 이름으로 비교한다.
const buttons = (page) => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label.trim()) ?? null);
// 공백을 뺀 이름이 같은 안내 창 단추를 눌러 실제 라벨로 클릭한다.
async function clickButton(page, name) { const label = await page.evaluate((wanted) => window.netstorm._mainMenu.dialog.buttons.find((button) => button.label.trim() === wanted)?.label, name); await clickLabel(page, label); }

// 현재 기술 부모가 가진 기술 종류 수를 원소(theme)별로 센다(group 10·theme 5 제외, 원문 CheatAllTechnology 의 대상 규칙).
const technologyByTheme = (page) => page.evaluate(() => {
  const session = window.netstorm._campaignWorld.session(), types = session.core().types, parent = session.currentTechnologyParent(), counts = [0, 0, 0, 0], wanted = [0, 0, 0, 0];
  for (let type = 0; type < types.length; type += 1) {
    const record = types[type]; if (record.group === 10 || record.theme === 5 || record.theme < 0 || record.theme > 3) continue;
    wanted[record.theme] += 1; if (session.priests().carrier.findContained(parent, 4, type, 0)) counts[record.theme] += 1;
  }
  return { counts, wanted };
});

test('Fury War: 원소를 고르면 그 원소의 기술이 모두 지급된다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => {
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6', 'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76']) done[`Done${name}`] = '1';
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } }));
  });
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Priest Training');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'bc8fury76')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  // 원본처럼 월드에 들어서면 게임이 멈춘 채로 시작 안내 창([A.], "Lets do this!")이 먼저 나온다. 월드 진입 정지(PauseMissionWorldEntry)가 없던 때에는 로딩 중 흐른 실제 시간 때문에
  // 1초 뒤 창([@1])이 이 창보다 앞서 열렸다. 이 단추를 누르면 게임이 풀리고 게임 시각 1초 뒤에 원소 선택 창(Wind/Rain/Thunder/Sun)이 나온다. (2026-11-23 WB04-E1281)
  await expect.poll(() => buttons(page), { timeout: 30000, message: '시작 안내 창' }).toEqual(['Lets do this!']);
  await clickButton(page, 'Lets do this!');
  await expect.poll(() => buttons(page), { timeout: 30000, message: '원소 선택 창' }).toEqual(['Wind', 'Rain', 'Thunder', 'Sun']);
  const before = await technologyByTheme(page); expect(before.wanted[1]).toBeGreaterThan(0);
  // 비(Rain, 원소 1)를 고르면 비 원소의 기술이 모두 생기고 다른 원소는 늘지 않는다.
  await clickButton(page, 'Rain'); await page.waitForTimeout(1500);
  const after = await technologyByTheme(page);
  expect(after.counts[1]).toBe(after.wanted[1]); expect(after.counts[1]).toBeGreaterThan(before.counts[1]);
  for (const other of [0, 2, 3]) expect(after.counts[other]).toBe(before.counts[other]);
  expect(errors).toEqual([]);
});
