// 원본 실행에서 확인한 스톰 파워 표시/생산 창의 실제 우클릭과 HELP 플래그 수명을 검사한다. (WB04-E1324)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs'), path = require('node:path');
// 원본10.78 실제 화면과 관찰 기록이다. 웹 결과로 기대 화면을 생성하지 않는다.
const ROOT = path.join(__dirname, '..', 'tests', 'fixtures', 'original-budget-help');
const ORIGINAL = JSON.parse(fs.readFileSync(path.join(ROOT, 'observation.json'), 'utf8'));

test('스톰 파워 실제 우클릭은 원본처럼 About 팝업을 열고 도움말 빈 값의 플래그도 보존한다', async ({ page }) => {
  test.setTimeout(180000); const errors = [];
  page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 시작 그림과 실제 미션 안내 단추를 같은 고정 시계로 진행한다.
  while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1)); await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1')); await page.evaluate(() => window.netstorm.stepFixedFrames(120));
  for (let index = 0; index < 4; index += 1) {
    const point = await page.evaluate(() => window.netstorm._mainMenu.controlPoint('MORE') ?? window.netstorm._mainMenu.controlPoint('OK'));
    if (!point) break; await page.mouse.click(point.x, point.y); await page.evaluate(() => window.netstorm.stepFixedFrames(5));
  }
  expect(await page.evaluate(() => Boolean(window.netstorm._mainMenu.dialog))).toBe(false);
  expect(ORIGINAL.exeSha256).toBe('a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236');
  const { readPng } = await import('../tools/image-files.mjs');
  // 두 원본 관찰을 같은 실제 좌표·제목·주제의 앱 입력에 대응한다.
  for (const [name, file] of [['budget', 'budget.png'], ['production', 'production.png']]) {
    const observed = ORIGINAL[name], native = readPng(fs.readFileSync(path.join(ROOT, file)));
    expect([native.width, native.height]).toEqual([1024, 768]);
    await page.mouse.move(...observed.point); await page.mouse.click(...observed.point, { button: 'right', delay: 60 }); await page.evaluate(() => window.netstorm.stepFixedFrames(20));
    const result = await page.evaluate(() => {
      const app = window.netstorm, root = app._clientGump, ui = app._campaignProduction.ui;
      // 실제 열린 메뉴의 F1Help 명령 인자와 표시 항목을 읽는다.
      for (const token of root.createState().entries) {
        const menu = token && root.node(token); if (!menu || !(menu.flags & 0x4000) || (menu.flags & 0x10000)) continue;
        const item = menu.items?.find((entry) => entry.command === 0x02000017);
        if (item) {
          let label = ''; const fragments = app._clientChatWindows.uiOwner.text.styleState().fragments;
          // 표시 조각의 실제 문자열을 순서대로 모은다(메뉴의 고정 label 칸은 스타일 항목이면 빈 값이다).
          for (let head = item.styled; head;) { const fragment = fragments.get(head); label += String.fromCharCode(...fragment.text).split('\0')[0]; head = fragment.next; }
          return { popup: root.blockingPopups(), topic: app._clientChatWindows.strings.get(item.parameter), label, flags: root.node(ui.node().budget).flags };
        }
      }
      return { popup: root.blockingPopups() };
    });
    expect(result.popup, name).toBe(true); expect(result.topic, name).toBe(`${observed.topic}Help`); expect(result.label, name).toBe(observed.item); expect(result.flags & 0x1200).toBe(0x1200);
    await page.screenshot({ path: test.info().outputPath(`${name}.png`) });
    await page.evaluate(() => window.netstorm._closeCampaignPopups()); await page.evaluate(() => window.netstorm.stepFixedFrames(2));
  }
  const flags = await page.evaluate(() => {
    const app = window.netstorm, token = app._campaignProduction.ui.node().budget, node = app._clientGump.node(token), values = [];
    app._setNodeHelp(token, '', true); values.push([node.flags & 0x1200, node.alternateTextPointer]);
    app._setNodeHelp(token, '', false); values.push([node.flags & 0x1200, node.textPointer]);
    app._setNodeHelp(token, 'Storm Power Available', false); app._setNodeHelp(token, 'moneyGump', true); values.push([node.flags & 0x1200, Boolean(node.textPointer && node.alternateTextPointer)]); return values;
  });
  expect(flags).toEqual([[0x1200, 0], [0x200, 0], [0x1200, true]]);
  // C 문자열의 첫 NUL은 문구와 주제 모두를 끝내며 기존 문자열 토큰도 반납한다(C++1363).
  const terminated = await page.evaluate(() => {
    const app = window.netstorm, token = app._campaignProduction.ui.node().budget, node = app._clientGump.node(token);
    app._setNodeHelp(token, 'Budget\0hidden', false); app._setNodeHelp(token, 'moneyGump\0hidden', true);
    // 실제 노드가 소유한 문자열과 뒤이은 빈 값 교체 전 토큰을 읽는다.
    const text = app._helpString(node.textPointer), word = app._helpString(node.alternateTextPointer), oldText = node.textPointer, oldWord = node.alternateTextPointer;
    app._setNodeHelp(token, '\0hidden', false); app._setNodeHelp(token, '\0hidden', true);
    return { text, word, flags: node.flags & 0x1200, textPointer: node.textPointer, wordPointer: node.alternateTextPointer, released: !app._helpStrings.has(oldText) && !app._helpStrings.has(oldWord) };
  });
  expect(terminated).toEqual({ text: 'Budget', word: 'moneyGump', flags: 0x200, textPointer: 0, wordPointer: 0, released: true });
  expect(errors).toEqual([]);
});
