// 영어 Tutorial2를 실제 Chromium에서 안내 A부터 완료 창과 Tutorial3 시작까지 자연 진행으로 확인한다. 한국어 흐름(korean-mission.spec.js)과 같은 조작을 영어 문구로 반복한다. (2026-11-20 WB04-E1091)
const { test, expect } = require('@playwright/test');

// 실제 게임 조작부의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    if (!logical) throw new Error(`조작부 없음: ${text}`);
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 열려 있는 안내 창의 마지막(주) 단추를 누른다. 영어 문구에 의존하지 않고 한국어 흐름의 '다음'·'확인'에 해당한다.
async function clickLast(page) {
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
  if (label === null) throw new Error('안내 창의 단추 없음');
  await clickLabel(page, label);
}

// 두 번째 튜토리얼의 영어 흐름 전체를 실제 미션에서 확인한다.
test('영어 Tutorial2를 시작부터 완료 창과 Tutorial3 시작까지 진행한다', async ({ page }) => {
  test.setTimeout(420000);
  const errors = []; page.on('pageerror', (error) => { errors.push(error.message); console.error('브라우저 예외:', error.message); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1' } }));
  });
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/?mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const second = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial2')?.label);
  expect(second).toBeTruthy(); await clickLabel(page, second);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  await page.screenshot({ path: test.info().outputPath('english-tutorial2.png') });
  await clickLast(page); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'A1.');
  const pictures = await page.evaluate(() => window.netstorm._mainMenu.dialog.body.softLines.flatMap((line) => line.fragments).filter((fragment) => fragment.picture).map(({picture}) => ({ width: picture.width, height: picture.height })));
  expect(pictures).toHaveLength(2); expect(pictures.every((picture) => picture.width > 0 && picture.height > 0)).toBe(true);
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-next.png') });
  // 마지막 안내를 닫고 실제 사제 그림을 오른쪽 클릭한다.
  await clickLast(page); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'A2.'); await clickLast(page);
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  const priest = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), sid = session.findPriestParticipation(session.localOwner());
    const slot = session.core().pool.slot(sid), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength), point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true)), rect = app.canvas.getBoundingClientRect();
    return { sid, x: rect.left + (point.x - 16) * rect.width / app._layout.logicalWidth, y: rect.top + (point.y - 11) * rect.height / app._layout.logicalHeight }; });
  await page.mouse.click(priest.x, priest.y, { button: 'right', delay: 60 });
  await page.waitForFunction(() => { const app = window.netstorm, root = app._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object) return true; return false; }, null, { timeout: 10000 });
  const rootMenu = await page.evaluate(() => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object) return token; return 0; });
  expect(await page.evaluate((token) => window.netstorm._clientGump.node(token).object, rootMenu)).toBe(priest.sid);
  await page.screenshot({ path: test.info().outputPath('english-priest-menu.png') });
  // 원본 메뉴 행을 실제 마우스로 눌러 건설 → 신전 → 윈드 신전을 선택한다.
  const clickMenuRow = async (token, index) => {
    const point = await page.evaluate(({ token, index }) => { const app = window.netstorm, root = app._clientGump, node = root.node(token), bounds = root.collection().bounds(token), rect = app.canvas.getBoundingClientRect();
      return { child: node.items[index].childId ? root.createState().entries[node.items[index].childId] : 0,
        x: rect.left + (bounds.left + 8) * rect.width / app._layout.logicalWidth,
        y: rect.top + (bounds.top + node.offsets[index] + node.heights[index] / 2) * rect.height / app._layout.logicalHeight }; }, { token, index });
    await page.mouse.move(point.x, point.y); await page.mouse.click(point.x, point.y, { delay: 60 });
    if (point.child) await page.waitForFunction((child) => { const node = window.netstorm._clientGump.node(child); return node && (node.flags & 1) && !node.expanding; }, point.child, { timeout: 10000 });
    return point.child;
  };
  const construct = await clickMenuRow(rootMenu, 0); expect(construct).toBeTruthy();
  const temples = await clickMenuRow(construct, 0); expect(temples).toBeTruthy();
  await clickMenuRow(temples, 0);
  await page.waitForFunction(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview !== 0, null, { timeout: 10000 });
  const preview = await page.evaluate(() => { const session = window.netstorm._campaignWorld.session(), drop = session.cursorDropSettings(), slot = session.core().pool.slot(drop.preview);
    return { type: new DataView(slot.buffer, slot.byteOffset, slot.byteLength).getUint16(18, true), costFlags: drop.costFlags }; });
  expect(preview).toEqual({ type: 123, costFlags: 0x160 });
  // 실제 마우스로 섬의 유효한 자리를 찾은 뒤 신전을 내려놓는다.
  let target = null;
  for (const [dx, dy] of [[60, 20], [-60, 20], [65, -35], [-65, -35], [35, 70], [-35, 70], [100, 35], [-100, 35]]) {
    await page.mouse.move(priest.x + dx, priest.y + dy); await page.waitForTimeout(100);
    if (await page.evaluate(() => Boolean(window.netstorm._campaignWorld.session().cursorDropSettings().legal))) { target = { x: priest.x + dx, y: priest.y + dy }; break; }
  }
  expect(target).toBeTruthy();
  const before = await page.evaluate(() => window.netstorm._campaignWorld.session().world().objects().filter(({object}) => object.type === 123 && object.owner === 1).length);
  await page.mouse.click(target.x, target.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().world().objects().filter(({object}) => object.type === 123 && object.owner === 1).length), { timeout: 15000 }).toBe(before + 1);
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-temple.png') });
  // 실제 사제가 건설 위치에 도착해 신전을 완성하고 튜토리얼이 다음 안내로 넘어가는지 기다린다.
  await page.waitForFunction(() => { const session = window.netstorm._campaignWorld.session(); return session.world().objects().some(({sid, object}) => object.type === 123 && object.owner === 1 && !(session.core().pool.slot(sid)[40] & 1)); }, null, { timeout: 120000 });
  await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'B.' && Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 30000 });
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-temple-complete.png') });
  // 다음 안내를 닫고 같은 사제로 썬 워크샾을 건설한다.
  await clickLast(page); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'B1.'); await clickLast(page);
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  const movedPriest = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), sid = session.findPriestParticipation(session.localOwner()), slot = session.core().pool.slot(sid), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength);
    const point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true)), rect = app.canvas.getBoundingClientRect(); let hit = null;
    // 완공된 신전과 겹칠 수 있으므로 현재 사제의 실제 적중 픽셀을 찾는다.
    for (let y = Math.trunc(point.y) - 50; y <= Math.trunc(point.y) + 20 && !hit; y += 1) for (let x = Math.trunc(point.x) - 50; x <= Math.trunc(point.x) + 20; x += 1) {
      if (app._campaignWorld.pick(x, y, 0) === sid) { hit = { x, y }; break; }
    }
    if (!hit) throw new Error('완공 뒤 사제의 적중 픽셀 없음');
    return { sid, x: rect.left + hit.x * rect.width / app._layout.logicalWidth, y: rect.top + hit.y * rect.height / app._layout.logicalHeight }; });
  await page.mouse.click(movedPriest.x, movedPriest.y, { button: 'right', delay: 60 });
  await page.waitForFunction(() => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object) return true; return false; }, null, { timeout: 10000 });
  const nextRoot = await page.evaluate(() => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object) return token; return 0; });
  const nextConstruct = await clickMenuRow(nextRoot, 0), workshops = await clickMenuRow(nextConstruct, 1); expect(workshops).toBeTruthy();
  await clickMenuRow(workshops, 0);
  await page.waitForFunction(() => window.netstorm._campaignWorld.session().cursorDropSettings().type === 85, null, { timeout: 10000 });
  let workshopTarget = null;
  for (const [dx, dy] of [[60, 20], [-60, 20], [65, -35], [-65, -35], [35, 70], [-35, 70], [100, 35], [-100, 35], [0, 90], [0, -90]]) {
    await page.mouse.move(movedPriest.x + dx, movedPriest.y + dy); await page.waitForTimeout(100);
    if (await page.evaluate(() => Boolean(window.netstorm._campaignWorld.session().cursorDropSettings().legal))) { workshopTarget = { x: movedPriest.x + dx, y: movedPriest.y + dy }; break; }
  }
  expect(workshopTarget).toBeTruthy(); await page.mouse.click(workshopTarget.x, workshopTarget.y, { delay: 60 });
  await page.waitForFunction(() => { const session = window.netstorm._campaignWorld.session(); return session.world().objects().some(({sid, object}) => object.type === 85 && object.owner === 1 && !(session.core().pool.slot(sid)[40] & 1)); }, null, { timeout: 120000 });
  await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'C.' && Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 30000 });
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-workshop-complete.png') });
  // 완성된 워크샾의 실제 적중 픽셀을 오른쪽 클릭한다.
  await clickLast(page); await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  const workshop = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), found = session.world().objects().find(({object}) => object.type === 85 && object.owner === 1);
    const slot = session.core().pool.slot(found.sid), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength), point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true)), rect = app.canvas.getBoundingClientRect(); let hit = null;
    // 다른 객체와 겹치지 않고 실제 워크샾이 선택되는 픽셀을 찾는다.
    for (let y = Math.trunc(point.y) - 100; y <= Math.trunc(point.y) + 30 && !hit; y += 2) for (let x = Math.trunc(point.x) - 100; x <= Math.trunc(point.x) + 30; x += 2) {
      if (app._campaignWorld.pick(x, y, 0) === found.sid) { hit = { x, y }; break; }
    }
    if (!hit) throw new Error('완공된 워크샾의 적중 픽셀 없음');
    return { sid: found.sid, x: rect.left + hit.x * rect.width / app._layout.logicalWidth, y: rect.top + hit.y * rect.height / app._layout.logicalHeight }; });
  await page.mouse.click(workshop.x, workshop.y, { button: 'right', delay: 60 });
  await page.waitForFunction((sid) => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object === sid) return true; return false; }, workshop.sid, { timeout: 10000 });
  const factoryRoot = await page.evaluate((sid) => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object === sid) return token; return 0; }, workshop.sid);
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-factory-menu.png') });
  const knowledge = await clickMenuRow(factoryRoot, 1); expect(knowledge).toBeTruthy();
  const selectedType = await page.evaluate((token) => window.netstorm._clientGump.node(token).items[0].parameter >>> 16, knowledge);
  await clickMenuRow(knowledge, 0);
  await page.waitForFunction((type) => window.netstorm._campaignWorld.session().production().contains(type), selectedType, { timeout: 15000 });
  await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'D.' && Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 30000 });
  // 자연 완공 후 지도 위쪽에는 이동한 비용 글자의 잔상이 남지 않는다. 전체 다시 그리기 전후의 같은 영역 해시가 일치해야 한다.
  // (2026-11-20 WB04-E1082: 실제 Uber HUD 도킹으로 상단 28줄이 임시 검은 바탕이 아니라 지도가 되어 검사 방식을 바꿨다)
  const topHash = () => page.evaluate(() => { const screen = window.netstorm._clientPaletteScreen, pixels = screen.lock(); let hash = 2166136261;
    try { for (let y = 0; y < 60; y += 1) for (let x = 100; x < screen.width; x += 1) hash = Math.imul(hash ^ pixels[y * screen.pitch + x], 16777619) >>> 0; return hash; }
    finally { screen.unlock(); } });
  const shownTop = await topHash(); await page.evaluate(() => window.netstorm._clientRenderer.invalidateAll()); await page.waitForTimeout(400);
  expect(await topHash()).toBe(shownTop);
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-research.png') });
  // D: 안내를 닫고 생산 격자의 썬 디스크 발사기를 왼쪽 클릭으로 집은 뒤 섬의 합법 위치에 내려놓는다.
  await clickLast(page); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'D1.'); await clickLast(page);
  const archers = () => page.evaluate(() => window.netstorm._campaignWorld.session().world().objects().filter(({object}) => object.type === 71 && object.owner === 1).length);
  // 현재 사제 주변에서 합법으로 판정되는 자리를 찾는다.
  const findDrop = async (offsets) => {
    const base = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), sid = session.findPriestParticipation(session.localOwner()), slot = session.core().pool.slot(sid), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength);
      const point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true)), rect = app.canvas.getBoundingClientRect();
      return { x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight }; });
    for (const [dx, dy] of offsets) {
      await page.mouse.move(base.x + dx, base.y + dy); await page.waitForTimeout(100);
      if (await page.evaluate(() => Boolean(window.netstorm._campaignWorld.session().cursorDropSettings().legal))) return { x: base.x + dx, y: base.y + dy };
    }
    return null;
  };
  // 생산 격자의 첫 항목을 누르고(실제 이미지 픽셀 적중) 합법 자리에 내려놓아 발사기 수가 하나 늘 때까지 기다린다.
  const placeArcher = async (offsets) => {
    await page.waitForFunction(() => !window.netstorm._gameClock.paused() && !window.netstorm._mainMenu.dialog, null, { timeout: 15000 });
    // 처음 세 개 뒤에는 생산 보충 시간이 지나야 다음 항목의 대기 표식(0x100)이 풀리므로 풀릴 때까지 기다린다.
    await page.waitForFunction(() => { const app = window.netstorm, root = app._clientGump, token = root.node(app._campaignProduction.ui.node().teleport).child; if (!token) return false; const raw = app._campaignWorld.session().core().pool.slot(root.node(token).content); return !((raw[20] | (raw[21] << 8)) & 0x100); }, null, { timeout: 240000 });
    const item = await page.evaluate(() => { const app = window.netstorm, root = app._clientGump, token = root.node(app._campaignProduction.ui.node().teleport).child, rect = app.canvas.getBoundingClientRect();
      if (!token) return null; const bounds = root.collection().bounds(token);
      return { x: rect.left + (bounds.left + bounds.right) / 2 * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + bounds.bottom) / 2 * rect.height / app._layout.logicalHeight }; });
    expect(item).toBeTruthy(); await page.mouse.move(item.x, item.y); await page.mouse.click(item.x, item.y, { delay: 60 });
    await page.waitForTimeout(300);
    await page.waitForFunction(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview !== 0, null, { timeout: 5000 });
    const spot = await findDrop(offsets); expect(spot).toBeTruthy();
    const before = await archers(); await page.mouse.click(spot.x, spot.y, { delay: 60 }); await expect.poll(archers, { timeout: 15000 }).toBe(before + 1);
  };
  await placeArcher([[60, 20], [-60, 20], [65, -35], [-65, -35], [35, 70], [-35, 70], [100, 35], [-100, 35], [0, 90], [0, -90], [120, 0], [-120, 0]]);
  await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'E.' && Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 30000 });
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-weapon-range.png') });
  // E: 두 번째 발사기를 내려놓으면 F(에너지)로 넘어간다. 발사기 완공 통지 명령이 입력 큐를 막지 않아야 두 번째 클릭이 전달된다.
  await clickLast(page); await placeArcher([[-60, 20], [60, 20], [-90, -20], [90, -20], [0, 90], [0, -90], [-120, 40], [120, 40]]);
  await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'F.' && Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 30000 });
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-energy.png') });
  // F: 에너지 안내의 다음 쪽을 닫고 완성된 신전을 왼쪽 클릭해 에너지 범위를 보인다. 몇 초 뒤 G(에너지, 세 번째)로 넘어간다.
  await clickLast(page); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'F1.');
  await clickLast(page); await page.waitForFunction(() => !window.netstorm._gameClock.paused() && !window.netstorm._mainMenu.dialog, null, { timeout: 15000 });
  const temple = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), found = session.world().objects().find(({object}) => object.type === 123 && object.owner === 1);
    const slot = session.core().pool.slot(found.sid), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength), point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true)), rect = app.canvas.getBoundingClientRect(); let hit = null;
    for (let y = Math.trunc(point.y) - 100; y <= Math.trunc(point.y) + 30 && !hit; y += 2) for (let x = Math.trunc(point.x) - 100; x <= Math.trunc(point.x) + 30; x += 2) { if (app._campaignWorld.pick(x, y, 0) === found.sid) { hit = { x, y }; break; } }
    if (!hit) throw new Error('완공된 신전의 적중 픽셀 없음');
    return { sid: found.sid, x: rect.left + hit.x * rect.width / app._layout.logicalWidth, y: rect.top + hit.y * rect.height / app._layout.logicalHeight }; });
  await page.mouse.move(temple.x, temple.y); await page.mouse.click(temple.x, temple.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.selected()), { timeout: 5000 }).toBe(temple.sid);
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-temple-selected.png') });
  await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'G.' && Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 30000 });
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-g.png') });
  // G: 발사기를 두 개 더 내려놓으면 H(에너지 소비)로 넘어간다. 자리는 사제 주변 고리 위의 여러 후보에서 합법 판정으로 찾는다.
  const ring = []; for (const radius of [50, 80, 110, 140, 170]) for (let angle = 0; angle < 360; angle += 30) ring.push([Math.round(Math.cos(angle * Math.PI / 180) * radius), Math.round(Math.sin(angle * Math.PI / 180) * radius * 0.7)]);
  await clickLast(page); await placeArcher(ring); await placeArcher(ring.slice().reverse());
  await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'H.' && Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 30000 });
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-h.png') });
  // H: 에너지는 건설할 때만 쓰인다는 안내 뒤 H1(회수)로 간다. 완성된 발사기를 오른쪽 클릭해 Salvage 를 고른다.
  await clickLast(page); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'H1.');
  await clickLast(page); await page.waitForFunction(() => !window.netstorm._gameClock.paused() && !window.netstorm._mainMenu.dialog, null, { timeout: 15000 });
  const archer = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), found = session.world().objects().find(({object}) => object.type === 71 && object.owner === 1);
    const slot = session.core().pool.slot(found.sid), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength), point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true)), rect = app.canvas.getBoundingClientRect(); let hit = null;
    for (let y = Math.trunc(point.y) - 60; y <= Math.trunc(point.y) + 20 && !hit; y += 1) for (let x = Math.trunc(point.x) - 60; x <= Math.trunc(point.x) + 20; x += 1) { if (app._campaignWorld.pick(x, y, 0) === found.sid) { hit = { x, y }; break; } }
    if (!hit) throw new Error('발사기의 적중 픽셀 없음'); return { sid: found.sid, x: rect.left + hit.x * rect.width / app._layout.logicalWidth, y: rect.top + hit.y * rect.height / app._layout.logicalHeight }; });
  await page.mouse.click(archer.x, archer.y, { button: 'right', delay: 60 }); await page.waitForTimeout(800);
  // 공통 객체 메뉴에는 회수(Salvage) 행이 있다. 원본 회수 명령(0x0200005d)을 가진 행을 눌러 발사기를 회수한다.
  await page.waitForFunction(() => { const root = window.netstorm._clientGump; for (let t = root.root().child; t; t = root.node(t).next) if (root.node(t).command === 26 && root.node(t).object) return true; return false; }, null, { timeout: 10000 });
  const archerMenu = await page.evaluate(() => { const root = window.netstorm._clientGump; for (let t = root.root().child; t; t = root.node(t).next) if (root.node(t).command === 26 && root.node(t).object) return t; return 0; });
  const salvageRow = await page.evaluate((token) => { const node = window.netstorm._clientGump.node(token); for (let index = 0; index < node.itemCount; index += 1) if ((node.items[index].command >>> 0) === 0x0200005d) return index; return -1; }, archerMenu);
  expect(salvageRow).toBeGreaterThanOrEqual(0); expect(await page.evaluate((token) => window.netstorm._clientGump.node(token).object, archerMenu)).toBe(archer.sid);
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-archer-menu.png') });
  await page.evaluate((token) => { window.__salvageBefore = window.netstorm._campaignWorld.session().world().objects().filter(({object}) => object.type === 71 && object.owner === 1).length; }, archerMenu);
  await clickMenuRow(archerMenu, salvageRow);
  // H 단계는 회수 완료 뒤 시간이 지나면 I(미션 완료)로 넘어간다.
  await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'I.' && Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 60000 });
  await page.screenshot({ path: test.info().outputPath('english-tutorial2-complete.png') });
  // 완료 창이 뜨면 미션 대본의 Config 효과가 완료 기록(DoneTutorial2)을 진행 저장소에 남긴다.
  const done = await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.progress') ?? '{}').values ?? {});
  expect(String(done.DoneTutorial2 ?? '')).toBe('1');
  // 완료 창의 다음 튜토리얼 단추는 현재 미션을 떠나 Tutorial3 을 시작한다.
  const buttons = await page.evaluate(() => window.netstorm._mainMenu.dialog.buttons.map((button) => button.label)); expect(buttons.length).toBe(2);
  await clickLabel(page, buttons[1]);
  await page.waitForFunction(() => window.netstorm._lastCampaignMission?.name === 'Tutorial3' && window.netstorm._campaignWorld && window.netstorm._clientGuide.current(), null, { timeout: 60000 });
  expect(errors).toEqual([]);
});
