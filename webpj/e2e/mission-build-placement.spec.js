// 사제의 우클릭 메뉴로 신전·워크샾·제단을 짓는 과정(메뉴 선택 → 건설 위치 미리보기 → 위치 클릭 → 건설)을 실제 Chromium 의 마우스 조작으로 확인한다. (2026-11-21 WB04-E1135)
// 특히 건설 위치를 지정할 때 미리보기(건물 그림·사각 테두리·비용 표시)가 마우스 위치를 따라 그려지는지 캔버스 픽셀로 확인한다.
// 연결 전에는 미리보기가 카메라 픽셀 원점만큼 어긋나 섬 밖(마우스에서 수백 픽셀 떨어진 곳)에 그려졌다. 유효 판정과 실제 배치는 마우스 위치 기준이라 눈에 띄지 않았다.
// 신전은 이미 신전을 가진 미션(The War Begins!)에서는 원본처럼 비활성이라 신전이 없는 Tutorial2 에서, 워크샾·제단은 The War Begins! 에서 확인한다.
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

// 메인 메뉴에서 묶음의 미션을 시작하고 열린 안내 창을 모두 닫아 시계가 흐르게 한다.
async function startMission(page, group, mission) {
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, group);
  const label = await page.evaluate((name) => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === name)?.label, mission);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await closeDialogs(page);
}

// 열려 있는 안내 창을 마지막(주) 단추로 모두 닫고 시계가 흐를 때까지 기다린다.
async function closeDialogs(page) {
  for (let index = 0; index < 10; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
}

// 사제를 우클릭해 건설 메뉴를 열고 [행 번호…] 경로로 건물을 골라 커서에 쥔다. 쥔 건물 TYPE 과 사제의 화면 좌표(브라우저 기준)를 돌려준다.
async function pickBuilding(page, path) {
  const priest = await page.evaluate(() => {
    const app = window.netstorm, world = app._campaignWorld, session = world.session(), sid = session.findPriestParticipation(session.localOwner()), hits = [];
    for (let y = 10; y < 760; y += 3) for (let x = 90; x < 1020; x += 3) if (world.pick(x, y, 0) === sid) hits.push([x, y]);
    const rect = app.canvas.getBoundingClientRect(), hit = hits[Math.floor(hits.length / 2)];
    return { sid, x: rect.left + hit[0] * rect.width / app._layout.logicalWidth, y: rect.top + hit[1] * rect.height / app._layout.logicalHeight };
  });
  await page.mouse.move(priest.x, priest.y); await page.mouse.click(priest.x, priest.y, { button: 'right', delay: 60 });
  await page.waitForFunction((sid) => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object === sid) return true; return false; }, priest.sid, { timeout: 10000 });
  let menu = await page.evaluate((sid) => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object === sid) return token; return 0; }, priest.sid);
  // 메뉴의 한 행을 실제 마우스로 누르고 하위 메뉴가 있으면 펼쳐질 때까지 기다린다.
  const clickRow = async (token, index) => {
    const point = await page.evaluate(({ token, index }) => {
      const app = window.netstorm, root = app._clientGump, node = root.node(token), bounds = root.collection().bounds(token), rect = app.canvas.getBoundingClientRect();
      return { child: node.items[index].childId ? root.createState().entries[node.items[index].childId] : 0,
        x: rect.left + (bounds.left + 8) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + node.offsets[index] + node.heights[index] / 2) * rect.height / app._layout.logicalHeight };
    }, { token, index });
    await page.mouse.move(point.x, point.y); await page.mouse.click(point.x, point.y, { delay: 60 });
    if (point.child) await page.waitForFunction((child) => { const node = window.netstorm._clientGump.node(child); return node && (node.flags & 1) && !node.expanding; }, point.child, { timeout: 10000 });
    return point.child;
  };
  // 첫 행은 항상 Construct, 이어서 종류별 경로를 따라간다.
  menu = await clickRow(menu, 0); expect(menu, '건설 메뉴').toBeTruthy();
  for (let depth = 0; depth < path.length; depth += 1) { const child = await clickRow(menu, path[depth]); if (depth < path.length - 1) { expect(child, '하위 메뉴').toBeTruthy(); menu = child; } }
  await page.waitForFunction(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview !== 0, null, { timeout: 10000 });
  const type = await page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().type); expect(type, '쥔 건물 종류').toBeGreaterThan(0);
  return { priest, type };
}

// 캔버스에서 미리보기 테두리(흰색 또는 빨간색 1픽셀 선)의 바깥 범위를 읽는다. 맵 영역(x≥85)만 본다.
const previewBox = (page) => page.evaluate(() => {
  const canvas = window.netstorm.canvas, data = canvas.getContext('2d').getImageData(0, 0, canvas.width, canvas.height).data; let minX = 1e9, minY = 1e9, maxX = -1, maxY = -1, count = 0;
  for (let y = 0; y < canvas.height; y += 1) for (let x = 85; x < canvas.width; x += 1) {
    const at = (y * canvas.width + x) * 4, r = data[at], g = data[at + 1], b = data[at + 2];
    if ((r === 255 && g === 255 && b === 255) || (r > 180 && g < 60 && b < 60)) { count += 1; if (x < minX) minX = x; if (x > maxX) maxX = x; if (y < minY) minY = y; if (y > maxY) maxY = y; }
  }
  return { count, minX, minY, maxX, maxY };
});

// 사제 주변을 훑어 유효한 자리를 찾고, 그 자리에서 미리보기 테두리 중심이 마우스에 가까운지 확인한 뒤 클릭해 건물이 하나 늘고 완성되는지 본다.
async function placeAndBuild(page, name, { priest, type }, outputPath) {
  const legal = () => page.evaluate(() => Boolean(window.netstorm._campaignWorld.session().cursorDropSettings().legal));
  let spot = null, box = null;
  // 사제 둘레를 안쪽 고리부터 30도 간격으로 훑는다(제단처럼 신전 가까이에서만 유효한 건물도 찾도록 반지름 280까지).
  const rings = [[0, 0]]; for (let radius = 40; radius <= 280; radius += 30) for (let angle = 0; angle < 360; angle += 30) rings.push([Math.round(radius * Math.cos(angle * Math.PI / 180)), Math.round(radius * 0.75 * Math.sin(angle * Math.PI / 180))]);
  for (const [dx, dy] of rings) {
    const x = priest.x + dx, y = priest.y + dy; if (x < 100 || x > 1000 || y < 30 || y > 740) continue;
    await page.mouse.move(x, y); await page.waitForTimeout(120);
    if (await legal()) { spot = { x, y }; box = await previewBox(page); break; }
  }
  expect(spot, `${name} 유효한 건설 자리`).toBeTruthy(); expect(box.count, `${name} 미리보기 테두리 픽셀`).toBeGreaterThan(60);
  const centerX = (box.minX + box.maxX) / 2, centerY = (box.minY + box.maxY) / 2;
  expect(Math.abs(centerX - spot.x), `${name} 미리보기 가로 위치(마우스 ${spot.x}, 중심 ${centerX})`).toBeLessThan(110);
  expect(Math.abs(centerY - spot.y), `${name} 미리보기 세로 위치(마우스 ${spot.y}, 중심 ${centerY})`).toBeLessThan(110);
  await page.screenshot({ path: outputPath });
  const countOf = () => page.evaluate((target) => window.netstorm._campaignWorld.session().world().objects().filter(({ object }) => object.owner === 1 && object.type === target).length, type);
  const before = await countOf(); await page.mouse.click(spot.x, spot.y, { delay: 60 });
  await expect.poll(countOf, { timeout: 15000, message: `${name} 배치` }).toBe(before + 1);
  // 사제가 도착해 건물을 완성할 때까지 기다린다(완성하면 건설 중 표시 비트가 꺼진다).
  await page.waitForFunction((target) => { const session = window.netstorm._campaignWorld.session(); return session.world().objects().some(({ sid, object }) => object.owner === 1 && object.type === target && !(session.core().pool.slot(sid)[40] & 1)); }, type, { timeout: 200000 });
}

// 새 페이지의 오류를 모은다.
function collectErrors(page) { const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); }); return errors; }

test('Tutorial2: 사제가 신전과 워크샾을 짓는 위치 지정이 마우스를 따른다', async ({ page }) => {
  test.setTimeout(600000); const errors = collectErrors(page);
  await startMission(page, 'Early Missions', 'Tutorial2');
  // 신전(Construct → Temple → 첫 신전)을 쥐고 놓는다. 완성하면 튜토리얼이 안내 창을 열어 시계를 멈춘다.
  await placeAndBuild(page, '신전', await pickBuilding(page, [0, 0]), test.info().outputPath('build-preview-temple.png'));
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 30000 }); await closeDialogs(page);
  // 같은 사제로 워크샾(Construct → Workshop → 첫 워크샾)을 쥐고 놓는다.
  await placeAndBuild(page, '워크샾', await pickBuilding(page, [1, 0]), test.info().outputPath('build-preview-workshop.png'));
  expect(errors).toEqual([]);
});

test('The War Begins!: 사제가 제단을 짓는 위치 지정이 마우스를 따른다', async ({ page }) => {
  test.setTimeout(420000); const errors = collectErrors(page);
  await startMission(page, 'Struggle For Freedom', 'TheWarBegins');
  // 시작 3000 으로도 제단(500)은 지을 수 있다. 워크샾은 이 미션의 작은 섬에서 배치 규칙상 자리가 없어 Tutorial2 에서 확인한다.
  await placeAndBuild(page, '제단', await pickBuilding(page, [2]), test.info().outputPath('build-preview-altar.png'));
  expect(errors).toEqual([]);
});

// 신전이 이미 있으면 Construct 하위 메뉴의 Temple 행은 원본처럼 비활성(어두운 갈색)이며, 원본은 그 행에도 어두운 하위 메뉴 표식 '›' 를 그린다. (2026-11-21 WB04-E1136)
// 비활성 색을 회색(basic[8])으로 쓰던 때는 표식이 석판 배경에 묻혀 글자 폭(약 40픽셀)까지만 어두운 픽셀이 있었다.
test('The War Begins!: 비활성 Temple 행에도 하위 메뉴 표식이 어두운 색으로 그려진다', async ({ page }) => {
  test.setTimeout(240000); const errors = collectErrors(page);
  await startMission(page, 'Struggle For Freedom', 'TheWarBegins');
  const priest = await page.evaluate(() => {
    const app = window.netstorm, world = app._campaignWorld, session = world.session(), sid = session.findPriestParticipation(session.localOwner()), hits = [];
    for (let y = 10; y < 760; y += 3) for (let x = 90; x < 1020; x += 3) if (world.pick(x, y, 0) === sid) hits.push([x, y]);
    const rect = app.canvas.getBoundingClientRect(), hit = hits[Math.floor(hits.length / 2)];
    return { sid, x: rect.left + hit[0] * rect.width / app._layout.logicalWidth, y: rect.top + hit[1] * rect.height / app._layout.logicalHeight };
  });
  await page.mouse.click(priest.x, priest.y, { button: 'right', delay: 60 });
  await page.waitForFunction((sid) => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object === sid) return true; return false; }, priest.sid, { timeout: 10000 });
  const root = await page.evaluate((sid) => { const r = window.netstorm._clientGump; for (let token = r.root().child; token; token = r.node(token).next) if (r.node(token).command === 26 && r.node(token).object === sid) return token; return 0; }, priest.sid);
  const row = await page.evaluate((token) => { const app = window.netstorm, r = app._clientGump, node = r.node(token), bounds = r.collection().bounds(token), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + (bounds.left + 8) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + node.offsets[0] + node.heights[0] / 2) * rect.height / app._layout.logicalHeight, child: node.items[0].childId ? r.createState().entries[node.items[0].childId] : 0 }; }, root);
  await page.mouse.move(row.x, row.y); await page.mouse.click(row.x, row.y, { delay: 60 });
  await page.waitForFunction((child) => { const node = window.netstorm._clientGump.node(child); return node && (node.flags & 1) && !node.expanding; }, row.child, { timeout: 10000 }); await page.waitForTimeout(400);
  // 첫 행(Temple)에서 어두운 갈색 픽셀이 가로로 퍼진 범위를 잰다. 글자만이면 약 40, 표식까지 있으면 약 49픽셀이다.
  const span = await page.evaluate((child) => {
    const app = window.netstorm, r = app._clientGump, node = r.node(child), bounds = r.collection().bounds(child), canvas = app.canvas, data = canvas.getContext('2d').getImageData(0, 0, canvas.width, canvas.height).data;
    const top = Math.round(bounds.top + node.offsets[0]), bottom = Math.round(top + node.heights[0]); let minX = 1e9, maxX = -1;
    for (let y = top; y < bottom; y += 1) for (let x = Math.round(bounds.left); x < Math.round(bounds.right); x += 1) { const at = (y * canvas.width + x) * 4; if (data[at] < 80 && data[at + 1] < 50 && data[at + 2] < 40) { if (x < minX) minX = x; if (x > maxX) maxX = x; } }
    return maxX - minX;
  }, row.child);
  expect(span, 'Temple 행 어두운 픽셀 가로 범위(글자 + 표식)').toBeGreaterThanOrEqual(46);
  expect(errors).toEqual([]);
});
