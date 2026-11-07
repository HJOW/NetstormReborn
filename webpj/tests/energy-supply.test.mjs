// 에너지 공급(수집·요구 문자 대응·평가·공급원 소비·정렬)과 83개 가상 표를 원본 10.78 관측으로 대조한다(WB04-E29, cpppj EnergySupplyTests·EnergySortTests 이식).
//  - energy-supply-x86.tsv 1058행: 다섯 연산(대응·수집·평가·덮개·소비)의 반환·공급 문구·잔여 문자·전체 30DWORD 선택 목록. 두 번째 정밀도 열은 '-' 이거나 같은 값이어야 한다.
//  - energy-sort-x86.tsv 567행: 정렬 직후와 연속 소비 뒤의 전체 목록·진단(321/336)·DWORD 반환. energy-provider-vtables.tsv 83행: 가상 표의 +a0.
// 계약: 알 수 없는 가상 표/요구 문자 거부, Override 우회, 진단 소비자 누락 거부. 실제 AI 건조 지점(RawAiDrySpots) 연결 검사는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E29: 에너지 공급 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawEnergySupply, energyProviderEntry } from '../www/core/raw-energy-supply.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E29)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const supplyRows = fixture('energy-supply-x86.tsv');
const sortRows = fixture('energy-sort-x86.tsv');
const vtableRows = fixture('energy-provider-vtables.tsv');

// 원문 DWORD 비트 읽기와 변환 도구이다. 문자열은 '-' 가 빈 값이며 나머지는 원본 바이트의 16진이다. (2026-11-08 WB04-E29)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const numbers = (text) => text.split(',').map(number);
const int32 = (value) => value | 0;
const hexToText = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));
const encode = (text) => (text === '' ? '-' : Buffer.from(text, 'latin1').toString('hex'));
// 슬롯 바이트에 리틀엔디언 DWORD 를 쓴다. (2026-11-08 WB04-E29)
const put = (bytes, offset, value) => { for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff; };
// 전체 30DWORD 를 리틀엔디언 바이트 16진으로 쓴다. (2026-11-08 WB04-E29)
const dwordsHex = (values) => { const bytes = new Uint8Array(120); values.forEach((value, index) => put(bytes, index * 4, value)); return Buffer.from(bytes).toString('hex'); };

// 실제 raw 풀/TYPE 와 명시 입력의 같은 참조를 실행하는 검사 장면이다(cpppj EnergyScene). (2026-11-08 WB04-E29)
class EnergyScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags2: 0, theme: 0 }));
        this.providers = new SquidPostPopList();
        this.selected = new SquidPostPopList();
        this.alliances = new Array(81).fill(0);
        this.mapLimit = 256;
        this.state = { radiusSquared: 484, editor: 0, special: 0, mission: 0, userCheat: 0, ignore: 0, allied: 1 };
        this.selectedStorage = new Array(30).fill(0);
        this.input = [];
        this.available = '';
        this.required = '';
        this.remainder = '';
        // 모든 관찰 슬롯을 실제 풀에서 확보한다.
        for (let sid = 5; sid <= 300; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // 기대값을 계산하지 않고 원문 실행기와 같은 입력을 준비한다.
    prepare(values, initial, demand) {
        const input = values;
        assert.equal(input.length, 30);
        this.input = input;
        this.available = initial;
        this.required = demand;
        this.remainder = 'sentinel';
        const state = this.state;
        Object.assign(state, { special: input[1], mission: input[2], userCheat: input[3], ignore: input[4], editor: input[5], allied: input[6], radiusSquared: bitsToFloat(input[16]) });
        this.mapLimit = int32(input[17]);
        this.providers.entries = new Array(input[8]).fill(0);
        this.providers.count = input[8];
        this.selected.entries = new Array(input[7]).fill(0);
        this.selected.count = input[23];
        // 선택 물리 30칸 중 실제 용량만 제품 컨테이너에 공급한다.
        for (let index = 0; index < 30; index += 1) {
            this.selectedStorage[index] = index % 2 === 0 ? 0 : 10 + index;
            if (index < this.selected.entries.length) this.selected.entries[index] = this.selectedStorage[index];
        }
        this.alliances.fill(0);
        // 공급 소유자 행/요청 소유자 열만 설정해 반대 방향의 동맹을 합성하지 않는다.
        for (let owner = 0; owner < 9; owner += 1) this.alliances[owner * 9 + input[9]] = (input[10] >>> owner) & 1;
        this.types[100].footX = int32(input[12]);
        this.types[100].footY = int32(input[13]);
        this.types[100].flags2 = input[22];
        // 원문이 읽는 raw 상태·좌표·현재 TYPE 원소/발자국을 독립 입력으로 채운다.
        for (let index = 0; index < Math.max(1, input[8]); index += 1) {
            const sid = 10 + index;
            const kind = 101 + (index % 8);
            if (index < input[8]) this.providers.entries[index] = sid;
            const raw = this.pool.physical(sid);
            raw.fill(0xa5);
            put(raw, 0, (input[20] >>> (index % 32)) & 1 ? 0x501ea0 : 0x501dd0);
            raw[10] = kind;
            raw[11] = input[24] & 0xff;
            raw[34] = (input[21] ? 1 + (index % 3) : input[9]) & 0xff;
            put(raw, 14, input[25]);
            put(raw, 18, input[26]);
            this.types[kind].footX = int32(input[27]);
            this.types[kind].footY = int32(input[28]);
            this.types[kind].theme = input[29] === 0 ? index % 4 : input[29] === 1 ? 1 : 6 + (index % 2);
        }
        const hooks = {
            requirement: (type) => { assert.equal(type, 100); return this.required; },
            providerEntry: (table) => { if (table === 0x501ea0) return 0x41c170; assert.equal(table, 0x501dd0); return 0x41c0f0; },
        };
        this.logic = new RawEnergySupply(this.pool, this.types, this.providers, this.selected, this.alliances, () => this.mapLimit, this.state, hooks);
    }

    // 실제 제품 API 의 반환과 목록을 원문 형식으로 기록한다.
    run() {
        const input = this.input;
        let result = 0;
        let take = '-';
        const x = bitsToFloat(input[14]);
        const y = bitsToFloat(input[15]);
        const operation = input[0];
        if (operation === 0) {
            const matched = this.logic.match(this.available, this.required, int32(input[19]));
            result = matched.value; this.available = matched.output; this.remainder = matched.remainder;
        } else if (operation === 1) this.available = this.logic.collect(x, y, 100, input[9], int32(input[19]));
        else if (operation === 2) {
            const evaluated = this.logic.evaluate(this.available, x, y, 100, input[9]);
            result = evaluated.value; this.available = evaluated.output;
        } else if (operation === 3) result = Number(this.logic.covers(10, [1, 2, 3, 4], x, y, bitsToFloat(input[16]), input[22]));
        else {
            const source = this.logic.takeSource();
            result = source ? 1 : 0;
            take = String(source);
        }
        // 용량 밖 인접 메모리는 초기 원본 그대로이며 제품의 소유 칸만 갱신한다.
        this.selected.entries.forEach((value, index) => { this.selectedStorage[index] = value; });
        return [String(result), encode(this.available), encode(this.remainder), String(this.selected.count), dwordsHex(this.selectedStorage), take];
    }
}

// 모든 고유 입력과 두 x87 정밀도의 명시 결과를 한 칸도 생략하지 않고 비교한다.
test('EnergySupply: 원본10.78 1058행(대응·수집·평가·덮개·소비의 반환/문구/잔여/목록)', { skip: !supplyRows && '원본 fixture 없음' }, () => {
    const scene = new EnergyScene();
    let rows = 0;
    for (const cells of supplyRows) {
        assert.equal(cells.length, 15);
        scene.prepare(numbers(cells[0]), hexToText(cells[1]), hexToText(cells[2]));
        const actual = scene.run();
        // 기본 53비트와 64비트의 개별 결과를 모두 확인한다.
        for (let index = 0; index < 6; index += 1) {
            assert.equal(actual[index], cells[3 + index], `행 ${rows} 열 ${index}`);
            assert.ok(cells[9 + index] === '-' || cells[9 + index] === actual[index], `행 ${rows} 열 ${index} 두 번째 정밀도`);
        }
        rows += 1;
    }
    assert.equal(rows, 1058);
});

// 기본 공급 0·알 수 없는 가상 표·손상된 요구를 성공으로 바꾸지 않는 계약을 검사한다.
test('EnergySupply: 가드와 현재 전역 조회', () => {
    const scene = new EnergyScene();
    const hooks = { providerEntry: () => 0xdeadbeef, requirement: () => 'w' };
    let special = 0;
    const logic = new RawEnergySupply(scene.pool, scene.types, scene.providers, scene.selected, scene.alliances, () => scene.mapLimit, scene.state, hooks, { special: () => special });
    let matched = logic.match('r', 'w');
    assert.deepEqual([matched.value, matched.output, matched.remainder], [0, 'w', 'r']);
    special = 1;
    scene.state.ignore = 1;
    matched = logic.match('r', 'w');
    assert.deepEqual([matched.value, matched.output], [1, 'w']);
    assert.equal(logic.pool, scene.pool);
    assert.throws(() => logic.match('r', 'x'), TypeError);
    assert.throws(() => logic.covers(0, [], 0, 0, 0, 0), RangeError);
    assert.throws(() => logic.covers(10, [], 0, 0, 0, 0), /미복원/);
    // 판본/TYPE 수/동맹 표/조회 계약이 어긋나면 거부한다.
    assert.throws(() => new RawEnergySupply(scene.pool, scene.types.slice(0, 187), scene.providers, scene.selected, scene.alliances, () => 256, scene.state, hooks), TypeError);
    assert.throws(() => new RawEnergySupply(scene.pool, scene.types, scene.providers, scene.selected, scene.alliances.slice(0, 80), () => 256, scene.state, hooks), TypeError);
    assert.throws(() => new RawEnergySupply(scene.pool, scene.types, scene.providers, scene.selected, scene.alliances, () => 256, scene.state, {}), TypeError);
});

// 원본 83개 가상 표의 +a0 DWORD 를 제품 분배 표와 한 항목씩 대조한다.
test('EnergySupply: 실제 공급 가상 표 83개', { skip: !vtableRows && '원본 fixture 없음' }, () => {
    assert.equal(vtableRows.length, 83);
    let supplying = 0;
    // 기본 0x41c0f0 과 다섯 실제 공급형 이외의 함수를 임의로 허용하지 않는다.
    for (const row of vtableRows) {
        assert.equal(row.length, 2);
        const entry = energyProviderEntry(number(row[0]));
        assert.equal(entry, number(row[1]));
        if (entry === 0x41c170) supplying += 1;
    }
    assert.equal(supplying, 5);
    assert.throws(() => energyProviderEntry(0), /미복원/);
});

// 실제 Pool 과 같은 선택 저장소에 원문 입력만 공급하는 정렬 검사 장면이다(cpppj SortScene). (2026-11-08 WB04-E29)
class SortScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags2: 0, theme: 0 }));
        this.providers = new SquidPostPopList();
        this.selected = new SquidPostPopList();
        this.alliances = new Array(81).fill(0);
        this.limit = 256;
        this.state = { radiusSquared: 484, editor: 0, special: 0, mission: 0, userCheat: 0, ignore: 0, allied: 1 };
        this.storage = new Array(30).fill(0);
        this.diagnostics = '';
        for (let sid = 5; sid <= 50; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // 전체 입력을 새로 공급하며 정렬 순서는 여기서 계산하지 않는다.
    prepare(row, text) {
        assert.equal(row.length, 68);
        this.input = row;
        this.demand = text;
        this.diagnostics = '';
        this.selected.entries = new Array(row[1]).fill(0);
        this.selected.count = row[2];
        // 원문은 용량 밖 인접 30칸도 입력으로 보존한다. 제품에는 실제 소유 용량만 제공한다.
        for (let index = 0; index < 30; index += 1) {
            const sid = 10 + index;
            this.storage[index] = (row[3] >>> index) & 1 ? 0 : (row[5] >>> index) & 1 ? 24001 : sid;
            if (index < this.selected.entries.length) this.selected.entries[index] = this.storage[index];
            const raw = this.pool.physical(sid);
            raw.fill(0xa5);
            raw[11] = (row[4] >>> index) & 1 ? 1 : 0;
            put(raw, 14, row[8 + index * 2]);
            put(raw, 18, row[9 + index * 2]);
        }
        const hooks = {
            providerEntry: () => { throw new Error('정렬 중 공급 가상 재조회'); },
            requirement: () => { throw new Error('정렬 중 TYPE 요구 재조회'); },
            sortDiagnostic: (why) => { if (this.diagnostics) this.diagnostics += ','; this.diagnostics += why === 'InvalidSource' ? '321' : '336'; },
        };
        this.logic = new RawEnergySupply(this.pool, this.types, this.providers, this.selected, this.alliances, () => this.limit, this.state, hooks);
    }

    // 제품이 소유한 칸만 반영하고 실제 전체 30DWORD 를 직렬화한다.
    bytes() {
        this.selected.entries.forEach((value, index) => { this.storage[index] = value; });
        return dwordsHex(this.storage);
    }

    // 실제 정렬/소비를 실행하고 단계별 저장소와 반환을 기록한다.
    run() {
        const input = this.input;
        let snapshot = '-';
        let taken = '-';
        if (input[0] !== 1) {
            this.logic.sortSources(this.demand, bitsToFloat(input[6]), bitsToFloat(input[7]));
            snapshot = `${this.selected.count}:${this.bytes()}`;
        }
        if (input[0] === 1 || input[0] === 2) {
            const items = [];
            // 첫 0 반환 뒤에도 원문과 같은 횟수의 빈 조회를 계속 확인한다.
            for (let index = 0; index < 32; index += 1) { const value = this.logic.takeSource(); items.push(`${value ? 1 : 0}:${value}`); }
            taken = items.join(',');
        }
        return [String(this.selected.count), this.bytes(), this.diagnostics === '' ? '-' : this.diagnostics, taken, snapshot];
    }
}

// 선택 결과만이 아니라 원본이 지운 꼬리·같은 메모리의 연속 소비·반복 진단까지 대조한다.
test('EnergySort: 원본10.78 567행(정렬 직후·꼬리·연속 소비·반복 진단)', { skip: !sortRows && '원본 fixture 없음' }, () => {
    const scene = new SortScene();
    let count = 0;
    for (const row of sortRows) {
        assert.equal(row.length, 12);
        scene.prepare(numbers(row[0]), hexToText(row[1]));
        const actual = scene.run();
        // 두 번째 정밀도의 생략은 원문이 둘의 전체 출력을 같다고 확인한 행에만 사용한다.
        for (let column = 0; column < 5; column += 1) {
            assert.equal(actual[column], row[2 + column], `행 ${count} 열 ${column}`);
            assert.ok(row[7 + column] === '-' || row[7 + column] === actual[column], `행 ${count} 열 ${column} 두 번째 정밀도`);
        }
        count += 1;
    }
    assert.equal(count, 567);
});

// 진단을 공급하지 않은 손상 목록을 성공 정렬로 바꾸지 않는다.
test('EnergySort: 진단 소비자 누락은 손상 목록을 거부한다', { skip: !sortRows && '원본 fixture 없음' }, () => {
    const scene = new SortScene();
    scene.prepare(numbers(sortRows[0][0]), 'x');
    scene.selected.entries = [24001];
    scene.selected.count = 1;
    const hooks = { providerEntry: energyProviderEntry, requirement: () => '' };
    const logic = new RawEnergySupply(scene.pool, scene.types, scene.providers, scene.selected, scene.alliances, () => scene.limit, scene.state, hooks);
    assert.throws(() => logic.sortSources('x', 100, 110), /원본 진단/);
});
