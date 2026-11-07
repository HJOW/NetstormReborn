// 임시 문자열 고리(Staticstring.cpp)와 Factory 가상 입력(004547d0)을 원본 10.78 관측/계약으로 대조한다(WB04-E18, cpppj StaticStringsTests·FactoryInputTests 이식).
//  - static-string-runtime-x86.tsv 204행: 두 8192 BYTE 영역 전체, 네 전역, 부분 중단(stop/fault), 하위 호출 순서를 대조한다.
//  - Factory 입력은 원본 관측 없이 cpppj 의 독립 계약 4건(경계 자금·자금 부족 우선·float 비교/NaN·편집기)을 옮긴다.
// cpppj 의 ClientStaticStrings/Client 번역 통합 검사는 해당 Client 단위를 옮길 때 대조한다. (2026-11-08 WB04-E18: 임시 문자열·Factory 입력 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { initializeStaticStrings, reserveStaticString, formatStaticString } from '../www/core/raw-static-strings.js';
import { RawFactoryInput, createFactoryInputState } from '../www/core/raw-factory-input.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E18)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'static-string-runtime-x86.tsv'));
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// Native 주소는 명시 입력 물리의 식별자로만 쓰는 두 8192 BYTE 영역과 외부 서식 입력 주소이다. (2026-11-08 WB04-E18)
const ARENA1 = 0x12010000;
const ARENA2 = 0x12020000;
const SOURCE = 0x12400000;
class StaticStop extends Error {}

// Native 기록과 같은 할당/offset 식별자로 주소를 정규화한다. (2026-11-08 WB04-E18)
function normalized(address) {
    if (!address) return '0';
    for (let index = 0; index < 2; index += 1) {
        const base = index ? ARENA2 : ARENA1;
        if (address >= base && address - base < 8192) return `${index + 1}+${address - base}`;
    }
    return `0x${address.toString(16)}`;
}

// 제품 전에 고정한 원본 204관측의 전체 물리/현재 전역/부분 중단을 대조한다.
test('임시 문자열 고리: 원본10.78 204행(두 8192 BYTE 영역·전역·부분 중단)', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const fields of rows) {
        assert.equal(fields.length, 8);
        const parameters = fields[0].split(',');
        assert.equal(parameters.length, 9);
        const [mode, capacity, initial, cursor, length, seed, repeat, pattern] = parameters.slice(0, 8).map(number);
        const change = parameters[8];
        // 미작성 입력 전체도 원문과 같다.
        const arenas = [new Uint8Array(8192), new Uint8Array(8192)];
        for (let block = 0; block < 2; block += 1) for (let offset = 0; offset < 8192; offset += 1) arenas[block][offset] = (seed + block * 71 + offset * 13) & 0xff;
        let size = capacity;
        let base = initial ? ARENA1 : 0;
        let current = initial ? ARENA1 + cursor : 0;
        let last = initial ? ARENA1 + 7 : 0;
        let result = 0;
        let status = 'normal';
        const trace = [];
        const globals = {
            get size() { return size; }, set size(value) { size = value; },
            get base() { return base; }, set base(value) { base = value; },
            get current() { return current; }, set current(value) { current = value; },
            get last() { return last; }, set last(value) { last = value; },
        };
        const memory = (address, bytes) => {
            // 선택 접근의 전체 폭만 실제 범위 검사한다.
            for (let block = 0; block < 2; block += 1) {
                const begin = block ? ARENA2 : ARENA1;
                if (address >= begin && address - begin <= 8192 && bytes <= 8192 - (address - begin)) return arenas[block].subarray(address - begin, address - begin + bytes);
            }
            throw new RangeError('검사 임시 고리 명시 물리 fault');
        };
        const hooks = {
            memory,
            assertion: (value) => { trace.push(`A:${value}`); if (change === 'assert-stop') throw new StaticStop(); },
            allocate: (value) => {
                trace.push(`N:${value}`);
                if (change === 'new-stop') throw new StaticStop();
                if (change === 'new-size4') size = 4;
                return change === 'new-null' ? 0 : ARENA2;
            },
            format: (destination, source) => {
                trace.push(`F:${normalized(destination)}:${normalized(source)}:7`);
                if (change === 'format-stop') throw new StaticStop();
                memory(destination, 1)[0] = 0x52;
                memory(destination + 1, 1)[0] = 0;
                if (change === 'format-cursor3') current = ARENA1 + 3;
            },
        };
        try {
            // 같은 현재 고리를 반복 호출 사이 유지한다.
            for (let invocation = 0; invocation < repeat; invocation += 1) {
                if (mode === 0) initializeStaticStrings(capacity, globals, hooks);
                else if (mode === 1) result = reserveStaticString(length, globals, hooks);
                else result = formatStaticString(length, pattern ? ARENA1 + pattern : SOURCE, globals, hooks);
            }
        } catch (error) {
            if (error instanceof StaticStop) status = 'stop';
            else if (error instanceof RangeError) status = 'fault';
            else throw error;
        }
        const label = `행 ${count} ${fields[0]}`;
        assert.equal(status, fields[1], `${label} 상태`);
        const expected = fields[2].split(',').map(number);
        assert.deepEqual([size >>> 0, base >>> 0, current >>> 0, last >>> 0], expected, `${label} 전역`);
        if (mode) assert.equal(result >>> 0, number(fields[3]), `${label} 결과`);
        assert.equal(Buffer.from(arenas[0]).toString('hex'), fields[4], `${label} 영역1`);
        assert.equal(Buffer.from(arenas[1]).toString('hex'), fields[5], `${label} 영역2`);
        assert.equal(trace.join(';') || '-', fields[6], `${label} 호출 순서`);
        count += 1;
    }
    assert.equal(count, 204);
});

// 실제 raw Factory 와 명시 비용/SP/예약·UI 경계를 구성하는 장면이다(cpppj FactoryInputScene). (2026-11-08 WB04-E18)
class FactoryInputScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.factory = this.pool.allocate(2);
        const raw = this.pool.allocated(this.factory);
        raw.fill(0);
        raw[0] = 0x48; raw[1] = 0x91; raw[2] = 0x50; // 가상 표 0x509148
        raw[10] = 85;
        raw[34] = 2;
        this.editor = 0; this.owner = 1; this.level = 0; this.stored = 0;
        this.cost = 500; this.money = 500; this.upgrading = false;
        this.trace = []; this.tells = [];
        this.state = createFactoryInputState({ editor: () => this.editor, localOwner: () => this.owner });
        this.hooks = {
            level: (sid) => { assert.equal(sid, this.factory); this.trace.push('level'); return this.level; },
            setLevel: (sid, value) => { assert.equal(sid, this.factory); this.stored = value; this.trace.push('set'); },
            adjustFrame: (sid) => { assert.equal(sid, this.factory); this.trace.push('frame'); },
            cost: (type) => { assert.equal(type, 85); this.trace.push('cost'); return this.cost; },
            money: (local) => { assert.equal(local, this.owner); this.trace.push('money'); return this.money; },
            upgrading: (sid) => { assert.equal(sid, this.factory); this.trace.push('upgrading'); return this.upgrading; },
            request: (sid) => { assert.equal(sid, this.factory); this.trace.push('request'); },
            tell: (section) => { this.tells.push(section); this.trace.push('tell'); },
            research: (sid, code, parameter) => { assert.deepEqual([sid, code, parameter], [this.factory, 0xff000080, 0xabcd0005]); this.trace.push('research'); return 17; },
            parent: (sid, code, parameter) => { assert.deepEqual([sid, code, parameter], [this.factory, 0xff0000af, 0xabcd0005]); this.trace.push('parent'); return 27; },
        };
    }

    make() { return new RawFactoryInput(this.pool, this.state, this.hooks); }

    // 상위 수정키가 있는 같은 실제 업그레이드 명령이다.
    upgrade() { return this.make().input(this.factory, 0xff000032, 0xabcd0005); }
}

// 비용과 정확히 같은 SP 도 요청하며 상한/타소유자 같은 추가 관문을 두지 않는다.
test('Factory 입력: 자금 경계와 추가 관문 없음', () => {
    const input = new FactoryInputScene();
    input.level = 3;
    assert.equal(input.upgrade(), 1);
    assert.equal(input.tells.length, 0);
    assert.deepEqual(input.trace, ['cost', 'money', 'upgrading', 'request']);
    assert.deepEqual([input.pool.slot(input.factory)[34], input.owner], [2, 1]);
    assert.equal(input.make().pool, input.pool);
});

// 진행 중이어도 자금 부족 안내가 먼저이며 예약 조회/요청은 하지 않는다.
test('Factory 입력: 자금 부족이 기존 업그레이드 검사보다 앞선다', () => {
    const input = new FactoryInputScene();
    input.money = 499;
    input.upgrading = true;
    assert.equal(input.upgrade(), 1);
    assert.deepEqual(input.tells, ['upgradeNoBucks']);
    assert.deepEqual(input.trace, ['cost', 'money', 'tell']);
});

// x87 비교의 unordered 는 자금 부족이며 정수 비용은 비교 전에 float 로 축소된다.
test('Factory 입력: NaN 은 자금 부족이고 비용은 float 로 축소된다', () => {
    const nan = new FactoryInputScene();
    nan.money = Number.NaN;
    assert.equal(nan.upgrade(), 1);
    assert.deepEqual(nan.tells, ['upgradeNoBucks']);
    const rounded = new FactoryInputScene();
    rounded.cost = 16777217;
    rounded.money = 16777216;
    rounded.upgrade();
    assert.equal(rounded.trace.at(-1), 'request');
    const infinity = new FactoryInputScene();
    infinity.money = Number.POSITIVE_INFINITY;
    infinity.upgrade();
    assert.equal(infinity.trace.at(-1), 'request');
    const negative = new FactoryInputScene();
    negative.cost = -500;
    negative.money = -500;
    negative.upgrade();
    assert.equal(negative.trace.at(-1), 'request');
});

// 편집기에서도 SP 를 먼저 조회하고 이미 진행 중이면 요청하지 않는다.
test('Factory 입력: 편집기는 SP 를 읽고 진행 중 관문을 유지한다', () => {
    const input = new FactoryInputScene();
    input.editor = 1;
    input.money = 0;
    input.upgrading = true;
    assert.equal(input.upgrade(), 1);
    assert.deepEqual(input.tells, ['alreadyUpgrading']);
    assert.deepEqual(input.trace, ['cost', 'money', 'upgrading', 'tell']);
});

// 순환·연구·부모 위임은 원본 순서와 수정키 포함 원문을 전달한다.
test('Factory 입력: 순환·연구·부모 위임', () => {
    const input = new FactoryInputScene();
    input.level = 2;
    assert.equal(input.make().input(input.factory, 0x03000063, 0), 1);
    assert.deepEqual([input.stored, input.trace.join(',')], [0, 'level,set,frame']);
    input.trace.length = 0;
    assert.equal(input.make().input(input.factory, 0xff000080, 0xabcd0005), 17);
    assert.equal(input.make().input(input.factory, 0xff0000af, 0xabcd0005), 27);
    assert.deepEqual(input.trace, ['research', 'parent']);
    assert.throws(() => new RawFactoryInput({ edition: 'cd1072' }, input.state, input.hooks), TypeError);
});
