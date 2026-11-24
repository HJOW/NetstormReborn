// 실제 미션 Start·보상 서비스가 같은 현재 예산 노드의 원본 재부착을 사용하는지 검사한다. Login 수신 결선은 같은 이름의 Node 통합 검사가 담당한다. (WB04-E1325)
const { test, expect } = require('@playwright/test');

test('실제 미션의 SP 갱신은 같은 예산 노드를 Push/Attach하고 금액·스타일·깊이를 유지한다', async ({ page }) => {
  test.setTimeout(180000); const errors = [];
  page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 실제 Start에 앞서 현재 Client 노드 재부착 호출만 관찰한다. 반환/화면/게임 상태는 바꾸지 않는다.
  while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => {
    const app = window.netstorm, root = app._clientGump, refresh = root.refreshStormPower.bind(root); window.__spRefreshCalls = [];
    root.refreshStormPower = (serial) => { window.__spRefreshCalls.push(serial); return refresh(serial); };
  });
  await page.evaluate(() => window.netstorm.stepFixedFrames(1)); await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1')); await page.evaluate(() => window.netstorm.stepFixedFrames(120));
  for (let index = 0; index < 4; index += 1) {
    const point = await page.evaluate(() => window.netstorm._mainMenu.controlPoint('MORE') ?? window.netstorm._mainMenu.controlPoint('OK'));
    if (!point) break; await page.mouse.click(point.x, point.y); await page.evaluate(() => window.netstorm.stepFixedFrames(5));
  }
  const result = await page.evaluate(() => {
    const app = window.netstorm, root = app._clientGump, token = app._campaignProduction.ui.node().budget, node = root.node(token), tree = root.tree();
    const before = [node.x, node.y, node.parent, node.depth, node.head, app._campaignWorld.session().sp(1), root.createState().count], operations = [];
    const push = tree.push.bind(tree), attach = root.attach.bind(root), start = window.__spRefreshCalls.includes(node.serial);
    tree.push = (current) => { if (current === token) operations.push(['push', current]); return push(current); };
    root.attach = (...args) => { if (args[0] === token) operations.push(['attach', ...args]); return attach(...args); };
    try {
      // 실제 앱이 Session 생성에 넘기는 보상 서비스는 같은 Client helper를 조회한다.
      app._campaignGameplayServices().refreshSp();
      return { start, before, after: [node.x, node.y, node.parent, node.depth, node.head, app._campaignWorld.session().sp(1), root.createState().count], operations,
        sameNode: root.node(token) === node, token, serial: node.serial, calls: window.__spRefreshCalls.slice(-1) };
    } finally { tree.push = push; root.attach = attach; }
  });
  expect(result.start).toBe(true); expect(result.sameNode).toBe(true); expect(result.after).toEqual(result.before);
  expect(result.calls).toEqual([result.serial]);
  expect(result.operations).toEqual([['push', result.token], ['attach', result.token, result.before[0], result.before[1], 0, 0]]);
  expect(errors).toEqual([]);
});
