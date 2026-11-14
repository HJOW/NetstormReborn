// NSInfo 묶음(ClientNsInfoGlobals 전역·HudNsInfo 일반 부착·HudNsInfoCreate 전체 생성)을 기준선 소스 계약과 손 계산 값으로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 생성 순서 전체는 기준선 HudNsInfoCreate.cpp 의 호출 순서를 시험 쪽에서 따로 손으로 펼쳐 비교하고, 기본 생성은 실제 GumpCreate 를 쓴다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E444)
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { ButtonGumpNode } from '../www/ui/button-gump.js';
import { ClientNsInfoGlobals } from '../www/core/client-ns-info-globals.js';
import { HudNsInfo, HudNsInfoNode } from '../www/ui/hud-ns-info.js';
import { HudBattleInfoNode, HudNsInfoChild, HudNsInfoCreate, HudPlayerButtonNode, HudPlayerInfoNode } from '../www/ui/hud-ns-info-create.js';

// 문자열을 바이트 배열로 바꾼다(UTF-8 이 아닌 0..255 바이트 그대로).
const bytes = (text) => Uint8Array.from(Array.from(text, (char) => char.charCodeAt(0) & 0xff));
// 이름 버퍼 보기를 바이트 문자열로 바꾼다.
const text = (view) => Array.from(view, (value) => String.fromCharCode(value)).join('');

// ---------- ClientNsInfoGlobals ----------

test('ClientNsInfoGlobals: 초기값은 PE 정적 값(종류 6/37/60, zone 형식 0x50c894)이고 이름 버퍼는 80바이트 0 이다', () => {
    const g = new ClientNsInfoGlobals(); assert.deepEqual([g.current, g.zone, g.serverSelection, g.nsKind, g.playerKind, g.battleKind, g.zoneFormat], [0, 0, 0, 6, 37, 60, 0x50c894]);
    assert.equal(g.serverName.length, 80); assert.ok(g.serverName.every((value) => value === 0)); assert.deepEqual(g.servers, []);
});

test('loadServerNames: 개행으로 나눈 비어 있지 않은 행을 순서대로 보관하고 모든 CR 을 지운다', () => {
    const g = new ClientNsInfoGlobals(); g.loadServerNames(bytes('a;Alpha;x\r\n\r\nb;Beta\n;;\nlast'));
    assert.deepEqual(g.servers, ['a;Alpha;x', 'b;Beta', ';;', 'last']);
    g.loadServerNames(bytes('a\rb\r\r\n\n\r\n')); assert.deepEqual(g.servers, ['ab']);
    g.loadServerNames(bytes('')); assert.deepEqual(g.servers, []); g.loadServerNames(bytes('\n\n\r\n')); assert.deepEqual(g.servers, []);
    g.loadServerNames(Uint8Array.of(0x61, 0, 0x62, 0xe9, 0x0a, 0x63)); assert.deepEqual(g.servers, ['a\0b\xe9', 'c']);
    g.loadServerNames(bytes('one\n')); assert.deepEqual(g.servers, ['one']); g.loadServerNames(bytes('one\ntwo')); assert.deepEqual(g.servers, ['one', 'two']);
});

test('loadServerNames: 행 길이 256(CR 포함)은 허용하고 257 은 RangeError 이며 오류 때 기존 목록을 유지한다', () => {
    const g = new ClientNsInfoGlobals(); g.servers = ['keep'];
    g.loadServerNames(bytes(`${'x'.repeat(256)}\nnext`)); assert.deepEqual(g.servers.map((line) => line.length), [256, 4]);
    g.loadServerNames(bytes(`${'x'.repeat(255)}\r\nok`)); assert.deepEqual(g.servers.map((line) => line.length), [255, 2]);
    g.servers = ['keep']; assert.throws(() => g.loadServerNames(bytes(`short\n${'x'.repeat(257)}\n`)), { name: 'RangeError', message: /긴 원본 행/ }); assert.deepEqual(g.servers, ['keep']);
    assert.throws(() => g.loadServerNames(bytes(`${'x'.repeat(256)}\r\n`)), RangeError); assert.throws(() => g.loadServerNames(bytes('x'.repeat(257))), RangeError);
});

test('resolveServerName LAN: 선택 -1 이면 번역("LAN Server")을 NUL 앞까지 복사하고 NUL 을 쓰되 뒤의 기존 바이트는 남긴다', () => {
    const g = new ClientNsInfoGlobals(); g.serverSelection = -1; g.serverName.set(bytes('XXXXXXXXXX')); const seen = [];
    const view = g.resolveServerName((key) => { seen.push(key); return 'ab'; }); assert.deepEqual(seen, ['LAN Server']); assert.equal(text(view), 'ab'); assert.equal(text(g.serverName.subarray(0, 10)), 'ab\0XXXXXXX');
    assert.equal(view.buffer, g.serverName.buffer); assert.equal(text(g.resolveServerName(() => 'q\0zz')), 'q'); assert.equal(text(g.resolveServerName(() => bytes('uint8'))), 'uint8');
    assert.equal(text(g.resolveServerName(() => 'x'.repeat(79))).length, 79); assert.equal(g.serverName[79], 0);
    assert.throws(() => g.resolveServerName(() => 'x'.repeat(80)), { name: 'RangeError', message: /80BYTE/ }); assert.throws(() => g.resolveServerName(), { name: 'Error', message: /번역 미연결/ });
    assert.equal(text(g.resolveServerName(() => `${'y'.repeat(79)}\0${'z'.repeat(30)}`)).length, 79);
    assert.throws(() => g.resolveServerName(() => `${'y'.repeat(80)}\0`), RangeError);
});

test('resolveServerName 유효 행: 버퍼를 0 으로 채운 뒤 첫 세미콜론 뒤~다음 세미콜론 앞(없으면 끝)을 최대 79바이트 복사하고 NUL 앞 행만 본다', () => {
    const g = new ClientNsInfoGlobals(); g.servers = ['id;Name;rest', 'a;Tail', 'noSemicolon', ';;', 'a;Na\0me;x', `z;${'n'.repeat(100)}`, ';\xe9\xff;']; g.serverName.fill(0x41);
    g.serverSelection = 0; assert.equal(text(g.resolveServerName()), 'Name'); assert.ok(g.serverName.subarray(4).every((value) => value === 0));
    g.serverSelection = 1; assert.equal(text(g.resolveServerName()), 'Tail'); g.serverSelection = 3; assert.equal(text(g.resolveServerName()), '');
    g.serverName.fill(0x41); g.serverSelection = 2; assert.equal(text(g.resolveServerName()), ''); assert.ok(g.serverName.every((value) => value === 0));
    g.serverSelection = 4; assert.equal(text(g.resolveServerName()), 'Na'); g.serverSelection = 5; assert.equal(text(g.resolveServerName()), 'n'.repeat(79)); assert.equal(g.serverName[79], 0);
    g.serverSelection = 6; assert.deepEqual(Array.from(g.resolveServerName()), [0xe9, 0xff]);
});

test('resolveServerName NUL 경계: 번역/행이 NUL 로 시작하면 빈 이름이고, 행의 NUL 앞 첫 글자까지 세미콜론 검색에 포함된다', () => {
    const g = new ClientNsInfoGlobals(); g.serverSelection = -1; g.serverName.fill(0x41); assert.equal(text(g.resolveServerName(() => '\0abc')), ''); assert.equal(g.serverName[0], 0); assert.equal(g.serverName[1], 0x41);
    g.servers = ['\0;Name;x', ';XNa\0q', ';\0']; g.serverSelection = 0; g.serverName.fill(0x41); assert.equal(text(g.resolveServerName()), ''); assert.ok(g.serverName.every((value) => value === 0));
    g.serverSelection = 1; assert.equal(text(g.resolveServerName()), 'XNa'); g.serverSelection = 2; assert.equal(text(g.resolveServerName()), '');
});

test('resolveServerName 무효 선택: 목록 밖/음수(-1 제외)는 기존 바이트를 보존하고 번역기도 요구하지 않는다', () => {
    const g = new ClientNsInfoGlobals(); g.servers = ['a;One']; g.serverName.set(bytes('kept\0old'));
    for (const selection of [-2, 1, 2, 1000, -2147483648]) { g.serverSelection = selection; assert.equal(text(g.resolveServerName()), 'kept', String(selection)); assert.equal(text(g.serverName.subarray(0, 8)), 'kept\0old'); }
    g.servers = []; g.serverSelection = 0; assert.equal(text(g.resolveServerName()), 'kept'); g.serverName.fill(0x7a); assert.equal(g.resolveServerName().length, 80);
});

// ---------- HudNsInfo ----------

// NSInfo 부착 틀이다. 노드 1(NSInfo)과 배경 2/3 를 둔다.
function infoRig({ meta = 0, known = 0, layout = true, viewStyle = false } = {}) {
    const log = []; const nodes = new Map(); const ns = new HudNsInfoNode(); const background = new GumpCreateNode(); const other = new GumpCreateNode(); nodes.set(1, ns); nodes.set(2, background); nodes.set(3, other); ns.background = 2;
    const values = { metamap: meta, playerKnown: known };
    const hooks = {
        node: (token) => (token === 1 ? ns : null), base: (token) => { log.push(['base', token]); return nodes.get(token) ?? null; },
        baseAttach: (...args) => log.push(['baseAttach', ...args]), push: (token) => log.push(['push', token]), attach: (...args) => log.push(['attach', ...args]),
    };
    if (layout) hooks.metamapLayout = (token) => log.push(['layout', token]);
    const view = viewStyle ? { metamap: () => values.metamap, playerKnown: () => values.playerKnown } : values;
    return { info: new HudNsInfo(view, hooks), ns, background, other, nodes, hooks, log, values };
}

test('HudNsInfoNode: 새 노드의 배경/명단/제목/구분선/전투는 0 이고 플레이어 8개 토큰은 0 이다', () => {
    const node = new HudNsInfoNode(); assert.deepEqual([node.background, node.roster, node.title, node.upperDivider, node.lowerDivider, node.battle], [0, 0, 0, 0, 0, 0]);
    assert.equal(node.players.length, 8); assert.ok(node.players.every((value) => value === 0)); assert.ok(node instanceof GumpCreateNode);
});

test('HudNsInfo 생성자: 노드/배경/부착/Push/가상 부착이나 모드 조회가 없으면 TypeError 다', () => {
    const good = infoRig().hooks;
    for (const name of ['node', 'base', 'baseAttach', 'push', 'attach']) { const hooks = { ...good }; delete hooks[name]; assert.throws(() => new HudNsInfo({ metamap: 0, playerKnown: 0 }, hooks), { name: 'TypeError', message: /같은 노드/ }, name); }
    assert.throws(() => new HudNsInfo({ metamap: () => 0 }, good), TypeError); assert.throws(() => new HudNsInfo(null, good), TypeError); assert.throws(() => new HudNsInfo({ metamap: 0, playerKnown: 0 }, undefined), TypeError);
    assert.ok(new HudNsInfo({ metamap: () => 0, playerKnown: () => 0 }, good));
});

test('HudNsInfo.attach 순서/크기: 기본 부착 → Push → 폭/높이(배경 배율의 float32) → 배경 부착(0,0,token,0), 메타맵 꺼짐이면 끝', () => {
    const r = infoRig(); r.ns.baseWidth = 100.5; r.ns.baseHeight = 33; r.background.scale = Math.fround(0.3); r.info.attach(1, 5, 6, 7, 8);
    assert.deepEqual(r.log, [['baseAttach', 1, 5, 6, 7, 8], ['push', 2], ['base', 2], ['base', 2], ['attach', 2, 0, 0, 1, 0]]);
    assert.equal(r.background.baseWidth, 100.5); assert.equal(r.background.width, Math.fround(100.5 * Math.fround(0.3))); assert.equal(r.background.baseHeight, 33); assert.equal(r.background.height, Math.fround(33 * Math.fround(0.3)));
    // 큰 폭과 배율의 배정도 곱은 float32 로 한 번만 좁힌다.
    r.ns.baseWidth = 16777216; r.background.scale = 1.5; r.info.attach(1, 0, 0, 0, 0); assert.equal(r.background.width, 25165824);
});

test('HudNsInfo.attach 메타맵 관문: 메타맵과 플레이어 인식이 모두 켜져야 전체 배치를 부르고 매 호출 현재 값을 읽는다', () => {
    for (const viewStyle of [false, true]) {
        for (const [meta, known, expected] of [[0, 0, false], [1, 0, false], [0, 1, false], [1, 1, true], [0x100, 0x100, true]]) {
            const r = infoRig({ meta, known, viewStyle }); r.info.attach(1, 0, 0, 0, 0); assert.equal(r.log.some((entry) => entry[0] === 'layout'), expected, `${viewStyle} ${meta} ${known}`);
        }
        const live = infoRig({ viewStyle }); live.info.attach(1, 0, 0, 0, 0); assert.equal(live.log.at(-1)[0], 'attach'); live.values.metamap = 1; live.values.playerKnown = 1; live.log.length = 0; live.info.attach(1, 0, 0, 0, 0); assert.deepEqual(live.log.at(-1), ['layout', 1]);
    }
    const missing = infoRig({ meta: 1, known: 1, layout: false }); assert.throws(() => missing.info.attach(1, 0, 0, 0, 0), { name: 'Error', message: /메타맵 배치 미연결/ });
    const off = infoRig({ meta: 0, known: 1, layout: false }); off.info.attach(1, 0, 0, 0, 0);
});

test('HudNsInfo.attach 재조회/오류: Push 가 배경을 바꾸면 새 배경에 쓰고, 첫 조회가 배경을 바꾸면 둘째 조회와 부착이 새 값을 쓰며 저장소/배경이 없으면 Error 다', () => {
    const pushed = infoRig(); pushed.ns.baseWidth = 10; pushed.ns.baseHeight = 20; pushed.hooks.push = (token) => { pushed.log.push(['push', token]); pushed.ns.background = 3; }; pushed.other.scale = 2;
    pushed.info.attach(1, 0, 0, 0, 0); assert.deepEqual(pushed.log.map((entry) => entry.join(':')), ['baseAttach:1:0:0:0:0', 'push:2', 'base:3', 'base:3', 'attach:3:0:0:1:0']); assert.deepEqual([pushed.other.width, pushed.other.height], [20, 40]); assert.equal(pushed.background.width, 0);
    const flip = infoRig(); flip.ns.baseWidth = 4; flip.ns.baseHeight = 8; let calls = 0; const base = flip.hooks.base; flip.hooks.base = (token) => { calls += 1; if (calls === 1) flip.ns.background = 3; return base(token); }; flip.other.scale = 1; flip.background.scale = 1;
    flip.info.attach(1, 0, 0, 0, 0); assert.deepEqual([flip.background.baseWidth, flip.other.baseHeight, flip.background.baseHeight], [4, 8, 0]); assert.deepEqual(flip.log.at(-1), ['attach', 3, 0, 0, 1, 0]);
    assert.throws(() => infoRig().info.attach(0, 0, 0, 0, 0), { name: 'Error', message: /저장소 없음/ }); assert.throws(() => infoRig().info.attach(9, 0, 0, 0, 0), /저장소 없음/);
    const none = infoRig(); none.ns.background = 0; assert.throws(() => none.info.attach(1, 0, 0, 0, 0), { name: 'Error', message: /현재 배경 없음/ }); assert.deepEqual(none.log.map((entry) => entry[0]), ['baseAttach', 'push']);
    const gone = infoRig(); gone.ns.background = 9; assert.throws(() => gone.info.attach(1, 0, 0, 0, 0), /현재 배경 없음/);
});

// ---------- HudNsInfoCreate ----------

// 생성 기록 틀이다. 할당은 크기에 맞는 실제 파생 클래스를 만들어 100번부터 토큰을 주고, 기본 생성은 실제 GumpCreate 가 한다.
function createRig({ failures = {}, readOverrides = {}, bogus = false } = {}) {
    const log = []; const nodes = new Map(); const createState = new GumpCreateState(); const factory = new GumpCreate(createState, { node: (token) => nodes.get(token) ?? null });
    const boxes = new Map(); const current = { value: 0xdead }; let next = 100; if (bogus) nodes.set(1, new GumpCreateNode());
    const classes = new Map([[0xd8, HudNsInfoNode], [0xbc, HudPlayerInfoNode], [0x16c, HudBattleInfoNode], [0xc8, HudPlayerButtonNode]]);
    const reads = { 0x540421: 6, 0x540440: 37, 0x540457: 60, 0x5ca924: 0x11, 0x5ca920: 0x22, 0x540bac: 0x50c894, 0x5467b8: 0xfffffffe, 0x5411c8: 0x33, 0x5bec3c: 0x44, 0x594fc8: 0, 0x595344: 0, ...readOverrides };
    const alloc = (size) => { const fail = failures[size]; const count = (alloc.counts[size] = (alloc.counts[size] ?? 0) + 1); if (fail === count || fail === true) { log.push(`alloc ${size.toString(16)} -> 0`); return 0; } const token = next; next += 1; nodes.set(token, new (classes.get(size) ?? GumpCreateNode)()); log.push(`alloc ${size.toString(16)} -> ${token}`); return token; };
    alloc.counts = {};
    const flagBox = (token) => { if (!boxes.has(token)) boxes.set(token, { value: 0x100 }); return boxes.get(token); };
    const construct = (name) => (token, ...args) => { log.push(`${name} ${[token, ...args].join(',')}`); factory.construct(token, 0, 0, 0, 1); return token; };
    const hooks = {
        node: (token) => nodes.get(token) ?? null, player: (token) => nodes.get(token) ?? null, battle: (token) => nodes.get(token) ?? null, button: (token) => nodes.get(token) ?? null,
        base: (token) => nodes.get(token) ?? null, textFlags: (token) => { log.push(`textFlags ${token}`); return flagBox(token); }, buttonFlags: (token) => { log.push(`buttonFlags ${token}`); return flagBox(token); },
        read: (address) => { log.push(`read ${address.toString(16)}`); return reads[address] ?? 0; }, allocate: alloc,
        baseConstruct: (token, ...args) => { log.push(`baseConstruct ${[token, ...args].join(',')}`); factory.construct(token, ...args); },
        background: construct('background'), scroll: construct('scroll'), text: construct('text'), stone: construct('stone'),
        buttonBase: (token, ...args) => { log.push(`buttonBase ${[token, ...args].join(',')}`); factory.construct(token, 0, 0, 0, 1); },
        scrollable: (token, value) => log.push(`scrollable ${token},${value}`), slider: (token, value) => log.push(`slider ${token},${value}`), wrap: (token, value) => log.push(`wrap ${token},${value}`),
        parseScroll: (token, value) => log.push(`parseScroll ${token},${value.toString(16)}`), parseText: (token, value) => log.push(`parseText ${token},${value.toString(16)}`),
        fitText: (...args) => log.push(`fitText ${args.join(',')}`), fitStone: (token) => log.push(`fitStone ${token}`),
        attach: (...args) => log.push(`attach ${args.join(',')}`), baseAttach: (...args) => log.push(`baseAttach ${args.join(',')}`),
        translate: (token) => { log.push(`translate ${token.toString(16)}`); return token + 0x1000; }, type: (value) => { log.push(`type ${value.toString(16)}`); return readOverrides.noType ? 0 : value + 1; },
        serverName: () => { log.push('serverName'); return 0x600000; }, formatZone: (format, server, zone) => { log.push(`formatZone ${format.toString(16)},${server.toString(16)},${zone}`); return 0x700000; },
        assertion: (line) => log.push(`assertion ${line}`), metamapLayout: (kind, token) => log.push(`layout ${kind},${token}`),
    };
    return { create: new HudNsInfoCreate(current, hooks), hooks, log, nodes, current, createState, boxes, reads };
}

test('HudPlayerInfoNode/HudBattleInfoNode/HudPlayerButtonNode: 새 노드의 미작성 필드와 22개 설정 쌍은 모두 0 이다', () => {
    const player = new HudPlayerInfoNode(); assert.deepEqual([player.text, player.master, player.button, player.start, player.player, player.buttonType, player.reserved], [0, 0, 0, 0, 0, 0, 0]);
    const battle = new HudBattleInfoNode(); assert.deepEqual([battle.title, battle.summary, battle.body, battle.battle, battle.changed, battle.mode], [0, 0, 0, 0, 0, 0]); assert.deepEqual(battle.settings, Array.from({ length: 22 }, () => [0, 0])); assert.deepEqual(Array.from(battle.bytes), [0, 0, 0, 0]);
    const button = new HudPlayerButtonNode(); assert.deepEqual([button.untouchedC0, button.entry], [0, 0]); assert.ok(button instanceof ButtonGumpNode);
});

// 손으로 펼친 같은 원문 순서의 기대 기록 조각이다(기준선 HudNsInfoCreate.cpp 의 각 생성자를 따로 읽어 쓴다).
const expectBackground = (token, flags) => [`alloc b8 -> ${token}`, 'read 5ca924', 'read 5ca920', `background ${token},1,1,${flags},1,34,17`];
const expectScroll = (token) => [`alloc d8 -> ${token}`, `scroll ${token},0,0,0,0`];
const expectText = (token) => [`alloc a4 -> ${token}`, `text ${token},0,0,0,0`];

test('HudNsInfoCreate.createHidden: 전체 자식 생성 순서(NSInfo→배경→명단→제목→구분선 2→전투 정보와 세 글→플레이어 8개와 각 자식)를 원문 순서 그대로 따른다', () => {
    const r = createRig(); const token = r.create.createHidden(); assert.equal(token, 100); assert.equal(r.current.value, 100);
    const expected = ['alloc d8 -> 100', 'baseConstruct 100,0,0,0,1', 'read 540421'];
    expected.push(...expectBackground(101, 0x02000000), 'attach 101,0,0,100,0');
    expected.push(...expectScroll(102), 'textFlags 102', 'scrollable 102,1', 'slider 102,1', 'attach 102,0,0,101,0');
    expected.push(...expectScroll(103), 'read 540bac', 'translate 50c894', 'read 5467b8', 'serverName', 'formatZone 50d894,600000,-2', 'parseScroll 103,700000', 'attach 103,0,0,101,0');
    expected.push(...expectBackground(104, 0x02a00000), 'attach 104,0,0,101,0', ...expectBackground(105, 0x02a00000), 'attach 105,0,0,101,0');
    expected.push('alloc 16c -> 106', 'baseConstruct 106,0,0,0,1', 'read 540457', ...expectScroll(107), 'scrollable 107,0', 'wrap 107,0', 'attach 107,0,0,106,0', ...expectText(108), 'fitText 108,0,0', 'attach 108,0,0,106,0',
        ...expectScroll(109), 'scrollable 109,0', 'wrap 109,0', 'attach 109,0,0,106,0', 'attach 106,0,0,101,0');
    // 플레이어 8개는 각각 부모 생성(할당 0xbc → 스크롤 글 → 버튼 → 마스터 글 → Start 돌)과 NSInfo 배경 부착이다.
    let player = 110;
    for (let index = 0; index < 8; index += 1) {
        const text = player + 1; const button = player + 2; const master = player + 3; const stone = player + 4;
        expected.push(`alloc bc -> ${player}`, `baseConstruct ${player},0,0,0,1`, 'read 540440', ...expectScroll(text), `scrollable ${text},0`, 'read 5411c8', 'type 33', `alloc c8 -> ${button}`, `buttonBase ${button},16,15,0`, `buttonFlags ${button}`,
            ...expectText(master), 'translate 50cd3c', `parseText ${master},50dd3c`, `fitText ${master},0,0`, `alloc 170 -> ${stone}`, 'translate 50cd2c', 'read 5bec3c', `stone ${stone},1,1,${0x50dd2c},68,0`, `fitStone ${stone}`,
            `attach ${text},0,0,${player},0`, `attach ${button},0,0,${player},0`, `attach ${stone},0,0,${player},0`, `attach ${master},0,0,${player},0`, `attach ${player},0,0,101,0`);
        player += 5;
    }
    assert.deepEqual(r.log, expected);
});

test('HudNsInfoCreate.construct 결과 필드: 종류 BYTE 는 부호 확장, 가상 표와 0x800, 자식 토큰/visible 비트는 원문대로이고 createHidden 만 NSInfo 를 숨긴다', () => {
    const r = createRig({ readOverrides: { 0x540421: 0x80, 0x540440: 0xff, 0x540457: 0x7f } }); const token = r.create.createHidden(); const ns = r.nodes.get(token);
    assert.deepEqual([ns.vtable, ns.command, ns.background, ns.roster, ns.title, ns.upperDivider, ns.lowerDivider, ns.battle], [0x50d074, 0xffffff80, 101, 102, 103, 104, 105, 106]);
    assert.deepEqual(Array.from(ns.players), [110, 115, 120, 125, 130, 135, 140, 145]); assert.equal(ns.flags & 0x800, 0x800); assert.equal(ns.flags & 1, 0);
    // 원문이 끄는 자식: 명단/제목/위·아래 구분선/전투 정보. 배경/플레이어/나머지 글은 켜진 채로 둔다.
    for (const child of [102, 103, 104, 105, 106]) assert.equal(r.nodes.get(child).flags & 1, 0, `child ${child}`); for (const child of [101, 107, 108, 109, 110, 111, 112, 113, 114]) assert.equal(r.nodes.get(child).flags & 1, 1, `kept ${child}`);
    assert.equal(r.boxes.get(102).value, 0x108); const player = r.nodes.get(110); assert.deepEqual([player.vtable, player.command, player.reserved, player.player, player.text, player.button, player.master, player.start, player.buttonType], [0x50cd64, 0xffffffff, 0, 0, 111, 112, 113, 114, 0x34]);
    assert.equal(r.boxes.get(112).value, 0x106); assert.equal(player.flags & 0x800, 0); const battle = r.nodes.get(106); assert.deepEqual([battle.vtable, battle.command, battle.title, battle.summary, battle.body, battle.mode, battle.battle, battle.changed], [0x50ce00, 127, 107, 108, 109, 2, 0, 0]);
    assert.equal(battle.flags & 0x800, 0x800);
    // 숨김 없이 construct 만 호출하면 NSInfo 의 visible 비트는 켜진 채다.
    const plain = createRig(); const token2 = plain.create.construct(plain.hooks.allocate(0xd8)); assert.equal(plain.nodes.get(token2).flags & 1, 1); assert.equal(plain.current.value, 0xdead);
});

test('HudNsInfoCreate.createHidden 할당 실패: 전체 생성을 하지 않고 current 를 0 으로 쓰며 0 을 돌려준다', () => {
    const r = createRig({ failures: { 0xd8: 1 } }); assert.equal(r.create.createHidden(), 0); assert.equal(r.current.value, 0); assert.deepEqual(r.log, ['alloc d8 -> 0']);
    assert.throws(() => new HudNsInfoCreate({ value: 1 }, {}).createHidden(), { name: 'Error', message: /실제 생성\/자식 효과 미연결/ });
});

test('HudNsInfoCreate 자식 할당 실패: 하위 생성자를 건너뛰고 0 토큰 부착은 실제 저장소 없음 Error 다(각 위치)', () => {
    for (const [size, which, label] of [[0xb8, 1, '배경'], [0x16c, 1, '전투 정보'], [0xbc, 1, '플레이어'], [0x170, 1, 'Start 돌'], [0xc8, 1, '버튼'], [0xa4, 1, '마스터 글']]) {
        const r = createRig({ failures: { [size]: which } });
        if (size === 0xc8 || size === 0xa4) { assert.throws(() => r.create.createHidden(), Error, label); continue; }
        assert.throws(() => r.create.createHidden(), { name: 'Error', message: /실제 파생 저장소 없음/ }, label);
    }
    // 실패한 할당이 0 이 아닌 토큰으로 바뀌어도 통과하지 않도록 토큰 1 에 실제 노드가 있는 틀에서도 같은 위치가 거부되어야 한다(둘째 0xd8 = 명단 글).
    for (const [size, which] of [[0xb8, 1], [0xd8, 2], [0xd8, 3], [0xb8, 2], [0x16c, 1], [0xbc, 1]]) assert.throws(() => createRig({ failures: { [size]: which }, bogus: true }).create.createHidden(), { name: 'Error', message: /실제 파생 저장소 없음/ }, size.toString(16) + ':' + which);
    // 배경 할당 실패: 배경 생성자를 부르지 않는다.
    const bg = createRig({ failures: { 0xb8: 1 } }); assert.throws(() => bg.create.createHidden()); assert.equal(bg.log.some((entry) => entry.startsWith('background')), false);
    // 버튼 할당 실패는 +ac 플래그 상자를 0 토큰으로 요구한다(훅이 받아서 처리한다).
    const button = createRig({ failures: { 0xc8: 1 } }); const calls = []; const flagHook = button.hooks.buttonFlags; button.hooks.buttonFlags = (token) => { calls.push(token); return flagHook(token); };
    assert.throws(() => button.create.createHidden(), /실제 파생 저장소 없음/); assert.deepEqual(calls, [0]);
});

test('HudNsInfoCreate.constructPlayer: 버튼 TYPE 이 0 이면 진단 1912 를 알리고 계속하며, Start 돌 할당 실패는 start 0 으로 fitStone(0) 뒤 부착에서 거부된다', () => {
    const r = createRig({ readOverrides: { noType: true } }); const token = r.hooks.allocate(0xbc); r.log.length = 0; assert.equal(r.create.constructPlayer(token), token);
    const player = r.nodes.get(token); assert.equal(player.buttonType, 0); assert.ok(r.log.includes('assertion 1912')); assert.equal(r.log.indexOf('assertion 1912') < r.log.findIndex((entry) => entry.startsWith('alloc c8')), true);
    const stoneless = createRig({ failures: { 0x170: 1 } }); const id = stoneless.hooks.allocate(0xbc); stoneless.log.length = 0; assert.throws(() => stoneless.create.constructPlayer(id), /실제 파생 저장소 없음/);
    assert.equal(stoneless.nodes.get(id).start, 0); assert.ok(stoneless.log.includes('fitStone 0')); assert.equal(stoneless.log.some((entry) => entry.startsWith('translate 50cd2c')), false);
});

test('HudNsInfoCreate.constructBattle: 22개 설정 쌍은 [0,1], 모드 2, 첫 BYTE 만 0 으로 쓰고 나머지 BYTE/기본 필드는 보존한다', () => {
    const r = createRig(); const token = r.hooks.allocate(0x16c); const node = r.nodes.get(token); node.bytes.set([7, 8, 9, 10]); for (const setting of node.settings) { setting[0] = 5; setting[1] = 6; } node.battle = 3; node.changed = 4; node.mode = 99;
    assert.equal(r.create.constructBattle(token), token); assert.equal(node.settings.length, 22); assert.ok(node.settings.every((setting) => setting[0] === 0 && setting[1] === 1)); assert.deepEqual(Array.from(node.bytes), [0, 8, 9, 10]);
    assert.deepEqual([node.mode, node.battle, node.changed, node.vtable, node.command], [2, 0, 0, 0x50ce00, 60]); assert.equal(new HudBattleInfoNode().settings.length, 22); assert.equal(new HudBattleInfoNode().mode, 0);
});

test('HudNsInfoCreate.constructButton: 기본 버튼 생성 뒤 가상 표와 +c4 만 쓰고 +c0 등 미작성 필드는 보존한다', () => {
    const r = createRig(); const token = r.hooks.allocate(0xc8); const node = r.nodes.get(token); node.untouchedC0 = 0x55; node.entry = 9; node.value = 77; r.log.length = 0;
    assert.equal(r.create.constructButton(token, 5, 6), token); assert.deepEqual(r.log, ['buttonBase 100,5,6,0']); assert.deepEqual([node.vtable, node.entry, node.untouchedC0], [0x5038e8, 0, 0x55]); assert.ok(node instanceof ButtonGumpNode);
    assert.throws(() => r.create.constructButton(0, 1, 1), /실제 파생 저장소 없음/); assert.throws(() => r.create.constructButton(555, 1, 1), /실제 파생 저장소 없음/);
});

test('HudNsInfoCreate 일반 부착: 공통 부착 뒤에만 메타맵을 읽고 둘 다 켜진 경우에만 종류/토큰으로 전체 배치를 부른다', () => {
    for (const [method, kind] of [['attachPlayer', HudNsInfoChild.Player], ['attachBattle', HudNsInfoChild.Battle]]) {
        const off = createRig(); off.create[method](7, 1.5, 2.5, 3, 4); assert.deepEqual(off.log, ['baseAttach 7,1.5,2.5,3,4', 'read 594fc8']);
        const half = createRig({ readOverrides: { 0x594fc8: 1 } }); half.create[method](7, 0, 0, 0, 0); assert.deepEqual(half.log, ['baseAttach 7,0,0,0,0', 'read 594fc8', 'read 595344']);
        const both = createRig({ readOverrides: { 0x594fc8: 2, 0x595344: 1 } }); both.create[method](7, 0, 0, 0, 0); assert.deepEqual(both.log.at(-1), `layout ${kind},7`);
        const miss = createRig({ readOverrides: { 0x594fc8: 2, 0x595344: 1 } }); delete miss.hooks.metamapLayout; assert.throws(() => miss.create[method](7, 0, 0, 0, 0), { name: 'Error', message: /미연결/ });
        const noBase = createRig(); delete noBase.hooks.baseAttach; assert.throws(() => noBase.create[method](7, 0, 0, 0, 0), /미연결/);
    }
    assert.deepEqual(HudNsInfoChild, { Player: 0, Battle: 1 });
});

test('HudNsInfoCreate 필수 효과: 도달한 위치의 하위 효과가 없으면 Error 다(각 생성 훅)', () => {
    for (const name of ['allocate', 'baseConstruct', 'read', 'background', 'scroll', 'text', 'stone', 'buttonBase', 'scrollable', 'slider', 'wrap', 'parseScroll', 'parseText', 'fitText', 'fitStone', 'attach', 'translate', 'type', 'serverName', 'formatZone', 'textFlags', 'buttonFlags', 'base', 'node', 'player', 'battle', 'button']) {
        const r = createRig(); delete r.hooks[name]; assert.throws(() => new HudNsInfoCreate({ value: 0 }, r.hooks).createHidden(), { name: 'Error', message: /미연결/ }, name);
    }
    // 진단 훅은 TYPE 이 0 일 때만 필요하다.
    const r = createRig(); delete r.hooks.assertion; assert.ok(new HudNsInfoCreate({ value: 0 }, r.hooks).createHidden());
});
