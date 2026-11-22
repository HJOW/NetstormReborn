// 객체 가상 6c 로 가는 명령(DetonateBomb·신전 WipeOut/Ascend/Descend·시나리오 객체 명령)이 입력 큐를 막지 않고 소비되는지, 처리하지 못하는 사건이 영구히 큐를 막지 않는지
// 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1150)
// 연결 전에는 메뉴의 Detonate!·Meltdown! 같은 항목이 만든 명령이 큐 머리에서 소비되지 않아 이후 모든 입력(선택·단축키)이 영구히 막혔다.
const { test, expect } = require('@playwright/test');

// 화면비를 환경 변수 NETSTORM_E2E_ASPECT('4:3' 기본, '16:9', '16:10')로 고른다.
const ASPECT = process.env.NETSTORM_E2E_ASPECT ?? '4:3';
const VIEWPORTS = { '4:3': { width: 1024, height: 768 }, '16:9': { width: 1280, height: 720 }, '16:10': { width: 1280, height: 800 } };
if (!VIEWPORTS[ASPECT]) throw new Error(`지원하지 않는 화면비 ${ASPECT}`);

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// The War Begins! 를 시작하고 안내 창을 닫아 시계가 흐르게 한 뒤 신전·사제 SID 를 돌려준다. 오류는 errors 에 모은다(expected 에 든 문구는 제외).
async function startWarBegins(page, errors, expected = []) {
  page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error' && !expected.some((text) => message.text().includes(text))) errors.push(message.text()); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 10; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
  return page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), types = app._gameAssets.types();
    return { temple: session.world().objects().find(({ object }) => object.owner === 1 && types[object.type - 70]?.assetName === 'rainVortex').sid, priest: session.findPriestParticipation(session.localOwner()) }; });
}

// 입력 큐 맨 앞 사건의 코드(16진)이다. 비었으면 null.
const head = (page) => page.evaluate(() => { const queue = window.netstorm._clientInputQueue, entry = queue.peek(); return entry ? entry.code.toString(16) : null; });

test('The War Begins!: 객체 명령이 입력 큐를 막지 않고 소비되며 이후 입력이 정상이다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; const ids = await startWarBegins(page, errors, ['Vortex.cpp:746']);
  // 명령 코드, 대상. 신전 세 명령은 신전 대상, 나머지는 사제 대상(공통 객체 명령에 위임되거나 미처리 0 이다). WipeOut 은 원본이 스스로 표시하는 개발자 진단(Vortex.cpp:746)을 낸다.
  const commands = [['WipeOut', 0x0300006f, ids.temple], ['Ascend', 0x0300007c, ids.temple], ['Descend', 0x030000a7, ids.temple], ['DetonateBomb', 0x03000074, ids.priest], ['InflictDamage', 0x030000e2, ids.priest],
    ['MakeMine', 0x030000af, ids.priest], ['Meltdown', 0x030000f8, ids.priest], ['TurnToObelisk', 0x030000ce, ids.priest], ['DestroyPiece', 0x03000015, 0]];
  for (const [name, code, target] of commands) {
    await page.evaluate(({ code, target }) => window.netstorm._clientInputQueue.pushCommand(code, 0, target >>> 0), { code, target });
    await expect.poll(() => head(page), { timeout: 5000, message: `${name} 명령이 큐 머리에서 소비되어야 한다` }).toBeNull();
  }
  // 이후 입력이 정상이다: P 로 사제가 선택된다.
  await page.keyboard.press('p'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._clientGameplay.selection.selected), '명령 뒤 P 선택').toBe(ids.priest);
  expect(errors).toEqual([]);
});

// 처리하지 못하는 사건이 3초(벽 시각) 넘게 큐 머리에 남으면 오류로 기록하고 버려 이후 입력이 영구히 막히지 않는다. 그 전에는 사건 순서를 지켜 머리에 남는다. (2026-11-21 WB04-E1150)
test('The War Begins!: 처리하지 못하는 사건은 3초 뒤 오류로 기록하고 버려 입력이 막히지 않는다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; const ids = await startWarBegins(page, errors, ['미구현 입력 사건을 버립니다']);
  await page.evaluate(() => window.netstorm._clientInputQueue.pushCommand(0x0300ffff, 0, 0)); await page.waitForTimeout(1500);
  expect(await head(page), '3초 전에는 순서를 지켜 머리에 남는다').toBe('300ffff');
  await expect.poll(() => head(page), { timeout: 10000, message: '3초 넘게 남은 미지원 사건은 버려져야 한다' }).toBeNull();
  await page.keyboard.press('p'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._clientGameplay.selection.selected), '버린 뒤 P 선택').toBe(ids.priest);
  expect(errors).toEqual([]);
});

// 객체 메뉴의 사제 기도(0x030000de)·운반체 Drop(0x03000076) 명령이(Cast 0x0300011a 는 같은 분기이나 내용물이 있는 운반체가 필요해 단위 시험이 맡는다) 입력 큐 머리에서 소비된다. 허용 목록에 없어 3초 뒤 "미구현 입력 사건을 버립니다" 로 버려지던 것을 연결했다. (2026-11-22 WB04-E1225)
test('The War Begins!: 사제 기도와 운반체 Drop 명령이 입력 큐에서 소비된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; const ids = await startWarBegins(page, errors);
  // 기도: packed y 의 하위 WORD 는 사제 SID, 상위 WORD 는 희망 종류이다. 소비 뒤 사제가 계속 선택 가능해야 한다.
  for (const [name, code, target] of [['Drop', 0x03000076, ids.priest], ['Pray', 0x030000de, ((0x20 << 16) | ids.priest) >>> 0]]) {
    await page.evaluate(({ code, target }) => window.netstorm._clientInputQueue.pushCommand(code, 0, target >>> 0), { code, target });
    await expect.poll(() => head(page), { timeout: 2500, message: `${name} 명령이 3초 안에(버려지기 전에) 소비되어야 한다` }).toBeNull();
  }
  await page.keyboard.press('p'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._clientGameplay.selection.selected), '명령 뒤 P 선택').toBe(ids.priest);
  expect(errors).toEqual([]);
});

