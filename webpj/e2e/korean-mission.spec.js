// 실제 Chromium의 한국어 미션 메뉴·브리핑과 글꼴 대체 경로를 확인한다.
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

test('한국어 Tutorial1 미션 메뉴와 브리핑을 표시한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => { errors.push(error.message); console.error('브라우저 예외:', error.message); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, '캠페인');
  const group = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.map(({label,command}) => ({label,command})));
  const early = group.find((item) => item.label.includes('초기')); expect(early).toBeTruthy(); await clickLabel(page, early.label);
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  expect(first).toMatch(/[가-힣]/); await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  // 원본 줄 나눔은 끝 공백의 전진 폭을 포함하므로 실제 글자 바깥쪽만 비교한다.
  const brief = await page.evaluate(() => { const app = window.netstorm, d = app._mainMenu.dialog; return { language: app.options.language, unicode: Boolean(app._resources.fonts.unicode),
    style: d.content.styleText, width: d.body.width, height: d.body.height, lines: d.body.softLines.length, maxLine: Math.max(...d.body.softLines.map((line) => { const last = line.fragments.at(-1), bytes = last?.bytes; return line.width - (bytes?.at(-1) === 32 ? app._resources.fonts.measure(bytes.slice(-1), last.context).width : 0); })), screen: [app._surface.width, app._surface.height], current: app._gameData.currentTell };  });
  expect(brief.language).toBe('korean'); expect(brief.unicode).toBe(true); expect(brief.style).toContain('적 사제를 붙잡아');
  expect(brief.style).toContain('NetStorm'); expect(brief.lines).toBeGreaterThan(0); expect(brief.maxLine).toBeLessThanOrEqual(brief.width);
  await page.screenshot({ path: test.info().outputPath('korean-mission.png') });
  await clickLabel(page, '다음'); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'A1.');
  const next = await page.evaluate(() => { const app = window.netstorm, d = app._mainMenu.dialog; return { style: d.content.styleText, width: d.body.width, maxLine: Math.max(...d.body.softLines.map((line) => { const last = line.fragments.at(-1), bytes = last?.bytes; return line.width - (bytes?.at(-1) === 32 ? app._resources.fonts.measure(bytes.slice(-1), last.context).width : 0); })) }; });
  expect(next.style).toContain('화면 이동'); expect(next.style).toContain('F4'); expect(next.maxLine).toBeLessThanOrEqual(next.width);
  await page.screenshot({ path: test.info().outputPath('korean-mission-next.png') });
  // 실제 생산 항목과 한국어 도움말 항목도 같은 미션에서 누른다.
  await clickLabel(page, '확인'); await page.waitForFunction(() => !window.netstorm._gameClock.paused() && window.netstorm._clientGump.node(window.netstorm._campaignProduction.ui.node().bridge).child);
  const item = await page.evaluate(() => { const app = window.netstorm, root = app._clientGump, token = root.node(app._campaignProduction.ui.node().bridge).child, bounds = root.collection().bounds(token), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + (bounds.left + bounds.right) / 2 * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + bounds.bottom) / 2 * rect.height / app._layout.logicalHeight }; });
  await page.mouse.click(item.x, item.y, { button: 'right', delay: 60 });
  await page.waitForFunction(() => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26) return true; return false; });
  const menu = await page.evaluate(() => { const app = window.netstorm, root = app._clientGump, rect = app.canvas.getBoundingClientRect(); let token = root.root().child;
    while (token && root.node(token).command !== 26) token = root.node(token).next; const bounds = root.collection().bounds(token);
    return { help: app._clientLocal.helpWord().get(), bridge: app._resources.translate('Bridge'), owner: app._resources.translate('Owner'), x: rect.left + (bounds.left + 8) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + 5) * rect.height / app._layout.logicalHeight }; });
  expect(menu.help).toBe('정보'); expect(menu.bridge).toBe('다리'); expect(menu.owner).toBe('소유자'); await page.screenshot({ path: test.info().outputPath('korean-production-menu.png') });
  await page.mouse.move(menu.x, menu.y); await page.mouse.click(menu.x, menu.y, { delay: 60 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.topic)); await clickLabel(page, '확인');
  // 정보 태그가 있는 실제 TYPE은 현지화된 능력치와 그림을 표시한다.
  await page.evaluate(() => { const app = window.netstorm, type = app._gameAssets.find('thunderFactory').block + 70; app._clientInputQueue.pushCommand(0x02000017, 0, type); });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.information));
  const typeHelp = await page.evaluate(() => window.netstorm._mainMenu.dialog.informationFactory());
  expect(typeHelp).toContain('분류: <c>생산</c>'); expect(typeHelp).toContain('체력:'); expect(typeHelp).toContain('스톰 파워 비용:'); await page.screenshot({ path: test.info().outputPath('korean-type-help.png') });
  await clickLabel(page, '확인'); expect(errors).toEqual([]);
});

// 두 번째 튜토리얼의 한국어 안내 페이지를 실제 미션에서 확인한다.
test('한국어 Tutorial2의 안내 페이지를 표시한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => { errors.push(error.message); console.error('브라우저 예외:', error.message); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1' } }));
  });
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, '캠페인'); await clickLabel(page, '초기 미션');
  const second = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial2')?.label);
  expect(second).toMatch(/[가-힣]/); await clickLabel(page, second);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  const brief = await page.evaluate(() => window.netstorm._mainMenu.dialog.content.styleText);
  expect(brief).toContain('건물'); expect(brief).toContain('사제');
  await page.screenshot({ path: test.info().outputPath('korean-tutorial2.png') });
  await clickLabel(page, '다음'); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'A1.');
  const next = await page.evaluate(() => window.netstorm._mainMenu.dialog.content.styleText);
  expect(next).toContain('신전'); expect(next).toContain('워크샾');
  const pictures = await page.evaluate(() => window.netstorm._mainMenu.dialog.body.softLines.flatMap((line) => line.fragments).filter((fragment) => fragment.picture).map(({picture}) => ({ width: picture.width, height: picture.height })));
  expect(pictures).toHaveLength(2); expect(pictures.every((picture) => picture.width > 0 && picture.height > 0)).toBe(true);
  await page.screenshot({ path: test.info().outputPath('korean-tutorial2-next.png') });
  // 마지막 안내를 닫고 실제 사제 그림을 오른쪽 클릭한다.
  await clickLabel(page, '다음'); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'A2.'); await clickLabel(page, '확인');
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  const priest = await page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), sid = session.findPriestParticipation(session.localOwner());
    const slot = session.core().pool.slot(sid), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength), point = app._clientGameplay.camera().snapshot().project(raw.getFloat32(14, true), raw.getFloat32(18, true)), rect = app.canvas.getBoundingClientRect();
    return { sid, x: rect.left + (point.x - 16) * rect.width / app._layout.logicalWidth, y: rect.top + (point.y - 11) * rect.height / app._layout.logicalHeight }; });
  await page.mouse.click(priest.x, priest.y, { button: 'right', delay: 60 });
  await page.waitForFunction(() => { const app = window.netstorm, root = app._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object) return true; return false; }, null, { timeout: 10000 });
  const rootMenu = await page.evaluate(() => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object) return token; return 0; });
  expect(await page.evaluate((token) => window.netstorm._clientGump.node(token).object, rootMenu)).toBe(priest.sid);
  expect(await page.evaluate(() => [window.netstorm._resources.translate('%sConstruct~.'), window.netstorm._resources.translate('%sTemple')])).toEqual(['%s건설~.', '%s신전']);
  await page.screenshot({ path: test.info().outputPath('korean-priest-menu.png') });
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
  expect(preview).toEqual({ type: 123, costFlags: 0x160 }); expect(errors).toEqual([]);
});
