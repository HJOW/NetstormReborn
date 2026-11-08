// 건설 배치 raw-construction-place.js(RawConstructionPlace·ConstructionPlaceState·makeConstructionPlaceHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionplace-x86.tsv 의 `originals` 1778행 — Place 1246행(타입·인자·방향·좌표·플레이어·로컬·차단·비용·잔액·서버·권한·전투 표시·표시 비트·abstract·품질·SID 윤곽 3종·슬롯 채움 3종·
//    프레임 표 4종·flags·최대 HP·제단 HP·기본 프레임·수명)·Charge 292·Refund 144·Reject 24·Money 48·Add 24 에서 하위 효과 순서(T 수신/N 표면 알림/O 소유자/P Pop/M 통지/G 저장소 읽기/L·A SP 가산/F·X·Y·D 문구)와
//    조각 슬롯 전체 바이트, Player 표시 필드·잔액 비트가 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 풀 불일치·필수 경계 누락 거부, 서버 번호·SID 개수 불일치·void 아님의 거부(효과 전), 실제 wire 경로(actualPlace)가 같은 입력의 typed 경로와 같은 효과·슬롯을 만들고 진단 4종
//    (player 9·isVoid·다리 품질 프레임 없음·SID 개수)을 정상 복귀 뒤 계속함, 실제 Player 기록 연결(표시 비트·이름), 어댑터의 풀 불일치 거부와 수신/소유자/SP 연결.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E138: 건설 배치 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { ConstructionPlaceState, RawConstructionPlace, makeConstructionPlaceHooks } from '../www/core/raw-construction-place.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E138)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionplace-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[1] === 'originals');

// 판본별 관측 행 수, 문구 조립 경계가 돌려주는 고정 토큰, 일반/제단 타입 번호, 가상 표 기록값, 풀 크기, 서버 영역 첫 번호, 플레이어 수이다. (2026-11-09 WB04-E138)
const ROWS = 1778;
const TOKEN = 'cheat-token';
const PLAIN = 100;
const ALTAR = 164;
const VTABLE = 0x11010000;
const CAPACITY = 24000;
const SERVER_FIRST = 15000;
const PLAYERS = 8;

// 예외가 나는지 본다. (2026-11-09 WB04-E138)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트를 소문자 16진수로 만든다. (2026-11-09 WB04-E138)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실행기의 frame_codes 와 같은 프레임 코드 표 입력이다. 0=기본, 1=역순, 2=부호 있는 플래그, 3=품질 프레임 없음. (2026-11-09 WB04-E138)
function inputFrames(profile) {
    const codes = [];
    const base = profile === 2 ? 0x80 : 0;
    // 방향 글자 A~P마다 번호 0~10과 품질 플래그 프레임 둘을 둔다.
    for (let side = 65; side <= 80; side += 1) {
        for (let number = 0; number <= 10; number += 1) codes.push({ side, variant: 80, number, flags: base });
        if (profile !== 3) {
            codes.push({ side, variant: 80, number: 11, flags: base | 0x20 });
            codes.push({ side, variant: 80, number: 12, flags: base | 0x40 });
        }
    }
    if (profile === 1) codes.reverse();
    codes.push(codes[0]);
    return codes;
}

// 배치 전 void 슬롯 입력이다. 0=0으로 채움, 1=0xff로 채움, 2=위치마다 다른 값. 상태는 void 를 켜고 free 를 끈다. (2026-11-09 WB04-E138)
function fillSlot(raw, profile, type) {
    for (let index = 0; index < raw.length; index += 1) raw[index] = profile === 0 ? 0 : profile === 1 ? 0xff : (index * 37 + 11) & 0xff;
    put(raw, 0, VTABLE);
    raw[10] = type & 0xff;
    raw[11] = (raw[11] | 4) & 0xfe;
}

// 조각 순서의 SID 입력이다. 0=클라이언트 영역, 1=서버 영역, 2=번갈아 섞음. (2026-11-09 WB04-E138)
function sidOf(profile, index) {
    const local = profile === 0 || (profile === 2 && index % 2 === 0);
    return (local ? 50 : SERVER_FIRST) + index;
}

// Player 표시 필드 입력이다. 홀수 번호는 0x400 밖의 비트를 모두 켠다. (2026-11-09 WB04-E138)
function playerFlags(player, flagged) {
    return (((player % 2 ? 0xfffffbff : 0) | (flagged ? 0x400 : 0)) >>> 0);
}

// 타입·프레임·전역·경계를 행마다 다시 준비하는 재생 장면이다. (2026-11-09 WB04-E138)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, maxHitPoints: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 }));
        this.frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
        this.state = new ConstructionPlaceState();
        this.books = new SquidPostPopState();
        this.rewardState = new SquidRewardState();
        this.rng = new GameRandom();
        this.store = new ScrambledSpStore(this.rng, () => 1);
        this.events = [];
        this.money = 0;
    }

    // 한 타입의 레코드와 프레임 표, 제단 HP, 전역 입력을 실행기의 setup 과 같게 준비한다. (2026-11-09 WB04-E138)
    prepare(type, cost, flags1, flags2, maxHp, altarHp, defaultFrame, frameProfile, local, blocked, server, authority, shown, flagged, life) {
        for (const record of this.types) Object.assign(record, { cost: 0, maxHitPoints: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 });
        for (const meta of this.frames) Object.assign(meta, { frames: [], defaultFrame: 0 });
        this.events = [];
        const record = this.types[type];
        record.cost = bitsToFloat(cost);
        record.flags1 = flags1;
        record.flags2 = flags2;
        record.maxHitPoints = maxHp | 0;
        this.types[ALTAR].maxHitPoints = altarHp | 0;
        this.frames[type] = { frames: inputFrames(frameProfile), defaultFrame: defaultFrame | 0 };
        this.state = new ConstructionPlaceState();
        Object.assign(this.state, { localPlayer: local, chargeBlocked: blocked, server, authority, battleShown: shown, bridgeLife: life | 0 });
        // Player 이름과 표시 비트를 번호마다 넣는다.
        for (let player = 0; player <= PLAYERS; player += 1) {
            this.state.playerNames[player] = `P${player}`;
            this.state.playerFlags[player] = playerFlags(player, flagged);
        }
    }

    // 실행기의 경계와 같은 일을 한다: 인자를 기록하고, 소유자 지정은 소유자 바이트만 쓰며, 수신은 같은 번호를 돌려준다. (2026-11-09 WB04-E138)
    hooks() {
        const bits = floatToBits;
        const le = (value, width) => Array.from({ length: width }, (_, index) => Math.floor(value / 2 ** (8 * index)) & 0xff);
        return {
            take: (type, sid) => { this.events.push(`T:${type}:${sid}`); return sid; },
            notifySurface: (sid) => this.events.push(`N:${sid}`),
            setOwner: (sid, player) => { this.pool.physical(sid)[34] = player & 0xff; this.events.push(`O:${sid}:${player}`); },
            pop: (sid, x, y, flags) => this.events.push(`P:${sid}:${bits(x)}:${bits(y)}:${flags}`),
            sendMoney: (target, notice) => this.events.push(`M:${target}:money:${hex([notice.player, notice.source, notice.kind, ...le(notice.amount >>> 0, 4)])}`),
            storedSp: (owner) => { this.events.push(`G:${owner}`); return bitsToFloat(this.money); },
            addLocalSp: (delta) => this.events.push(`L:${bits(delta)}`),
            addPlayerSp: (owner, delta) => this.events.push(`A:${owner}:${bits(delta)}`),
            format: (key, name) => { this.events.push(`F:${key}:${name}`); return TOKEN; },
            tellOthers: (player, text) => this.events.push(`X:${player}:${text}`),
            tellPlayer: (player, text) => this.events.push(`Y:${player}:${text}`),
            tell: (text) => this.events.push(`D:${text}`),
            sendReject: (target, notice) => this.events.push(`M:${target}:reject:${hex([...le(bits(notice.x), 4), ...le(bits(notice.y), 4), notice.type, notice.argument, notice.direction])}`),
        };
    }

    // 현재 타입 표로 새 보상 모듈과 배치 몸체를 만든다(보상은 생성 때 타입 표를 복사한다). (2026-11-09 WB04-E138)
    place(hooks = this.hooks()) {
        const reward = new SquidReward(this.pool, this.types, this.books, this.rewardState, this.store);
        return new RawConstructionPlace(this.pool, this.types, this.frames, this.state, reward, hooks);
    }
}

test('배치: 원본 10.78 관측 1778행(Place·Charge·Refund·Reject·Money·Add)의 효과 순서·슬롯·부가 결과가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const scene = new Scene();
    const counts = {};
    // 연산 종류에 따라 입력 칸을 풀어 같은 호출을 한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const op = row[0];
        counts[op] = (counts[op] ?? 0) + 1;
        const v = row[2].split(',').map(Number);
        let slots = '-';
        let extra = '-';
        if (op === 'Place') {
            assert.equal(v.length, 26);
            scene.prepare(v[0], v[8], v[19], v[20], v[21], v[22], v[23], v[18], v[6], v[7] !== 0, v[10] !== 0, v[11] !== 0, v[12] !== 0, v[13] !== 0, v[24]);
            scene.money = v[9];
            const sids = [];
            // 실제 디코더가 센 조각 수만큼 void 슬롯 입력을 준비한다.
            for (let index = 0; index < v[25]; index += 1) {
                sids.push(sidOf(v[16], index));
                fillSlot(scene.pool.physical(sids[index]), v[17], v[0]);
            }
            scene.place().place({ type: v[0], x: bitsToFloat(v[3]), y: bitsToFloat(v[4]), argument: v[1], direction: v[2], player: v[5], sids, abstract: v[14], quality: v[15] });
            // 조각 순서대로 슬롯 전체를 16진수로 잇는다.
            if (sids.length > 0) slots = sids.map((sid) => hex(scene.pool.slot(sid))).join(',');
            const player = v[5] | 0;
            if (player > 0 && player < 9) extra = String(scene.state.playerFlags[v[5]]);
        } else {
            // 직접 호출은 한 칸 타입의 비용과 전역만 쓴다. 나머지 입력은 실행기의 기본값과 같다.
            const typed = op === 'Charge' || op === 'Refund' || op === 'Reject';
            const type = typed ? v[0] : PLAIN;
            const local = op === 'Money' || op === 'Add' ? v[1] : v[2];
            const cost = op === 'Charge' ? v[6] : typed ? v[4] : 0;
            const authority = op === 'Charge' && v[3] !== 0;
            const flagged = op === 'Charge' && v[4] !== 0;
            const shown = op === 'Charge' ? v[5] !== 0 : op === 'Money' ? v[2] !== 0 : typed && v[3] !== 0;
            scene.prepare(type, cost, 0, 0, 0, 0, 0, 0, local, false, true, authority, shown, flagged, 5);
            const place = scene.place();
            if (op === 'Charge') { place.chargeShortfall(v[0], v[1], bitsToFloat(v[7]), bitsToFloat(v[8]), v[9], v[10]); extra = String(scene.state.playerFlags[v[1]]); }
            else if (op === 'Refund') place.refund(v[0], v[1]);
            else if (op === 'Reject') place.reject(v[0], v[1], bitsToFloat(v[5]), bitsToFloat(v[6]), v[7], v[8]);
            else if (op === 'Money') { scene.money = v[3]; extra = String(floatToBits(place.money(v[0]))); }
            else { assert.equal(op, 'Add'); place.addMoney(v[0], bitsToFloat(v[2])); }
        }
        const events = scene.events.length === 0 ? '-' : scene.events.join(';');
        assert.equal(events, row[3], `${op} ${row[2]}`);
        assert.equal(slots, row[4], `${op} ${row[2]}`);
        assert.equal(extra, row[5], `${op} ${row[2]}`);
    }
    assert.deepEqual(counts, { Place: 1246, Charge: 292, Refund: 144, Reject: 24, Money: 48, Add: 24 });
});

test('배치: 풀 불일치·필수 경계 누락을 거부하고 서버 번호·SID 개수 불일치는 효과 전에 거부한다', () => {
    const scene = new Scene();
    scene.prepare(PLAIN, floatToBits(250), 0x10, 0, 120, 0, 3, 0, 1, false, true, true, false, false, 5);
    for (const name of ['take', 'notifySurface', 'setOwner', 'pop', 'sendMoney', 'storedSp', 'addLocalSp', 'addPlayerSp', 'format', 'tellOthers', 'tellPlayer', 'tell', 'sendReject']) {
        assert.ok(throws(() => scene.place({ ...scene.hooks(), [name]: undefined })), name);
    }
    const reward = new SquidReward(scene.pool, scene.types, scene.books, scene.rewardState, scene.store);
    const otherPool = new SidPool(CAPACITY, { server: false });
    assert.ok(throws(() => new RawConstructionPlace(otherPool, scene.types, scene.frames, scene.state, reward, scene.hooks())));
    assert.ok(throws(() => new RawConstructionPlace(scene.pool, scene.types.slice(0, 171), scene.frames, scene.state, reward, scene.hooks())));
    const place = scene.place();
    assert.equal(place.pool, scene.pool);
    fillSlot(scene.pool.physical(50), 0, PLAIN);
    const base = { type: PLAIN, x: 20, y: 21, argument: 0, direction: 0, player: 2, abstract: 0, quality: 0 };
    // 서버 번호 9, 타입 번호 범위 밖, 조각 수와 다른 SID 개수는 효과 전에 거부한다.
    assert.ok(throws(() => place.place({ ...base, player: 9, sids: [50] })));
    assert.ok(throws(() => place.place({ ...base, type: 300, sids: [50] })));
    assert.ok(throws(() => place.place({ ...base, sids: [] })));
    assert.ok(throws(() => place.place({ ...base, sids: [50, 51] })));
    assert.deepEqual(scene.events, []);
    // void 가 아닌 조각은 쓰기 전에 거부한다(비용 판정의 SP 효과는 이미 일어난다).
    scene.pool.physical(50)[11] &= ~4;
    assert.ok(throws(() => place.place({ ...base, player: 1, sids: [50] })));
});

// 값형 요청을 실제 wire 경로로 돌려 typed 경로와 같은 입력에서 효과·슬롯을 비교한다. (2026-11-09 WB04-E138)
function runBoth(v) {
    const make = () => {
        const scene = new Scene();
        scene.prepare(v[0], v[8], v[19], v[20], v[21], v[22], v[23], v[18], v[6], v[7] !== 0, v[10] !== 0, v[11] !== 0, v[12] !== 0, v[13] !== 0, v[24]);
        scene.money = v[9];
        const sids = [];
        for (let index = 0; index < v[25]; index += 1) {
            sids.push(sidOf(v[16], index));
            fillSlot(scene.pool.physical(sids[index]), v[17], v[0]);
        }
        return { scene, sids };
    };
    const request = { type: v[0], x: bitsToFloat(v[3]), y: bitsToFloat(v[4]), argument: v[1], direction: v[2], player: v[5], abstract: v[14], quality: v[15] };
    const typed = make();
    typed.scene.place().place({ ...request, sids: typed.sids });
    const wire = make();
    wire.scene.place({ ...wire.scene.hooks(), assertion: () => { throw new Error('진단이 불리면 안 된다'); } }).actualPlace({ ...request, sids: [] }, wire.sids.length, (index) => wire.sids[index]);
    return { typed, wire };
}

test('배치: 실제 wire 경로(actualPlace)가 같은 입력의 typed 경로와 같은 효과·슬롯을 만든다', { skip: rows.length === 0 }, () => {
    let compared = 0;
    // 정상 관측(진단이 필요 없는 입력)을 모두 바이트 경로로 재생한다.
    for (const row of rows) {
        if (row[0] !== 'Place') continue;
        const v = row[2].split(',').map(Number);
        // 서버 번호·다리 품질 프레임 없음 입력은 진단 경로 시험에서 다룬다.
        if (v[5] === 9 || (v[0] === 82 && v[18] === 3 && (v[15] === 1 || v[15] === 3))) continue;
        let expected;
        try { expected = runBoth(v); } catch { continue; }
        assert.equal(expected.wire.scene.events.join(';'), expected.typed.scene.events.join(';'), row[2]);
        assert.deepEqual(expected.wire.sids.map((sid) => hex(expected.wire.scene.pool.slot(sid))), expected.typed.sids.map((sid) => hex(expected.typed.scene.pool.slot(sid))), row[2]);
        compared += 1;
    }
    assert.ok(compared > 1000, `비교 ${compared}`);
});

test('배치: 실제 wire 경로의 진단 4종은 Client 진단 장치에 보고하고 복귀하면 같은 순서로 계속한다', () => {
    const scene = new Scene();
    scene.prepare(PLAIN, floatToBits(0), 0x10, 0, 120, 0, 3, 0, 1, false, true, true, false, false, 5);
    const reports = [];
    const hooks = { ...scene.hooks(), assertion: (expression, file, line) => reports.push([expression, file, line]) };
    const place = scene.place(hooks);
    fillSlot(scene.pool.physical(50), 0, PLAIN);
    const base = { type: PLAIN, x: 20, y: 21, argument: 0, direction: 0, player: 2, sids: [], abstract: 0, quality: 0 };
    // 서버 플레이어 번호(9)는 진단 뒤에도 계속해 배치한다.
    place.actualPlace({ ...base, player: 9 }, 1, () => 50);
    assert.deepEqual(reports, [['playerId != GAME_SERVER_ID', '\\Ns\\O\\Construction.cpp', 0x273]]);
    assert.ok(scene.events.some((event) => event.startsWith('P:50:')));
    // void 가 아닌 조각은 isVoid 진단 뒤 계속하고, 선언 개수가 다르면 마지막에 i == sidCount 진단이다.
    reports.length = 0;
    scene.events.length = 0;
    scene.pool.physical(50)[11] &= ~4;
    place.actualPlace(base, 2, () => 50);
    assert.deepEqual(reports.map((report) => report[2]), [0x282, 0x2f2]);
    assert.equal(reports[0][0], 'isVoid()');
    assert.equal(reports[1][0], 'i == sidCount');
    // 진단 장치가 없으면 성공을 합성하지 않고 거부한다.
    assert.ok(throws(() => scene.place().actualPlace({ ...base, player: 9 }, 1, () => 50)));
    // 다리 품질 프레임이 없으면 Rifttype 진단 뒤 -1 을 프레임에 쓴다.
    reports.length = 0;
    scene.prepare(82, floatToBits(0), 0, 0, 120, 0, 3, 3, 1, false, true, true, false, false, 5);
    fillSlot(scene.pool.physical(50), 0, 82);
    const bridge = scene.place({ ...scene.hooks(), assertion: (expression, file, line) => reports.push([expression, file, line]) });
    bridge.actualPlace({ ...base, type: 82, quality: 1 }, 1, () => 50);
    assert.deepEqual(reports, [['0', '\\Ns\\O\\Rifttype.cpp', 0x16a]]);
    assert.equal(hex(scene.pool.slot(50).subarray(36, 40)), 'ffffffff');
    assert.ok(throws(() => bridge.actualPlace(base, 1, null)));
});

test('배치: 실제 Player 기록을 연결하면 표시 비트와 이름을 매 호출 직접 읽고 쓴다', () => {
    const scene = new Scene();
    scene.prepare(PLAIN, floatToBits(250), 0x10, 0, 120, 0, 3, 0, 1, false, true, true, false, false, 5);
    const place = scene.place();
    const players = new Uint8Array(9 * 0xb4);
    assert.ok(throws(() => place.bindPlayerRecords(new Uint8Array(10))));
    place.bindPlayerRecords(players);
    // 3번 플레이어의 이름 "Bob" 과 이미 켜진 다른 표시 비트를 둔다.
    const offset = 3 * 0xb4;
    players.set(Buffer.from('Bob\0'), offset + 0xc);
    put(players, offset + 0x54, 0x00001001, 4);
    place.chargeShortfall(PLAIN, 3, 20, 21, 0, 0);
    // 처음이므로 다른 플레이어에게 이름 인자로 안내하고, 표시 비트 0x400 은 다른 비트를 보존한 채 켜진다.
    assert.ok(scene.events.includes(`F:ClientCheated(%s):Bob`));
    assert.ok(scene.events.includes(`X:3:${TOKEN}`));
    assert.equal(hex(players.subarray(offset + 0x54, offset + 0x58)), '01140000');
    // 이미 보고된 뒤에는 안내하지 않는다.
    scene.events.length = 0;
    place.chargeShortfall(PLAIN, 3, 20, 21, 0, 0);
    assert.ok(!scene.events.some((event) => event.startsWith('X:')));
    // 이름에 NUL 이 없으면 인접 필드까지 읽지 않고 거부한다.
    players.fill(0x41, offset + 0xc, offset + 0x2c);
    put(players, offset + 0x54, 0, 4);
    assert.ok(throws(() => place.chargeShortfall(PLAIN, 3, 20, 21, 0, 0)));
    // Player 번호가 8 을 넘으면 거부한다.
    assert.ok(throws(() => place.chargeShortfall(PLAIN, 9, 20, 21, 0, 0)));
});

test('배치: 어댑터는 같은 풀의 수신/소유자/SP 모듈만 잇는다', () => {
    const scene = new Scene();
    scene.prepare(PLAIN, floatToBits(250), 0, 0, 120, 0, 3, 0, 1, false, true, true, false, false, 5);
    const calls = [];
    const reward = new SquidReward(scene.pool, scene.types, scene.books, scene.rewardState, scene.store);
    const factory = { pool: scene.pool, take: (type, sid) => { calls.push(['take', type, sid]); return sid + 1; } };
    const owner = { pool: scene.pool, set: (sid, player) => calls.push(['set', sid, player]) };
    const store = { get: (player) => { calls.push(['get', player]); return floatToBits(12.5); } };
    const spy = { pool: scene.pool, addSp: (player, delta) => calls.push(['addSp', player, delta]) };
    const other = new SidPool(CAPACITY, { server: false });
    const base = scene.hooks();
    assert.ok(throws(() => makeConstructionPlaceHooks(other, factory, owner, reward, store, scene.state, base)));
    assert.ok(throws(() => makeConstructionPlaceHooks(scene.pool, { pool: other, take: () => 0 }, owner, reward, store, scene.state, base)));
    assert.ok(throws(() => makeConstructionPlaceHooks(scene.pool, factory, owner, reward, null, scene.state, base)));
    const connected = makeConstructionPlaceHooks(scene.pool, factory, owner, spy, store, scene.state, base);
    // Pop·표면 알림·문구·메시지 경계는 받은 그대로 둔다.
    assert.equal(connected.pop, base.pop);
    assert.equal(connected.notifySurface, base.notifySurface);
    assert.equal(connected.sendReject, base.sendReject);
    assert.equal(connected.take(100, 50), 51);
    connected.setOwner(50, 3);
    assert.equal(connected.storedSp(2), 12.5);
    connected.addLocalSp(4.5);
    connected.addPlayerSp(3, -2.5);
    assert.deepEqual(calls, [['take', 100, 50], ['set', 50, 3], ['get', 2], ['addSp', 1, 4.5], ['addSp', 3, -2.5]]);
});
