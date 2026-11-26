// 앱의 미션 소유권·자산 장치가 현재 Client 수명을 사용하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { InputQueue } from '../www/core/input-queue.js';
import { GamePalette } from '../www/assets/palette.js';
import { ClientTraceGlobals } from '../www/core/client-diagnostics.js';
import { readData } from './helpers.mjs';

// 설정은 호출마다 현재 객체를 읽고 생산 갱신은 숨겨진 실제 Gump에도 전달한다.
test('Netstorm 미션 장치: 현재 설정과 생산 갱신', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    assert.throws(() => services.ownership.themeMode(), /설정 수명 누락/);
    assert.throws(() => services.ownership.refreshProduction(), /Gump 수명 누락/);
    app._initializeClientRuntime(false); app._layout = { logicalWidth: 100, logicalHeight: 80 }; app._initializeClientGump();
    const events = [], gump = app._clientGump;
    const child = gump.createBase(0, 10, 10, 0x800, (event, first, second) => { events.push([event, first, second]); return 1; });
    gump.attach(child, 0, 0); gump.node(child).flags &= ~1;
    try {
        app._gameData = { readInt: (key, fallback) => ({ value: key === 'theammode' ? -1 : fallback }) };
        assert.equal(services.ownership.themeMode(), 0xffffffff);
        app._gameData = { readInt: (_key, fallback) => ({ value: fallback }) };
        assert.equal(services.ownership.themeMode(), 1);
        services.ownership.refreshProduction(); services.assets.postPop.refreshProduction();
        assert.deepEqual(events, [[0x02000003, 0, 0], [0x02000003, 0, 0]]);
    } finally { gump.dispose(); }
    assert.throws(() => services.ownership.refreshProduction(), /반납한 Client Gump/);
});

// 개발자 명령과 자산 명령은 현재 입력 고리를 사용하며 교체한 큐를 다시 조회한다.
test('Netstorm 미션 장치: 현재 입력 큐와 명령 정수 폭', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    app._clientInputQueue = null; assert.throws(() => services.developerQueue(1, 2, 3), /입력 큐 수명 누락/);
    const first = new InputQueue(), second = new InputQueue(); app._clientInputQueue = first;
    services.developerQueue(0x03000075, -1, 2); app._clientInputQueue = second;
    services.assets.queueCommand(0x02000042, 0xffffffff, -2);
    assert.deepEqual(first.pop(false), { code: 0x03000075, x: -1, y: 2 });
    assert.deepEqual(second.pop(false), { code: 0x02000042, x: -1, y: -2 });
});

// 카메라는 현재 Screen을, 표면은 현재 Renderer의 전체·부분 변경 표를 사용한다.
test('Netstorm 미션 장치: 화면 크기와 교체된 실제 변경 표', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    assert.throws(() => services.cameraScreenSize(), /Screen 수명 누락/);
    assert.throws(() => services.surfaceDisplay.suppressed(), /Renderer 수명 누락/);
    assert.throws(() => services.surfaceDisplay.invalidate({ left: 1, top: 1, right: 5, bottom: 5 }, 0), /Renderer 수명 누락/);
    app._clientPaletteScreen = { width: 1024, height: 768 }; app._initializeClientRenderer(1024, 768);
    const first = app._clientRenderer;
    assert.deepEqual(services.cameraScreenSize(), { x: 1024, y: 768 }); assert.equal(services.surfaceDisplay.suppressed(), true);
    first.clearDirty(); app._displayDirty = false;
    services.surfaceDisplay.invalidate({ left: 1, top: 2, right: 5, bottom: 7 }, 4);
    assert.equal(app._displayDirty, false); assert.deepEqual(first.cameraDirtyEntries(), [{ rect: { left: 1, top: 2, right: 5, bottom: 7 }, flags: 4 }]);
    // 실제 앱의 그리기 진입점에서도 부분 영역이 전체 영역으로 승격되지 않아야 한다.
    app._ctx = {}; app._layout = { visible: true, logicalWidth: 1024, logicalHeight: 768 };
    app._surface = {}; app._mainMenu = { dialog: null, dirty: false }; app._campaignDisplayActive = () => true;
    app._campaignCameraDevice = () => ({ applyCameraFrame() {} });
    app._drawCampaignRendererFrame = () => assert.deepEqual(first.cameraDirtyEntries(), [{ rect: { left: 1, top: 2, right: 5, bottom: 7 }, flags: 4 }]);
    app._render(0);
    app._clientPaletteScreen = { width: 1366, height: 768 }; app._initializeClientRenderer(1366, 768);
    const second = app._clientRenderer; assert.notEqual(second, first);
    assert.deepEqual(services.cameraScreenSize(), { x: 1366, y: 768 }); assert.equal(services.surfaceDisplay.suppressed(), true);
    // 다른 UI가 이미 요청한 전체 Canvas 표시는 부분 갱신 전달이 지우지 않는다.
    app._displayDirty = true;
    second.clearDirty(); services.surfaceDisplay.invalidate({ left: 1025, top: 3, right: 1030, bottom: 9 }, 0);
    assert.equal(app._displayDirty, true);
    assert.equal(second.cameraDirtyEntries().length, 1); assert.equal(first.cameraDirtyEntries().length, 1);
});

// 실제 Screen·Renderer·Gump를 거치는 즉시 카메라 이동과 잘못된 수명을 검사한다.
test('Netstorm 미션 장치: 실제 화면의 카메라 적용', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    assert.throws(() => services.cameraCommit({}), /실제 Client 수명 불일치/);
    app._initializeClientRuntime(false); app._layout = { logicalWidth: 640, logicalHeight: 480, visible: false };
    app._initializeClientPalette({ palette: GamePalette.parse(readData('d/GIFCLOUD.COL')) }); app._initializeClientGump(); app._initializeClientRendererView();
    const gump = app._clientGump, child = gump.createBase(1, 20, 20, 0x401, () => 0); gump.attach(child, 0, 0);
    const camera = app._clientGameplay.camera(), screen = app._clientPaletteScreen, calls = [], scroll = screen.scroll.bind(screen);
    screen.scroll = (...args) => { calls.push(args); scroll(...args); }; app._clientRenderer.clearDirty(); app._displayDirty = false;
    try {
        camera.requestPixels(30, 20); services.cameraCommit({ camera });
        assert.deepEqual(camera.rendererView().pixels, { x: 30, y: 20 }); assert.deepEqual(camera.snapshot().pixels, { x: 30, y: 20 });
        assert.deepEqual(calls, [[0, 0, 640, 480, 30, 20]]); assert.equal(app._clientGameplay.cursor.lockDepth, 1);
        assert.equal(app._displayDirty, true); assert.equal(app._clientRenderer.cameraRedraw.value, 1);
        assert.deepEqual(app._clientRenderer.cameraDirtyEntries(), [{ rect: { left: 610, top: 0, right: 640, bottom: 480 }, flags: 0 }, { rect: { left: 0, top: 460, right: 640, bottom: 480 }, flags: 0 }]);
        assert.throws(() => services.cameraCommit({ camera: {} }), /수명 불일치/);
    } finally { gump.dispose(); }
});

// 희생 의식의 곡 요청은 현재 장면 음악 구동기로 가며, 구동기를 교체해도 다시 조회하고 구동기가 없으면(소리 끔) 던지지 않는다. (2026-11-20 WB04-E1120)
test('Netstorm 미션 장치: 희생 의식 음악 요청은 현재 장면 음악 구동기로 간다', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    assert.doesNotThrow(() => services.requestMusic('sacrifice.mus'));
    const first = [], second = []; app._sceneMusic = { request: (name) => first.push(name) };
    services.requestMusic('sacrifice.mus'); app._sceneMusic = { request: (name) => second.push(name) }; services.requestMusic('other.mus');
    assert.deepEqual(first, ['sacrifice.mus']); assert.deepEqual(second, ['other.mus']);
});

// World 가 없으면 반납이 실제 State 정리를 호출하지 않아 캡처 기한과 건물 숨김 값을 보존한다. 실제 World 의 초기화는 campaign-request-frame 통합 검사에서 확인한다.
test('Netstorm World 반납: World 가 없으면 캡처 기한과 건물 숨김을 보존한다', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false);
    app._clientLocal.captureEnd().value = 7; app._releaseCampaignWorld(); assert.equal(app._clientLocal.captureEnd().value, 7);
    app._clientGameplay.hideBuildings().value = 1; app._releaseCampaignWorld(); assert.equal(app._clientGameplay.hideBuildings().value, 1);
    assert.equal(app._campaignWorld, null);
});

// 원본 CampaignClientServices 의 소리·안내 서비스(보호막 안내음·번역, 유닛 상실음, 사제 낙하음, 섬 피격 경고 사건, Cast/Drop 비용 표시, 중복 워크샾 진단, 기술 획득)가
// 현재 재생기·생산 UI·Tell·설정으로 가는지 확인한다. 연결이 없으면 해당 사건에서 '미연결' 오류로 프레임이 멈췄다. (2026-11-20 WB04-E1122)
test('Netstorm 미션 장치: 보호막·유닛 상실·낙하 소리와 섬 피격 경고·비용 표시', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices(), calls = [];
    app._clientSoundPlayer = { playByName: (...args) => calls.push(['byName', ...args]), playNameAt: (...args) => calls.push(['nameAt', ...args]) };
    // 보호막 안내음은 안내 구조체의 다섯 인자를 이름 기반 전역 재생(반복·음량·좌우·우선·한도)으로 넘기고 번역은 키 그대로다.
    services.priest.shield.noticeSound({ file: 'shield.wav', arguments: [0, 3, -4, 1, 2] });
    assert.deepEqual(calls.pop(), ['byName', 'shield.wav', 0, 3, -4, 1, 2]); assert.equal(services.priest.shield.translate('ShieldNotice'), 'ShieldNotice');
    // Tell 은 기존처럼 현재 안내 창이다.
    const tells = []; app._openTell = (section) => { tells.push(section); return true; };
    const fresh = app._campaignGameplayServices(); fresh.priest.shield.tell('islandUnderAttack'); assert.deepEqual(tells, ['islandUnderAttack']);
    services.unitLost(7); assert.deepEqual(calls.pop(), ['byName', 'unitLost.wav', 0, 0, 0, 1, 0]);
    services.priest.fallSound(3, 10.5, 20.25); assert.deepEqual(calls.pop(), ['nameAt', 10.5, 20.25, 'priestFall.wav', 0]);
    // 섬 피격 경고 사건은 종류 12 안내를 지우고, 생산 UI 가 없으면 아무 일도 없다. 비용 표시는 생산 UI 가 없으면 오류다.
    assert.doesNotThrow(() => services.hit.general.warningEvent()); assert.throws(() => services.carrierDropPopup(5, 1, 2, 3), /생산 UI 미설치/);
    const removed = [], shown = []; app._campaignProduction = { ui: { warnings: () => ({ remove: (...args) => removed.push(args) }), showSp: (...args) => shown.push(args) } };
    services.hit.general.warningEvent(); assert.deepEqual(removed, [[12, 0, 0]]);
    services.carrierDropPopup(-400, 30, 40, 9); assert.deepEqual(shown, [[-400, 30, 40, 9]]);
    // 재생기가 없으면 소리 서비스는 조용히 넘기지 않고 오류를 낸다.
    app._clientSoundPlayer = null; assert.throws(() => services.unitLost(1), /재생기 미준비/);
});

test('Netstorm 미션 장치: 중복 워크샾 진단과 기술 획득 이름·설정·그림 서식·안내', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    // 중복 워크샾 경고는 채널 4 로 원본 문구와 정수 좌표를 쓴다(실제 진단 장치는 현재 Client 수명이 필요하다).
    app._clientTrace = new ClientTraceGlobals(); const lines = [], info = console.info;
    try { console.info = (text) => lines.push(text); services.assets.factory.warnDuplicate(1, 10.7, -3.2); } finally { console.info = info; }
    assert.deepEqual(lines, ['WARNING!  Extra workshop destroyed at (10,-3)\n']); assert.equal(typeof services.assets.postPop.playImpactSound, 'function');
    // 기술 획득: 현재 TYPE 설명을 번역해 이름으로, 값은 같은 설정 저장소에 문자열로 쓰고, 그림 서식의 %d 는 부호 있는 정수다.
    assert.throws(() => services.newTechnology.name(1), /수명 누락/); assert.throws(() => services.newTechnology.configure('a', 'b'), /저장소 누락/);
    const sets = [], tells = []; app._gameData = { set: (key, value) => sets.push([key, value]) }; app._resources = { translate: (text) => `번역:${text}` };
    app._campaignWorld = { session: () => ({ core: () => ({ types: [{ description: 'Zero' }, { description: 'One' }] }) }) }; app._openTell = (section) => { tells.push(section); return true; };
    assert.equal(services.newTechnology.name(1), '번역:One'); assert.throws(() => services.newTechnology.name(2), RangeError); assert.throws(() => services.newTechnology.name(-1), RangeError);
    services.newTechnology.configure('global.newTechName', '이름'); services.newTechnology.configure('global.newTechPic', 7); assert.deepEqual(sets, [['global.newTechName', '이름'], ['global.newTechPic', '7']]);
    assert.equal(services.newTechnology.format('<!`"%d.*`">', -3), '<!`"-3.*`">'); assert.equal(services.newTechnology.format('x%dy', 4.9), 'x4y');
    services.newTechnology.tell('NewTech'); assert.deepEqual(tells, ['NewTech']);
});

// 포획 안내의 Player 이름: owner 만 채운 기록을 현재 Player 표(전투 중이면 1..8)에서 풀어 이름을 돌려주고, 못 찾으면 SOMEONE 이다. (2026-11-20 WB04-E1122)
test('Netstorm 미션 장치: 포획 안내 Player 이름은 owner 로 현재 Player 표를 조회한다', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    assert.throws(() => services.priestTakenName(2, ''), /수명 누락/);
    const players = new Uint8Array(9 * 180), chat = new Uint8Array(100 * 48);
    const put = (owner, name, subscriber) => { players.set([...name].map((c) => c.charCodeAt(0)), owner * 180 + 0xc); new DataView(players.buffer).setUint32(owner * 180 + 0x2c, subscriber, true); };
    put(2, 'Marquis', 77); put(5, 'Duke', 99);
    let battle = 1; const gameplay = { player: { battleOwner: 1 }, localAddress: () => ({ value: 0 }), world: { editor: 0 } };
    app._clientPlayers = { records: () => players }; app._clientChatRecords = chat; app._clientGameplay = gameplay; app._clientSubscriber = { mySubscriber: () => ({ value: 5 }) };
    app._surfaceStorage = { ownerMode: { get battle() { return battle; }, challenge: 0 } };
    assert.equal(services.priestTakenName(2, 'Me'), 'Marquis'); assert.equal(services.priestTakenName(5, 'Me'), 'Duke');
    // 전투가 아니고 표에도 채팅에도 없으면 SOMEONE 이다. 로컬 owner 는 현재 이름을 쓴다.
    assert.equal(services.priestTakenName(7, 'Me'), 'SOMEONE'); battle = 0; assert.equal(services.priestTakenName(2, 'Me'), 'SOMEONE');
    gameplay.player.battleOwner = 4; assert.equal(services.priestTakenName(4, 'Me'), 'Me');
});

// 안내 창 단추의 송금·동맹 이름은 원본 이름표의 명령 번호로 입력 큐에 올라가고(대상은 두 인자에 같게), 다른 이름은 건드리지 않는다. (2026-11-20 WB04-E1126)
test('Netstorm 안내 창 단추: 송금·동맹 이름을 명령 번호로 입력 큐에 올린다', () => {
    const app = new Netstorm(); app._clientInputQueue = new InputQueue();
    assert.equal(app._queueDialogPlayerCommand({ name: 'SendAllBucks', argument: ' 2 ' }), true); assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x02000118, x: 2, y: 2 });
    assert.equal(app._queueDialogPlayerCommand({ name: 'sendbucks', argument: '7' }), true); assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x020000ec, x: 7, y: 7 });
    assert.equal(app._queueDialogPlayerCommand({ name: 'ConfirmAlliance', argument: '5' }), true); assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x0200007e, x: 5, y: 5 });
    assert.equal(app._queueDialogPlayerCommand({ name: 'RequestAlliance', argument: 'oops' }), true); assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x020000fb, x: 0, y: 0 });
    // 다른 이름이거나 큐가 없으면 아무것도 올리지 않는다.
    assert.equal(app._queueDialogPlayerCommand({ name: 'Tell', argument: 'x' }), false); assert.equal(app._clientInputQueue.peek(), null);
    app._clientInputQueue = null; assert.equal(app._queueDialogPlayerCommand({ name: 'SendAllBucks', argument: '2' }), false);
});

// 브리핑·완료 창의 지식 단추(Review Knowledge=ShowTechnology, GetTechnology)는 미션 월드가 있을 때만 명령 번호로 큐에 올라가고 인자는 두 칸에 같게 실린다. CheatAllTechnology 는 본문 명령이라 단추 경로에서 제외한다. (2026-11-22 WB04-E1227)
test('Netstorm 안내 창 단추: ShowTechnology·GetTechnology 는 미션 월드가 있을 때만 큐에 올린다', () => {
    const app = new Netstorm(); app._clientInputQueue = new InputQueue(); const warnings = [], warn = console.warn;
    try {
        console.warn = (text) => warnings.push(text);
        // 월드가 없으면 큐에 올리지 않고(묵은 명령이 다음 미션에서 실행되지 않게) 소비만 한다.
        assert.equal(app._queueDialogPlayerCommand({ name: 'ShowTechnology', argument: '55' }), true); assert.equal(app._clientInputQueue.peek(), null); assert.equal(warnings.length, 1);
        app._campaignWorld = {};
        assert.equal(app._queueDialogPlayerCommand({ name: 'ShowTechnology', argument: '55' }), true); assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x020000a0, x: 55, y: 55 });
        assert.equal(app._queueDialogPlayerCommand({ name: 'showtechnology', argument: '55' }), true); assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x020000a0, x: 55, y: 55 });
        assert.equal(app._queueDialogPlayerCommand({ name: 'GetTechnology', argument: '76' }), true); assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x0200009f, x: 76, y: 76 });
        assert.equal(app._queueDialogPlayerCommand({ name: 'ShowTecSideMenu', argument: '0' }), true); assert.deepEqual(app._clientInputQueue.pop(false), { code: 0x02000132, x: 0, y: 0 });
        // 철자가 틀린 이름(GetTechnolgy)과 본문 명령 CheatAllTechnology 는 단추로 올리지 않는다.
        assert.equal(app._queueDialogPlayerCommand({ name: 'GetTechnolgy', argument: '76' }), false); assert.equal(app._queueDialogPlayerCommand({ name: 'CheatAllTechnology', argument: '1' }), false); assert.equal(app._clientInputQueue.peek(), null);
    } finally { console.warn = warn; }
});

// 전체 Fort snapshot 의 두 읽기 장치: 현재 Guide 의 저장 토큰·이름(NUL 포함)·순번·플래그와 Client 단일 Badges 장부(28x220)이다. 없으면 기술 획득이 오류로 멈췄다. (2026-11-22 WB04-E1227)
test('Netstorm 미션 장치: 전체 Fort snapshot 의 Mission 읽기 장치와 Badges 장부', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    assert.equal(services.fortBadges.records().length, 28 * 220); assert.equal(app._campaignGameplayServices().fortBadges, services.fortBadges);
    // Guide 가 없으면 현재 Mission 이 없다(토큰 0).
    assert.equal(services.fortMission.current(), 0); assert.throws(() => services.fortMission.name(), /Mission 없는/); assert.throws(() => services.fortMission.field(1, 0x58), /수명 만료/);
    const runtime = { saveToken: 7, init: { name: 'Tutorial1' }, state: { sequence: 3, flags: 0x80000001 } }; app._clientGuide = { current: () => ({ runtime }) };
    assert.equal(services.fortMission.current(), 7); assert.deepEqual([...services.fortMission.name()], [...Buffer.from('Tutorial1'), 0]);
    assert.equal(services.fortMission.field(7, 0x58), 3); assert.equal(services.fortMission.field(7, 0x54), 0x80000001);
    assert.throws(() => services.fortMission.field(8, 0x58), /수명 만료/); assert.throws(() => services.fortMission.field(7, 0x5c), RangeError); assert.throws(() => services.fortMission.assertion(), /원문 이름 진단/);
});

// 스크립트 명령 Warning: 미션 HUD 의 경고판이 있으면 그 문구를 올리고(서식 부호 포함 그대로), 없으면 콘솔에만 남긴다. (2026-11-22 WB04-E1228)
test('Netstorm 스크립트 명령: Warning 은 HUD 경고판에 올리고 HUD 가 없으면 콘솔에 남긴다', () => {
    const app = new Netstorm(), added = [], warnings = [], warn = console.warn;
    try {
        console.warn = (text) => warnings.push(text);
        assert.equal(app._runScriptCommand({ name: 'Warning', argument: '~B~E~yOur Priest has been Immobilized' }), true); assert.deepEqual(added, []); assert.equal(warnings.length, 1);
        app._campaignProduction = { ui: { warnings: () => ({ add: (text) => added.push(text) }) } };
        assert.equal(app._runScriptCommand({ name: 'warning', argument: '~B~E~yOur Island Is Under Attack' }), true); assert.deepEqual(added, ['~B~E~yOur Island Is Under Attack']); assert.equal(warnings.length, 1);
    } finally { console.warn = warn; }
});

