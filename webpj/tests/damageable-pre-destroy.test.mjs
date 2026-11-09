// Damageable 삭제 전처리 raw-damageable-pre-destroy.js(RawDamageablePreDestroy)를 원본 10.78 관측 damageablepredestroy-x86.tsv 와 cpppj `DamageablePreDestroyTests.cpp` 계약으로 대조한다(WB04-E251).
//  - 관측 1,632행(originals): 직접 Damageable(D)와 Carrier 경유(C) 전체 반환에서 효과 순서(붕괴 C·폭발 X·소리 A0/A1·사제 해방 소리 F·해방 R·공통 body B·Carrier 후처리 T)와 인자, 현재 전역(boss·contained TYPE·사제 TYPE) 세 값,
//    다섯 입력 슬롯(50,60..63)의 전체 바이트를 대조한다. 효과 도중 좌표·extra·boss·TYPE·next 가 바뀌는 네 가지 변이 스타일을 포함한다. 같은 1,632행을 Native 몸체(enableNativeBody)로 한 번 더 재생하며 이때
//    추가 선검사 훅이 한 번도 불리지 않아야 한다.
//  - 계약: 하위 효과 누락·짧은 공간 지도 거부, 잘못된 SID/free·contained 상태/범위 밖 TYPE/비유한 좌표는 효과 전에 거부(풀·사건 불변), 추상/매장은 좌표를 읽지 않고 공통 body 까지 전달, 공통 사전 검사 거부는 효과를 시작하지 않음,
//    다른 풀의 Carrier 연결 거부, Native 가 소리 콜백 뒤 바뀐 사제 TYPE 을 다음 내용물 비교에서 읽음.
// CD 10.72(originalCD)·10.37 행은 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E251)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { DAMAGEABLE_SOUND, DamageablePreDestroyState, RawDamageablePreDestroy, makeDamageablePreDestroyHooks } from '../www/core/raw-damageable-pre-destroy.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, put, readRows } from './raw-support.mjs';

// 각 PE 의 관측 행 수와 분석 도구의 합성 풀 크기, 입력 슬롯 번호(부모 50, 내용물 60..63)이다. (2026-11-10 WB04-E251)
const ROWS = 1632;
const CAPACITY = 24000;
const IDS = [50, 60, 61, 62, 63];
const SOURCE = 50;

// 10.78(originals) 관측 행만 쓴다. 파일이 없으면 검사를 건너뛴다. (2026-11-10 WB04-E251)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'damageablepredestroy-x86.tsv'));
const rows = allRows?.filter((row) => row[0] === 'originals') ?? null;

// 10진 문자열 DWORD 를 부호 없는 수로 읽는다. (2026-11-10 WB04-E251)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// cpppj `DamageableScene`: 같은 풀·상태·처리기를 조립해 입력과 결과를 대조한다. 풀 번호 5~63 을 확보하고 부모(50)와 내용물 후보(60..63)를 입력 슬롯으로 쓴다. (2026-11-10 WB04-E251)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.finderState = new ContainedFinderState();
        this.state = new DamageablePreDestroyState();
        this.spots = new Uint8Array(65536);
        this.events = [];
        this.style = 0;
        this.initialBoss = 0;
        this.freeCount = 0;
        this.validations = 0;
        // 원래 client 할당으로 비교 대상 번호를 확보한다.
        for (let id = 5; id <= 63; id += 1) assert.equal(this.pool.allocate(2), id);
        this.slots = IDS.map((sid) => this.pool.allocated(sid));
        this.finderState.checkingDead = true;
    }

    // 효과 뒤의 좌표를 비트 그대로 입력한다.
    coordinates(x, y) {
        put(this.slots[0], 14, floatToBits(x));
        put(this.slots[0], 18, floatToBits(y));
    }

    // 실행기의 경계별 raw/전역 변경만 공급하며 기대 결과는 계산하지 않는다.
    mutation(marker) {
        if (this.style === 1) {
            if (marker === 'C') { this.coordinates(21.00001, 22.00001); this.slots[0][40] = 9; }
            if (marker === 'A0') { this.coordinates(22.75, 23.9); this.finderState.boss = this.initialBoss === 0; }
            if (marker === 'X') this.coordinates(23, 24);
            if (marker === 'A1') this.coordinates(24.00001, 25.00001);
        }
        if (this.style === 2 && marker === 'F' && this.freeCount === 1) {
            this.finderState.boss = this.initialBoss === 0;
            this.finderState.containedType = 46;
            this.state.priestType = 159;
            put(this.slots[0], 6, 0, 2);
            put(this.slots[1], 4, 0, 2);
            this.slots[2][10] = 46;
            put(this.slots[2], 18, 159, 2);
        }
        if (this.style === 3 && marker === 'B') {
            this.finderState.boss = true;
            this.finderState.containedType = 6;
            put(this.slots[0], 6, 63, 2);
            this.slots[4][10] = 6;
            put(this.slots[4], 20, 1, 2);
        }
    }

    // 외부 호출 순서를 원본 관찰과 같은 문자열 조각으로 저장한다.
    record(event) { this.events.push(event); }

    // 실제 PE 에서 대체한 함수/구간만 같은 외부 효과로 공급한다.
    hooks() {
        return {
            collapse: (sid) => { this.record(`C:${sid}`); this.mutation('C'); },
            explosion: (sid) => { this.record(`X:${sid}`); this.mutation('X'); },
            sound: (event) => {
                if (event.sound === DAMAGEABLE_SOUND.priestFree) {
                    assert.deepEqual([event.x, event.y], [0, 0]);
                    this.record('F');
                    this.freeCount += 1;
                    this.mutation('F');
                } else {
                    const marker = event.sound === DAMAGEABLE_SOUND.collapse ? 'A0' : 'A1';
                    this.record(`${marker}:${floatToBits(event.x)}:${floatToBits(event.y)}`);
                    this.mutation(marker);
                }
            },
            releaseContained: (sid) => this.record(`R:${sid}`),
            validateBase: () => { this.validations += 1; },
            base: (sid, flags) => { this.record(`B:${sid}:${flags}`); this.mutation('B'); },
        };
    }

    // 입력 슬롯/spot/전역은 행의 입력 칸에서만 구성한다.
    prepare(row) {
        assert.equal(row.length, 17);
        this.events = [];
        this.validations = 0;
        this.freeCount = 0;
        this.style = number(row[13]);
        this.initialBoss = number(row[4]);
        this.finderState.boss = this.initialBoss !== 0;
        this.finderState.containedType = number(row[5]);
        this.state.priestType = number(row[6]);
        this.spots.fill(0);
        // 지정 map 셀만 표시하므로 좌표 경계의 선택이 실제 출력에 영향을 준다.
        for (const mark of row[10].split(';')) {
            const [x, y, value] = mark.split(',').map(number);
            this.spots[y * 256 + x] = value & 0xff;
        }
        // 주변/미지정 raw 도 전체 비교한다.
        for (const raw of this.slots) {
            raw.fill(0xab);
            raw[10] = 158;
            raw[11] = 0;
            raw[40] = 0;
            put(raw, 4, 0);
        }
        this.slots[0][11] = number(row[2]);
        this.slots[0][40] = number(row[3]);
        put(this.slots[0], 14, number(row[8]));
        put(this.slots[0], 18, number(row[9]));
        put(this.slots[0], 6, number(row[11]), 2);
        // 번호·타입·상태·next·kind·mask 를 입력 그대로 쓴다.
        for (const node of row[12].split(';')) {
            if (node === '') continue;
            const values = node.split(',').map(number);
            const raw = this.slots[values[0] - 60 + 1];
            raw[10] = values[1];
            raw[11] = values[2];
            put(raw, 4, values[3], 2);
            put(raw, 18, values[4], 2);
            put(raw, 20, values[5], 2);
        }
    }

    // 입력 5개 raw 를 실행기의 순서로 직렬화한다.
    slotsText() { return IDS.map((sid) => Buffer.from(this.pool.slot(sid)).toString('hex')).join('|'); }

    // 현재 권한/타입/사제 kind 전역의 전체 DWORD 관찰이다.
    globalsText() { return `${this.finderState.boss ? 1 : 0},${this.finderState.containedType},${this.state.priestType}`; }
}

// 직접 Damageable 와 실제 Carrier 두 경로를 각각 실제 PE 의 출력과 대조한다. native 이면 원문 몸체를 선택한다. (2026-11-10 WB04-E251)
function replay(native) {
    const scene = new Scene();
    let count = 0;
    // 후보 상태/바뀐 next/현재 전역을 구현에서 추정하지 않고 독립 행을 재생한다.
    for (const row of rows) {
        scene.prepare(row);
        const damageable = new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.state, scene.hooks());
        if (native) damageable.enableNativeBody();
        if (row[1] === 'D') damageable.preDestroy(SOURCE, number(row[7]));
        else {
            const carrier = new RawCarrierPreDestroy(scene.pool, scene.finderState, makeDamageablePreDestroyHooks(scene.pool, damageable, { dependentFound: () => scene.record('T') }));
            carrier.preDestroy(SOURCE, number(row[7]));
        }
        assert.equal(scene.globalsText(), row[14], `행 ${count} 전역`);
        assert.equal(scene.events.join(';'), row[15], `행 ${count} 사건`);
        assert.equal(scene.slotsText(), row[16], `행 ${count} 슬롯`);
        assert.equal(scene.validations, native ? 0 : (row[1] === 'D' ? 1 : 2), `행 ${count} 선검사 횟수`);
        count += 1;
    }
    assert.equal(count, ROWS);
}

test('damageable-pre-destroy: 원본 10.78 1,632행(직접·Carrier 경유)의 효과 순서·전역·슬롯 전체가 typed 경로와 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    replay(false);
});

test('damageable-pre-destroy: 같은 1,632행을 Native 몸체로 재생해도 같고 선검사 훅은 불리지 않는다', { skip: !rows && '원본 fixture 없음' }, () => replay(true));

// 미연결 공간/효과·잘못된 자산·비유한 ordinary 좌표는 효과 전에 거부한다.
test('damageable-pre-destroy: 하위 효과 누락·잘못된 SID/상태/TYPE/좌표 거부, 추상 좌표 무시, 공통 검사 거부, 다른 풀 거부', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    scene.prepare(rows[0]);
    const base = new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.state, scene.hooks());
    assert.throws(() => new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.state, { ...scene.hooks(), sound: null }), TypeError);
    assert.throws(() => new RawDamageablePreDestroy(scene.pool, scene.spots.subarray(0, 1), scene.finderState, scene.state, scene.hooks()), TypeError);
    const reject = (sid) => {
        const before = scene.slotsText();
        assert.throws(() => base.preDestroy(sid, 0x300000), `SID ${sid}`);
        assert.deepEqual(scene.events, []);
        assert.equal(scene.slotsText(), before);
    };
    for (const sid of [0, 1, CAPACITY]) reject(sid);
    scene.slots[0][11] = 1;
    reject(SOURCE);
    scene.slots[0][11] = 8;
    reject(SOURCE);
    scene.slots[0][11] = 2;
    scene.slots[0][10] = 6;
    reject(SOURCE);
    scene.slots[0][10] = 255;
    reject(SOURCE);
    scene.slots[0][10] = 188;
    reject(SOURCE);
    scene.slots[0][10] = 158;
    put(scene.slots[0], 14, 0x7fc00000);
    reject(SOURCE);
    scene.coordinates(256, 21);
    reject(SOURCE);
    // 추상/매장은 좌표를 읽지 않으므로 NaN 도 공통 body 까지 전달한다.
    put(scene.slots[0], 14, 0x7fc00000);
    scene.slots[0][40] = 9;
    base.preDestroy(SOURCE, 0x300000);
    assert.deepEqual(scene.events, ['B:50:3145728']);
    // 공통 사전 검사가 거부하면 효과를 시작하지 않는다.
    scene.prepare(rows[0]);
    const hooks = scene.hooks();
    hooks.validateBase = () => { throw new Error('공통 효과 거부'); };
    const blocked = new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.state, hooks);
    assert.throws(() => blocked.preDestroy(SOURCE, 1));
    assert.deepEqual(scene.events, []);
    // 다른 풀의 Carrier 연결은 거부한다.
    assert.throws(() => makeDamageablePreDestroyHooks(new SidPool(CAPACITY, { server: false }), base, {}), TypeError);
});

// 첫 priestFree 소리가 현재 사제 TYPE 을 바꾸면 다음 contained 후보 비교도 새 DWORD 를 읽는다.
test('damageable-pre-destroy: Native 는 소리 콜백 뒤 바뀐 사제 TYPE 을 다음 내용물 비교에서 읽는다', () => {
    const scene = new Scene();
    const [parent, first, second] = scene.slots;
    parent.fill(0);
    first.fill(0);
    second.fill(0);
    parent[10] = 74;
    put(parent, 6, 60, 2);
    put(parent, 14, floatToBits(1));
    put(parent, 18, floatToBits(1));
    first[10] = 6;
    second[10] = 6;
    put(first, 4, 61, 2);
    put(first, 18, 158, 2);
    put(second, 18, 159, 2);
    scene.spots[257] = 6;
    scene.finderState.boss = false;
    const priest = { value: 158 };
    scene.state.priestType = 999;
    scene.state.bindPriestType(priest);
    let sounds = 0;
    let bases = 0;
    const body = new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.state, {
        collapse: () => assert.fail('붕괴 효과는 없다'),
        explosion: () => assert.fail('폭발 효과는 없다'),
        sound: (event) => { assert.equal(event.sound, DAMAGEABLE_SOUND.priestFree); sounds += 1; priest.value = 159; },
        releaseContained: () => assert.fail('권한이 없어 해방하지 않는다'),
        validateBase: () => assert.fail('Native 는 선검사하지 않는다'),
        base: () => { bases += 1; },
    });
    body.enableNativeBody();
    body.preDestroy(SOURCE, 0);
    assert.deepEqual([sounds, bases, scene.state.priestType], [2, 1, 999]);
});

// fixture 가 구별하지 못하는 경계(변형 시험에서 살아남은 입력)를 닫는 계약이다. (2026-11-10 WB04-E251)
//  - spot 은 비트 2(값 2)와 비트 4(값 4) 어느 쪽이든 내용물 순회를 연다. 보드 좌표 하한은 `좌표 + 0.9999 ≤ -1` 에서 거부한다(정확히 -1 이 되는 float 가 있다). 상한 `≥ 256` 의 정확히 256 은 float 로 만들 수 없는 동등 경계다.
//  - 효과가 현재 boss 를 바꾸면 해방 판단은 바뀐 현재값을 본다(공유 소유자). Native 의 감긴 Y*256+X: y = 2^24 는 *256 에서 2^32 로 감겨 0 이 되어 spot[x] 를 읽고(typed 는 거부), y = 2^63 은 CRT 절삭 표현 밖이라 거부한다.
//  - Native 진입은 해방 장치를 호출마다 받고 없으면 거부한다. 자산 TYPE 하한은 70 이다(69 거부). 두 좌표를 읽는 순서(Native 는 Y 먼저)는 둘 다 같은 오류라 구별되지 않는 동등 경계다.
test('damageable-pre-destroy: spot 비트·좌표 하한·현재 boss·Native 감김 인덱스와 해방 장치 요구', () => {
    // 부모 50(TYPE 74, 좌표 (1,1))와 사제 kind(158)인 내용물 60 하나를 만든다. spot(1,1)=257 은 호출마다 지정한다.
    const build = (spotValue) => {
        const scene = new Scene();
        const [parent, child] = scene.slots;
        parent.fill(0);
        child.fill(0);
        parent[10] = 74;
        put(parent, 6, 60, 2);
        put(parent, 14, floatToBits(1));
        put(parent, 18, floatToBits(1));
        child[10] = 6;
        put(child, 18, 158, 2);
        scene.spots[257] = spotValue;
        scene.finderState.boss = false;
        const events = [];
        const hooks = {
            collapse: (sid) => events.push(`C:${sid}`),
            explosion: () => assert.fail('폭발 효과는 없다'),
            sound: (event) => events.push(event.sound),
            releaseContained: (sid) => events.push(`R:${sid}`),
            validateBase: () => {},
            base: () => events.push('B'),
        };
        const body = new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.state, hooks);
        return { scene, body, events, hooks, parent };
    };
    // spot 비트: 값 2·4·6·3·5 는 순회가 열려 사제 해방 소리가 한 번이고, 0·1·8·9 는 열리지 않는다.
    for (const [value, opened] of [[2, 1], [4, 1], [6, 1], [3, 1], [5, 1], [0, 0], [1, 0], [8, 0], [9, 0]]) {
        const { body, events } = build(value);
        body.preDestroy(SOURCE, 0);
        assert.equal(events.filter((event) => event === 'priestFree').length, opened, `spot ${value}`);
    }
    // 좌표 하한: 정확히 -1 이 되는 float(-(1+0.9999f), 비트 0xbffffcb9)는 거부하고 한 비트 작은 크기(0xbffffcb8)는 받아들인다.
    {
        const { scene, body, events, parent } = build(0);
        put(parent, 14, 0xbffffcb9);
        assert.throws(() => body.preDestroy(SOURCE, 0), RangeError);
        assert.deepEqual(events, []);
        put(scene.slots[0], 14, 0xbffffcb8);
        body.preDestroy(SOURCE, 0);
        assert.deepEqual(events, ['B']);
    }
    // 효과가 현재 boss 를 바꾸면 해방 판단은 바뀐 현재값(공유 소유자)을 본다.
    {
        const { scene, body, events, hooks } = build(0);
        const owner = { value: false };
        scene.finderState.bindBoss(owner);
        const collapse = hooks.collapse;
        hooks.collapse = (sid) => { collapse(sid); owner.value = true; };
        scene.finderState.boss = false;
        const shared = new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.state, hooks);
        shared.preDestroy(SOURCE, 0x200000);
        assert.deepEqual(events.filter((event) => event.startsWith('R:')), ['R:50']);
        assert.equal(body.pool, shared.pool);
    }
    // Native: y = 2^24 는 Y*256 이 2^32 로 감겨 spot[x] 를 읽고(typed 는 거부), y = 2^63 은 표현 밖이라 거부한다.
    {
        const { scene, body, events, parent } = build(0);
        put(parent, 18, floatToBits(16777216));
        assert.throws(() => body.preDestroy(SOURCE, 0), RangeError);
        scene.spots[1] = 6;
        const native = new RawDamageablePreDestroy(scene.pool, scene.spots, scene.finderState, scene.state, {
            collapse: () => {}, explosion: () => {}, sound: (event) => events.push(event.sound), releaseContained: () => {}, validateBase: () => {}, base: () => events.push('B'),
        });
        native.enableNativeBody();
        native.preDestroy(SOURCE, 0);
        assert.deepEqual(events, ['priestFree', 'B']);
        put(scene.slots[0], 18, 0x5f000000);
        assert.throws(() => native.preDestroy(SOURCE, 0), RangeError);
    }
    // Native 진입은 해방 장치를 호출마다 받고, 권한이 있는데 장치가 없으면 거부한다.
    {
        const { scene, body, events } = build(0);
        scene.finderState.boss = true;
        assert.throws(() => body.preDestroyNative(SOURCE, 0, undefined), /해방 장치 미연결/);
        body.preDestroyNative(SOURCE, 0, (sid) => events.push(`N:${sid}`));
        assert.deepEqual(events.filter((event) => event.startsWith('N:')), ['N:50']);
        assert.equal(events.filter((event) => event === 'R:50').length, 0);
    }
    // 자산 TYPE 하한: 69 는 거부하고 70 부터 받아들인다.
    {
        const { scene, body, events, parent } = build(0);
        parent[10] = 69;
        assert.throws(() => body.preDestroy(SOURCE, 0));
        assert.deepEqual(events, []);
        parent[10] = 70;
        body.preDestroy(SOURCE, 0);
        assert.ok(events.includes('B'));
        assert.ok(scene);
    }
});
