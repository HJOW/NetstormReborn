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
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/');
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
  expect(state.frame).toBeGreaterThan(0); expect(state.paused).toBe(true); expect(state.width).toBe(1366); expect(state.visiblePixels).toBe(true);
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
  expect(errors).toEqual([]);
});
