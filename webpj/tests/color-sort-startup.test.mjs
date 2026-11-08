// ColorSort 시작 부모 raw-color-sort-startup.js(initializeColorSort·initializeColorSortGradient)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: colorsort-startup-x86.tsv 116행(두 x87 정밀도의 정상 100·부분 중단 16, 부모에 부동소수 계산이 없어 결과가 같음) — 진입점(startup/gradient)·로드 플래그·세 현재 슬롯·할당 실패 프로필·명시 하위 변이
//    → 완료 분류(return 또는 중단 경계), 현재 전역 5개, 세 할당 prefix 4바이트와 입력 source 8바이트의 부분 쓰기, 하위 호출 인자/순서(A 진단 / F 해제 / N 할당 / C primary 생성 / S 검색 생성 / Q qsort / G 색띠 생성의 60바이트 인자 hex).
//  - 계약: NULL 할당의 자연 분기는 다른 하위를 요구하지 않고 선택된 누락 진단은 이전 상태를 보존한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E77: ColorSort 시작 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { initializeColorSort, initializeColorSortGradient } from '../www/core/raw-color-sort-startup.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E77)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'colorsort-startup-x86.tsv'));

// 원문 명시 하위 세 할당 토큰, 생성자 진입의 logical 값, 독립 입력 수(정상)이다. (2026-11-08 WB04-E77)
const BLOCKS = Object.freeze([0x12001000, 0x12006000, 0x1200c000]);
const LOGICAL = 0x12400000;
const ROWS = 116;
const NORMAL_ROWS = 100;

// 예외가 나는지 본다. (2026-11-08 WB04-E77)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E77)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 명시 하위 callback 의 중단을 물리 오류와 구별하는 표식이다. (2026-11-08 WB04-E77)
class Stop extends Error {
    constructor(cause) { super(cause); this.cause2 = cause; }
}

// 부모 기대를 계산하지 않고 명시 하위 생성·할당·원시 물리만 제공하는 장치이다. (2026-11-08 WB04-E77)
class Device {
    // 논리 입력(0 또는 11·22·33)을 실제 격리 토큰으로 대응시킨다.
    static token(logical) { return logical === 0 ? 0 : BLOCKS[logical / 11 - 1]; }

    // 원문 출력과 무관한 입력을 같은 현재 상태로 초기화한다.
    constructor(input) {
        assert.equal(input.length, 7);
        this.kind = input[0];
        this.loaded = Number(input[1]);
        this.primary = Device.token(Number(input[2]));
        this.search = Device.token(Number(input[3]));
        this.gradient = Device.token(Number(input[4]));
        this.profile = Number(input[5]);
        this.mutation = input[6];
        this.count = 0xffffffff;
        this.blocks = BLOCKS.map(() => new Uint8Array(4).fill(0xa5));
        this.source = Uint8Array.from(Buffer.from('original'));
        this.calls = new Map();
        this.events = [];
    }

    // 실제 인자를 기록한 뒤 하위 종류의 번호를 반환한다.
    event(name, text) {
        this.events.push(text);
        const next = (this.calls.get(name) ?? 0) + 1;
        this.calls.set(name, next);
        return `${name}${next}`;
    }

    // 명시 하위 복귀 직전에만 전역을 바꾸거나 중단한다.
    change(boundary) {
        const mutation = this.mutation;
        if (mutation === `${boundary}-stop`) throw new Stop(boundary);
        if (mutation === `${boundary}-sort22`) this.primary = Device.token(22);
        else if (mutation === `${boundary}-sort33` || mutation === `${boundary}-current33`) this.primary = Device.token(33);
        else if (mutation === `${boundary}-search0`) this.search = 0;
        else if (mutation === `${boundary}-search33`) this.search = Device.token(33);
        else if (mutation === `${boundary}-count91`) this.count = 91;
        else if (mutation === `${boundary}-count7`) this.count = 7;
        else if (mutation === `${boundary}-source`) this.source = Uint8Array.from(Buffer.from('changed\0', 'latin1'));
    }

    // 토큰의 같은 실제 prefix 만 조회한다. 해제/복제 여부를 부모가 추측하지 않는다.
    memory(token) {
        const index = BLOCKS.indexOf(token);
        if (index < 0) throw new RangeError('ColorSort 검사 미등록 토큰');
        return this.blocks[index];
    }

    // 하위 생성이 쓰는 marker 는 실제 생성 알고리즘을 대신한 명시 장치 입력이다.
    marker(token, text) {
        this.memory(token).set(Buffer.from(text, 'latin1').subarray(0, 4));
    }

    // 현재 전역을 읽고 쓰는 속성 뷰이다.
    globals() {
        const device = this;
        return {
            get primary() { return device.primary; }, set primary(value) { device.primary = value; },
            get search() { return device.search; }, set search(value) { device.search = value; },
            get gradient() { return device.gradient; }, set gradient(value) { device.gradient = value; },
            get count() { return device.count; }, set count(value) { device.count = value; },
            get paletteLoaded() { return device.loaded; }
        };
    }

    // 부모가 호출한 하위 경계만 관측하며 현재 slot/분기/직접 store 는 제품이 실행한다.
    hooks() {
        return {
            assertion: (line) => { assert.equal(line, 451); this.event('assert', 'A:451'); this.change('assert'); },
            release: (token) => { const boundary = this.event('free', `F:${token}`); this.change(boundary); },
            allocate: (amount) => {
                const index = amount === 0x3800 ? 0 : amount === 0x5008 ? 1 : amount === 0x5020 ? 2 : 3;
                assert.ok(index < 3);
                const token = (this.profile & (1 << index)) !== 0 ? 0 : BLOCKS[index];
                const boundary = this.event('alloc', `N:${amount}:${token}`);
                this.change(boundary);
                return token;
            },
            construct: (target, logical) => {
                assert.equal(logical, LOGICAL);
                this.event('sort', `C:${target}:${logical}:${hex(this.source)}`);
                this.marker(target, 'SORT');
                this.change('sort');
                return this.mutation === 'sort-return0' ? 0 : target;
            },
            constructSearch: (target) => { this.event('search', `S:${target}:${this.primary}`); this.marker(target, 'BASE'); this.change('search'); },
            memory: (token) => this.memory(token),
            sort: (target, size, width, comparator) => { this.event('qsort', `Q:${target},${size},${width},${comparator}`); this.change('qsort'); },
            constructGradient: (target, from, to, number, width) => {
                // 같은 60바이트 실제 인자 표현이다: RGB 세 double, 두 번째 RGB 세 double, DWORD 개수, double 폭.
                const raw = new DataView(new ArrayBuffer(60));
                [...from, ...to].forEach((value, index) => raw.setFloat64(index * 8, value, true));
                raw.setUint32(48, number, true);
                raw.setFloat64(52, width, true);
                this.event('gradient', `G:${target}:${this.primary}:${hex(new Uint8Array(raw.buffer))}`);
                this.marker(target, 'GRAD');
                this.change('gradient');
                return this.mutation === 'gradient-return0' ? 0 : target;
            }
        };
    }

    // 선택 진입점의 정상/명시 중단을 그대로 반환한다.
    run() {
        try {
            if (this.kind === 'startup') initializeColorSort(LOGICAL, this.globals(), this.hooks());
            else initializeColorSortGradient(this.globals(), this.hooks());
        } catch (error) {
            if (error instanceof Stop) return error.cause2;
            throw error;
        }
        return 'return';
    }

    // 현재 슬롯을 원문 fixture 순서로 표현한다.
    values() { return `${this.primary},${this.search},${this.gradient},${this.count},${this.loaded}`; }

    // 실제 prefix 와 source 의 부분 쓰기를 모두 비교한다.
    raw() { return `${hex(this.blocks[0])}:${hex(this.blocks[1])}:${hex(this.blocks[2])}:${hex(this.source)}`; }

    // 기록한 호출을 순서대로 연결한다.
    trace() { return this.events.join(';'); }
}

test('ColorSortStartup: 원본 부모 116관측(정상 100·부분 중단 16)의 현재 전역·부분 물리·하위 호출 인자가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    let normal = 0;
    // 두 control 의 전체 116행을 생략하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const device = new Device(row[1].split(','));
        const status = device.run();
        const label = `행 ${count}: 입력 ${row[1]}`;
        assert.equal(status, row[2], label);
        assert.equal(device.values(), row[3], label);
        assert.equal(device.raw(), row[4], label);
        assert.equal(device.trace(), row[5], label);
        count += 1;
        if (status === 'return') normal += 1;
    }
    assert.ok(count === ROWS && normal === NORMAL_ROWS);
});

test('ColorSortStartup: NULL 할당의 자연 분기는 다른 하위를 요구하지 않고 선택된 누락 진단은 이전 상태를 보존한다', () => {
    const globals = { primary: 0, search: 0, gradient: 0, count: 19, paletteLoaded: 1 };
    let allocations = 0;
    const hooks = { allocate: () => { allocations += 1; return 0; } };
    initializeColorSort(7, globals, hooks);
    assert.ok(allocations === 3 && globals.primary === 0 && globals.search === 0 && globals.gradient === 0 && globals.count === 10);
    // 진단 하위가 미연결이면 free 앞에서 중단한다.
    globals.paletteLoaded = 0;
    globals.primary = 9;
    assert.ok(throws(() => initializeColorSort(7, globals, hooks)));
    assert.ok(globals.primary === 9 && allocations === 3);
    // 해제 하위가 미연결이면 현재 비영 슬롯을 지우지 않는다.
    globals.paletteLoaded = 1;
    assert.ok(throws(() => initializeColorSort(7, globals, hooks)));
    assert.equal(globals.primary, 9);
    // 검색 객체의 물리가 vtable 폭보다 짧으면 정렬 전에 거부한다.
    const short = { primary: 0, search: 0, gradient: 0, count: 0, paletteLoaded: 1 };
    const shortHooks = { allocate: (amount) => (amount === 0x5008 ? 0x1000 : 0), constructSearch: () => {}, memory: () => new Uint8Array(3), sort: () => { throw new Error('도달 금지'); } };
    assert.ok(throws(() => initializeColorSort(7, short, shortHooks)));
});
