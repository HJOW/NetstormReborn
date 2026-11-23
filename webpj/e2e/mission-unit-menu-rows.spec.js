// 운반체·폭탄(Archer 계열) 우클릭 메뉴의 특수 행과 운반체의 줍기·내려놓기를 실제 Chromium 에서 확인한다. (2026-11-24 WB04-E1310~E1311)
//  - 짐을 실은 운반체(Bulf): 내용물이 있으면 메뉴에 'Carrying' 머리말 아래 Drop 행(명령 0x03000076, 인자 = Bulf SID)이 생기고, 그 행을 누르면 명령이 입력 큐에 올라
//    Session 이 소비해(Cast/Drop 성공 0x030000b2·내 너겟 납품 0x020000b4 통지) 너겟이 내용물에서 빠진다.
//  - 줍기: 내 Bulf 를 실제 마우스로 선택하고 월드의 너겟을 누르면 줍기 명령(0x030000b1, 인자 = 너겟 SID·Bulf SID)이 소비되어 너겟이 월드에서 사라지고 Bulf 메뉴에 Drop 행이 생긴다(E1311).
//  - 폭탄(원본 +a8 00420b30 / +ac 0041e660 'Archer 계열' 파생): 비편집 모드에서 내 폭탄 메뉴에 Detonate! 행(명령 0x03000074, 인자 = 폭탄 SID)이 생기고, 누르면 명령이 소비된다.
// 시작 상태는 Priest Training 5(bc5walkers76): 내 Bulf(15005)가 있다. 폭탄·너겟 내용물은 실제 미션에 없어 앱 Session 의 객체 생성 경로(`world().create/pop`·`createContained`)로 만든다.
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// Priest Training 5 를 시작해 안내 창을 닫고 게임 시계가 도는 상태까지 간다.
async function startPriestTraining5(page) {
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Priest Training');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'bc5walkers76')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 10; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
}

// 입력 큐에 올라가는 명령(push·pushCommand)을 `window.__pushed` 에 기록한다. 항목은 [메서드, 코드(16진), 인자...(16진)] 이다.
async function spyInputQueue(page) {
  await page.evaluate(() => {
    const queue = window.netstorm._clientInputQueue; window.__pushed = [];
    for (const name of ['push', 'pushCommand']) {
      const original = queue[name].bind(queue);
      queue[name] = (...args) => { window.__pushed.push([name, ...args.map((value) => (typeof value === 'number' ? value.toString(16) : typeof value))]); return original(...args); };
    }
  });
}

// 현재 열린 객체 메뉴(명령 26 노드, 대상 SID)에서 지정한 명령의 항목 위치(브라우저 좌표)와 인자·플래그를 찾는다. 없으면 null 이다.
function findRow(page, sid, command) {
  return page.evaluate(({ target, code }) => {
    const app = window.netstorm, root = app._clientGump, rect = app.canvas.getBoundingClientRect();
    // 메뉴 노드(항목 배열을 가진 노드)를 자식까지 재귀로 훑는다.
    const search = (token) => {
      const node = root.node(token);
      if (node.items && node.itemCount) for (let index = 0; index < node.itemCount; index += 1) if (node.items[index].command === code) {
        const bounds = root.collection().bounds(token);
        return { x: rect.left + (bounds.left + 12) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + node.offsets[index] + node.heights[index] / 2) * rect.height / app._layout.logicalHeight,
          parameter: node.items[index].parameter, flags: node.items[index].flags };
      }
      for (let child = node.child; child; child = root.node(child).next) { const found = search(child); if (found) return found; }
      return null;
    };
    for (let token = root.root().child; token; token = root.node(token).next) { const node = root.node(token); if (node.command === 26 && node.object === target) { const found = search(token); if (found) return found; } }
    return null;
  }, { target: sid, code: command });
}

test('Priest Training 5: 너겟을 실은 내 Bulf 의 우클릭 메뉴에 Drop 행이 있고 눌러 너겟을 내려놓는다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startPriestTraining5(page);
  // 내 Bulf 에 너겟을 싣고(내용물 생성) 그 Bulf 의 화면 적중점을 찾는다.
  const setup = await page.evaluate(() => {
    const app = window.netstorm, world = app._campaignWorld, session = world.session(), types = app._gameAssets.types(); let bulf = null;
    for (const { sid, object } of session.world().objects()) if (object.owner === 1 && types[object.type - 70]?.assetName === 'bulf') { bulf = sid; break; }
    const nugget = session.createContained(bulf, 154, 0, 0, 0x10, 0), hits = [];
    for (let y = 10; y < 760; y += 4) for (let x = 90; x < 1020; x += 4) if (world.pick(x, y, 0) === bulf) hits.push([x, y]);
    const rect = app.canvas.getBoundingClientRect(), hit = hits[Math.floor(hits.length / 2)];
    return { bulf, nugget, x: rect.left + hit[0] * rect.width / app._layout.logicalWidth, y: rect.top + hit[1] * rect.height / app._layout.logicalHeight };
  });
  expect(setup.bulf, '내 Bulf').toBeGreaterThan(0); expect(setup.nugget, '실은 너겟').toBeGreaterThan(0);
  // 실제 우클릭 → 그 Bulf 의 객체 메뉴가 열리고 Drop 행(활성)이 있다.
  await page.mouse.move(setup.x, setup.y); await page.mouse.click(setup.x, setup.y, { button: 'right', delay: 60 });
  await page.waitForFunction((sid) => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object === sid) return true; return false; }, setup.bulf, { timeout: 10000 });
  const row = await findRow(page, setup.bulf, 0x03000076);
  expect(row, 'Drop 행(0x03000076)').toBeTruthy(); expect(row.parameter, 'Drop 인자는 Bulf SID').toBe(setup.bulf); expect(row.flags & 0x10000, '활성 행').toBeTruthy();
  const state = () => page.evaluate(({ nugget }) => { const app = window.netstorm, raw = app._campaignWorld.session().core().pool.slot(nugget); return { state: raw[11], queue: app._clientInputQueue.size }; }, setup);
  expect((await state()).state & 8, '내리기 전에는 내용물(contained)').toBe(8);
  await spyInputQueue(page);
  await page.mouse.move(row.x, row.y); await page.mouse.click(row.x, row.y, { delay: 60 });
  // Drop 명령이 올라가고 Session 이 소비해 성공 통지(0x030000b2)와 너겟 납품(0x020000b4)이 뒤따르며 너겟이 내용물에서 빠진다.
  await expect.poll(() => page.evaluate(() => window.__pushed.map((entry) => entry[1]).join(',')), { timeout: 15000 }).toContain('30000b2');
  const pushed = await page.evaluate(() => window.__pushed.filter((entry) => entry[0] === 'pushCommand'));
  expect(pushed.find((entry) => entry[1] === '3000076')?.[3], 'Drop 명령 인자(Bulf SID)').toBe(setup.bulf.toString(16));
  expect(pushed.some((entry) => entry[1] === '20000b4'), '내 너겟 납품 통지').toBe(true);
  await expect.poll(async () => (await state()).state & 8, { timeout: 15000, message: '너겟이 내용물에서 빠짐' }).toBe(0);
  await expect.poll(async () => (await state()).queue, { timeout: 5000, message: '입력 큐가 비워짐' }).toBe(0);
  expect(await page.evaluate(() => window.netstorm._clientGump.blockingPopups()), '메뉴가 닫힘').toBe(false);
  expect(errors).toEqual([]);
});

test('Priest Training 5: 내 폭탄(Archer 계열 파생 메뉴)의 우클릭 메뉴에 Detonate! 행이 있고 눌러 명령이 소비된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startPriestTraining5(page);
  // 내 폭탄(bombExplodeSmall, TYPE 113)을 내 섬에 만들어 메뉴를 연다. 폭탄은 화면 적중 대상이 아니라 앱의 객체 메뉴 열기 입구를 부른다(우클릭 적중 뒤와 같은 호출).
  const bomb = await page.evaluate(() => {
    const app = window.netstorm, world = app._campaignWorld.session().world(), sid = world.create(113, 0);
    world.setAssetOwner(sid, 1); world.pop(sid, 133, 186.5, 0);
    return { sid, opened: app._campaignProduction.ui.openObject(sid, { code: 0, x: 400, y: 300 }) };
  });
  expect(bomb.opened, '폭탄 메뉴 열기').toBe(1);
  const row = await findRow(page, bomb.sid, 0x03000074);
  expect(row, 'Detonate! 행(0x03000074)').toBeTruthy(); expect(row.parameter, 'Detonate 인자는 폭탄 SID').toBe(bomb.sid); expect(row.flags & 0x10000, '활성 행').toBeTruthy();
  expect(await findRow(page, bomb.sid, 0x030000ce), '비편집 모드에는 Turn To Obelisk! 행이 없다').toBeNull();
  await spyInputQueue(page);
  await page.mouse.move(row.x, row.y); await page.mouse.click(row.x, row.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.__pushed.filter((entry) => entry[0] === 'pushCommand').map((entry) => entry[1]).join(',')), { timeout: 10000 }).toContain('3000074');
  const pushed = await page.evaluate(() => window.__pushed.find((entry) => entry[0] === 'pushCommand' && entry[1] === '3000074'));
  expect(pushed[3], 'Detonate 명령 인자(폭탄 SID)').toBe(bomb.sid.toString(16));
  await expect.poll(() => page.evaluate(() => window.netstorm._clientInputQueue.size), { timeout: 5000, message: '입력 큐가 비워짐' }).toBe(0);
  expect(await page.evaluate(() => window.netstorm._clientGump.blockingPopups()), '메뉴가 닫힘').toBe(false);
  expect(errors).toEqual([]);
});

test('Priest Training 5: 내 Bulf 를 선택하고 월드의 너겟을 누르면 줍고, 그 Bulf 의 메뉴에 Drop 행이 생겨 눌러 내려놓는다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startPriestTraining5(page);
  // 내 섬 위(Bulf 옆)에 주인 없는 너겟을 놓고 Bulf 와 너겟의 화면 적중점을 찾는다. 너겟은 실제 미션에 없어 객체 생성 경로로 만든다(너겟 TYPE 154).
  const setup = await page.evaluate(() => {
    const app = window.netstorm, world = app._campaignWorld, session = world.session(), w = session.world(), types = app._gameAssets.types(); let bulf = null;
    for (const { sid, object } of w.objects()) if (object.owner === 1 && types[object.type - 70]?.assetName === 'bulf') { bulf = sid; break; }
    const nugget = w.create(154, 0); w.setAssetOwner(nugget, 0); w.pop(nugget, 136, 184, 0);
    const rect = app.canvas.getBoundingClientRect(), scan = (target) => {
      const hits = []; for (let y = 10; y < 760; y += 2) for (let x = 90; x < 1020; x += 2) if (world.pick(x, y, 0) === target) hits.push([x, y]);
      const hit = hits[Math.floor(hits.length / 2)];
      return hit ? { x: rect.left + hit[0] * rect.width / app._layout.logicalWidth, y: rect.top + hit[1] * rect.height / app._layout.logicalHeight } : null;
    };
    return { bulf, nugget, bulfHit: scan(bulf), nuggetHit: scan(nugget) };
  });
  expect(setup.bulfHit, 'Bulf 적중점').toBeTruthy(); expect(setup.nuggetHit, '너겟 적중점').toBeTruthy();
  const nuggetState = () => page.evaluate(({ nugget }) => window.netstorm._campaignWorld.session().core().pool.slot(nugget)[11], setup);
  expect(await nuggetState() & 3, '월드의 너겟은 살아 있다').toBe(0);
  // Bulf 를 선택하고(왼쪽 클릭) 너겟을 누르면 줍기 명령이 올라가 소비된다.
  await page.mouse.move(setup.bulfHit.x, setup.bulfHit.y); await page.mouse.click(setup.bulfHit.x, setup.bulfHit.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.selected?.() ?? null), { timeout: 5000, message: 'Bulf 선택' }).toBe(setup.bulf);
  await spyInputQueue(page);
  await page.mouse.move(setup.nuggetHit.x, setup.nuggetHit.y); await page.mouse.click(setup.nuggetHit.x, setup.nuggetHit.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.__pushed.filter((entry) => entry[0] === 'pushCommand').map((entry) => entry[1]).join(',')), { timeout: 10000 }).toContain('30000b1');
  const pickup = await page.evaluate(() => window.__pushed.find((entry) => entry[0] === 'pushCommand' && entry[1] === '30000b1'));
  expect([pickup[2], pickup[3]], '줍기 명령 인자(너겟 SID, Bulf SID)').toEqual([setup.nugget.toString(16), setup.bulf.toString(16)]);
  await expect.poll(async () => (await nuggetState()) & 3, { timeout: 10000, message: '월드의 너겟이 줍혀 사라짐' }).toBe(3);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientInputQueue.size), { timeout: 5000, message: '입력 큐가 비워짐' }).toBe(0);
  // 이제 그 Bulf 의 메뉴(실제 우클릭)에 Drop 행이 생기고, 눌러 내려놓으면 성공 통지가 뒤따른다.
  // 줍는 동안 Bulf 가 걸어 움직였으므로 지금 위치의 화면 적중점을 다시 찾는다.
  const moved = await page.evaluate((sid) => {
    const app = window.netstorm, world = app._campaignWorld, hits = [];
    for (let y = 10; y < 760; y += 3) for (let x = 90; x < 1020; x += 3) if (world.pick(x, y, 0) === sid) hits.push([x, y]);
    const rect = app.canvas.getBoundingClientRect(), hit = hits[Math.floor(hits.length / 2)];
    return hit ? { x: rect.left + hit[0] * rect.width / app._layout.logicalWidth, y: rect.top + hit[1] * rect.height / app._layout.logicalHeight } : null;
  }, setup.bulf);
  expect(moved, '줍은 뒤 Bulf 적중점').toBeTruthy();
  await page.mouse.move(moved.x, moved.y); await page.mouse.click(moved.x, moved.y, { button: 'right', delay: 60 });
  await page.waitForFunction((sid) => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object === sid) return true; return false; }, setup.bulf, { timeout: 10000 });
  const row = await findRow(page, setup.bulf, 0x03000076);
  expect(row, '줍은 뒤 Drop 행').toBeTruthy(); expect(row.parameter).toBe(setup.bulf);
  await spyInputQueue(page);
  await page.mouse.move(row.x, row.y); await page.mouse.click(row.x, row.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.__pushed.map((entry) => entry[1]).join(',')), { timeout: 15000 }).toContain('30000b2');
  expect(errors).toEqual([]);
});
