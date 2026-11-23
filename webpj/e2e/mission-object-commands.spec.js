// 객체 가상 6c 로 가는 명령(DetonateBomb·신전 WipeOut/Ascend/Descend·시나리오 객체 명령)이 입력 큐를 막지 않고 소비되는지, 처리하지 못하는 사건이 영구히 큐를 막지 않는지
// 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1150)
// 연결 전에는 메뉴의 Detonate!·Meltdown! 같은 항목이 만든 명령이 큐 머리에서 소비되지 않아 이후 모든 입력(선택·단축키)이 영구히 막혔다.
const { test, expect } = require('@playwright/test');

// 화면비를 환경 변수 NETSTORM_E2E_ASPECT('4:3' 기본, '16:9', '16:10')로 고른다.
const ASPECT = process.env.NETSTORM_E2E_ASPECT ?? '4:3';
const VIEWPORTS = { '4:3': { width: 1024, height: 768 }, '16:9': { width: 1280, height: 720 }, '16:10': { width: 1280, height: 800 } };
if (!VIEWPORTS[ASPECT]) throw new Error(`지원하지 않는 화면비 ${ASPECT}`);

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// The War Begins! 를 시작하고 안내 창을 닫아 시계가 흐르게 한 뒤 신전·사제 SID 를 돌려준다. 오류는 errors 에 모은다(expected 에 든 문구는 제외).
async function startWarBegins(page, errors, expected = []) {
  page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error' && !expected.some((text) => message.text().includes(text))) errors.push(message.text()); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 10; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
  return page.evaluate(() => { const app = window.netstorm, session = app._campaignWorld.session(), types = app._gameAssets.types();
    return { temple: session.world().objects().find(({ object }) => object.owner === 1 && types[object.type - 70]?.assetName === 'rainVortex').sid, priest: session.findPriestParticipation(session.localOwner()) }; });
}

// 입력 큐 맨 앞 사건의 코드(16진)이다. 비었으면 null.
const head = (page) => page.evaluate(() => { const queue = window.netstorm._clientInputQueue, entry = queue.peek(); return entry ? entry.code.toString(16) : null; });

test('The War Begins!: 객체 명령이 입력 큐를 막지 않고 소비되며 이후 입력이 정상이다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; const ids = await startWarBegins(page, errors, ['Vortex.cpp:746']);
  // 명령 코드, 대상. 신전 세 명령은 신전 대상, 나머지는 사제 대상(공통 객체 명령에 위임되거나 미처리 0 이다). WipeOut 은 원본이 스스로 표시하는 개발자 진단(Vortex.cpp:746)을 낸다.
  const commands = [['WipeOut', 0x0300006f, ids.temple], ['Ascend', 0x0300007c, ids.temple], ['Descend', 0x030000a7, ids.temple], ['DetonateBomb', 0x03000074, ids.priest], ['InflictDamage', 0x030000e2, ids.priest],
    ['MakeMine', 0x030000af, ids.priest], ['Meltdown', 0x030000f8, ids.priest], ['TurnToObelisk', 0x030000ce, ids.priest], ['DestroyPiece', 0x03000015, 0]];
  for (const [name, code, target] of commands) {
    await page.evaluate(({ code, target }) => window.netstorm._clientInputQueue.pushCommand(code, 0, target >>> 0), { code, target });
    await expect.poll(() => head(page), { timeout: 5000, message: `${name} 명령이 큐 머리에서 소비되어야 한다` }).toBeNull();
  }
  // 이후 입력이 정상이다: P 로 사제가 선택된다.
  await page.keyboard.press('p'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._clientGameplay.selection.selected), '명령 뒤 P 선택').toBe(ids.priest);
  expect(errors).toEqual([]);
});

// 처리하지 못하는 사건이 3초(벽 시각) 넘게 큐 머리에 남으면 오류로 기록하고 버려 이후 입력이 영구히 막히지 않는다. 그 전에는 사건 순서를 지켜 머리에 남는다. (2026-11-21 WB04-E1150)
test('The War Begins!: 처리하지 못하는 사건은 3초 뒤 오류로 기록하고 버려 입력이 막히지 않는다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; const ids = await startWarBegins(page, errors, ['미구현 입력 사건을 버립니다']);
  await page.evaluate(() => window.netstorm._clientInputQueue.pushCommand(0x0300ffff, 0, 0)); await page.waitForTimeout(1500);
  expect(await head(page), '3초 전에는 순서를 지켜 머리에 남는다').toBe('300ffff');
  await expect.poll(() => head(page), { timeout: 10000, message: '3초 넘게 남은 미지원 사건은 버려져야 한다' }).toBeNull();
  await page.keyboard.press('p'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._clientGameplay.selection.selected), '버린 뒤 P 선택').toBe(ids.priest);
  expect(errors).toEqual([]);
});

// 객체 메뉴의 사제 기도(0x030000de)·운반체 Drop(0x03000076) 명령이(Cast 0x0300011a 는 같은 분기이나 내용물이 있는 운반체가 필요해 단위 시험이 맡는다) 입력 큐 머리에서 소비된다. 허용 목록에 없어 3초 뒤 "미구현 입력 사건을 버립니다" 로 버려지던 것을 연결했다. (2026-11-22 WB04-E1225)
test('The War Begins!: 사제 기도와 운반체 Drop 명령이 입력 큐에서 소비된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; const ids = await startWarBegins(page, errors);
  // 기도: packed y 의 하위 WORD 는 사제 SID, 상위 WORD 는 희망 종류이다. 소비 뒤 사제가 계속 선택 가능해야 한다.
  for (const [name, code, target] of [['Drop', 0x03000076, ids.priest], ['Pray', 0x030000de, ((0x20 << 16) | ids.priest) >>> 0]]) {
    await page.evaluate(({ code, target }) => window.netstorm._clientInputQueue.pushCommand(code, 0, target >>> 0), { code, target });
    await expect.poll(() => head(page), { timeout: 2500, message: `${name} 명령이 3초 안에(버려지기 전에) 소비되어야 한다` }).toBeNull();
  }
  await page.keyboard.press('p'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._clientGameplay.selection.selected), '명령 뒤 P 선택').toBe(ids.priest);
  expect(errors).toEqual([]);
});

// 브리핑·완료 창의 `Review Knowledge`(ShowTechnology)와 `GetTechnology` 단추가 실제로 동작한다. 원본 스크립트에 각각 313개·101개가 있는데, 웹은 이름을 "아직 구현되지 않았습니다"로 넘겨 눌러도 아무 일이 없었다. (2026-11-22 WB04-E1227)
test('The War Begins!: 브리핑의 Review Knowledge 단추가 지식 창을 열고 GetTechnology 단추가 기술을 지급한다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; const ids = await startWarBegins(page, errors);
  // F8 브리핑 → Review Knowledge: 대화창이 닫히고 지식 창(Storm 카드)이 열린다.
  await page.keyboard.press('F8');
  await expect.poll(() => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null), { timeout: 5000, message: 'F8 브리핑 단추' }).toEqual(['Review Knowledge', 'Play Mission']);
  expect(await page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().isOpen()), '열기 전').toBe(false);
  await clickLabel(page, 'Review Knowledge');
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().isOpen()), { timeout: 5000, message: 'Review Knowledge 뒤 지식 창' }).toBe(true);
  expect(await page.evaluate(() => window.netstorm._mainMenu.dialog), '브리핑 창은 닫힌다').toBeNull();
  // GetTechnology 단추: 아직 없는 기술 한 장을 Session 에 지급한다. 이 단추가 큐에 올리는 명령은 인자(기술 번호)를 y 에 싣는다.
  const granted = await page.evaluate(() => {
    const app = window.netstorm, session = app._campaignWorld.session(), types = session.core().types, parent = session.currentTechnologyParent(), finder = session.priests().carrier;
    const type = types.findIndex((record, index) => record.group !== 10 && record.theme !== 5 && !finder.findContained(parent, 4, index, 0));
    if (type < 0) return { type: -1 };
    app._runDialogCommand({ name: 'GetTechnology', argument: String(type) });
    return { type };
  });
  expect(granted.type, '지급할 기술 후보').toBeGreaterThanOrEqual(0);
  await expect.poll(() => page.evaluate((type) => { const session = window.netstorm._campaignWorld.session(); return session.priests().carrier.findContained(session.currentTechnologyParent(), 4, type, 0) !== 0; }, granted.type), { timeout: 5000, message: 'GetTechnology 뒤 기술 카드' }).toBe(true);
  expect(errors).toEqual([]);
});

// 도움말 링크 이동(`#priestType` 같은 *Type 주제)에서 주제 이름으로 TYPE 을 조회해 정보 패널(Hits/Range/Cost 표)과 유닛 그림이 붙는다. 이전에는 링크 이동·Back 이 정보와 그림을 지워 본문만 나왔다. (2026-11-22 WB04-E1229)
test('The War Begins!: 도움말의 *Type 링크 이동에서도 TYPE 정보 패널과 그림이 나온다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  const result = await page.evaluate(() => {
    const app = window.netstorm; app._openHelp('F1Help'); const help = app._mainMenu.dialog;
    const before = { info: typeof help.informationFactory, picture: help.picture !== null };
    app._openHelp('priestType'); const linked = { topic: help.topic, info: typeof help.informationFactory, picture: help.picture !== null, html: typeof help.informationFactory === 'function' ? String(help.informationFactory()).slice(0, 300) : '' };
    app._openHelp('campaignHelp'); const plain = { info: typeof help.informationFactory, picture: help.picture !== null };
    help.back(); const back = { topic: help.topic, info: typeof help.informationFactory, picture: help.picture !== null };
    return { before, linked, plain, back };
  });
  expect(result.before, '링크 전 F1Help').toEqual({ info: 'object', picture: false });
  expect(result.linked.topic).toBe('priestType'); expect(result.linked.info, 'priestType 정보 패널').toBe('function'); expect(result.linked.picture, 'priestType 그림').toBe(true);
  expect(result.plain, '*Type 이 아닌 주제는 정보 없음').toEqual({ info: 'object', picture: false });
  expect(result.back, 'Back 은 priestType 을 다시 조회').toEqual({ topic: 'priestType', info: 'function', picture: true });
  expect(errors).toEqual([]);
});

// Session 이 큐에 올리는 통지 명령(내 너겟 납품 0x020000b4·Cast/Drop 성공 0x030000b2·UpdateContents 0x02000039)이 허용 목록에 없어 3초 동안 모든 입력을 막고 오류로 버려졌다. 이제 큐 머리에서 바로 소비되고 뒤의 입력이 막히지 않는다. (2026-11-22 WB04-E1231)
test('The War Begins!: 세션 통지 명령 세 종류가 입력을 막지 않고 바로 소비된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; const ids = await startWarBegins(page, errors);
  for (const [name, code] of [['내 너겟 납품', 0x020000b4], ['Cast/Drop 성공', 0x030000b2], ['UpdateContents', 0x02000039]]) {
    await page.evaluate((value) => window.netstorm._clientInputQueue.pushCommand(value, 0, 0), code);
    await expect.poll(() => head(page), { timeout: 1500, message: `${name} 통지가 3초 대기 없이 소비되어야 한다` }).toBeNull();
  }
  await page.keyboard.press('p'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._clientGameplay.selection.selected), '통지 뒤 P 선택').toBe(ids.priest);
  expect(errors).toEqual([]);
});

// Tell 단추 `MissionAbort,0`(Priest Training 성공·시간초과 창의 Leave Missions 등)은 원본처럼 곧바로 떠나지 않고 ABORT 확인 창(Main Menu·Replay Mission·Continue Mission)을 연다. (2026-11-22 WB04-E1231)
test('The War Begins!: MissionAbort,0 단추는 ABORT 확인 창을 열고 Continue Mission 으로 계속할 수 있다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  await page.evaluate(() => window.netstorm._runDialogCommand({ name: 'MissionAbort', argument: '0' }));
  await expect.poll(() => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null), { timeout: 5000, message: 'ABORT 확인 창' }).toEqual(['Main Menu', 'Replay Mission', 'Continue Mission']);
  expect(await page.evaluate(() => Boolean(window.netstorm._campaignWorld)), '확인 전에는 미션이 계속된다').toBe(true);
  await clickLabel(page, 'Continue Mission');
  await expect.poll(() => page.evaluate(() => window.netstorm._mainMenu.dialog), { timeout: 5000 }).toBeNull();
  expect(await page.evaluate(() => Boolean(window.netstorm._campaignWorld)), 'Continue 뒤 미션 유지').toBe(true);
  // 인자 1 은 곧바로 떠난다(LeaveBattle 예약).
  await page.evaluate(() => window.netstorm._runDialogCommand({ name: 'MissionAbort', argument: '1' }));
  await expect.poll(() => page.evaluate(() => Boolean(window.netstorm._campaignWorld)), { timeout: 8000, message: 'MissionAbort,1 이탈' }).toBe(false);
  expect(errors).toEqual([]);
});

// 프레임 끝 Player 결과 알림(원본 NotifyPlayerResults): 로컬 Player 기록의 대기 결과(+0xa0)가 2/3 이면 제단 등급(+0x71)을 올리고 'AltarUpgrade' 안내를 연 뒤 대기 값을 지운다. 앱이 이 호출을 하지 않아 제단 승급 안내가 한 번도 뜨지 않았다. (2026-11-22 WB04-E1233)
test('The War Begins!: 대기 중인 제단 승급 결과가 AltarUpgrade 안내와 제단 등급으로 소비된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  const before = await page.evaluate(() => {
    const app = window.netstorm, session = app._campaignWorld.session(), owner = session.localOwner(), players = session.playerRecords(); const record = players.subarray(owner * 180, owner * 180 + 180);
    window.__tells = []; const open = app._openTell.bind(app); app._openTell = (name, options) => { window.__tells.push(name); return open(name, options); };
    // 기술 링크(+0x9c)가 없으면 원문처럼 소비하지 않으므로 현재 기술 부모를 흉내 내는 대신 이미 있는 값을 확인한다.
    return { owner, link: record[0x9c] | (record[0x9d] << 8) | (record[0x9e] << 16) | (record[0x9f] << 24), grade: record[0x71] };
  });
  expect(before.link, '기술 링크(+0x9c)').not.toBe(0);
  await page.evaluate(() => { const session = window.netstorm._campaignWorld.session(), owner = session.localOwner(), record = session.playerRecords().subarray(owner * 180, owner * 180 + 180); record[0xa0] = 2; record[0xa1] = 0; record[0xa2] = 0; record[0xa3] = 0; });
  await expect.poll(() => page.evaluate(() => window.__tells.includes('AltarUpgrade')), { timeout: 5000, message: 'AltarUpgrade 안내' }).toBe(true);
  const after = await page.evaluate(() => { const session = window.netstorm._campaignWorld.session(), owner = session.localOwner(), record = session.playerRecords().subarray(owner * 180, owner * 180 + 180); return { grade: record[0x71], pending: record[0xa0] }; });
  expect(after, '등급 상승과 대기 값 소거').toEqual({ grade: Math.max(1, before.grade), pending: 0 });
  expect(errors).toEqual([]);
});

// F1 은 열린 Tell·확인 창을 도움말로 바꿔치우지 않는다(원본 OpenQuickHelpWindow 는 모달이 도움말이 아니면 열지 않는다). 창이 없으면 도움말이 열리고, 도움말이 열려 있으면 그대로 도움말이다. (2026-11-22 WB04-E1235)
test('The War Begins!: F1 은 열린 안내 창을 도움말로 바꾸지 않는다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  const kind = () => page.evaluate(() => window.netstorm._mainMenu.dialog?.constructor.name ?? null);
  await page.evaluate(() => window.netstorm._openTell('ABORT'));
  expect(await kind(), 'ABORT 확인 창').toBe('TellWindow');
  await page.keyboard.press('F1'); await page.waitForTimeout(400);
  expect(await kind(), 'F1 뒤에도 확인 창').toBe('TellWindow');
  await page.evaluate(() => window.netstorm._mainMenu.setDialog(null)); await page.keyboard.press('F1');
  await expect.poll(kind, { timeout: 3000, message: '창이 없으면 도움말' }).toBe('HelpWindow');
  await page.keyboard.press('F1'); await page.waitForTimeout(300);
  expect(await kind(), '도움말이 열린 채 F1').toBe('HelpWindow');
  expect(errors).toEqual([]);
});

// HUD 우클릭 빠른 도움말(원본 BaseHelp): 미니맵("Sky Overview")·스톰 파워 표시·생산 창을 우클릭하면 제목과 Help 항목 팝업이 열리고, Help 를 누르면 `minimapGumpHelp` 같은 주제의 도움말 창이 열린다. 이전에는 노드에 도움말 문자열이 없어 아무 반응이 없었다. (2026-11-22 WB04-E1236)
test('The War Begins!: HUD 우클릭 도움말 팝업이 열리고 Help 항목이 해당 주제 도움말을 연다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  const targets = [['minimap', 'Sky Overview', 'minimapGumpHelp'], ['budget', 'Storm Power Available', 'moneyGumpHelp'], ['bridge', 'Production Window', 'teleportViewHelp']];
  for (const [name, title, topic] of targets) {
    // 노드의 도움말 문구·주제 이름이 설정돼 있고, 같은 노드를 우클릭하면 팝업이 열린다.
    const opened = await page.evaluate(({ name }) => {
      const app = window.netstorm, root = app._clientGump, production = app._campaignProduction, n = production.ui.node();
      const token = name === 'minimap' ? production.minimap.token : n[name], node = root.node(token), position = root.collection().position(token);
      const result = { title: app._helpString(node.textPointer), word: app._helpString(node.alternateTextPointer) };
      root.closePopups(); const handled = root.handle(token, 0x00020000, position.x + 5, position.y + 5);
      let menu = null; for (let t = root.root().child; t; t = root.node(t).next) { const child = root.node(t); if (child.command === 26) menu = t; }
      const items = menu ? root.node(menu).items.filter((item) => item && item.command !== undefined).map((item) => ({ command: item.command, parameter: item.parameter })) : [];
      return { ...result, handled, popup: root.blockingPopups(), items };
    }, { name });
    expect(opened.title, `${name} 제목`).toBe(title); expect(opened.handled, `${name} 우클릭 처리`).toBe(1); expect(opened.popup, `${name} 팝업`).toBe(true);
    // Help 항목은 F1 도움말 명령(0x02000017)에 주제 문자열 토큰을 싣는다: 같은 명령을 올려 해당 주제의 도움말 창이 열리는지 본다.
    const help = opened.items.find((item) => (item.command & 0x03ffffff) === 0x02000017);
    expect(help, `${name} Help 항목`).toBeTruthy();
    await page.evaluate(() => window.netstorm._closeCampaignPopups());
    await page.evaluate((item) => window.netstorm._clientInputQueue.pushCommand(item.command, 0, item.parameter), help);
    await expect.poll(() => page.evaluate(() => window.netstorm._mainMenu.dialog?.topic ?? null), { timeout: 5000, message: `${name} 도움말 주제` }).toBe(topic);
    await page.evaluate(() => window.netstorm._mainMenu.setDialog(null));
  }
  // 실제 마우스 우클릭도 같은 팝업을 연다(미니맵).
  const spot = await page.evaluate(() => { const app = window.netstorm, root = app._clientGump, token = app._campaignProduction.minimap.token, position = root.collection().position(token), node = root.node(token), rect = app.canvas.getBoundingClientRect();
    root.closePopups(); return { x: rect.left + (position.x + node.width / 2) * rect.width / app._layout.logicalWidth, y: rect.top + (position.y + node.height / 2) * rect.height / app._layout.logicalHeight }; });
  await page.mouse.move(spot.x, spot.y); await page.mouse.click(spot.x, spot.y, { button: 'right', delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGump.blockingPopups()), { timeout: 5000, message: '실제 우클릭 도움말 팝업' }).toBe(true);
  expect(errors).toEqual([]);
});

// 도움말 첫 주제의 Back 은 닫은 뒤 직전 팝업(F6 지식 창) 명령을 다시 큐에 올려 지식 창이 다시 열린다(원본 QuickHelpControl). (2026-11-22 WB04-E1237)
test('The War Begins!: 도움말 첫 주제의 Back 은 직전 팝업인 지식 창을 다시 연다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  await page.keyboard.press('F6');
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().isOpen()), { timeout: 5000, message: 'F6 지식 창' }).toBe(true);
  expect(await page.evaluate(() => window.netstorm._clientHelpPopup.command >>> 0), '지식 창이 남긴 팝업 표식').toBe(0x020000a0);
  await page.evaluate(() => { const app = window.netstorm; app._closeCampaignPopups(); app._openHelp('F1Help'); });
  expect(await page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().isOpen()), '도움말이 열린 동안 지식 창은 닫혀 있다').toBe(false);
  await page.evaluate(() => window.netstorm._mainMenu.dialog.back());
  await expect.poll(() => page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().isOpen()), { timeout: 5000, message: 'Back 뒤 지식 창 복귀' }).toBe(true);
  expect(await page.evaluate(() => window.netstorm._mainMenu.dialog), '도움말 창은 닫힌다').toBeNull();
  expect(errors).toEqual([]);
});


// 채팅 창의 도우미 단추(`>`)를 우클릭하면 제목 "Chat Helper" 와 Help 항목 팝업이 열리고, Help 를 누르면 `chathelperHelp` 주제의 도움말 창이 열린다(원본 RawMenuChat.inc:33 의 title→SetHelpText·help→SetHelpWord).
// 이전에는 두 훅이 빈 함수라 노드에 도움말 문구·주제가 없어 아무 반응이 없었다. (2026-11-23 WB04-E1253)
test('The War Begins!: 채팅 도우미 단추 우클릭 도움말이 열리고 Help 항목이 chathelperHelp 를 연다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  // F3 로 채팅 창을 보이고 Enter 로 입력줄을 연다(도우미 단추는 입력줄이 열려 있을 때만 보인다).
  await page.locator('canvas').focus(); await page.keyboard.press('F3'); await page.waitForTimeout(800); await page.keyboard.press('Enter'); await page.waitForTimeout(600);
  const helper = await page.evaluate(() => { const app = window.netstorm, ui = app._campaignProduction.ui.chat(), b = ui.report().helper.bounds, rect = app.canvas.getBoundingClientRect();
    return { bounds: b, x: rect.left + (b.left + b.right) / 2 * rect.width / app._layout.logicalWidth, y: rect.top + (b.top + b.bottom) / 2 * rect.height / app._layout.logicalHeight }; });
  expect(helper.bounds.bottom - helper.bounds.top, '도우미 단추가 화면에 보인다').toBeGreaterThan(4);
  // 노드에 도움말 문구(번역한 제목)와 주제 이름이 설정돼 있다.
  expect(await page.evaluate(() => { const app = window.netstorm, node = app._clientGump.node(app._campaignProduction.ui.chat().report().helper.token); return { title: app._helpString(node.textPointer), word: app._helpString(node.alternateTextPointer) }; }), '도우미 노드 도움말').toEqual({ title: 'Chat Helper', word: 'chathelper' });
  // 실제 마우스 우클릭이 도우미 단추에 닿아 팝업이 열린다.
  await page.evaluate(() => window.netstorm._clientGump.closePopups());
  await page.mouse.move(helper.x, helper.y); await page.mouse.click(helper.x, helper.y, { button: 'right', delay: 60 });
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGump.blockingPopups()), { timeout: 5000, message: '도우미 우클릭 도움말 팝업' }).toBe(true);
  // 팝업 메뉴의 Help 항목은 F1 도움말 명령(0x02000017)에 주제 문자열 토큰을 싣는다.
  const popup = await page.evaluate(() => { const root = window.netstorm._clientGump; let menu = 0; for (let t = root.root().child; t; t = root.node(t).next) { if (root.node(t).command === 26) menu = t; }
    const items = menu ? root.node(menu).items.filter((item) => item && item.command !== undefined).map((item) => ({ command: item.command, parameter: item.parameter })) : []; return { menu, items }; });
  const help = popup.items.find((item) => (item.command & 0x03ffffff) === 0x02000017); expect(help, 'Help 항목').toBeTruthy();
  await page.evaluate(() => window.netstorm._closeCampaignPopups());
  await page.evaluate((item) => window.netstorm._clientInputQueue.pushCommand(item.command, 0, item.parameter), help);
  await expect.poll(() => page.evaluate(() => window.netstorm._mainMenu.dialog?.topic ?? null), { timeout: 5000, message: '도우미 도움말 주제' }).toBe('chathelperHelp');
  expect(errors).toEqual([]);
});

// 팝업을 닫으면 다음 프레임에 화면에서 실제로 지워진다(원본 RequestClosePopups 뒤 common.changed 의 화면 갱신과 같은 결과). 채팅 도우미 팝업을 열었다가 `_closeCampaignPopups` 로 닫고 그 영역 픽셀로 확인한다. (2026-11-23 WB04-E1256)
test('The War Begins!: 팝업을 닫으면 그 영역이 다음 프레임에 다시 그려진다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  await page.locator('canvas').focus(); await page.keyboard.press('F3'); await page.waitForTimeout(800); await page.keyboard.press('Enter'); await page.waitForTimeout(600);
  // 팝업이 차지할 영역(도우미 단추 위쪽 지도 가장자리 띠)의 논리 사각형이다.
  const helper = await page.evaluate(() => { const app = window.netstorm, b = app._campaignProduction.ui.chat().report().helper.bounds, rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + (b.left + b.right) / 2 * rect.width / app._layout.logicalWidth, y: rect.top + (b.top + b.bottom) / 2 * rect.height / app._layout.logicalHeight }; });
  // 현재 캔버스에서 논리 사각형의 픽셀 값을 읽는다.
  const grab = (box) => page.evaluate((r) => { const app = window.netstorm, canvas = app.canvas, context = canvas.getContext('2d'), scale = canvas.width / app._layout.logicalWidth;
    return Array.from(context.getImageData(Math.round(r.left * scale), Math.round(r.top * scale), Math.round((r.right - r.left) * scale), Math.round((r.bottom - r.top) * scale)).data); }, box);
  const diff = (a, b) => { let count = 0; for (let index = 0; index < a.length; index += 4) if (Math.abs(a[index] - b[index]) + Math.abs(a[index + 1] - b[index + 1]) + Math.abs(a[index + 2] - b[index + 2]) > 60) count += 1; return count / (a.length / 4); };
  // 팝업을 한 번 열어 차지하는 논리 사각형을 재고 닫는다.
  const open1 = async () => { await page.mouse.move(helper.x, helper.y); await page.mouse.down(); await page.waitForTimeout(100); await page.mouse.up(); await page.waitForTimeout(700); };
  await open1();
  const box = await page.evaluate(() => { const root = window.netstorm._clientGump; let menu = 0; for (let t = root.root().child; t; t = root.node(t).next) { if (root.node(t).command === 26) menu = t; }
    const b = root.collection().bounds(menu); return { left: b.left + 4, top: b.top + 4, right: b.right - 4, bottom: Math.min(b.bottom, 760) - 4 }; });
  await page.evaluate(() => window.netstorm._closeCampaignPopups()); await page.waitForTimeout(700);
  const before = await grab(box);
  await open1();
  expect(await page.evaluate(() => window.netstorm._clientGump.blockingPopups()), '도우미 메뉴가 열림').toBe(true);
  const open = await grab(box);
  expect(diff(before, open), '팝업이 영역을 덮는다').toBeGreaterThan(0.4);
  await page.evaluate(() => window.netstorm._closeCampaignPopups()); await page.waitForTimeout(700);
  expect(await page.evaluate(() => window.netstorm._clientGump.blockingPopups()), '팝업이 닫힘').toBe(false);
  const after = await grab(box);
  expect(diff(open, after), '닫은 뒤 영역이 다시 그려진다').toBeGreaterThan(0.25);
  expect(diff(before, after), '닫은 뒤 영역이 팝업 전과 같다(애니메이션 오차 허용)').toBeLessThan(0.1);
  expect(errors).toEqual([]);
});

// 생산 다리 격자는 시간이 지나 새 다리 조각이 생기면(Session 의 생산 내용물 갱신) 다음 프레임들에 화면에서 실제로 바뀐다(원본 UpdateContents 뒤 common.changed 의 무효화와 같은 결과). 새 조각이 생겨 격자 내용물이 늘 때까지 영역 픽셀을 지켜본다.
// (기술 지급(CheatAllTechnology)은 생산 패널이 아니라 사제의 건설 메뉴에 나타나므로 이 시험의 대상이 아니다.) (2026-11-23 WB04-E1257)
test('The War Begins!: 새 다리 조각이 생기면 생산 다리 격자가 화면에서 바뀐다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; await startWarBegins(page, errors);
  const grab = (box) => page.evaluate((r) => { const app = window.netstorm, canvas = app.canvas, context = canvas.getContext('2d'), scale = canvas.width / app._layout.logicalWidth;
    return Array.from(context.getImageData(Math.round(r.left * scale), Math.round(r.top * scale), Math.round((r.right - r.left) * scale), Math.round((r.bottom - r.top) * scale)).data); }, box);
  const diff = (a, b) => { let count = 0; for (let index = 0; index < a.length; index += 4) if (Math.abs(a[index] - b[index]) + Math.abs(a[index + 1] - b[index + 1]) + Math.abs(a[index + 2] - b[index + 2]) > 60) count += 1; return count / (a.length / 4); };
  // 생산 패널의 다리 격자(위쪽 슬롯) 영역이다.
  const box = await page.evaluate(() => { const app = window.netstorm, root = app._clientGump, b = root.collection().bounds(app._campaignProduction.ui.node().bridge); return { left: b.left + 2, top: b.top + 2, right: b.right - 2, bottom: b.bottom - 2 }; });
  const first = await grab(box); let changed = 0;
  // 새 조각이 도착하는 동안(최대 40초) 영역이 처음 모습과 5% 넘게 달라지는지 본다.
  for (let wait = 0; wait < 40 && changed < 0.05; wait += 1) { await page.waitForTimeout(1000); changed = diff(first, await grab(box)); }
  expect(changed, '새 다리 조각이 화면에 나타난다').toBeGreaterThan(0.05);
  expect(errors).toEqual([]);
});
