// 한국어 모드의 미션 화면에서 ESC 메뉴 막대·Game/View/Options 항목과 Shift-F9 정지 안내가 한국어로 나오는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1118)
const { test, expect } = require('@playwright/test');

// 공통 Tell의 실제 글꼴/줄폭을 검사할 표시 창 목록이다. HUD 전용 Warning은 별도 Node 검사에서 다룬다.
const COMMON_DIALOGS = ['ABORT', 'TryAgain', 'DenySalvage', 'BuildNoBucks', 'NoTechTheme', 'UpgradeNoBucks', 'SalvageVortex', 'SalvageInf', 'NewTech', 'AltarUpgrade',
  'AlreadyUpgrading', 'AskSend', 'SacrificeForStormpower', 'WarnCantUpgradeAltarOffHomeIsland', 'EarlySacrifice', 'AllianceRequested', 'AllianceSpam', 'AllianceComplete',
  'TellGifWritten', 'NewRank', 'ProdPending', 'BridgesKilled', 'NotImplemented'];

// 공통 대화창은 목표 달성·희생을 자동 수행하지 않고 실제 표시 입구만 호출해 확인한다.
test('한국어 공통 안내 23개가 실제 글꼴로 번역되고 화면 안에 배치된다', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  const rows = [];
  // 최소 화면과 기본 화면에서 같은 표시 입구와 실제 글꼴로 검사한다.
  for (const size of [{ width: 640, height: 480 }, { width: 1024, height: 768 }]) {
    await page.setViewportSize(size);
    // 4:3의 논리 화면은 1024×768이며 640×480에서는 같은 화면을 축소 표시한다.
    await page.waitForFunction((width) => window.netstorm.canvas.getBoundingClientRect().width === width, size.width);
    for (const name of COMMON_DIALOGS) {
      const row = await page.evaluate((section) => {
        const app = window.netstorm;
        // 공통 창을 단독으로 열 때도 미션이 공급하는 치환값을 명시하여 미해결 키 문자열의 폭을 재지 않는다.
        app._gameData.set('mission.title', '전쟁의 시작!'); app._gameData.set('global.newTechName', '썬 디스크 발사기'); app._gameData.set('global.newTechPic', '');
        app._gameData.set('InstallDir', ''); app._gameData.set('GifOutSpec', 'pic0.gif');
        const targets = { AllianceRequested: 'AllianceRequested(~b,간수,2)', AllianceSpam: 'AllianceSpam(3,~b,간수)', AllianceComplete: 'AllianceComplete(간수)', AskSend: 'AskSend(2)', NewRank: 'NewRank(2)' };
        app._openTell('Blank'); app._openTell(targets[section] ?? section); const dialog = app._mainMenu.dialog;
        if (!dialog) return { section, missing: true };
        // 줄끝 공백을 빼고 마지막 실제 글자까지의 폭을 측정한다.
        const lines = dialog.body.softLines.map((line) => {
          let advance = 0, visible = 0;
          for (const fragment of line.fragments) {
            const full = fragment.picture ? { width: fragment.picture.width ?? 0 } : dialog.body.fonts.measure(fragment.bytes, fragment.context);
            let end = fragment.bytes?.length ?? 0; while (end > 0 && fragment.bytes[end - 1] === 32) end -= 1;
            const content = fragment.picture ? full : dialog.body.fonts.measure(fragment.bytes.subarray(0, end), fragment.context);
            if (fragment.picture || end > 0) visible = Math.max(visible, advance + content.width); advance += full.width;
          }
          return visible;
        });
        return { section, bounds: dialog.bounds(), width: app._layout.logicalWidth, height: app._layout.logicalHeight,
          maxLine: Math.max(0, ...lines), bodyWidth: dialog.body.baseWidth, labels: dialog.buttons.map((button) => button.label), text: dialog.content.styleText };
      }, name);
      rows.push(row); expect(row.missing, name).not.toBe(true);
      expect(row.text, name).toMatch(/[가-힣]/u);
      expect(row.labels.every((label) => /[가-힣]/u.test(label)), `${name} 단추`).toBe(true);
      expect(row.maxLine, `${name} 실제 글자 줄폭`).toBeLessThanOrEqual(row.bodyWidth);
      expect(row.bounds.left, name).toBeGreaterThanOrEqual(0); expect(row.bounds.top, name).toBeGreaterThanOrEqual(0);
      expect(row.bounds.right, name).toBeLessThanOrEqual(row.width); expect(row.bounds.bottom, name).toBeLessThanOrEqual(row.height);
      if (['ABORT', 'SalvageVortex', 'BuildNoBucks'].includes(name)) await page.screenshot({ path: test.info().outputPath(`${size.width}-${name}.png`) });
    }
  }
  await test.info().attach('common-dialogs.json', { body: JSON.stringify(rows, null, 2), contentType: 'application/json' });
  expect(errors).toEqual([]);
});

// 실제 게임 조작부의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}
// 메뉴 작성기에 쓴 항목 라벨이다.
const labelsOf = (page, token) => page.evaluate((current) => window.netstorm._campaignProduction.ui.gameMenu().labels(current), token);
// 메뉴 항목 한 줄의 화면 좌표와 자식 메뉴 토큰이다(가로 메뉴는 항목 폭×번호, 세로 메뉴는 저장된 offset).
const rowPoint = (page, token, index) => page.evaluate(({ current, row }) => {
  const app = window.netstorm, root = app._clientGump, node = root.node(current), bounds = root.collection().bounds(current), rect = app.canvas.getBoundingClientRect();
  const horizontal = (node.menuFlags & 0x40000) !== 0, localX = horizontal ? node.itemWidth * (row + 0.5) : 20, localY = horizontal ? (bounds.bottom - bounds.top) / 2 : node.offsets[row] + node.heights[row] / 2;
  return { child: node.items[row].childId ? root.createState().entries[node.items[row].childId] : 0, x: rect.left + (bounds.left + localX) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + localY) * rect.height / app._layout.logicalHeight };
}, { current: token, row: index });
// 한 줄을 눌러 하위 메뉴가 있으면 완전히 펼쳐질 때까지 기다리고 자식 토큰을 돌려준다.
async function clickRow(page, token, index) {
  const point = await rowPoint(page, token, index); await page.mouse.move(point.x, point.y); await page.mouse.click(point.x, point.y, { delay: 60 });
  if (point.child) await page.waitForFunction((child) => { const node = window.netstorm._clientGump.node(child); return node && (node.flags & 1) && !node.expanding; }, point.child, { timeout: 10000 });
  return point.child;
}

test('한국어 미션 화면: ESC 막대·Game/View/Options 항목과 정지 안내가 한국어이다', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?lang=ko&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, '캠페인'); await clickLabel(page, '자유를 위한 투쟁');
  const mission = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, mission);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  // 시작 직후 진행 창 문구가 한국어였는지 확인할 수는 없으므로(이미 지나감) 번역 장치에서 직접 확인한다.
  expect(await page.evaluate(() => window.netstorm._resources.translate('Starting Mission...'))).toBe('미션 시작 중...');
  await clickLabel(page, '미션 시작'); await page.waitForFunction(() => window.netstorm._gameClock.gameSeconds() > 1 && window.netstorm._campaignProduction?.ui?.warnings(), null, { timeout: 30000 });
  // 막대를 열어 라벨을 읽는다.
  await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(400);
  const bar = await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token()), top = await labelsOf(page, bar);
  expect(top).toEqual(expect.arrayContaining(['게임', '보기', '옵션', '플레이어', '정보']));
  // Game 하위: 미션 목표·다시 시작·나가기가 한국어이다.
  const game = await clickRow(page, bar, top.indexOf('게임')), gameLabels = await labelsOf(page, game);
  expect(gameLabels.join('|')).toContain('미션 다시 시작'); expect(gameLabels.join('|')).toContain('미션 나가기');
  await page.screenshot({ path: test.info().outputPath('korean-ingame-game.png') });
  // View 하위: 단축키 표기(~iF4~)를 보존한 한국어 항목이 있다.
  const view = await clickRow(page, bar, top.indexOf('보기')), viewLabels = await labelsOf(page, view);
  expect(viewLabels.join('|')).toMatch(/건물 숨기기/u); expect(viewLabels.join('|')).toMatch(/플레이어 목록 보기/u);
  await page.screenshot({ path: test.info().outputPath('korean-ingame-view.png') });
  // 막대를 닫고 Shift-F9 로 정지하면 오른쪽 위 안내가 한국어이다.
  await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(300);
  await page.keyboard.press('Shift+F9'); await page.waitForFunction(() => window.netstorm._gameClock.paused(), null, { timeout: 10000 }); await page.waitForTimeout(400);
  const report = await page.evaluate(() => window.netstorm._campaignProduction.ui.warnings().report());
  expect(report).toContain('게임 일시 정지!');
  await page.screenshot({ path: test.info().outputPath('korean-ingame-paused.png') });
  await page.keyboard.press('Shift+F9'); await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 10000 });
  // 실제 ESC → 게임 → 미션 나가기에서 공통 Tell도 한국어이며 계속/나가기 명령이 유지된다.
  await page.keyboard.press('Escape');
  await page.waitForFunction(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen());
  let token = await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token());
  let leaveGame = await clickRow(page, token, 0);
  await clickRow(page, leaveGame, (await labelsOf(page, leaveGame)).indexOf('미션 나가기'));
  await expect.poll(() => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.map((button) => button.label))).toEqual(['메인 메뉴', '미션 다시 시작', '미션 계속']);
  // 명시적 줄바꿈은 표시 문장을 바꾸지 않으므로 공백을 정규화하여 본문을 비교한다.
  expect((await page.evaluate(() => window.netstorm._mainMenu.dialog.content.styleText)).replace(/\s+/gu, ' ')).toContain('메인 메뉴로 돌아가시겠습니까?');
  await page.screenshot({ path: test.info().outputPath('korean-leave-mission.png') });
  await clickLabel(page, '미션 계속');
  await page.waitForFunction(() => !window.netstorm._mainMenu.dialog && window.netstorm._campaignDisplayActive());
  await page.keyboard.press('Escape');
  await page.waitForFunction(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen());
  token = await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token());
  leaveGame = await clickRow(page, token, 0);
  await clickRow(page, leaveGame, (await labelsOf(page, leaveGame)).indexOf('미션 나가기'));
  await clickLabel(page, '메인 메뉴');
  await page.waitForFunction(() => !window.netstorm._campaignWorld && window.netstorm.state === 'mainMenu');
  expect(errors).toEqual([]);
});

// 한국어 미션 화면의 HUD 우클릭 도움말: 스톰 파워·생산 창·미니맵 제목이 한국어이며 채팅 채널 이름도 번역표로 번역된다.
test('한국어 미션 화면: 스톰 파워·생산 창 우클릭 도움말 제목과 채팅 채널 이름이 한국어이다', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?lang=ko&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, '캠페인'); await clickLabel(page, '자유를 위한 투쟁');
  const mission = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, mission);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  await clickLabel(page, '미션 시작'); await page.waitForFunction(() => window.netstorm._gameClock.gameSeconds() > 1 && window.netstorm._campaignProduction?.ui?.warnings(), null, { timeout: 30000 });
  // 노드의 도움말 제목(텍스트 포인터의 문자열)과 같은 노드를 우클릭했을 때 팝업이 열리는지 본다.
  const titles = await page.evaluate(() => {
    const app = window.netstorm, root = app._clientGump, production = app._campaignProduction, n = production.ui.node(), out = {};
    for (const [name, token] of [['minimap', production.minimap.token], ['budget', n.budget], ['bridge', n.bridge], ['middle', n.middle], ['bottom', n.bottom]]) {
      const node = root.node(token), position = root.collection().position(token); root.closePopups();
      out[name] = { title: app._helpString(node.textPointer), word: app._helpString(node.alternateTextPointer), handled: root.handle(token, 0x00020000, position.x + 5, position.y + 5), popup: root.blockingPopups() };
    }
    root.closePopups(); return out;
  });
  expect(titles.budget.title).toBe('사용 가능한 스톰 파워'); expect(titles.budget.word).toBe('moneyGump');
  for (const name of ['bridge', 'middle', 'bottom']) { expect(titles[name].title, `${name} 제목`).toBe('생산 창'); expect(titles[name].word, `${name} 주제`).toBe('teleportView'); }
  // 원본 본체의 고정 제목을 웹 조립부에서 번역하며 주제 이름은 그대로다.
  expect(titles.minimap.title).toBe('하늘 전체 보기'); expect(titles.minimap.word).toBe('minimapGump');
  for (const name of Object.keys(titles)) { expect(titles[name].handled, `${name} 우클릭 처리`).toBe(1); expect(titles[name].popup, `${name} 팝업`).toBe(true); }
  // 채팅 채널 이름(원본이 입력과 비교하는 여덟 키 가운데 번역 항목이 있는 것)은 번역표로 번역된다.
  const channels = await page.evaluate(() => ['BATTLE', 'EVERYONE', 'ALLIES', 'OBSERVER'].map((key) => window.netstorm._resources.translate(key)));
  expect(channels).toEqual(['전투', '모두', '동맹', 'OBSERVER']);
  // 생산 창(격자)을 실제 마우스로 우클릭하면 도움말 팝업이 열린다.
  const spot = await page.evaluate(() => { const app = window.netstorm, root = app._clientGump, token = app._campaignProduction.ui.node().bridge, position = root.collection().position(token), node = root.node(token), rect = app.canvas.getBoundingClientRect();
    root.closePopups(); return { x: rect.left + (position.x + node.width / 2) * rect.width / app._layout.logicalWidth, y: rect.top + (position.y + node.height / 2) * rect.height / app._layout.logicalHeight }; });
  await page.mouse.move(spot.x, spot.y); await page.mouse.click(spot.x, spot.y, { button: 'right', delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGump.blockingPopups()), { timeout: 5000, message: '실제 우클릭 도움말 팝업' }).toBe(true);
  await page.screenshot({ path: test.info().outputPath('korean-hud-help-popup.png') });
  expect(errors).toEqual([]);
});

// 브라우저가 실제로 받는 한국어 번역표(www/data/d/xlat.korean + manifest)에 미션 화면 문구 15개가 들어 있는지 확인한다: 커서 배치 경고·생산 힌트·객체 정보 줄·Players 메뉴 Send 줄·도움말 없음 안내. (2026-11-23 WB04-E1249)
test('한국어 번역표: 커서 경고·생산 힌트·객체 정보·Send 줄·도움말 없음 안내가 번역된다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?lang=ko&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  const keys = ["Priest can't get here.", 'Must be placed over constructing artifact', 'Cannot place here', 'Energy not satisfied', 'Only on Controlled Island',
    '~0~w~ENot enough Storm Power to build this', '~0~w~ENone in inventory', '~yCapturable!~.\n', '~yNot Connected!~.\n', 'No help information available',
    'Send %s300~[IfortGump.3]', 'Send %s700~[IfortGump.3]', 'Send %s1000~[IfortGump.3]', 'Send %s X~[IfortGump.3]', 'Send %s All~[IfortGump.3]'];
  const result = await page.evaluate((list) => list.map((key) => window.netstorm._resources.translate(key)), keys);
  expect(result).toEqual(['사제가 이곳에 올 수 없습니다.', '건설 중인 아티팩트 위에 배치해야 합니다', '여기에는 배치할 수 없습니다', '에너지 조건을 만족하지 않습니다', '통제 중인 섬에서만 가능합니다',
    '~0~w~E이것을 건설할 스톰 파워가 부족합니다', '~0~w~E보유한 것이 없습니다', '~y포획 가능!~.\n', '~y연결되지 않음!~.\n', '사용할 수 있는 도움말 정보가 없습니다',
    '%s300~[IfortGump.3] 보내기', '%s700~[IfortGump.3] 보내기', '%s1000~[IfortGump.3] 보내기', '%s X~[IfortGump.3] 보내기', '%s 전부~[IfortGump.3] 보내기']);
  expect(errors).toEqual([]);
});

// 한국어 미션 화면에서 채팅 도우미 `>` 를 눌러 Chat Helper 메뉴를 열면, 원본 ChatWindow::HelperMenu 가 번역 호출로 받는 고정 라벨이 모두 한국어로 번역되는지 실제 번역 요청을 기록해 확인한다. (2026-11-23 WB04-E1250)
// `Colors 0 to 21`·`BGColors …`·`Icons …` 는 원본이 문자열을 이어 붙여 번역에 넘기는 동적 범위 라벨이라 번역표 키가 될 수 없고 영어로 남는다(원본 동작).
test('한국어 채팅 도우미 메뉴: 고정 라벨이 한국어로 번역된다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?aspect=4:3&lang=ko');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, '캠페인'); await clickLabel(page, '자유를 위한 투쟁');
  const mission = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, mission);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  await clickLabel(page, '미션 시작'); await page.waitForTimeout(2000); await page.locator('canvas').focus();
  await page.keyboard.press('Enter'); await page.waitForTimeout(400);
  // 번역표 클래스의 translate 에 기록 장치를 끼운다(앱의 자원 객체는 동결돼 있어 같은 모듈의 클래스를 감싼다).
  await page.evaluate(async () => { const { XlatTable } = await import('/assets/xlat.js'); window.__tr = []; const original = XlatTable.prototype.translate; XlatTable.prototype.translate = function (key, ...rest) { const result = original.call(this, key, ...rest); window.__tr.push([key, result]); return result; }; });
  const helper = await page.evaluate(() => { const app = window.netstorm, b = app._campaignProduction.ui.chat().report().helper.bounds, rect = app.canvas.getBoundingClientRect(); return { x: rect.left + (b.left + b.right) / 2 * rect.width / app._layout.logicalWidth, y: rect.top + (b.top + b.bottom) / 2 * rect.height / app._layout.logicalHeight }; });
  await page.mouse.move(helper.x, helper.y); await page.mouse.down(); await page.waitForTimeout(100); await page.mouse.up(); await page.waitForTimeout(800);
  expect(await page.evaluate(() => window.netstorm._clientGump.blockingPopups()), 'Chat Helper 메뉴').toBe(true);
  const recorded = new Map(await page.evaluate(() => window.__tr));
  const expected = [['Chat Helper:', '채팅 도우미:'], ['Edit Back Format', 'Back 서식 편집'], ['Edit Front Format', 'Front 서식 편집'], ['Format', '서식'], ['Allow Chat Sound', '채팅 소리 허용'], ['Chat Effects ', '채팅 효과 '],
    ['Text Colors', '글자색'], ['Background Colors', '배경색'], ['Icons', '아이콘'], ['KeyWords', '키워드'], ['~BBold', '~B굵게'], ['~rRed', '~r빨강'], ['Styles', '스타일'], ['Prefabricated Colors', '미리 정한 색'], ['Commands', '명령'], ['Insert', '삽입'], ['Copy', '복사'], ['Paste', '붙여넣기'], ['Mimic', '흉내 내기'], ['Sound', '소리'], ['Icon', '아이콘'], ['Emboss Offset', '엠보스 위치'], ['Colors %d to %d', '색상 %d ~ %d'], ['BGColors %d to %d', '배경색 %d ~ %d'], ['Icons %d to %d', '아이콘 %d ~ %d']];
  for (const [key, text] of expected) expect(recorded.get(key), key).toBe(text);
  // 요청된 고정 라벨(범위 라벨 제외)에 영어로 남은 것이 없다.
  const english = [...recorded].filter(([key, text]) => key === text && /[A-Za-z]{3,}/u.test(key) && !/^(?:Colors|BGColors|Icons) \d+ to \d+$/u.test(key) && !key.startsWith('.'));
  expect(english, '번역되지 않은 고정 라벨').toEqual([]);
  await page.screenshot({ path: test.info().outputPath('korean-chat-helper-menu.png') });
  expect(errors).toEqual([]);
});
