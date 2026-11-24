// Early Mission의 실제 Cast 행·입력 큐·단독 요청/수신·주문 활성화·효과 정리를 검증한다. (WB04-E1318)
const { test, expect } = require('@playwright/test');
test.use({ locale: 'en-US' });

// 같은 고정 시각 앱으로 Tutorial3을 시작하고 실제 안내 단추를 눌러 조작 가능한 상태로 만든다.
async function startTutorial(page) {
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/debug.html?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click(); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  // 타이틀 그림 구간은 고정 시각으로만 넘긴다.
  while (await page.evaluate(() => window.netstorm.state !== 'mainMenu')) await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => { const app = window.netstorm; app.stepFixedFrames(1); app._beginCampaignFromDialog('Tutorial3'); app.stepFixedFrames(120); });
  // MORE/OK 단추는 실제 브라우저 클릭으로 닫고 관문 복귀 프레임도 진행한다.
  for (let index = 0; index < 10; index += 1) {
    const point = await page.evaluate(() => {
      const app = window.netstorm, p = app._mainMenu.controlPoint('MORE') ?? app._mainMenu.controlPoint('OK');
      if (!p) return null;
      const rect = app.canvas.getBoundingClientRect();
      return { x: rect.left + p.x * rect.width / app._layout.logicalWidth, y: rect.top + p.y * rect.height / app._layout.logicalHeight };
    });
    if (point) await page.mouse.click(point.x, point.y);
    await page.evaluate(() => window.netstorm.stepFixedFrames(5));
    if (await page.evaluate(() => window.netstorm._campaignDisplayActive() && !window.netstorm._gameClock.paused())) break;
  }
  const state = await page.evaluate(() => { const a = window.netstorm; return { active: a._campaignDisplayActive(), paused: a._gameClock.paused(), state: a.state, dialog: a._mainMenu.dialog?.title, popup: Boolean(a._mainMenu.popup), more: a._mainMenu.controlPoint('MORE'), ok: a._mainMenu.controlPoint('OK') }; });
  expect(state, JSON.stringify(state)).toMatchObject({ active: true, paused: false });
}

// 실제 열린 객체 메뉴의 Cast 행을 자식 노드까지 찾아 클릭 좌표·활성 비트를 돌려준다.
function castRow(page, target) {
  return page.evaluate((sid) => {
    const app = window.netstorm, root = app._clientGump, rect = app.canvas.getBoundingClientRect();
    // 메뉴 노드의 원본 항목 기하를 사용한다.
    const find = (token) => {
      const node = root.node(token);
      // 항목 순서대로 Cast 명령을 찾는다.
      for (let index = 0; index < (node.itemCount ?? 0); index += 1) if (node.items[index].command === 0x0300011a) {
        const bounds = root.collection().bounds(token);
        return { x: rect.left + (bounds.left + 12) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + node.offsets[index] + node.heights[index] / 2) * rect.height / app._layout.logicalHeight, flags: node.items[index].flags };
      }
      // 하위 메뉴도 같은 순서로 조회한다.
      for (let child = node.child; child; child = root.node(child).next) { const found = find(child); if (found) return found; }
      return null;
    };
    // 대상 객체가 소유한 최상위 메뉴만 선택한다.
    for (let token = root.root().child; token; token = root.node(token).next) {
      const node = root.node(token); if (node.command === 26 && node.object === sid) return find(token);
    }
    return null;
  }, target);
}

test('Tutorial3: 골렘의 Heal Cast 행이 실제 주문 효과와 정리까지 오류 없이 진행한다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startTutorial(page);
  // 실제 Session 생성·Pop·내용물 부착으로 한 주문을 가진 골렘을 준비한다. 승패·게이트·효과 상태는 주입하지 않는다.
  const setup = await page.evaluate(async () => {
    const { typeNumber } = await import('/assets/type-table.js');
    const app = window.netstorm, s = app._campaignWorld.session(), world = s.world(), c = s.core();
    const priest = world.objects().find(({ object }) => object.owner === 1 && object.type === 158).object;
    const golemType = app._gameAssets.typeTable.find('sunWalker'), caster = world.create(golemType, 0);
    world.setAssetOwner(caster, 1); world.pop(caster, priest.x, priest.y, 0);
    // 원문 Heal은 시전자 자신을 제외하므로 별도의 인접 골렘에 실제 피해 입력을 준비한다.
    const patient = world.create(golemType, 0); world.setAssetOwner(patient, 1); world.pop(patient, priest.x - 1, priest.y, 0);
    const maximum = s.hitPoints().maxHitPoints(patient), beforeHp = Math.max(1, Math.trunc(maximum / 4)), raw = c.pool.allocated(patient);
    new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(26, beforeHp, true);
    const content = s.createContained(caster, 116, 0, 1, 0, 0), created = [], create = world.create.bind(world);
    world.create = (type, flags) => { const sid = create(type, flags); if (type === 116) created.push(sid); return sid; };
    window.__castCreated = created; app._campaignWorld.finishKernelFrame();
    return { caster, patient, content, maximum, beforeHp, beforeSp: s.sp(1), cost: s.hitPoints().typeCostUnits(116), castTime: typeNumber(app._gameAssets.types()[116 - 70].definition, 'casttime') ?? 0,
      opened: app._campaignProduction.ui.openObject(caster, { code: 0, x: 400, y: 300 }) };
  });
  expect(setup.opened).toBe(1); const row = await castRow(page, setup.caster);
  expect(row, '주문 보유 운반체의 Cast 행').toBeTruthy(); expect(row.flags & 0x10000).toBeTruthy();
  await page.mouse.click(row.x, row.y); await page.evaluate(() => window.netstorm.stepFixedFrames(12));
  const active = await page.evaluate(() => { const s = window.netstorm._campaignWorld.session(); return { created: window.__castCreated.slice(), sp: s.sp(1) }; });
  expect(active.created).toHaveLength(1); expect(active.sp).toBe(setup.beforeSp - setup.cost);
  // 활성화 이후 원래 게임 시계·Regular로 시전과 효과를 끝낸다.
  const frames = Math.ceil((setup.castTime + 2) / 0.025);
  expect(frames).toBeLessThan(2000);
  await page.evaluate((count) => window.netstorm.stepFixedFrames(count), frames);
  expect(await page.evaluate((sid) => window.netstorm._campaignWorld.session().core().pool.slot(sid)[11] & 3, active.created[0])).toBe(3);
  const afterHp = await page.evaluate((sid) => { const raw = window.netstorm._campaignWorld.session().core().pool.slot(sid); return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(26, true); }, setup.patient);
  expect(afterHp).toBe(Math.trunc((setup.beforeHp + setup.maximum) / 2));
  expect(await page.evaluate(() => window.netstorm._clientInputQueue.size)).toBe(0);
  expect(errors).toEqual([]);
});
