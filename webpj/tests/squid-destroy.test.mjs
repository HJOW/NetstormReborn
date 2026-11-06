// 공통 삭제기(RawSquidDestroy)·다리 삭제 훅(RawBridgeLifecycle)·Unpop 처리기(SquidUnpop)를 원본 10.78 관측과 대조한다(WB04-D2).
//  - cpppj/tests/fixtures/destroy-x86.tsv 의 10.78(originals) Destroy 600행·Bridge 64행(두 x87 정밀도 639/895): 외부 효과 순서, 대상/주변 일곱 슬롯 바이트,
//    풀 카운터·두 free 머리·두 꼬리, 풀 전체·네 단계 해시·spot·삭제 기록의 Adler-32, 선택, pre/post 깊이.
//    장면 구성과 외부 효과 대체는 cpppj tests/RawDestroyTests.cpp 와 같다(기대값은 웹이 만들지 않는다).
//  - 역할 1/2(서버/예측 SID root)의 준비는 cpppj 와 같이 실제 SquidFactory.take(74)를 쓴다(2026-11-06 WB04-D4: D2 의 syntheticTake 대체를 교체).
//    free 슬롯에 대한 원본 Take(004af610)는 그 슬롯의 VT·TYPE·상태·zOrder 만 쓰고 free 목록/카운터를 바꾸지 않으며, 장면이 곧바로 50바이트 전체를 덮어쓴다.
// (2026-11-05 WB04-D2: 공통 삭제기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, adler32, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawSquidFinder } from '../www/core/raw-squid-finder.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { RawBridgeLifecycle, BRIDGE_LIFECYCLE_EFFECT, makeBridgeLifecycleHooks } from '../www/core/bridge-lifecycle.js';
import { SquidFactory } from '../www/core/squid-factory.js';

// 원본 관측 파일이다. (2026-11-05 WB04-D2)
const DESTROY_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'destroy-x86.tsv');
const destroyRows = readRows(DESTROY_FIXTURE)?.filter((row) => row[1] === 'originals') ?? null;

// 처음 일곱 client SID(실제 할당 순서)와 풀 크기이다. (2026-11-05 WB04-D2)
const IDS = [5, 6, 7, 8, 9, 10, 11];
const CAPACITY = 32768;

// 장면 좌표(20.75f, 21.9f)이다. (2026-11-05 WB04-D2)
const X = Math.fround(20.75), Y = Math.fround(21.9);

// cpppj RawDestroyTests Scene 과 같은 입력 장면이다. 외부 가상 훅만 명시적으로 대체하고 공간 해제/반납은 실제 웹 구현을 부른다. (2026-11-05 WB04-D2)
class DestroyScene {
    // server 는 풀 서버 권한, role 은 root 종류(0 client 5번, 1 서버 15009, 2 예측 23002), inputCase 는 입력 변형, bridge 는 다리 장면 여부이다.
    constructor(server, role, inputCase, bridge) {
        // 실제 풀·해시·spot·188칸 타입 표와 같은 풀의 처리기들이다.
        this.pool = new SidPool(CAPACITY, { server });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, zOrder: 0 }));
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.bridge = new RawBridgeLifecycle(this.pool, this.types);
        this.finder = new RawSquidFinder(this.pool, this.hash, this.types);
        // 다리 장면 여부·입력 변형·외부 효과 기록·선택 SID 이다.
        this.bridgeMode = bridge;
        this.scenario = inputCase;
        this.events = [];
        // 처음 일곱 슬롯은 실제 client 할당 경로이다.
        for (const expected of IDS) assert.equal(this.pool.allocate(2), expected);
        const layout = this.pool.layout;
        this.root = role === 0 ? IDS[0] : (role === 1 ? layout.serverFirst + 9 : layout.predictableFirst + 1);
        // 서버/예측 root 는 실제 Take(004af610)로 free 슬롯을 수신한다(VT+48 은 free 라 부르지 않음).
        if (role) new SquidFactory(this.pool, this.types, { unpop: this.unpop }).take(74, this.root);
        const active = [this.root, ...IDS.filter((sid) => sid !== this.root)];
        this.selected = inputCase % 2 ? this.root : 0;
        // fixture 의 합성 raw/타입/등록 입력만 옮긴다.
        for (const sid of active) {
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            const index = IDS.indexOf(sid);
            let type = sid === this.root ? 74 : 74 + index;
            let state = sid === this.root ? (inputCase === 1 ? 4 : inputCase === 2 ? 2 : 0) : 0;
            if (!bridge && inputCase >= 3 && inputCase <= 6 && (sid === IDS[1] || (inputCase !== 3 && sid === IDS[2]))) {
                type = inputCase === 3 ? 20 : 75;
                state = inputCase === 3 ? 0 : 8;
            }
            if (bridge) {
                if (sid === IDS[1] || sid === IDS[4]) type = 75;
                if (sid === IDS[2]) state = 2;
            }
            put(raw, 0, bridge && sid === this.root ? 0x5034c8 : 0x501dd0);
            raw[10] = type;
            raw[11] = state;
            put(raw, 14, floatToBits(X));
            put(raw, 18, floatToBits(Y));
            const record = this.types[type];
            record.flags1 = 0;
            record.flags2 = bridge && sid === this.root ? 4 : 0;
            record.footX = 1;
            record.footY = 1;
            if (bridge && sid === IDS[5]) record.flags2 = 0x10000;
            if (!(state & 12) && type >= 70) {
                const level = sid === this.root ? 0 : 1;
                put(raw, 4, this.hash.bucket(level, X, Y), 2);
                this.hash.setBucket(level, X, Y, sid);
            }
        }
        if (!bridge && inputCase >= 3 && inputCase <= 6) {
            put(this.raw(this.root), 6, IDS[1], 2);
            put(this.raw(IDS[1]), 4, inputCase === 3 ? 0 : IDS[2], 2);
            if (inputCase !== 3) put(this.raw(IDS[2]), 4, 0, 2);
        }
        if (bridge) {
            this.types[74].flags2 = 4;
            put(this.raw(6), 12, 7, 2);
            put(this.raw(6), 8, 8, 2);
            put(this.raw(9), 12, 6, 2);
            put(this.raw(9), 8, 8, 2);
        }
        // 같은 풀을 쓰는 효과 처리기이다. 다리 링크 삭제는 실제 공통 destroy 로 재진입한다.
        this.hooks = { emit: (event) => this.emit(event), selected: () => this.selected };
        this.bridgeHooks = makeBridgeLifecycleHooks(this.finder, (event) => this.emitBridge(event));
    }

    // 장면 슬롯의 물리 쓰기 대여이다(cpppj 의 준비 시점 span 과 같이 반납 뒤에도 같은 바이트).
    raw(sid) { return this.pool.physical(sid); }

    // 효과 직전 상태까지 남기는 훅 사건이다(P/O/A/B).
    hookEvent(tag, sid, flags) {
        this.events.push(`${tag}:${sid}:${flags}:${this.pool.slot(sid)[11]}`);
    }

    // 원본 가상 release/destroy·선택·전파·공통 훅의 명시적 대체이다.
    emit(event) {
        switch (event.effect) {
        case SQUID_DESTROY_EFFECT.preDestroy:
            this.hookEvent('P', event.sid, event.flags);
            if (this.bridgeMode && event.sid === this.root) this.bridge.preDestroy(this.root, event.flags, 75, this.bridgeHooks);
            else {
                this.destroy.completePreDestroy();
                if (event.sid === this.root && this.scenario === 6) put(this.raw(this.root), 6, 7, 2);
                if (event.sid === this.root && this.scenario === 7) this.raw(this.root)[11] |= 4;
            }
            break;
        case SQUID_DESTROY_EFFECT.postDestroy:
            this.hookEvent('O', event.sid, event.flags);
            if (this.bridgeMode && event.sid === this.root) this.bridge.postDestroy(this.root, event.flags, this.bridgeHooks);
            else this.destroy.completePostDestroy();
            break;
        case SQUID_DESTROY_EFFECT.releaseDependent:
            this.events.push(`R:${event.sid}:${event.parent}`);
            put(this.raw(event.sid), 4, 0, 2);
            if (this.scenario === 5 && event.sid === IDS[1]) this.raw(event.sid)[11] = 1;
            break;
        case SQUID_DESTROY_EFFECT.destroyDependent:
            this.events.push(`C:${event.sid}:${event.flags}`);
            this.raw(event.sid)[11] |= 2;
            break;
        case SQUID_DESTROY_EFFECT.transmit:
            this.events.push(`T:${event.sid}:${event.flags}`);
            break;
        case SQUID_DESTROY_EFFECT.clearSelection:
            this.events.push(`S:${this.selected}`);
            this.selected = 0;
            break;
        default:
            throw new Error(`모르는 삭제 효과 ${event.effect}`);
        }
    }

    // 다리 훅의 효과 대체이다. 링크 삭제는 실제 공통 destroy, 파편/소리/낙하는 사건 기록이다.
    emitBridge(event) {
        switch (event.effect) {
        case BRIDGE_LIFECYCLE_EFFECT.destroyLink: this.destroy.destroy(event.sid, event.flags, this.hooks); break;
        case BRIDGE_LIFECYCLE_EFFECT.basePreDestroy: this.hookEvent('A', event.sid, event.flags); this.destroy.completePreDestroy(); break;
        case BRIDGE_LIFECYCLE_EFFECT.basePostDestroy: this.hookEvent('B', event.sid, event.flags); this.destroy.completePostDestroy(); break;
        case BRIDGE_LIFECYCLE_EFFECT.notifyRemoval: this.events.push(`N:${event.sid}`); break;
        case BRIDGE_LIFECYCLE_EFFECT.fallSound: this.events.push(`W:${floatToBits(event.x)}:${floatToBits(event.y)}`); break;
        case BRIDGE_LIFECYCLE_EFFECT.fallWalker: this.events.push(`F:${event.sid}`); this.raw(event.sid)[11] |= 2; break;
        default: throw new Error(`모르는 다리 효과 ${event.effect}`);
        }
    }

    // 원본 사건 문자열이다(빈 목록은 '-').
    eventText() { return this.events.length ? this.events.join(';') : '-'; }

    // 네 단계 해시 전체 WORD 의 Adler-32 이다.
    hashAdler() {
        const parts = [0, 1, 2, 3].map((level) => this.hash.entries(level));
        const bytes = new Uint8Array(parts.reduce((sum, part) => sum + part.length * 2, 0));
        let offset = 0;
        // 0단계부터 각 WORD 를 낮은 바이트부터 넣는다.
        for (const part of parts) {
            // 머리 WORD 를 차례로 넣는다.
            for (const value of part) {
                bytes[offset] = value & 0xff;
                bytes[offset + 1] = value >>> 8;
                offset += 2;
            }
        }
        return adler32(bytes);
    }

    // client/서버 삭제 기록 20쌍씩의 Adler-32 이다.
    logAdler() {
        const bytes = new Uint8Array(2 * 20 * 8);
        let offset = 0;
        // client 기록, 서버 기록 순서이다.
        for (const client of [true, false]) {
            // 레코드마다 SID·TYPE DWORD 를 넣는다.
            for (const record of this.pool.deletions(client)) {
                put(bytes, offset, record.sid);
                put(bytes, offset + 4, record.type);
                offset += 8;
            }
        }
        return adler32(bytes);
    }

    // 원본 관측 행과 사건·슬롯·전체 상태를 비교한다.
    check(row, label) {
        assert.equal(this.eventText(), row[7], `${label} 사건`);
        // 대상과 주변 슬롯은 모든 바이트를 직접 비교한다.
        for (const entry of row[8].split(';')) {
            const [sid, hex] = entry.split(':');
            assert.equal(Buffer.from(this.pool.slot(Number(sid))).toString('hex'), hex, `${label} 슬롯 ${sid}`);
        }
        const pool = this.pool;
        const actual = [pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.firstFree(false), pool.tail(true), pool.tail(false),
            adler32(pool.bytes()), this.hashAdler(), adler32(this.spots), this.logAdler(), this.selected, this.destroy.preDepth, this.destroy.postDepth];
        assert.deepEqual(actual, row[9].split(',').map(Number), `${label} 상태`);
    }
}

// 같은 종류의 10.78 행을 모두 재생한다. (2026-11-05 WB04-D2)
function replay(kind, expected) {
    const rows = destroyRows.filter((row) => row[0] === kind);
    // 행마다 독립 장면을 만들고 root 를 삭제한다.
    for (const row of rows) {
        assert.equal(row.length, 10);
        const scene = new DestroyScene(row[3] === '1', Number(row[4]), Number(row[6]), kind === 'Bridge');
        scene.destroy.destroy(scene.root, Number(row[5]) >>> 0, scene.hooks);
        scene.check(row, row.slice(0, 7).join(' '));
    }
    assert.equal(rows.length, expected);
}

// 공통 자산 삭제: 원본 10.78 600행(종속 체인 변형·서버/예측 root·선택·전파)과 같다.
test('RawSquidDestroy: 원본10.78 Destroy 600행과 같다', { skip: !destroyRows && '원본 fixture 없음' }, () => {
    replay('Destroy', 600);
});

// 다리 삭제: 같은 탐색기를 유지하며 링크를 실제 공통 destroy 로 재귀 삭제·반납한 원본 64행과 같다.
test('RawBridgeLifecycle: 원본10.78 Bridge 64행과 같다', { skip: !destroyRows && '원본 fixture 없음' }, () => {
    replay('Bridge', 64);
});

// dead 를 가장 먼저 표시하므로 중첩·반복 삭제가 외부 효과와 반납을 늘리지 않는다.
test('RawSquidDestroy: dead 가 중첩·반복 삭제를 막는다', () => {
    const scene = new DestroyScene(true, 0, 0, false);
    const emit = scene.hooks.emit;
    scene.hooks.emit = (event) => {
        if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) scene.destroy.destroy(scene.root, 0, scene.hooks);
        emit(event);
    };
    scene.destroy.destroy(scene.root, 0, scene.hooks);
    const before = adler32(scene.pool.bytes());
    const log = scene.logAdler();
    const count = scene.events.length;
    scene.destroy.destroy(scene.root, 0, {});
    assert.equal(adler32(scene.pool.bytes()), before);
    assert.equal(scene.logAdler(), log);
    assert.equal(scene.events.length, count);
    assert.equal(scene.destroy.preDepth, 0);
    assert.equal(scene.destroy.postDepth, 0);
});

// 훅 깊이 불균형은 해당 효과 뒤에 멈추며 이미 한 dead/공간 해제는 되돌리지 않는다.
test('RawSquidDestroy: pre/post 완료 누락은 그 경계에서 멈춘다', () => {
    const pre = new DestroyScene(true, 0, 0, false);
    pre.hooks.emit = () => {};
    assert.throws(() => pre.destroy.destroy(pre.root, 0, pre.hooks), /pre completion/);
    assert.equal(pre.pool.slot(pre.root)[11], 2);
    assert.equal(pre.destroy.preDepth, 1);
    assert.equal(pre.pool.deletions(true)[0].sid, 0);
    const post = new DestroyScene(true, 0, 0, false);
    post.hooks.emit = (event) => { if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) post.destroy.completePreDestroy(); };
    assert.throws(() => post.destroy.destroy(post.root, 0, post.hooks), /post completion/);
    assert.equal(post.pool.slot(post.root)[11], 6, '공간 해제(void)까지는 실행한다');
    assert.equal(post.destroy.postDepth, 1);
    assert.equal(post.pool.deletions(true)[0].sid, 0, 'SID 는 반납하지 않는다');
});

// 원본 assert 에 해당하는 미지원 입력(권한·훅·범위·free/contained/Form root)은 dead 표시 전에 거부한다.
test('RawSquidDestroy: 권한·범위·root 를 변경 전에 확인한다', () => {
    const scene = new DestroyScene(false, 1, 0, false);
    const before = adler32(scene.pool.bytes());
    assert.throws(() => scene.destroy.destroy(scene.root, 0, scene.hooks), /authority/);
    assert.throws(() => scene.destroy.destroy(scene.root, 8, {}), /hooks/);
    assert.throws(() => scene.destroy.destroy(0, 8, scene.hooks), RangeError);
    assert.throws(() => scene.destroy.destroy(32768, 8, scene.hooks), RangeError);
    assert.equal(adler32(scene.pool.bytes()), before);
    assert.throws(() => new RawSquidDestroy(new SidPool(CAPACITY), scene.unpop, scene.types), /mismatch/);
    // free/contained root 는 지원 범위 밖이다.
    for (const state of [1, 8]) {
        scene.raw(scene.root)[11] = state;
        assert.throws(() => scene.destroy.destroy(scene.root, 8, scene.hooks), /unsupported root/);
        assert.equal(scene.pool.slot(scene.root)[11], state);
    }
    scene.raw(scene.root)[11] = 0;
    scene.raw(scene.root)[10] = 20;
    assert.throws(() => scene.destroy.destroy(scene.root, 8, scene.hooks), /unsupported root/);
    assert.equal(scene.pool.slot(scene.root)[11], 0);
});

// 종속 상태(free·contained 아닌 자산)와 next 순환은 root 의 공간 해제·반납 전에 검출한다.
test('RawSquidDestroy: 잘못된 종속 체인은 공간 해제 전에 멈춘다', () => {
    // 세 손상 변형을 각각 검사한다.
    for (const scenario of [0, 1, 2]) {
        const scene = new DestroyScene(true, 0, 4, false);
        if (scenario === 0) scene.raw(6)[11] = 1;
        if (scenario === 1) scene.raw(6)[11] = 0;
        if (scenario === 2) put(scene.raw(6), 4, 6, 2);
        assert.throws(() => scene.destroy.destroy(scene.root, 0, scene.hooks), /dependent/);
        assert.equal(scene.pool.slot(scene.root)[11], 2);
        assert.equal(scene.pool.deletions(true)[0].sid, 0);
    }
});

// 다리 낙하 사건 값은 두 _ftol 하위 비트를 합친 signed DWORD 의 수치이다.
test('RawBridgeLifecycle: 낙하 사건 값과 훅 확인', () => {
    assert.equal(RawBridgeLifecycle.delayedFallPayload(20.75, 21.9), 21 * 256 + 20);
    assert.equal(RawBridgeLifecycle.delayedFallPayload(-1, 0), 255);
    assert.equal(RawBridgeLifecycle.delayedFallPayload(0, -1), Math.fround(-256));
    assert.equal(RawBridgeLifecycle.delayedFallPayload(Number.NaN, 3), 768);
    const scene = new DestroyScene(true, 0, 0, true);
    assert.throws(() => scene.bridge.preDestroy(scene.root, 0, 75, { emit: () => {} }), TypeError);
    // abstract/buried 다리는 탐색기 없이 공통 단계만 부른다.
    scene.raw(scene.root)[40] = 8;
    const events = [];
    scene.bridge.postDestroy(scene.root, 3, { emit: (event) => events.push(event.effect) });
    assert.deepEqual(events, [BRIDGE_LIFECYCLE_EFFECT.basePostDestroy]);
    assert.equal(get(scene.pool.slot(scene.root), 0), 0x5034c8);
});

// 탐색 사각형: preDestroy 는 원본 보정값(float 비트 0x3f7fff58 ≈ 0.99999, 올림이 아님)을 더해 절삭한 칸의 ±1, postDestroy 는 보정 없는 한 칸이다.
// 원본 Bridge 64행은 모든 객체가 같은 칸이라 이 보정 차이를 드러내지 않으므로 사각형을 직접 확인한다. (2026-11-05 WB04-D2)
test('RawBridgeLifecycle: 삭제 전후 탐색 사각형의 좌표 보정', () => {
    const scene = new DestroyScene(true, 0, 0, true);
    const areas = [];
    const hooks = { begin: (area) => { areas.push(area); return 0; }, next: () => 0, emit: () => {} };
    const cases = [[20.75, 21.9, 21, 22], [20, 7, 20, 7], [20.00002, 7.5, 21, 8]];
    // 좌표마다 사각형 중심을 확인한다.
    for (const [x, y, cx, cy] of cases) {
        put(scene.raw(scene.root), 14, floatToBits(x));
        put(scene.raw(scene.root), 18, floatToBits(y));
        areas.length = 0;
        scene.bridge.preDestroy(scene.root, 0, 75, hooks);
        scene.bridge.postDestroy(scene.root, 0, hooks);
        assert.deepEqual(areas[0], { left: cx - 1, top: cy - 1, right: cx + 1, bottom: cy + 1 }, `pre ${x},${y}`);
        assert.deepEqual(areas[1], { left: Math.trunc(Math.fround(x)), top: Math.trunc(Math.fround(y)), right: Math.trunc(Math.fround(x)), bottom: Math.trunc(Math.fround(y)) }, `post ${x},${y}`);
    }
});
