// 실제 미션의 Client Gump·Session Form 통지·가상 삭제를 통해 격자 부모 소실 보완을 검사한다.
const { test, expect } = require('@playwright/test');

// 세 수명 경로는 독립 미션에서 실행해 FIFO 할당과 UI 등록 상태를 공유하지 않는다.
for (const mode of ['close', 'reuse', 'reconnect']) {
  test(`생산 격자 부모 소실: 실제 Tutorial2 ${mode}`, async ({ page }) => {
    test.setTimeout(240000);
    const errors = [];
    page.on('pageerror', error => errors.push(error.stack ?? error.message));
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    await page.setViewportSize({ width: 1024, height: 768 });
    await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
    await page.goto('/debug.html?fixed=25&aspect=4:3&title=1&lang=en');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
    await page.locator('canvas').click(); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
    // 원래 타이틀·메뉴·로그인 프레임을 통과하며 목표 수행이나 승패 상태는 변경하지 않는다.
    while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
    await page.evaluate(() => { const app = window.netstorm; app.stepFixedFrames(1); app._beginCampaignFromDialog('Tutorial2'); });
    await page.evaluate(() => window.netstorm.stepFixedFrames(120));
    const result = await page.evaluate(async selected => {
      const { ClientInventoryGrid } = await import('./ui/client-inventory-grid.js');
      const app = window.netstorm, root = app._clientGump, session = app._campaignWorld.session(), world = session.world(), pool = session.core().pool;
      if (!session.localConnectionReady()) throw new Error('실제 Session 준비 미완료');
      const grid = new ClientInventoryGrid(root, session.productionContext(), { cursorPoint: { x: 0, y: 0 }, cursorFlags: 0 }, {
        now: () => app._gameClock.wallSeconds(), baseHandle: () => 0, allocateItem() { throw new Error('빈 검사 부모의 항목 생성'); },
      });
      // 검사 격자와 빈 TYPE80 부모만 추가한다. 원래 미션의 생산 패널·기술 부모는 유지한다.
      const token = grid.create(2, 2, 0x40), node = root.node(token), parent = world.create(80, 2);
      node.baseWidth = node.width = 40; node.baseHeight = node.height = 40; root.attach(token, 200, 200); root.bindObject(token, parent);
      const oldForm = node.form; world.destroy(parent, 0);
      const lost = { flags: node.flags & 0x40, object: node.object, form: node.form, free: pool.slot(oldForm)[11] & 1 };
      let reused = null, newParent = 0, newForm = 0;
      if (selected === 'reuse') {
        // 실제 Client FIFO 풀에서 옛 Form 번호가 다른 자산에 배정될 때까지 할당을 보유한다.
        for (let count = 0; count < pool.capacity; count++) { const sid = world.create(80, 2); if (sid === oldForm) { reused = pool.slot(sid).slice(); break; } }
        if (!reused) throw new Error('실제 FIFO Form 번호 재사용 미도달');
      }
      if (selected === 'reconnect') { newParent = world.create(80, 2); root.bindObject(token, newParent); newForm = node.form; }
      const newParentMatches = !newForm || session.gumpFormHost().parent(newForm) === newParent;
      root.remove(token);
      return { parent, lost, removed: root.node(token) === null, newParentMatches, newFormFree: !newForm || Boolean(pool.slot(newForm)[11] & 1), reusedPreserved: !reused || reused.every((byte, index) => byte === pool.slot(oldForm)[index]) };
    }, mode);
    expect(result.lost).toEqual({ flags: 0, object: result.parent, form: 0, free: 1 });
    expect(result.removed).toBe(true); expect(result.newParentMatches).toBe(true); expect(result.newFormFree).toBe(true); expect(result.reusedPreserved).toBe(true);
    await page.evaluate(() => window.netstorm.stepFixedFrames(10)); expect(errors).toEqual([]);
    await page.screenshot({ path: test.info().outputPath(`grid-parent-${mode}.png`) });
  });
}
