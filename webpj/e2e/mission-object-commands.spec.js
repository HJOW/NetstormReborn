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

