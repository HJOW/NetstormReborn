// 실제 미션 화면의 좌측 사이드바(Uber HUD 틀)가 원본 위치·크기로 도킹되고 화면 크기 변경에도 유지되는지 확인한다. (2026-11-20 WB04-E1082)
const fs = require('node:fs');
const path = require('node:path');
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// Tutorial1 진입 → 안내 닫기 → 게임 시계 시작까지 진행한다. url의 화면비 지정(예: ?aspect=4:3)에 따라 논리 화면이 정해진다.
async function startTutorial(page, size, url) {
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize(size); await page.goto(url);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  expect(first).toBeTruthy(); await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  for (let index = 0; index < 8; index += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (label === null) break; await clickLabel(page, label);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused() && window.netstorm._campaignProduction?.hud?.token, null, { timeout: 20000 });
}

// 현재 HUD 자식의 기본 위치·크기(x, y, 폭, 높이)와 도킹된 지도 영역을 읽는다.
const readHud = (page) => page.evaluate(() => {
  const app = window.netstorm, hud = app._campaignProduction.hud, root = app._clientGump, node = hud.node(), box = (token) => { const item = root.node(token); return [item.x, item.y, item.baseWidth, item.baseHeight]; };
  return { layout: [app._layout.logicalWidth, app._layout.logicalHeight], map: hud.report().map, vertical: box(node.vertical), minimapDivider: box(node.minimapDivider), chatDivider: box(node.chatDivider), minimap: box(node.minimap), production: box(node.production),
    knowledge: node.knowledge, chat: node.chat, rootIsHud: root.createState().entries[0] === hud.token };
});

test('4:3 Tutorial1의 좌측 HUD 틀은 원본 위치로 도킹되고 하단 미니맵 칸이 원본 스크린샷과 일치한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await startTutorial(page, { width: 1024, height: 768 }, '/?aspect=4:3');
  const hud = await readHud(page);
  expect(hud).toEqual({ layout: [1024, 768], map: { left: 85, top: 0, right: 1024, bottom: 768 }, vertical: [80, 0, 5, 768], minimapDivider: [0, 688, 80, 5], chatDivider: [85, 768, 939, 5],
    minimap: [0, 693, 80, 75], production: [0, 0, 80, 688], knowledge: 0, chat: hud.chat, rootIsHud: true });
  expect(hud.chat, '채팅 창 root(E1169 이후 실제 창)').toBeGreaterThan(0);
  // 원본 10.78 스크린샷(게임 영역은 오프셋 4,32)의 하단 구분선·세로 구분선 아랫부분·미니맵 칸 85×82 픽셀과 모두 같아야 한다.
  const png = fs.readFileSync(path.join(__dirname, '..', '..', 'screenShots', 'Bridge the Gap - Started.png')).toString('base64');
  await page.waitForTimeout(500);
  const mismatched = await page.evaluate(async (base64) => {
    const bitmap = await createImageBitmap(await (await fetch(`data:image/png;base64,${base64}`)).blob());
    const canvas = new OffscreenCanvas(bitmap.width, bitmap.height), context = canvas.getContext('2d'); context.drawImage(bitmap, 0, 0);
    const original = context.getImageData(4, 32 + 686, 85, 82).data, ours = window.netstorm.canvas.getContext('2d').getImageData(0, 686, 85, 82).data;
    let count = 0; for (let at = 0; at < original.length; at += 4) if (original[at] !== ours[at] || original[at + 1] !== ours[at + 1] || original[at + 2] !== ours[at + 2]) count += 1;
    return count;
  }, png);
  expect(mismatched).toBe(0); expect(errors).toEqual([]);
});

test('미션 중 창 크기를 16:9·16:10·대형으로 바꿔도 HUD 틀이 같은 도킹을 유지한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await startTutorial(page, { width: 1024, height: 768 }, '/');
  for (const [size, layout] of [[{ width: 1366, height: 768 }, [1366, 768]], [{ width: 1280, height: 800 }, [1228, 768]], [{ width: 1920, height: 1080 }, [1920, 1080]], [{ width: 1024, height: 768 }, [1024, 768]]]) {
    await page.setViewportSize(size); await page.waitForFunction(([width, height]) => window.netstorm._layout.logicalWidth === width && window.netstorm._layout.logicalHeight === height, layout);
    await page.waitForTimeout(300);
    const [width, height] = layout, hud = await readHud(page);
    expect(hud.layout).toEqual(layout); expect(hud.map).toEqual({ left: 85, top: 0, right: width, bottom: height });
    expect(hud.vertical).toEqual([80, 0, 5, height]); expect(hud.minimapDivider).toEqual([0, height - 80, 80, 5]); expect(hud.chatDivider[1]).toBe(height);
    expect(hud.minimap).toEqual([0, height - 75, 80, 75]); expect(hud.production).toEqual([0, 0, 80, height - 80]);
  }
  expect(errors).toEqual([]);
});
