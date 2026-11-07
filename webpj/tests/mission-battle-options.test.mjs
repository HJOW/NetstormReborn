// 미션 전투 옵션(초기화·범위·비용·값·밀도·사용표)을 원본 10.78 관측으로 대조한다(WB04-E37, cpppj MissionBattleOptionsTests 이식).
//  - mission-option-reset-x86.tsv 296행(초기화 F/P 변형: 이웃/예약 바이트 보존, 네 번째 float, 실제 파생값), mission-battle-options-x86.tsv 3305행(x87 특수 float·0x67 부모의 288 조합: Range/Cost/Value/Clear/Apply의 범위·값·TYPE 비용 188개·사용표·옵션 바이트).
//  - 계약: 현재 옵션/선택 TYPE/범위표 변경을 생성자 사본이 가리지 않음, signaling NaN 의 quiet 변환, 잘못된 판본/크기와 범위 밖 인덱스 거부, 밀도 난수의 반값 범위.
// 실제 로그인/수신 소비자와 AI·지형 밀도 연결은 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E37: 미션 전투 옵션 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { RawMissionBattleOptions, resetMissionBattleOptions } from '../www/core/raw-mission-battle-options.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E37)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const resetRows = fixture('mission-option-reset-x86.tsv');
const optionRows = fixture('mission-battle-options-x86.tsv');

// 원문 DWORD 읽기(감김), 16진 변환, 리틀엔디언 DWORD 쓰기이다. (2026-11-08 WB04-E37)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const put = (bytes, at, value) => { for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (8 * index)) & 0xff; };
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };

// 두 실제 float 비트를 리틀엔디언 원문 16진으로 쓴다. (2026-11-08 WB04-E37)
const rangeText = (range, squared) => { const raw = new Uint8Array(8); put(raw, 0, range.bits); put(raw, 4, squared.bits); return hex(raw); };

// 입력 저장소는 원본 전 관측 바이트이며 결과를 계산해 기대값으로 쓰지 않는다.
test('미션 옵션 초기화: 원본10.78 296행(이웃/예약 보존·네 번째 float·파생값)', { skip: !resetRows && '원본 fixture 없음' }, () => {
    assert.equal(resetRows.length, 296);
    for (const row of resetRows) {
        assert.equal(row.length, 9);
        const bytes = Uint8Array.from(Buffer.from(row[4], 'hex'));
        assert.equal(bytes.length, 103);
        const raw = bytes.subarray(8, 8 + 87);
        const types = Array.from({ length: 188 }, () => ({ cost: 0 }));
        const use = new Uint8Array(256);
        // 실제 4 float 입력과 같은 현재 타입/사용표이다.
        const table = Uint32Array.of(floatBits(14), floatBits(22), floatBits(30), number(row[3]));
        const range = { bits: 0 };
        const squared = { bits: 0 };
        const value = { value: 0 };
        const options = new RawMissionBattleOptions({ options: raw, types, typeUse: use, rangeTable: table, selectedType: () => 0, range, rangeSquared: squared, value });
        if (row[0] === 'F') raw[4] = number(row[2]) & 0xff;
        else resetMissionBattleOptions(raw);
        if (row[0] === 'P' || row[0] === 'F') { options.updateRange(); assert.equal(rangeText(range, squared), row[6], `${row[0]} 범위`); }
        if (row[0] === 'P') { assert.equal(String(options.typeCost()), row[7]); assert.equal(String(options.currentValue()), row[8]); }
        assert.equal(hex(bytes), row[5], `${row[0]} 옵션 바이트(이웃 포함)`);
    }
});

// 잘못된 판본/물리 크기는 쓰기 전에 거부한다.
test('미션 옵션 초기화: 잘못된 크기는 쓰기 전에 거부한다', () => {
    const bytes = new Uint8Array(87).fill(0x5a);
    const original = Uint8Array.from(bytes);
    assert.throws(() => resetMissionBattleOptions(bytes.subarray(0, 86)), TypeError);
    assert.throws(() => resetMissionBattleOptions(new Uint8Array(0)), TypeError);
    assert.deepEqual(Array.from(bytes), Array.from(original));
    resetMissionBattleOptions(bytes);
    assert.deepEqual(Array.from(bytes.subarray(0, 22)), [0, 2, 2, 0, 3, 0, 2, 1, 1, 0, 0, 1, 0, 1, 1, 2, 2, 1, 2, 2, 0, 0]);
    // BYTE 23..62 는 보존하고 71..78 에 색 0..7 을 설치한다.
    assert.ok(bytes.subarray(23, 63).every((byte) => byte === 0x5a));
    assert.deepEqual(Array.from(bytes.subarray(71, 79)), [0, 1, 2, 3, 4, 5, 6, 7]);
});

// 원본과 같은 입력 저장소를 만들되 결과 계산은 제품에 맡긴다(cpppj BattleOptionFixture). (2026-11-08 WB04-E37)
class BattleOptionScene {
    constructor(input) {
        const pattern = number(input[5]);
        this.options = new Uint8Array(87);
        this.types = Array.from({ length: 188 }, () => ({ cost: 0 }));
        this.use = new Uint8Array(264);
        this.table = new Uint32Array(3);
        this.selected = number(input[4]);
        this.value = { value: 0x12345678 };
        this.range = { bits: 0x418c0000 };
        this.squared = { bits: 0x43992000 };
        // 원본과 같은 87바이트 입력을 전부 작성한다.
        for (let index = 0; index < this.options.length; index += 1) this.options[index] = (index * 11 + pattern) & 0xff;
        this.options[4] = number(input[1]) & 0xff;
        this.options[0x10] = number(input[2]) & 0xff;
        this.options[0xf] = number(input[3]) & 0xff;
        // TYPE 의 기존 비용 입력은 각각 다른 유한값이다.
        this.types.forEach((type, index) => { type.cost = Math.fround(100.25 + index); });
        // 양쪽 이웃을 포함한 사용표 초기 바이트를 공급한다.
        for (let index = 0; index < this.use.length; index += 1) this.use[index] = (index * 7 + pattern) & 0xff;
        // NaN 도 숫자 변환 없이 원문 32비트로 보존한다.
        for (let index = 0; index < 3; index += 1) this.table[index] = number(input[index + 6]);
    }

    // 모든 필드는 현재 원문을 빌리며 selected 변경도 다음 호출에 보인다.
    state() {
        return { options: this.options, types: this.types, typeUse: this.use.subarray(4, 260), rangeTable: this.table, selectedType: () => this.selected, range: this.range, rangeSquared: this.squared, value: this.value };
    }

    // 현재 188 TYPE 비용을 전부 읽어 선택 대상 밖의 보존도 대조한다.
    costText() {
        const raw = new Uint8Array(188 * 4);
        this.types.forEach((type, index) => put(raw, index * 4, floatBits(type.cost)));
        return hex(raw);
    }
}

// x87 특수 float 와 실제 0x67 부모의 288 조합을 독립 원본 출력과 비교한다.
test('미션 전투 옵션: 원본10.78 3305행(Range/Cost/Value/Clear/Apply의 범위·값·TYPE 비용·사용표·옵션)', { skip: !optionRows && '원본 fixture 없음' }, () => {
    const unique = new Set();
    let count = 0;
    for (const row of optionRows) {
        if (row[0] === 'input') continue;
        assert.equal(row.length, 8);
        assert.ok(!unique.has(row[0]), '중복 입력');
        unique.add(row[0]);
        const input = row[0].split(',');
        const scene = new BattleOptionScene(input);
        const options = new RawMissionBattleOptions(scene.state());
        let result = 0;
        if (input[0] === 'Range') options.updateRange();
        else if (input[0] === 'Cost') result = options.typeCost() >>> 0;
        else if (input[0] === 'Value') result = options.currentValue();
        else if (input[0] === 'Clear') options.clearTypes();
        else if (input[0] === 'Apply') options.applyDerived();
        else assert.fail(`알 수 없는 연산 ${input[0]}`);
        const label = `행 ${count + 1} ${row[0]}`;
        assert.equal(String(result), row[1], `${label} 결과`);
        assert.equal(rangeText(scene.range, scene.squared), row[2], `${label} 범위`);
        assert.equal(String(scene.value.value), row[3], `${label} 값`);
        assert.equal(input[0] === 'Apply' ? scene.costText() : '-', row[4], `${label} 비용`);
        assert.equal(input[0] === 'Clear' ? hex(scene.use) : '-', row[5], `${label} 사용표`);
        assert.equal(hex(scene.options), row[6], `${label} 옵션`);
        count += 1;
    }
    assert.equal(count, 3305);
});

// 현재 옵션/선택 TYPE/범위표의 변경을 생성자 사본이나 캐시가 가리지 못하게 한다.
test('미션 전투 옵션: 현재 저장소 재조회·signaling NaN quiet·잘못된 입력·밀도', () => {
    const scene = new BattleOptionScene('Apply,0,0,0,1,17,1096810496,1102053376,1106247680'.split(','));
    const options = new RawMissionBattleOptions(scene.state());
    options.applyDerived();
    const squareOf = (bits) => bits;
    assert.deepEqual([scene.range.bits, scene.types[1].cost, scene.value.value], [floatBits(14), 1000, 0]);
    scene.options[4] = 1;
    scene.options[0x10] = 3;
    scene.options[0xf] = 5;
    scene.selected = 187;
    scene.table[1] = floatBits(18);
    options.applyDerived();
    assert.deepEqual([scene.range.bits, scene.squared.bits, scene.types[187].cost, scene.types[1].cost, scene.value.value], [floatBits(18), floatBits(324), 5000, 1000, 150]);
    // 현재 CPU FLD/FSTP 진단의 두 signaling NaN 변환이다(quiet 비트만 켜고 제곱은 피연산자 NaN 보존).
    for (const bits of [0x7f800001, 0xff800001]) {
        scene.table[1] = bits;
        options.updateRange();
        assert.deepEqual([scene.range.bits, squareOf(scene.squared.bits)], [(bits | 0x00400000) >>> 0, (bits | 0x00400000) >>> 0]);
    }
    // 잘못된 저장소 크기와 범위 밖 인덱스/대상은 거부한다.
    const short = scene.state();
    short.typeUse = short.typeUse.subarray(0, 255);
    assert.throws(() => new RawMissionBattleOptions(short), TypeError);
    scene.options[4] = 3;
    assert.throws(() => options.updateRange(), RangeError);
    scene.options[4] = 0;
    scene.options[0x10] = 4;
    assert.throws(() => options.typeCost(), RangeError);
    scene.options[0xf] = 6;
    assert.throws(() => options.currentValue(), RangeError);
    scene.selected = 188;
    assert.throws(() => options.setSelectedTypeCost(1), RangeError);
    // 밀도: 옵션 7 이 고른 한계의 반값 범위에서 난수를 두 번 뽑아 합친다(난수 소비와 감김 합계).
    scene.options[7] = 2;
    const minimum = { value: 0 };
    const maximum = { value: 0 };
    const percent = { value: 0 };
    const random = new GameRandom(12345);
    const mirror = new GameRandom(12345);
    options.initializeDensity(random, minimum, maximum, percent);
    assert.deepEqual([minimum.value, maximum.value], [20, 40]);
    assert.equal(percent.value, (mirror.next(10) + 10 + mirror.next(10) + 10) >>> 0);
    scene.options[7] = 4;
    assert.throws(() => options.minimumDensity(), RangeError);
    assert.throws(() => options.maximumDensity(), RangeError);
});
