// ESC 게임 메뉴 부모(Game·View·Options·Players)와 가로 메뉴막대 조립·전환의 항목 구성·조건·순서를 대역 작성기로 확인한다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1084)
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawGameMenus } from '../www/ui/raw-game-menus.js';
import { ClientGameMenuBar } from '../www/ui/client-game-menu-bar.js';
import { gameMenuCommandValue } from '../www/core/game-menu-commands.js';

// 작성기 호출을 순서대로 기록하는 hooks 이다. take 는 지금까지 쌓은 항목 목록을 토큰으로 돌려준다.
function recorder(globals = {}, config = {}) {
    const log = [], stack = [[]], menus = [];
    const hooks = {
        global: (address) => { const command = gameMenuCommandValue(address); if (command !== null) return command >>> 0; if (address in globals) return globals[address]; throw new Error(`전역 미설정 ${address.toString(16)}`); },
        translate: (key) => key, config: (key) => config[key] ?? 0, demo: () => Boolean(globals.demo),
        vertical: () => { stack.push([]); log.push('vertical'); }, fancy: () => { stack.push([]); log.push('fancy'); },
        separator: () => { stack.at(-1).push({ separator: true }); }, title: (text) => { stack.at(-1).push({ title: text }); },
        item: (label, code, parameter, include, enabled) => { stack.at(-1).push({ label, code, parameter, include, enabled }); },
        checked: (check, label, code, parameter, enabled) => { stack.at(-1).push({ label, code, parameter, check, enabled }); },
        submenu: (label, child, enabled, check) => { stack.at(-1).push({ label, child, enabled, check }); },
        take: () => { menus.push(stack.pop()); return menus.length; },
        current: () => menus.length + 1, node: () => ({ baseWidth: 100, baseHeight: 60 }), attachCurrent: (x, y, parent) => log.push(['attach', x, y, parent]),
        player: (owner) => { const bytes = new Uint8Array(180), name = owner === 1 ? 'You' : 'AI'; bytes.set([...name].map((c) => c.charCodeAt(0)), 0xc); new DataView(bytes.buffer).setUint32(0x2c, owner === 3 ? 0 : 7, true); return bytes; },
        refreshPriest: (owner) => log.push(['refresh', owner]), playerMenu: (subscriber) => 1000 + subscriber,
    };
    return { hooks, log, menus };
}
// 항목 라벨 목록이다(구분선은 빈 문자열).
const labels = (rows) => rows.map((row) => row.separator ? '' : row.label ?? row.title);

test('Game: 미션은 목표 보기·재시작·나가기, 데모는 재시작·나가기만 두며 Quit Game 은 웹 범위 밖이다', () => {
    const mission = recorder({ 0x5c85a4: 0, 0x594fa4: 1 }); new RawGameMenus(mission.hooks).game();
    assert.deepEqual(labels(mission.menus[0]), ['Review Mission Objectives - ~iF8~.', '', 'Restart Mission', 'Leave Mission']);
    assert.deepEqual(mission.menus[0].map((row) => row.code), [0x02000091, undefined, 0x0200008d, 0x0200008c]);
    const demo = recorder({ 0x5c85a4: 0, 0x594fa4: 1, demo: 1 }); new RawGameMenus(demo.hooks).game();
    assert.deepEqual(labels(demo.menus[0]), ['Restart Demo', 'Exit Demo']); assert.equal(demo.menus[0][1].parameter, 1);
    assert.throws(() => new RawGameMenus(recorder({ 0x5c85a4: 1, 0x594fa4: 1 }).hooks).game(), /웹 범위 밖/u);
});

test('View: 전투 화면에서만 게임 조작행을 두고 채팅 행은 미션에서 제외하며 체크 값은 현재 전역과 설정을 읽는다', () => {
    const globals = { 0x5c85a4: 0, 0x594fc8: 0, 0x594fb8: 0, 0x594fbc: 1, 0x594fa4: 1, 0x594fa0: 1, 0x59a8ac: 1, 0x540c70: 1 };
    const view = recorder(globals, { theammode: 1, useBanners: 0 }); new RawGameMenus(view.hooks).view(); const rows = view.menus[0];
    assert.deepEqual(labels(rows), ['', 'Hide buildings - ~iF2~.', 'Hide/Show Island Themes - ~iShift F3~.', 'Hide/Show chat - ~iF3~.', 'View Home Temple - ~iF4~.', 'View Your Priest - ~iF5~.',
        'View Netstorm Knowledge - ~iF6~.', 'Hide Island Ownership - ~iF7~.', 'Review Mission Objectives - ~iF8~.', 'View Player List - ~iF9~.']);
    const by = (text) => rows.find((row) => row.label === text);
    assert.equal(by('Hide buildings - ~iF2~.').check, 1); assert.equal(by('Hide/Show Island Themes - ~iShift F3~.').check, 0); assert.equal(by('Hide/Show Island Themes - ~iShift F3~.').parameter, 'SetIslands(0)');
    assert.equal(by('Hide/Show chat - ~iF3~.').include, 0); assert.equal(by('View Your Priest - ~iF5~.').parameter, 1); assert.equal(by('Hide Island Ownership - ~iF7~.').parameter, 'useBanners=1');
    // 목표 보기 8번은 브리지 미션 전역이 없으면 빠지고, 앱이 연결하지 않은 명령·설정 키는 비활성으로 보인다.
    const plain = recorder({ ...globals, 0x594fa0: 0 }); new RawGameMenus(plain.hooks).view(); assert.ok(!labels(plain.menus[0]).includes('Review Mission Objectives - ~iF8~.'));
    const gated = recorder(globals); gated.hooks.enabled = (code) => code !== 0x020000f0; gated.hooks.configEnabled = (key) => key !== 'useBanners'; new RawGameMenus(gated.hooks).view();
    assert.equal(gated.menus[0].find((row) => row.label === 'View Player List - ~iF9~.').enabled, 0); assert.equal(gated.menus[0].find((row) => row.label === 'Hide Island Ownership - ~iF7~.').enabled, 0);
    assert.equal(gated.menus[0].find((row) => row.label === 'Hide buildings - ~iF2~.').enabled, 1);
});

test('Options: 소리·음량·화면 이동·정지·회전 항목을 원본 순서로 두고 음량 하위의 체크는 현재 단계를 따른다', () => {
    const globals = { 0x5c85a4: 0, 0x594fbc: 1, 0x594fa4: 1, 0x594fb8: 0, 0x594fc8: 0, 0x5c7b14: 1, 0x531900: 3, 0x531904: 5, 0x55b4b0: 0 };
    const options = recorder(globals, { sound: 1, music: 0, edgeScroll: 1, rotmode: 0 }); new RawGameMenus(options.hooks).options();
    // 하위 메뉴(음량 두 개)가 먼저 꺼내지고 마지막에 Options 본문이 꺼내진다.
    assert.equal(options.menus.length, 3); const [soundVolume, musicVolume, body] = options.menus;
    assert.deepEqual(soundVolume.map((row) => [row.label, row.check]), [['Volume 1', 0], ['Volume 2', 0], ['Volume 3', 1], ['Volume 4', 0], ['Volume 5', 0]]);
    assert.deepEqual(musicVolume.map((row) => row.check), [0, 0, 0, 0, 1]);
    // 효과음 음량은 0x020000e6, 음악 음량은 0x020000e7 명령이며 인자는 단계 숫자다.
    assert.equal(soundVolume[0].code, 0x020000e6); assert.equal(musicVolume[0].code, 0x020000e7); assert.equal(soundVolume[2].parameter, 3); assert.equal(musicVolume[4].parameter, 5);
    assert.deepEqual(labels(body), ['Sound On', 'Play Music', 'Wind Noise', 'Speaker Swap L/R', 'Sound Effect Volume', 'Music Volume', '', 'Edge Scroll in Fullscreen', 'Cursor Snap To Grid',
        'Auto-Demo', 'Tell Tips at Startup', 'Pause - ~iShift-F9~.', '', 'Counterclockwise Piece Rotation - ~iC~.']);
    assert.equal(body[0].check, 1); assert.equal(body[0].parameter, 'sound=0'); assert.equal(body[1].check, 0); assert.equal(body[1].parameter, 'music=1');
    assert.equal(body[4].child, 1); assert.equal(body[4].enabled, 1);
});

test('Options의 웹 관리 행은 실제 Tell 명령을 사용하며 호스트가 제공할 때만 붙는다', () => {
    const globals = { 0x5c85a4: 0, 0x594fbc: 1, 0x594fa4: 1, 0x594fb8: 0, 0x594fc8: 0, 0x5c7b14: 1, 0x531900: 3, 0x531904: 5, 0x55b4b0: 0 };
    const options = recorder(globals); options.hooks.modMenuLabel = () => '모드 관리';
    new RawGameMenus(options.hooks).options(); const last = options.menus.at(-1).at(-1);
    assert.deepEqual(last, { label: '모드 관리', code: gameMenuCommandValue(0x542948), parameter: 'Patch1101.Mods', include: 1, enabled: 1 });
    const original = recorder(globals); new RawGameMenus(original.hooks).options();
    assert.equal(original.menus.at(-1).some((row) => row.parameter === 'Patch1101.Mods'), false);
});

test('Players: 현재 Player 1..8 중 subscriber 가 있는 행만 이름과 하위 메뉴로 만들고 메타맵은 만들지 않는다', () => {
    const players = recorder({ 0x594fc8: 0, 0x594fa4: 1 }); players.hooks.global = ((original) => (address) => address > 0x52f81e && address < 0x52f827 ? 0 : original(address))(players.hooks.global);
    new RawGameMenus(players.hooks).players(); const rows = players.menus[0];
    assert.equal(rows.length, 7); assert.equal(rows[0].label, 'You'); assert.equal(rows[0].child, 1007); assert.deepEqual(players.log.filter((entry) => entry[0] === 'refresh').map((entry) => entry[1]), [1, 2, 4, 5, 6, 7, 8]);
    const meta = recorder({ 0x594fc8: 1 }); assert.equal(new RawGameMenus(meta.hooks).players(), 0); assert.equal(meta.menus.length, 0);
});

// 막대 전환 시험용 대역 Gump 루트와 메뉴 작성기다. 실제 가로 메뉴 대신 호출 순서와 토큰 수명만 확인한다.
function barScene() {
    const nodes = new Map(), entries = new Uint32Array(8), log = []; let next = 10; entries[0] = 1; nodes.set(1, { serial: 1, flags: 0 });
    const root = { node: (token) => nodes.get(token) ?? null, createState: () => ({ entries, collection: { root: 1 } }), mapView: () => ({ left: 85, top: 0, right: 1024, bottom: 768 }),
        remove: (token) => { nodes.delete(token); const index = entries.indexOf(token); if (index > 0) entries[index] = 0; log.push(['remove', token]); } };
    const menu = { closePopups: () => log.push('closePopups'), bindGameMenus: (bar, menus) => {
        const stack = [];
        Object.assign(bar, { horizontal: () => { stack.push([]); return 0; }, vertical: () => { stack.push([]); }, take: () => { stack.pop(); return ++next; }, separator: () => {}, submenu: () => {}, item: () => {},
            width: (token, width) => log.push(['width', width]), serial: (token) => nodes.get(token).serial });
        Object.assign(menus, { vertical: bar.vertical, fancy: bar.vertical, take: bar.take, separator: () => {}, submenu: () => {}, item: (...args) => log.push(['menuItem', ...args]), checked: () => {}, title: () => {}, node: () => null, current: () => 1, attachCurrent: () => {} });
        // 가로 막대의 토큰은 새 노드로 등록해 attach 훅이 부모에 붙인다.
        bar.horizontal = () => { const token = 99; nodes.set(token, { serial: 500, flags: 0 }); entries[3] = token; return token; }; bar.attach = (token, x, y, parent, mode) => log.push(['attach', x, y, parent, mode]);
    } };
    return { root, menu, log, nodes };
}
const devices = (extra = {}) => ({ global: (address) => ({ 0x594fbc: 1, 0x594fa4: 1, 0x5c85a4: 0 })[address] ?? 0, translate: (key) => key, config: () => 0, demo: () => false, player: () => new Uint8Array(180),
    refreshPriest: () => {}, playerMenu: () => 1, enabled: () => true, storeText: () => 1, about: () => 'About', technicalHelp: () => 0, width: () => 1024, ...extra });

test('ClientGameMenuBar: 같은 생성순번의 막대가 있으면 지우고 없으면 열며 Edit·Cheat 는 읽기 값 0 이다', () => {
    const scene = barScene(), bar = new ClientGameMenuBar(scene.root, scene.menu, devices());
    assert.equal(bar.isOpen(), false); assert.equal(bar.toggle(), true); assert.equal(bar.isOpen(), true); assert.equal(bar.token(), 99);
    assert.deepEqual(scene.log.find((entry) => entry[0] === 'width'), ['width', 939]); assert.deepEqual(scene.log.find((entry) => entry[0] === 'attach'), ['attach', 85, 0, 1, 1]);
    assert.equal(bar.open(), 99);
    assert.equal(bar.toggle(), false); assert.equal(bar.isOpen(), false); assert.ok(scene.log.includes('closePopups')); assert.equal(bar.close(), false);
    assert.throws(() => new ClientGameMenuBar(scene.root, scene.menu, {}), /장치 누락/u); assert.throws(() => new ClientGameMenuBar(null, scene.menu, devices()), /필요/u);
});

test('ClientGameMenuBar는 선택 관리 장치를 실제 Options 부모까지 전달한다', () => {
    const scene = barScene(), bar = new ClientGameMenuBar(scene.root, scene.menu, devices({ modMenuLabel: () => '모드 관리' }));
    bar.open(); assert.ok(scene.log.some((entry) => entry[0] === 'menuItem' && entry[1] === '모드 관리' && entry[3] === 'Patch1101.Mods' && entry[4] === 1 && entry[5] === 1));
    assert.equal(bar.close(), true);
});
