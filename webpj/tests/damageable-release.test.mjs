// Damageable 종속 해방 raw-damageable-release.js(RawDamageableRelease·DamageableReleaseState·makeDamageableReleaseHooks)를 원본 10.78 관측 damageablerelease-x86.tsv 와 cpppj `DamageableReleaseTests.cpp` 계약으로 대조한다(WB04-E252).
//  - 관측 1,504행(originals): 직접 Damageable(D)와 Carrier 경유(C) 전체 반환에서 효과 순서(붕괴 C·폭발 X·소리 A0/A1·사제 해방 소리 F·사제 생성 P·일반 생성 N·표면 알림 S·소유자 O·Pop L·공통 body B·Carrier 후처리 T)와 인자(좌표 float 비트·kind·WORD),
//    현재 전역(boss·contained TYPE·사제 TYPE / battle·허용 genus·다리 TYPE), 입력 슬롯 다섯 개(50,60..63)와 새 슬롯 네 개(100..103)의 전체 바이트를 대조한다. 첫 생성 경계에서 WORD·next·좌표·boss·전역·spot 이 바뀌는 두 가지 변이를 포함한다.
//    같은 1,504행을 Native 진입(Damageable enableNativeBody + releaseNative 의 현재 전역 getter)으로 한 번 더 재생한다.
//  - 계약(damageable_release_guards_and_binding): 하위 효과 누락·짧은 공간/TYPE 표 거부, 잘못된 SID·free 부모·비유한 좌표는 효과 전에 거부, 다른 풀의 연결 거부.
//  - 경계 계약: fixture 가 구별하지 못해 변형 시험에서 살아남은 입력을 닫는다(아래 마지막 검사의 주석).
// CD 10.72(originalCD)·10.37 행은 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E252)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { DAMAGEABLE_SOUND, DamageablePreDestroyState, RawDamageablePreDestroy, makeDamageablePreDestroyHooks } from '../www/core/raw-damageable-pre-destroy.js';
import { DamageableReleaseState, RawDamageableRelease, makeDamageableReleaseHooks } from '../www/core/raw-damageable-release.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, get, put, readRows } from './raw-support.mjs';

// 판본별 관측 행 수(독립 입력 752개 × 두 호출 경로), 분석 도구의 합성 풀 크기, 입력 슬롯(부모 50·내용물 60..63)과 생성 경계가 돌려주는 새 슬롯 번호, 합성 가상 표 기록값이다. (2026-11-10 WB04-E252)
const ROWS = 1504;
const CAPACITY = 24000;
const IDS = [50, 60, 61, 62, 63];
const NEW_IDS = [100, 101, 102, 103];
const SOURCE = 50;
const RELEASE_VTABLE = 0x20000100;

// 10.78(originals) 관측 행만 쓴다. 파일이 없으면 검사를 건너뛴다. (2026-11-10 WB04-E252)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'damageablerelease-x86.tsv'));
const rows = allRows?.filter((row) => row[0] === 'originals') ?? null;

// 10진 문자열 DWORD 를 부호 없는 수로 읽는다. (2026-11-10 WB04-E252)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 188칸 TYPE 입력이다(발자국·genus 는 장면이 채운다). (2026-11-10 WB04-E252)
const blankTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));

// cpppj `ReleaseScene`: 같은 풀·상태·처리기를 조립해 입력과 결과를 대조한다. 풀 번호 5~103 을 확보한다. (2026-11-10 WB04-E252)
class ReleaseScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.finderState = new ContainedFinderState();
        this.preState = new DamageablePreDestroyState();
        this.state = new DamageableReleaseState();
        this.spots = new Uint8Array(65536);
        this.types = blankTypes();
        this.events = [];
        this.change = 0;
        this.born = 0;
        this.changed = false;
        // 실행기와 같은 번호 범위만 확보한다.
        for (let id = 5; id <= 103; id += 1) assert.equal(this.pool.allocate(2), id);
        this.slots = IDS.map((sid) => this.pool.allocated(sid));
        this.newSlots = NEW_IDS.map((sid) => this.pool.allocated(sid));
        this.finderState.checkingDead = true;
    }

    // 외부 효과를 독립 관찰 형식으로 기록한다.
    record(event) { this.events.push(event); }

    // 좌표는 raw float 비트를 그대로 쓴다.
    coordinates(x, y) {
        put(this.slots[0], 14, floatToBits(x));
        put(this.slots[0], 18, floatToBits(y));
    }

    // 첫 생성 경계에서 독립 실행기와 같은 입력 변경만 공급한다.
    mutation() {
        if (!this.change || this.changed) return;
        this.changed = true;
        if (this.change === 1) {
            put(this.slots[1], 12, 0xfedc, 2);
            put(this.slots[1], 4, 0, 2);
            this.coordinates(23.25, 24.75);
            this.finderState.boss = false;
            put(this.slots[2], 18, 158, 2);
            put(this.slots[2], 20, 32, 2);
            this.state.allowedGenus = 0xffffffff;
        } else {
            this.state.battle = false;
            this.state.allowedGenus = 0;
            this.finderState.containedType = 46;
            this.slots[3][10] = 46;
            this.state.bridgeType = 90;
            this.spots[21 * 256 + 20] = 16;
        }
    }

    // 실제 하위 함수 대체만 제공하며 좌표/타입 분기와 WORD 복사는 구현에 맡긴다.
    hooks() {
        return {
            spawnPriest: (x, y, kind, word) => {
                this.record(`P:${floatToBits(x)}:${floatToBits(y)}:${kind}:${word}`);
                this.mutation();
            },
            create: (kind, flags) => {
                assert.ok(flags === 0 && this.born < this.newSlots.length);
                const index = this.born;
                this.born += 1;
                const raw = this.newSlots[index];
                raw.fill(0xcd);
                put(raw, 0, RELEASE_VTABLE);
                raw[10] = kind & 0xff;
                raw[11] = 0;
                const sid = NEW_IDS[index];
                this.record(`N:${kind}:${flags}:${sid}`);
                this.mutation();
                return sid;
            },
            setOwner: (sid, owner) => { assert.equal(owner, 0); this.record(`O:${sid}:${owner}`); },
            pop: (sid, x, y, flags) => { assert.equal(flags, 0); this.record(`L:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}`); },
            notifySurface: (sid) => this.record(`S:${sid}`),
        };
    }

    // 이미 대조한 효과/소리/공통 경계이며 해방은 주어진 장치(typed 연결 또는 Native 진입)에 연결한다.
    preHooks(releaseContained) {
        return {
            collapse: (sid) => this.record(`C:${sid}`),
            explosion: (sid) => this.record(`X:${sid}`),
            sound: (event) => {
                if (event.sound === DAMAGEABLE_SOUND.priestFree) this.record('F');
                else this.record(`${event.sound === DAMAGEABLE_SOUND.collapse ? 'A0' : 'A1'}:${floatToBits(event.x)}:${floatToBits(event.y)}`);
            },
            releaseContained,
            validateBase: () => {},
            base: (sid, flags) => this.record(`B:${sid}:${flags}`),
        };
    }

    // 입력 칸만 사용하여 현재 타입 표·공간·raw 를 준비한다.
    prepare(row) {
        assert.equal(row.length, 25);
        this.events = [];
        this.born = 0;
        this.changed = false;
        this.change = number(row[19]);
        this.finderState.boss = number(row[4]) !== 0;
        this.finderState.containedType = number(row[5]);
        this.preState.priestType = number(row[6]);
        this.state.battle = number(row[17]) !== 0;
        this.state.allowedGenus = number(row[18]);
        this.state.bridgeType = 82;
        this.spots.fill(0);
        // 각 map 표시를 실행기의 셀에 기록한다.
        for (const mark of row[10].split(';')) {
            const [x, y, value] = mark.split(',').map(number);
            this.spots[y * 256 + x] = value & 0xff;
        }
        // 미지정 raw 바이트도 독립 실행기의 채움 값과 일치시킨다.
        for (const raw of this.slots) {
            raw.fill(0xab);
            raw[10] = 158;
            raw[11] = 0;
            raw[40] = 0;
            put(raw, 4, 0);
        }
        this.slots[0][10] = 160;
        this.slots[0][11] = number(row[2]);
        this.slots[0][40] = number(row[3]);
        put(this.slots[0], 14, number(row[8]));
        put(this.slots[0], 18, number(row[9]));
        put(this.slots[0], 6, number(row[11]), 2);
        // next/kind/flags 와 후보 수명 비트를 추가 필터 없이 쓴다.
        for (const node of row[12].split(';')) {
            if (node === '') continue;
            const values = node.split(',').map(number);
            const raw = this.slots[values[0] - 59];
            raw[10] = values[1];
            raw[11] = values[2];
            put(raw, 4, values[3], 2);
            put(raw, 18, values[4], 2);
            put(raw, 20, values[5], 2);
        }
        // 생성 대상의 WORD 복사에서 상위 비트가 남는지 관찰할 입력이다.
        for (let index = 1; index < this.slots.length; index += 1) put(this.slots[index], 12, 0x8100 + IDS[index], 2);
        // 실제 생성 여부와 관계없이 네 개의 슬롯을 모두 비교한다.
        for (const raw of this.newSlots) raw.fill(0xcd);
        this.types = blankTypes();
        this.types[160].flags2 = number(row[16]);
        this.types[160].footX = number(row[14]);
        this.types[160].footY = number(row[15]);
        this.types[158].flags2 = 0x200000;
        this.types[82].flags2 = 2;
        this.types[90].flags2 = 0x40000000;
    }

    // 부모와 후보, 새 슬롯의 전체 바이트를 관찰한다.
    slotsText() { return IDS.map((sid) => Buffer.from(this.pool.slot(sid)).toString('hex')).join('|'); }
    newSlotsText() { return NEW_IDS.map((sid) => Buffer.from(this.pool.slot(sid)).toString('hex')).join('|'); }

    // 원본의 현재 boss/contained/priest 전역과 해방 중 바뀔 수 있는 현재 전투/허용 genus/다리 타입 전역이다.
    globalsText() { return `${this.finderState.boss ? 1 : 0},${this.finderState.containedType},${this.preState.priestType}`; }
    releaseGlobalsText() { return `${this.state.battle ? 1 : 0},${this.state.allowedGenus},${this.state.bridgeType}`; }
}

// 실제 Damageable/Carrier 전체 출력과 새 자산 WORD 까지 재생한다. native 이면 실제 장기 owner 와 같은 Native 두 진입을 쓴다. (2026-11-10 WB04-E252)
function replay(native) {
    const scene = new ReleaseScene();
    let count = 0;
    // 구현에서 기대 분기를 만들지 않고 각 원본 관찰과 비교한다.
    for (const row of rows) {
        scene.prepare(row);
        const release = new RawDamageableRelease(scene.pool, scene.types, scene.spots, scene.finderState, scene.state, scene.hooks());
        const preHooks = native
            ? scene.preHooks((sid) => release.releaseNative(sid, () => scene.state.battle, () => scene.state.bridgeType))
            : makeDamageableReleaseHooks(scene.pool, release, scene.preHooks(null));
        const damageable = new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.preState, preHooks);
        if (native) damageable.enableNativeBody();
        if (row[1] === 'D') damageable.preDestroy(SOURCE, number(row[7]));
        else {
            const carrier = new RawCarrierPreDestroy(scene.pool, scene.finderState, makeDamageablePreDestroyHooks(scene.pool, damageable, { dependentFound: () => scene.record('T') }));
            carrier.preDestroy(SOURCE, number(row[7]));
        }
        assert.equal(scene.globalsText(), row[20], `행 ${count} 전역`);
        assert.equal(scene.events.join(';'), row[21], `행 ${count} 사건`);
        assert.equal(scene.slotsText(), row[22], `행 ${count} 슬롯`);
        assert.equal(scene.releaseGlobalsText(), row[23], `행 ${count} 해방 전역`);
        assert.equal(scene.newSlotsText(), row[24], `행 ${count} 새 슬롯`);
        count += 1;
    }
    assert.equal(count, ROWS);
}

test('damageable-release: 원본 10.78 1,504행(직접·Carrier 경유)의 효과 순서·좌표·WORD·전역·슬롯 전체가 typed 경로와 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    replay(false);
});

test('damageable-release: 같은 1,504행을 Native 진입(현재 전역 getter)으로 재생해도 같다', { skip: !rows && '원본 fixture 없음' }, () => replay(true));

// 자료/하위 경계 누락·잘못된 부모와 서로 다른 풀 연결을 검사한다(cpppj damageable_release_guards_and_binding).
test('damageable-release: 하위 효과 누락·짧은 공간/TYPE 표·잘못된 SID·free 부모·비유한 좌표·다른 풀 연결 거부', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new ReleaseScene();
    scene.prepare(rows[0]);
    const build = (types, spots, hooks) => new RawDamageableRelease(scene.pool, types, spots, scene.finderState, scene.state, hooks);
    assert.throws(() => build(scene.types, scene.spots, { ...scene.hooks(), create: null }), TypeError);
    assert.throws(() => build(scene.types, scene.spots.subarray(0, 1), scene.hooks()), TypeError);
    assert.throws(() => build(scene.types.slice(0, 1), scene.spots, scene.hooks()), TypeError);
    const release = build(scene.types, scene.spots, scene.hooks());
    assert.throws(() => release.release(0));
    scene.slots[0][11] = 1;
    assert.throws(() => release.release(SOURCE));
    scene.slots[0][11] = 2;
    put(scene.slots[0], 14, 0x7fc00000);
    assert.throws(() => release.release(SOURCE), RangeError);
    assert.deepEqual(scene.events, []);
    assert.throws(() => makeDamageableReleaseHooks(new SidPool(CAPACITY, { server: false }), release, {}), TypeError);
    assert.equal(typeof makeDamageableReleaseHooks(scene.pool, release, { keep: 1 }).releaseContained, 'function');
    assert.ok(get(scene.slots[0], 14) === 0x7fc00000);
});

// 경계 계약용 작은 장면이다: 부모 50(TYPE 160, 내용물 머리 60)과 해방 가능(mask 0x20) 후보를 kinds 순서로 60.. 에 잇는다. 기본 후보는 다리 TYPE(82, genus 2 = 초기 허용 genus 에 포함)이다.
// 반환한 run(native) 는 typed release 또는 Native 진입(현재 전역 getter)을 부르고 사건 목록을 돌려준다. 훅이 받는 좌표는 언제나 단정도 값이어야 한다. (2026-11-10 WB04-E252)
function contract({ footX = 1, footY = 1, genus = 0, x = 20, y = 21, xBits = null, yBits = null, parentState = 0, parentType = 160, kinds = [82] } = {}) {
    const scene = new ReleaseScene();
    // 부모와 후보 raw 를 0 으로 비운 뒤 필요한 칸만 쓴다.
    for (const raw of scene.slots) raw.fill(0);
    const [parent] = scene.slots;
    parent[10] = parentType;
    parent[11] = parentState;
    put(parent, 6, 60, 2);
    put(parent, 14, xBits ?? floatToBits(x));
    put(parent, 18, yBits ?? floatToBits(y));
    // 후보를 순서대로 잇고 마지막 next 는 0 이다.
    kinds.forEach((kind, index) => {
        const raw = scene.slots[index + 1];
        raw[10] = 6;
        put(raw, 4, index + 1 < kinds.length ? 61 + index : 0, 2);
        put(raw, 12, 0x8100 + 60 + index, 2);
        put(raw, 18, kind, 2);
        put(raw, 20, 0x20, 2);
    });
    scene.finderState.containedType = 6;
    scene.finderState.boss = false;
    scene.types[parentType].footX = footX;
    scene.types[parentType].footY = footY;
    scene.types[parentType].flags2 = genus;
    scene.types[158].flags2 = 0x200000;
    scene.types[82].flags2 = 2;
    const hooks = scene.hooks();
    const { spawnPriest, pop } = hooks;
    hooks.spawnPriest = (px, py, kind, word) => { assert.ok(Math.fround(px) === px && Math.fround(py) === py); spawnPriest(px, py, kind, word); };
    hooks.pop = (sid, px, py, flags) => { assert.ok(Math.fround(px) === px && Math.fround(py) === py); pop(sid, px, py, flags); };
    const release = new RawDamageableRelease(scene.pool, scene.types, scene.spots, scene.finderState, scene.state, hooks);
    const run = (native = false) => {
        scene.events = [];
        scene.born = 0;
        if (native) release.releaseNative(SOURCE, () => scene.state.battle, () => scene.state.bridgeType);
        else release.release(SOURCE);
        return scene.events;
    };
    return { scene, hooks, release, run, parent };
}

// 사건 목록에서 Pop(L)·사제 생성(P)의 좌표 비트 쌍만 뽑는다. (2026-11-10 WB04-E252)
const placed = (events, marker) => events.filter((event) => event.startsWith(`${marker}:`)).map((event) => event.split(':').slice(marker === 'L' ? 2 : 1, marker === 'L' ? 4 : 3).map(Number));

// fixture 가 구별하지 못하는 경계(변형 시험에서 살아남은 입력)를 닫는 계약이다. (2026-11-10 WB04-E252)
//  - 스냅 편향은 단정도 0.99999(0x3f7fff58)를 double 로 더한다: x 비트 0x3727ffff 는 0칸, 0x37280000 은 1칸, 0x3727c5ad(≈1.0e-5; double 0.99999 를 더하면 1 을 넘음)는 0칸이다.
//  - 부모 중심 보정 genus 는 0x80000000 과 0x00400000 어느 한 비트로도 켜지고 다른 비트로는 켜지지 않는다. 중심은 float 로 한 번 저장된다(x = 0.1f·발자국 2 의 중심+0.5 는 0xbecccccc; 저장을 빼면 0xbecccccd).
//  - typed 부모 자격: free(1)·contained(8) 상태와 TYPE 69 는 거부하고 70 은 받는다. 좌표 하한은 `좌표 + 편향 ≤ −1` 에서 거부한다(정확히 −1 이 되는 float 0xbfffffac, 한 비트 작은 0xbfffffab 는 0칸).
//  - 발자국: X 가 1 이면 Y 와 무관하게 현재 좌표를 쓰고(Y 범위도 보지 않음), 아니면 중심을 쓰며 typed 는 X·Y 각각 1..256 밖을 거부한다.
//  - dead 부모는 권한(boss)이 없어도 순회한다. spot 은 비트 2 와 비트 4 어느 쪽이든 생성을 연다. 다리 TYPE 은 DWORD 전체로 비교한다(0x152 는 82 와 다름).
//  - 종속 kind 는 WORD 전체로 TYPE 표를 조회한다(188·414 는 범위 오류). 허용 genus 는 공유 소유자의 현재값을 후보마다 다시 읽는다.
//  - Native: 부모 자격·발자국을 확인하지 않고, 음수 좌표는 signed 절삭값을 float 로 고정하며 spot 은 감긴 Y*256+X 의 물리 BYTE 를 읽는다(범위 밖 물리는 거부, ±2^63 밖 좌표는 거부). 현재 전역 getter 가 없으면 거부한다.
// 동등 변형(결과가 같아 구별하지 않음): 공간 조회 편향 0.9999 의 값·정밀도(고정한 점이 언제나 정수 float), 스냅 정수의 float 저장(절삭값이 언제나 float 로 표현됨), 중심+0.5 의 float 저장(중심이 이미 float 격자라 덧셈이 정확),
//   typed 좌표 상한의 정확히 256(float 로 만들 수 없음).
test('damageable-release: 스냅 편향·중심 genus·부모 자격·발자국·spot 비트·다리 TYPE DWORD·kind WORD·공유 허용 genus·Native 절삭/감김', () => {
    // 스냅 편향 경계: 다리 후보가 고정한 점 (칸X, 21)에 Pop 된다.
    for (const [bits, cellX] of [[0x3727ffff, 0], [0x37280000, 1], [0x3727c5ad, 0]]) {
        const { scene, run } = contract({ xBits: bits });
        scene.spots[21 * 256 + cellX] = 6;
        assert.deepEqual(placed(run(), 'L'), [[floatToBits(cellX), floatToBits(21)]], `x 비트 ${bits.toString(16)}`);
    }
    // 부모 중심 보정 genus: 사제 후보(전투)가 받는 좌표이다. 발자국 (2,2)·좌표 (20,21) 의 중심+0.5 는 (19.5,20.5)이다.
    for (const [genus, centered] of [[0x80000000, true], [0x00400000, true], [0x80400000, true], [0, false], [0x7fbfffff, false]]) {
        const { scene, run } = contract({ footX: 2, footY: 2, genus, kinds: [158] });
        scene.state.battle = true;
        const expected = centered ? [floatToBits(19.5), floatToBits(20.5)] : [floatToBits(20), floatToBits(21)];
        assert.deepEqual(placed(run(), 'P'), [expected], `genus ${genus.toString(16)}`);
    }
    // 중심의 float 저장: 한 축씩 0.1f 를 주어 두 축을 따로 본다. 발자국 (2,2), 다른 축 20 의 중심+0.5 는 19.5 이다.
    for (const [xBits, yBits, expected] of [[0x3dcccccd, floatToBits(20), [0xbecccccc, floatToBits(19.5)]], [floatToBits(20), 0x3dcccccd, [floatToBits(19.5), 0xbecccccc]]]) {
        const { scene, run } = contract({ footX: 2, footY: 2, genus: 0x80400000, xBits, yBits, kinds: [158] });
        scene.state.battle = true;
        assert.deepEqual(placed(run(), 'P'), [expected]);
    }
    // typed 부모 자격: 내용물이 없어도 free/contained 상태와 TYPE 69 는 거부하고 TYPE 70 은 받는다.
    // free 비트(1)는 풀의 할당 슬롯 대여가 먼저 거부하므로(동등 방어) 오류 종류를 가리지 않고, contained(8)만 이 몸체의 상태 검사가 거부한다.
    for (const [state, pattern] of [[1, /./], [9, /./], [8, /부모 상태 오류/]]) {
        const { run, parent } = contract({ parentState: state });
        put(parent, 6, 0, 2);
        assert.throws(() => run(), pattern, `상태 ${state}`);
    }
    assert.throws(() => contract({ parentType: 69 }).run(), /부모 상태 오류/);
    {
        const { scene, run } = contract({ parentType: 70 });
        scene.spots[21 * 256 + 20] = 6;
        assert.equal(placed(run(), 'L').length, 1);
    }
    // typed 좌표 하한.
    assert.throws(() => contract({ xBits: 0xbfffffac }).run(), RangeError);
    {
        const { scene, run } = contract({ xBits: 0xbfffffab });
        scene.spots[21 * 256] = 6;
        assert.deepEqual(placed(run(), 'L'), [[0, floatToBits(21)]]);
    }
    // 발자국: (1,2)는 현재 좌표, (2,1)은 중심이다. 좌표 (20.5,21.5) → 현재 좌표 스냅 (21,22) / 중심 (19.5,21) 스냅 (20,21).
    for (const [footX, footY, cellX, cellY] of [[1, 2, 21, 22], [2, 1, 20, 21], [1, 0, 21, 22], [1, 257, 21, 22]]) {
        const { scene, run } = contract({ footX, footY, x: 20.5, y: 21.5 });
        scene.spots[cellY * 256 + cellX] = 6;
        assert.deepEqual(placed(run(), 'L'), [[floatToBits(cellX), floatToBits(cellY)]], `발자국 ${footX},${footY}`);
    }
    for (const [footX, footY] of [[0, 2], [2, 0], [257, 2], [2, 257], [-1, 2]]) {
        assert.throws(() => contract({ footX, footY }).run(), /발자국 범위 오류/, `발자국 ${footX},${footY}`);
    }
    {
        const { scene, run } = contract({ footX: 256, footY: 256, x: 200, y: 200 });
        scene.spots[72 * 256 + 72] = 6;
        assert.deepEqual(placed(run(), 'L'), [[floatToBits(72), floatToBits(72)]]);
    }
    // dead 부모는 권한이 없어도 순회한다(checking 켜짐).
    {
        const { scene, run } = contract({ parentState: 2 });
        scene.spots[21 * 256 + 20] = 6;
        assert.equal(scene.finderState.currentBoss, false);
        assert.equal(placed(run(), 'L').length, 1);
    }
    // spot 비트: 2·4·6·7 은 생성하고 0·1·8·0x10·0x12·0x16 은 건너뛴다.
    for (const [value, created] of [[2, 1], [4, 1], [6, 1], [7, 1], [0, 0], [1, 0], [8, 0], [0x10, 0], [0x16, 0], [0x12, 0]]) {
        const { scene, run } = contract();
        scene.spots[21 * 256 + 20] = value;
        assert.equal(placed(run(), 'L').length, created, `spot ${value}`);
    }
    // 다리 TYPE 은 DWORD 전체 비교이다.
    for (const [bridgeType, notified] of [[82, 1], [0x152, 0], [83, 0]]) {
        const { scene, run } = contract();
        scene.spots[21 * 256 + 20] = 6;
        scene.state.bridgeType = bridgeType;
        assert.equal(run().filter((event) => event === 'S:100').length, notified, `다리 TYPE ${bridgeType}`);
    }
    // 종속 kind 는 WORD 전체로 조회한다. 전투에서는 허용 genus 를 보지 않지만 사제 genus 조회는 범위를 확인한다.
    for (const kind of [188, 414, 0xffff]) {
        assert.throws(() => contract({ kinds: [kind] }).run(), /타입 범위 오류/, `kind ${kind}`);
        const { scene, run } = contract({ kinds: [kind] });
        scene.state.battle = true;
        assert.throws(() => run(), /타입 범위 오류/, `전투 kind ${kind}`);
    }
    // 허용 genus 공유 소유자: 첫 생성 효과가 현재값을 0 으로 바꾸면 다음 후보는 건너뛴다. 독립 필드 값은 쓰지 않는다.
    {
        const { scene, hooks, run } = contract({ kinds: [82, 82] });
        scene.spots[21 * 256 + 20] = 6;
        const owner = { value: 2 };
        scene.state.allowedGenus = 0;
        scene.state.bindAllowedGenus(owner);
        assert.throws(() => scene.state.bindAllowedGenus(7), TypeError);
        const { create } = hooks;
        hooks.create = (kind, flags) => { const sid = create(kind, flags); owner.value = 0; return sid; };
        assert.equal(placed(run(), 'L').length, 1);
        scene.state.bindAllowedGenus(null);
        scene.state.allowedGenus = 2;
        assert.equal(placed(run(), 'L').length, 2);
    }
    // Native: 음수 좌표 −5.5 는 −4 로 고정되고 spot 은 감긴 인덱스 21*256−3 을 읽는다. typed 는 거부한다.
    {
        const { scene, run } = contract({ x: -5.5 });
        scene.spots[21 * 256 - 3] = 6;
        assert.throws(() => run(false), RangeError);
        assert.deepEqual(placed(run(true), 'L'), [[floatToBits(-4), floatToBits(21)]]);
    }
    // Native: y = 2^24 는 Y*256 이 2^32 로 감겨 spot[x] 를 읽고, y = 300 은 물리 범위 밖이라 거부한다.
    {
        const { scene, run } = contract({ y: 16777216 });
        scene.spots[20] = 6;
        assert.deepEqual(placed(run(true), 'L'), [[floatToBits(20), floatToBits(16777216)]]);
        assert.throws(() => contract({ y: 300 }).run(true), /물리 부족/);
    }
    // Native: ±2^63 표현 경계. 2^63 과 −2^63−2^40 은 거부하고 −2^63(하위 DWORD 0)은 받는다.
    assert.throws(() => contract({ xBits: 0x5f000000 }).run(true), /CRT/);
    assert.throws(() => contract({ xBits: 0xdf000001 }).run(true), /CRT/);
    {
        const { scene, run } = contract({ xBits: 0xdf000000 });
        scene.spots[21 * 256] = 6;
        assert.deepEqual(placed(run(true), 'L'), [[0, floatToBits(21)]]);
    }
    // Native: 부모 자격(상태·TYPE 하한)과 발자국을 확인하지 않는다.
    {
        const { scene, run } = contract({ parentState: 9, parentType: 6, footX: 0, footY: 0 });
        scene.finderState.checkingDead = false;
        scene.spots[21 * 256 + 20] = 6;
        assert.throws(() => run(false));
        assert.deepEqual(placed(run(true), 'L'), [[floatToBits(20), floatToBits(21)]]);
    }
    // Native 진입은 현재 전역 getter 두 개를 요구한다.
    {
        const { release } = contract();
        assert.throws(() => release.releaseNative(SOURCE, undefined, () => 82), TypeError);
        assert.throws(() => release.releaseNative(SOURCE, () => false, 82), TypeError);
    }
    // 10.78 풀이 아니면 구성 때 거부한다.
    {
        const { scene, hooks } = contract();
        assert.throws(() => new RawDamageableRelease({ edition: 'cd1072' }, scene.types, scene.spots, scene.finderState, scene.state, hooks), /10\.78/);
    }
});
