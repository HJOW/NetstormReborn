// 실제 브라우저에서 캠페인 목록 클릭부터 첫 Tutorial 화면까지 연결을 확인한다.
const { test, expect } = require('@playwright/test');

// 현재 조작부의 논리 좌표를 Canvas CSS 좌표로 바꾸어 실제 마우스로 누른다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    if (!logical) throw new Error(`조작부 없음: ${text}`);
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('캠페인 목록에서 첫 Tutorial의 월드와 안내를 표시한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/debug.html');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  expect(first).toBeTruthy(); await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  const state = await page.evaluate(() => {
    const app = window.netstorm;
    return { title: document.title, frame: app._clientRenderer.drawCount, paused: app._gameClock.paused(), width: app._surface.width,
      visiblePixels: app._surface.pixels.some((pixel) => pixel !== 0) };
  });
  expect(state.frame).toBeGreaterThan(0); expect(state.paused).toBe(true); expect(state.width).toBe(1280); expect(state.visiblePixels).toBe(true);
  // 첫 안내를 실제 버튼으로 닫고 월드의 사제 그림을 마우스로 선택한다.
  // 여러 안내 페이지의 실제 버튼을 끝까지 누른다.
  for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (label === null) break;
    await clickLabel(page, label);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  const priest = await page.evaluate(() => {
    const app = window.netstorm, world = app._campaignWorld, session = world.session(), sid = session.findPriestParticipation(session.localOwner());
    const slot = session.core().pool.slot(sid), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength);
    const point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true));
    const rect = app.canvas.getBoundingClientRect();
    return { sid, x: rect.left + (point.x - 16) * rect.width / app._layout.logicalWidth, y: rect.top + (point.y - 11) * rect.height / app._layout.logicalHeight };
  });
  expect(priest.sid).toBeGreaterThan(0); await page.mouse.click(priest.x, priest.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.selected())).toBe(priest.sid);
  // 실제 좌클릭 이동 요청이 같은 로컬 Session 패킷에 도달하는지 확인한다.
  const destination = await page.evaluate((sid) => {
    const app = window.netstorm, session = app._campaignWorld.session(), slot = session.core().pool.slot(sid);
    const raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength), point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true) + 2, raw.getFloat32(18, true));
    const rect = app.canvas.getBoundingClientRect();
    return { count: session.localConnectionPackets().length, startX: raw.getFloat32(14, true), startY: raw.getFloat32(18, true),
      x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight };
  }, priest.sid);
  await page.mouse.click(destination.x, destination.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().localConnectionPackets().length)).toBeGreaterThan(destination.count);
  expect(await page.evaluate(() => window.netstorm._campaignWorld.session().localConnectionPackets().at(-1))).toBe(0x40);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientInputQueue.size)).toBe(0);
  await page.waitForFunction(({ sid, x, y }) => {
    const bytes = window.netstorm._campaignWorld.session().core().pool.slot(sid), raw = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    return raw.getFloat32(14, true) !== x || raw.getFloat32(18, true) !== y;
  }, { sid: priest.sid, x: destination.startX, y: destination.startY }, { timeout: 15000 });
  // 이동 첫 좌표만 관찰하고 끝내지 않고 발소리 단계까지 게임 시계와 프레임이 계속 진행되는지 확인한다.
  const movementTime = await page.evaluate(() => window.netstorm._gameClock.time().game);
  await page.waitForFunction((start) => window.netstorm._gameClock.time().game >= start + 12, movementTime, { timeout: 25000 });
  // 자동 생산 프레임이 실제 기술 부모에 다리 내용물을 공급한다.
  const bridgeCount = await page.evaluate(async () => {
    const { RawContainedFinder } = await import('./core/raw-contained-finder.js');
    const session = window.netstorm._campaignWorld.session(), pool = session.core().pool, finder = new RawContainedFinder(pool, session.productionFinder());
    let count = 0;
    for (let sid = finder.begin(session.currentTechnologyParent()); sid; sid = finder.next()) { const raw = pool.slot(sid); if ((raw[20] | (raw[21] << 8)) & 0x40) count += 1; }
    return count;
  });
  expect(bridgeCount).toBeGreaterThan(0);
  // 실제 화면에 설치된 생산 격자의 다리를 마우스로 집고 취소한다.
  const productionItem = await page.evaluate(() => {
    const app = window.netstorm, root = app._clientGump, panel = app._campaignProduction.ui.node();
    const token = root.node(panel.bridge).child, item = root.node(token), bounds = root.collection().bounds(token), rect = app.canvas.getBoundingClientRect();
    return { sid: item.content, x: rect.left + (bounds.left + bounds.right) / 2 * rect.width / app._layout.logicalWidth,
      y: rect.top + (bounds.top + bounds.bottom) / 2 * rect.height / app._layout.logicalHeight };
  });
  await page.mouse.click(productionItem.x, productionItem.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview), { timeout: 15000 }).toBe(productionItem.sid);
  await page.keyboard.press('Escape'); await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview)).toBe(0);
  // 실제 Q 입력은 첫 생산 슬롯의 다리를 같은 커서에 집고 Escape는 돌려놓는다.
  await page.keyboard.press('q');
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview), { timeout: 15000 }).toBeGreaterThan(0);
  const taken = await page.evaluate(() => { const session = window.netstorm._campaignWorld.session(); return { lock: session.cursorDropSettings().lockDepth, parts: session.cursor().lifecycle.previewCount }; });
  expect(taken.lock).toBe(0); expect(taken.parts).toBeGreaterThan(0);
  const previewFrame = await page.evaluate(() => window.netstorm._clientRenderer.drawCount);
  await page.waitForFunction((before) => window.netstorm._clientRenderer.drawCount > before, previewFrame, { timeout: 15000 });
  const previewCursor = await page.evaluate(() => ({ number: window.netstorm._wantedCursor(), css: window.netstorm.canvas.style.cursor }));
  expect([2, 3]).toContain(previewCursor.number); expect(previewCursor.css).toContain('url(');
  // 오른쪽 버튼과 왼쪽 방향키는 같은 원본 커서의 방향을 각각 +2와 -2로 돌린다.
  const initialDirection = await page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().direction);
  await page.mouse.click(destination.x, destination.y, { button: 'right', delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().direction)).toBe((initialDirection + 2) % 8);
  await page.keyboard.press('ArrowLeft');
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().direction)).toBe(initialDirection);
  await page.keyboard.press('Escape'); await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview)).toBe(0);
  // P/R은 같은 실제 사제를 선택하고 반복 입력은 사제 보기 명령을 큐에서 소비한다.
  await page.keyboard.press('p'); await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.selected())).toBe(priest.sid);
  await page.keyboard.press('r'); await expect.poll(() => page.evaluate(() => window.netstorm._clientInputQueue.size)).toBe(0);
  // 화면 오른쪽 가장자리에 포인터를 두면 실제 카메라의 현재 X 원점이 이동한다.
  const edge = await page.evaluate(() => {
    const app = window.netstorm, rect = app.canvas.getBoundingClientRect();
    return { before: app._clientGameplay.camera().snapshot().pixels.x, x: rect.right - 0.1, y: rect.top + rect.height / 2 };
  });
  await page.mouse.move(edge.x, edge.y);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x), { timeout: 15000 }).toBeGreaterThan(edge.before);
  // F4의 원본 ViewHome 큐가 튜토리얼 단계와 실제 카메라에 반영된다.
  const centered = await page.evaluate(() => {
    const app = window.netstorm, rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + rect.width / 2, y: rect.top + rect.height / 2, stage: app._clientGuide.current().runtime.gate.stage };
  });
  await page.mouse.move(centered.x, centered.y); const nextPanned = await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x);
  await page.keyboard.press('n'); await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.selected())).toBe(priest.sid);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x), { timeout: 15000 }).toBeLessThan(nextPanned);
  const nextView = await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x);
  await page.mouse.move(edge.x, edge.y);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x), { timeout: 15000 }).toBeGreaterThan(nextView);
  await page.mouse.move(centered.x, centered.y); const priestPanned = await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x);
  await page.keyboard.press('F5');
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x), { timeout: 15000 }).toBeLessThan(priestPanned);
  const priestView = await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x);
  await page.mouse.move(edge.x, edge.y);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x), { timeout: 15000 }).toBeGreaterThan(priestView);
  await page.mouse.move(centered.x, centered.y); const panned = await page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x);
  await page.keyboard.press('F4');
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.stage), { timeout: 15000 }).toBe(centered.stage + 1);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGameplay.camera().snapshot().pixels.x), { timeout: 15000 }).toBeLessThan(panned);
  // 다음 안내를 실제 버튼으로 닫고 생산 다리를 실제 화면에서 내려놓는다.
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (label === null) break;
    await clickLabel(page, label);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  await page.keyboard.press('q'); await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview)).toBeGreaterThan(0);
  const placement = await page.evaluate(() => {
    const app = window.netstorm, session = app._campaignWorld.session(), core = session.core(), cursor = session.cursor(), drop = session.cursorDropSettings();
    const records = app._clientPlayers.records(), temple = new DataView(records.buffer, records.byteOffset + session.localOwner() * 180 + 0x7c, 4).getUint32(0, true);
    const slot = core.pool.slot(temple), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength);
    const home = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true));
    // 실제 커서의 배치 판정으로 신전 주변 후보를 확인한다.
    for (let y = home.y - 150; y <= home.y + 150; y += 11) {
      for (let x = home.x - 200; x <= home.x + 200; x += 16) {
        if (x < 0 || y < 28 || x >= app._layout.logicalWidth || y >= app._layout.logicalHeight) continue;
        cursor.update(x, y);
        if (drop.legal) {
          const rect = app.canvas.getBoundingClientRect();
          return { x: rect.left + x * rect.width / app._layout.logicalWidth, y: rect.top + y * rect.height / app._layout.logicalHeight,
            packets: session.localConnectionPackets().length, bridges: session.world().objects().filter(({ object }) => object.type === core.bridgeState.bridgeType).length };
        }
      }
    }
    throw new Error('실제 다리 배치 가능 지점이 없습니다');
  });
  await page.mouse.move(placement.x, placement.y);
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().legal)).toBe(1);
  await page.mouse.click(placement.x, placement.y, { delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview)).toBe(0);
  await page.waitForFunction(({ packets, bridges }) => {
    const session = window.netstorm._campaignWorld.session(), core = session.core();
    return session.localConnectionPackets().slice(packets).includes(0x3e) && session.world().objects().filter(({ object }) => object.type === core.bridgeState.bridgeType).length > bridges;
  }, placement, { timeout: 15000 });
  expect(errors).toEqual([]);
});

// 같은 실제 캠페인 목록에서 두 번째 Tutorial의 로그인과 안내까지 이어진다.
test('캠페인 목록에서 두 번째 Tutorial의 월드와 안내를 표시한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1' } }));
  });
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/debug.html');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const second = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial2')?.label);
  expect(second).toBeTruthy(); await clickLabel(page, second);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  const state = await page.evaluate(() => ({ tutorial: window.netstorm._clientGuide.current().runtime.gate.tutorialNumber, frame: window.netstorm._clientRenderer.drawCount, paused: window.netstorm._gameClock.paused() }));
  expect(state.tutorial).toBe(2); expect(state.frame).toBeGreaterThan(0); expect(state.paused).toBe(true);
  // 두 번째 미션의 실제 여러 안내 페이지를 닫고 게임 시계·월드 프레임을 진행한다.
  for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (label === null) break;
    await clickLabel(page, label);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
  await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 5, started, { timeout: 15000 });
  expect(errors).toEqual([]);
});

// 선행 완료가 있는 Tutorial 3~6은 각자의 실제 원본 목록·Fort·단계 판정기를 사용한다.
for (let number = 3; number <= 6; number += 1) {
  test(`캠페인 목록에서 Tutorial${number} 월드와 안내를 표시한다`, async ({ page }) => {
    const errors = []; page.on('pageerror', (error) => errors.push(error.message));
    await page.addInitScript((previous) => {
      localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
      localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { [`DoneTutorial${previous}`]: '1' } }));
    }, number - 1);
    await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/debug.html');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
    await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
    const label = await page.evaluate((mission) => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === mission)?.label, `Tutorial${number}`);
    expect(label).toBeTruthy(); await clickLabel(page, label);
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
    await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
    expect(await page.evaluate(() => window.netstorm._clientGuide.current().runtime.gate.tutorialNumber)).toBe(number);
    // 실제 여러 안내 페이지를 끝까지 누른 뒤 잠금이 풀리는지 확인한다.
    for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
      const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
      if (button === null) break;
      await clickLabel(page, button);
    }
    await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
    const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
    await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 3, started, { timeout: 15000 });
    expect(errors).toEqual([]);
  });
}

// 첫 일반 캠페인 미션도 실제 목록에서 열고 World를 진행한다.
test('캠페인 목록에서 TheWarBegins 월드와 안내를 표시한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/debug.html');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  expect(label).toBeTruthy(); await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  expect(await page.evaluate(() => window.netstorm._clientGuide.current().script.name.toLowerCase())).toBe('thewarbegins');
  // 원본 브리핑 페이지를 닫고 실시간 월드 진행을 관찰한다.
  for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (button === null) break;
    await clickLabel(page, button);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
  await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 12, started, { timeout: 30000 });
  expect(errors).toEqual([]);
});

// 선행 캠페인 완료 뒤 두 번째 미션의 실제 목록·World를 진행한다.
test('캠페인 목록에서 MasterOfWhirligigs 월드와 안내를 표시한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTheWarBegins: '1' } }));
  });
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/debug.html');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'MasterOfWhirligigs')?.label);
  expect(label).toBeTruthy(); await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  expect(await page.evaluate(() => window.netstorm._clientGuide.current().script.name.toLowerCase())).toBe('masterofwhirligigs');
  // 원본 브리핑을 끝까지 닫는다.
  for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (button === null) break;
    await clickLabel(page, button);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
  await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 12, started, { timeout: 30000 });
  expect(errors).toEqual([]);
});

// 첫 캠페인의 후속 네 미션은 직전 완료 조건을 사용해 실제 목록에서 연다.
for (const [mission, previous] of [['SaveTheIsland', 'MasterOfWhirligigs'], ['FragileFortune', 'SaveTheIsland'], ['ThunderingPower', 'FragileFortune'], ['DissolvedAlliance', 'ThunderingPower']]) {
  test(`캠페인 목록에서 ${mission} 월드와 안내를 표시한다`, async ({ page }) => {
    const errors = []; page.on('pageerror', (error) => errors.push(error.message));
    await page.addInitScript((finished) => {
      localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
      localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { [`Done${finished}`]: '1' } }));
    }, previous);
    await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/debug.html');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
    // 실제 Tell 동작은 그대로 실행하면서 공격 경고 도달 여부만 관찰한다.
    if (mission === 'DissolvedAlliance') await page.evaluate(() => {
      const app = window.netstorm, open = app._openTell.bind(app); window.testTellNames = [];
      app._openTell = (name, ...args) => { window.testTellNames.push(name); return open(name, ...args); };
    });
    await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
    const label = await page.evaluate((name) => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === name)?.label, mission);
    expect(label).toBeTruthy(); await clickLabel(page, label);
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
    await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
    expect(await page.evaluate(() => window.netstorm._clientGuide.current().script.name.toLowerCase())).toBe(mission.toLowerCase());
    // 실제 브리핑의 버튼을 마지막까지 누른다.
    for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
      const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
      if (button === null) break;
      await clickLabel(page, button);
    }
    await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
    const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
    await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 6, started, { timeout: 20000 });
    if (mission === 'DissolvedAlliance') expect(await page.evaluate(() => window.testTellNames)).toContain('islandUnderAttack');
    expect(errors).toEqual([]);
  });
}

// 두 번째 캠페인의 미션은 직전 진행 값으로 원본 목록에서 잠금을 푼다.
for (const [mission, previous] of [['TheNoose', 'DissolvedAlliance'], ['RainVsRain', 'TheNoose'], ['RunForIt', 'RainVsRain'], ['GuardMyBack', 'RunForIt'], ['Surrounded', 'GuardMyBack']]) {
  test(`캠페인 목록에서 ${mission} 두 번째 묶음을 실행한다`, async ({ page }) => {
    const errors = []; page.on('pageerror', (error) => errors.push(error.message));
    await page.addInitScript((finished) => {
      localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
      localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { [`Done${finished}`]: '1' } }));
    }, previous);
    await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/debug.html');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
    await clickLabel(page, 'Campaign'); await clickLabel(page, 'A Nation Rises');
    const label = await page.evaluate((name) => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === name)?.label, mission);
    expect(label).toBeTruthy(); await clickLabel(page, label);
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
    await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
    expect(await page.evaluate(() => window.netstorm._clientGuide.current().script.name.toLowerCase())).toBe(mission.toLowerCase());
    // 실제 브리핑의 버튼을 마지막까지 누른다.
    for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
      const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
      if (button === null) break;
      await clickLabel(page, button);
    }
    await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
    const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
    await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 6, started, { timeout: 20000 });
    expect(errors).toEqual([]);
  });
}

// 마지막 캠페인의 미션은 직전 진행 값으로 원본 목록에서 잠금을 푼다.
for (const [mission, previous] of [['BreakingThrough', 'Surrounded'], ['ToTheRescue', 'BreakingThrough'], ['Vicious', 'ToTheRescue'], ['EnemyTerritory', 'Vicious'], ['FinalConfrontation', 'EnemyTerritory']]) {
  test(`캠페인 목록에서 ${mission} 마지막 묶음을 실행한다`, async ({ page }) => {
    const errors = []; page.on('pageerror', (error) => errors.push(error.message));
    await page.addInitScript((finished) => {
      localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
      localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { [`Done${finished}`]: '1' } }));
    }, previous);
    await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/debug.html');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
    await clickLabel(page, 'Campaign'); await clickLabel(page, 'Complete Victory');
    const label = await page.evaluate((name) => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === name)?.label, mission);
    expect(label).toBeTruthy(); await clickLabel(page, label);
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
    await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
    expect(await page.evaluate(() => window.netstorm._clientGuide.current().script.name.toLowerCase())).toBe(mission.toLowerCase());
    // 실제 브리핑의 버튼을 마지막까지 누른다.
    for (let pageIndex = 0; pageIndex < 8; pageIndex += 1) {
      const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
      if (button === null) break;
      await clickLabel(page, button);
    }
    await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
    const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
    await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 6, started, { timeout: 20000 });
    expect(errors).toEqual([]);
  });
}
