// 타입 레코드 완성(WB04-E265, 순서4 4-A 첫 단위)을 cpppj RiftTypeTable 검사와 원본 10.78 관측으로 대조한다.
//  - CreationTests SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata: .type 의 maxHitPoints/zorder(이름+오프셋·숫자)가 레코드에 담기고 Factory 가 같은 레코드로 슬롯을 초기화한다.
//  - type-default-cost-x86.tsv 의 10.78 1500행(TypeDefaultCostTests): 그룹·레벨·비용·운반/일반/사제/사람 flags 조합의 비용 비트.
//  - RiftTypeTests: 실제 자산의 range/hpPerSec/useairdamage/airdamage 와 합성 중복 속성의 float 비트, archertype-x86.tsv 22행의 방향 수/물리 프레임 수.
// 실제 게임·브라우저 실행이 아니다. 모든 기대값은 fixture 나 보호 자산의 원래 선언이며 웹 계산으로 만들지 않는다. (2026-11-10 WB04-E265)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, floatToBits } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { TYPE_LOAD_ORDER, parseTypeDefinition } from '../www/assets/type-table.js';
import { buildTypeRecords } from '../www/assets/type-flags.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-10 WB04-E265)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const costRows = fixture('type-default-cost-x86.tsv');
const archerRows = fixture('archertype-x86.tsv');

// 아무 속성도 없는 정의이다(원본 RiftTypeDefinition 기본값: 클러스터 없음, 기본 프레임 0). (2026-11-10 WB04-E265)
const EMPTY = Object.freeze({ name: '', constructorWords: [], flags: [], properties: [], clusters: [], defaultFrame: 0, gumpFrame: -1, helpFrame: -1, baseFrame: -1 });
// 비트 문자열을 부호 없는 수로 읽는다. (2026-11-10 WB04-E265)
const bits = (text) => Number(BigInt(text) & 0xffffffffn);
// 첫 자산(번호 70)만 정의를 주고 나머지는 빈 정의인 전체 로더 입력이다. (2026-11-10 WB04-E265)
const firstOnly = (definition) => TYPE_LOAD_ORDER.map((_, index) => (index === 0 ? definition : EMPTY));
// 지정 번호의 자산에만 정의를 주는 전체 로더 입력이다. (2026-11-10 WB04-E265)
const only = (number, definition) => TYPE_LOAD_ORDER.map((_, index) => (index + 70 === number ? definition : EMPTY));

// .type 의 HP/깊이가 레코드에 연결되고 Factory 가 새 슬롯에 쓰는지 본다(CreationTests, 패치판).
test('타입 로더: maxHitPoints·zorder(이름+오프셋/숫자)·생성자 주소가 Factory 새 SID 초기화에 연결된다', () => {
    const definitions = TYPE_LOAD_ORDER.map((order, index) => {
        // base 74 에는 상수 이름+오프셋, base 80 에는 숫자 깊이를 넣는다.
        const body = index === 4 ? '{ maxHitPoints=65536; zorder="zoBRIDGE_CONNECTOR - 3"; }' : (index === 10 ? '{ zorder=511; }' : '{}');
        return parseTypeDefinition(`typename ${order} ${body} A00:default:"a.gif"#0;`);
    });
    const records = buildTypeRecords(definitions);
    assert.deepEqual([records[74].maxHitPoints, records[74].zOrder], [65536, -13]);
    assert.ok(records[80].zOrder === 511 && records[82].constructorAddress !== 0 && records[74].constructorAddress === 0);
    assert.equal(records[82].constructorAddress, typeConstructorAddress(82));
    const pool = new SidPool(32768);
    const factory = new SquidFactory(pool, records.map((record) => ({ ...record })));
    const sid = factory.create(74);
    const bytes = pool.slot(sid);
    // zOrder -13 의 하위 BYTE 243, HP 65536 의 낮은 두 BYTE 0 과 셋째 BYTE 1(패치 +28).
    assert.deepEqual([bytes[35], bytes[26], bytes[27], bytes[28]], [243, 0, 0, 1]);
    assert.equal(factory.create(82), sid + 1);
});

// zorder 문자열은 첫 식별자의 표 순서 부분 일치와 선택적 +/- 오프셋으로 읽고 모르는 이름/큰 오프셋은 거부한다.
test('타입 로더: zorder 문자열은 부분 일치·오프셋 가감을 따르고 잘못된 입력을 거부한다', () => {
    const zOrderOf = (text) => buildTypeRecords(firstOnly(parseTypeDefinition(`typename a { zorder="${text}"; } A00:default:"a.gif"#0;`)))[70].zOrder;
    assert.equal(zOrderOf('zoNONE'), -127);
    assert.equal(zOrderOf('zoFLYERS + 5'), -25);
    assert.equal(zOrderOf('zoUBERGUMP - 100'), -140);
    // 표 순서의 첫 부분 일치: 'zoBRIDGE' 는 zoBRIDGE(0)가 zoBRIDGE_CONNECTOR(-10)보다 앞이다.
    assert.equal(zOrderOf('zoBRIDGE'), 0);
    // 대소문자를 구분하는 부분 문자열 비교이므로 소문자 이름은 표에 없다.
    assert.throws(() => zOrderOf('zonone'), /알 수 없는 zorder/);
    assert.throws(() => zOrderOf('zoNONE + 2147483648'), RangeError);
    // 숫자 깊이는 0 쪽으로 절삭한 정수이다.
    assert.equal(buildTypeRecords(firstOnly(parseTypeDefinition('typename a { zorder=-3.9; } A00:default:"a.gif"#0;')))[70].zOrder, -3);
});

// 비용 후처리의 기본 가격(1500행). 원문 기대 비트는 fixture 에서 읽는다.
test('타입 로더: 그룹·레벨·명시 비용·운반/일반/사제/사람 조합의 비용 비트가 원본 10.78 1500행과 같다', { skip: !costRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 판본 접두가 originals 인 행만 쓴다(CD 10.72 는 범위 밖).
    for (const row of costRows) {
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 7);
        const number = Number(row[1]);
        const group = Number(row[2]);
        const flags = bits(row[3]);
        const level = Number(row[4]);
        const before = new DataView(new ArrayBuffer(4));
        before.setUint32(0, bits(row[5]), true);
        const definition = { ...EMPTY, flags: [], properties: [] };
        if (group !== 10) definition.properties.push({ name: 'group', value: group === 0 ? 'archer' : group === 3 ? 'aviary' : group === 7 ? 'walker' : 'balloon' });
        definition.properties.push({ name: 'level', value: level + 1 });
        definition.properties.push({ name: 'cost', value: before.getFloat32(0, true) });
        if (flags & 0x10000) definition.flags.push('walker');
        if (flags & 0x20000) definition.flags.push('balloon');
        if (flags & 0x400) definition.flags.push('guy');
        if (flags & 0x200000) definition.flags.push('priest');
        const record = buildTypeRecords(only(number, definition))[number];
        assert.equal(floatToBits(record.cost), bits(row[6]), `행 ${count}: ${row.join(',')}`);
        count += 1;
    }
    assert.equal(count, 1500);
});

// 보호 자산에서 생략된 비용 속성과 명시 비용의 보존(TypeDefaultCostTests ProtectedAssets).
test('타입 로더: 실제 자산의 생략 비용은 기본 단가로, 명시 비용은 그대로 남는다', () => {
    const { types, table } = realCaptureAssets();
    assert.ok(types[133].cost === 400 && types[72].cost === 400);
    assert.ok(types[158].cost === 0 && types[70].cost === 0);
    assert.equal(types[71].cost, 300);
    assert.ok(types[128].cost === 400 && types[135].cost === 500);
    const hasCost = (number) => table.definitions[number - 70].properties.some((property) => property.name.toLowerCase() === 'cost');
    assert.ok(!hasCost(133) && !hasCost(72));
});

// 실제 자산의 range/hpPerSec/useairdamage/airdamage 는 정의의 선언값(float32/절삭 정수)이고 내장 타입은 원문 초기 0 이다(RiftTypeTests 실자산 3건).
test('타입 로더: 실제 자산의 사거리·피해율·공중 피해율이 정의와 같고 내장 타입은 0 이다', () => {
    const { types, table } = realCaptureAssets();
    const numberOf = (definition, name) => {
        const found = definition.properties.filter((property) => property.name.toLowerCase() === name && typeof property.value === 'number');
        return found.length ? found[found.length - 1].value : 0;
    };
    let checked = 0;
    let airEnabled = 0;
    let rateOverZero = 0;
    // 모든 자산의 세 수치를 정의와 대조한다.
    table.definitions.forEach((definition, index) => {
        const number = 70 + index;
        assert.equal(types[number].range, Math.fround(numberOf(definition, 'range')));
        assert.equal(types[number].damagePerSecond, Math.fround(numberOf(definition, 'hppersec')));
        const mode = Math.trunc(numberOf(definition, 'useairdamage'));
        assert.equal(types[number].useAirDamage, mode);
        assert.equal(types[number].airDamagePerSecond, Math.fround(numberOf(definition, 'airdamage')));
        if (mode !== 0) airEnabled += 1;
        if (types[number].damagePerSecond > 0) rateOverZero += 1;
        checked += 1;
    });
    // 실제 패치 자산 수와 대표 값: TYPE 71 의 피해율 15, 공중 피해 모드 1/율 20 이 유일한 비영 선언이다.
    assert.equal(checked, table.definitions.length);
    assert.ok(rateOverZero > 0 && types[71].damagePerSecond === 15);
    assert.deepEqual([airEnabled, types[71].useAirDamage, types[71].airDamagePerSecond], [1, 1, 20]);
    for (let number = 0; number < 70; number += 1) {
        assert.ok(types[number].range === 0 && types[number].damagePerSecond === 0 && types[number].useAirDamage === 0 && types[number].airDamagePerSecond === 0, `내장 ${number}`);
    }
});

// 순서가 있는 중복 속성은 마지막 값이 float32 비트로 남는다(RiftTypeRange_DefaultDuplicateOrderAndFloatBits 외).
test('타입 로더: 중복 range/hpPerSec/useairdamage/airdamage 는 마지막 값이고 대소문자를 구분하지 않는다', () => {
    const changed = parseTypeDefinition('typename air { range=3.25; range=-7.5; hpPerSec=3.25; HpPeRsEc=-7.5; useairdamage=2.9; USEAIRDAMAGE=-1.75; airdamage=3.25; AIRDAMAGE=-7.5; } A00:default:"a.gif"#0;');
    const records = buildTypeRecords(firstOnly(changed));
    assert.equal(floatToBits(records[70].range), 0xc0f00000);
    assert.equal(floatToBits(records[70].damagePerSecond), 0xc0f00000);
    assert.equal(floatToBits(records[70].airDamagePerSecond), 0xc0f00000);
    // -1.75 는 0 쪽으로 절삭해 -1 이다.
    assert.equal(records[70].useAirDamage, -1);
    // 다음 TYPE 은 영향을 받지 않는다.
    assert.deepEqual([records[71].range, records[71].damagePerSecond, records[71].useAirDamage, records[71].airDamagePerSecond], [0, 0, 0, 0]);
});

// 아처 소비자의 방향 수/물리 프레임 수(TYPE+110/+114)를 원본 22입력 관측과 대조한다(RiftTypeAnimation_PatchGoldenMetadata).
test('타입 로더: 방향 수와 물리 프레임 수가 원본 10.78 22입력 관측과 같고 내장 타입은 0 이다', { skip: !archerRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 접두 행은 빈 정의, 전체 callback 행은 16진 입력 텍스트를 읽는다.
    for (const row of archerRows) {
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 8);
        const text = Buffer.from(row[4] === '-' ? '' : row[4], 'hex').toString('latin1');
        const definition = row[1] === 'prefix' ? EMPTY : parseTypeDefinition(text);
        const records = buildTypeRecords(firstOnly(definition));
        const [directions, frameCount] = row[5].split(',').slice(0, 2).map(Number);
        assert.deepEqual([records[70].directions, records[70].frameCount], [directions, frameCount], `행 ${count}`);
        for (let number = 0; number < 70; number += 1) assert.deepEqual([records[number].directions, records[number].frameCount], [0, 0]);
        count += 1;
    }
    assert.equal(count, 22);
});

// 레코드 초기값과 theme/techBit/hotFootRatio/level/kind 의 변환(원문 초기화와 속성 순서).
test('타입 로더: theme·techBit·hotFootRatio·level·종류 필드는 원본 변환과 초기값을 따른다', () => {
    const empty = buildTypeRecords(firstOnly(EMPTY));
    // 전체 TYPE 의 theme 초기값은 패치 5 이고 level 은 0 이다.
    assert.ok(empty.every((record) => record.theme === 5 && record.level === 0 && record.hotspotX === 0 && record.hotspotY === 0 && record.kind === '' && record.zOrder === 0));
    const parsed = parseTypeDefinition('typename a typeflags walker; { theme="s"; techBit=70000.7; hotFootRatioX=1.5; hotFootRatioY=0.75; level=3; } A00:default:"a.gif"#0;');
    const record = buildTypeRecords(firstOnly(parsed))[70];
    // 't' 계열 문자 매핑(여기서는 s: 인덱스 3 % 5), 정수 절삭, 기준점은 16/11 배율의 0 쪽 절삭, level 은 자료값 − 1, 종류는 첫 종류 단어.
    assert.deepEqual([record.theme, record.techBitIndex, record.hotspotX, record.hotspotY, record.level, record.kind], [3, 70000, 24, 8, 2, 'walker']);
    assert.throws(() => buildTypeRecords(firstOnly(parseTypeDefinition('typename a { hotFootX=1; } A00:default:"a.gif"#0;'))), /hotFootX/);
    assert.throws(() => buildTypeRecords(firstOnly(parseTypeDefinition('typename a { theme="~"; } A00:default:"a.gif"#0;'))), RangeError);
    // maxHitPoints 를 두 번 선언하면 원문 assert(maxHitPoints == 0) 가 난다.
    assert.throws(() => buildTypeRecords(firstOnly(parseTypeDefinition('typename a { maxHitPoints=5; maxHitPoints=6; } A00:default:"a.gif"#0;'))), /maxHitPoints == 0/);
});

// theme 문자열의 첫 BYTE 가 TYPE+98 조회 표의 인덱스(% 5)나 기본값(90 미만 5, 그 외 0)을 고른다(변형 확인으로 추가: 모듈러·기본값 상수가 앞 시험에서 구별되지 않았다).
test('타입 로더: theme 첫 글자별 조회 표와 기본값', () => {
    const themeOf = (text) => buildTypeRecords(firstOnly(parseTypeDefinition(`typename a { theme="${text}"; } A00:default:"a.gif"#0;`)))[70].theme;
    // 'wrtsWRTS.?>' 의 앞 10글자는 위치 % 5, 표에 없는 글자는 90 미만이면 5, 그 이상이면 0.
    const cases = { w: 0, r: 1, t: 2, s: 3, W: 4, R: 0, T: 1, S: 2, '.': 3, '?': 4, '>': 5, A: 5, a: 0 };
    for (const [text, expected] of Object.entries(cases)) assert.equal(themeOf(text), expected, text);
});

// techBit 는 float32 로 줄인 뒤 0 쪽 절삭 낮은 DWORD 이며 비유한/2^63 이상은 0 이다.
test('타입 로더: techBit 는 float32 축약 뒤 절삭한 낮은 DWORD 이다', () => {
    const techBitOf = (value) => buildTypeRecords(firstOnly(parseTypeDefinition(`typename a { techBit=${value}; } A00:default:"a.gif"#0;`)))[70].techBitIndex;
    assert.equal(techBitOf(70000.7), 70000);
    // 2^24+1 은 float32 로 2^24 가 된다.
    assert.equal(techBitOf(16777217), 16777216);
    assert.equal(techBitOf(-2.5), -2);
    // 2^63 이상은 0, 2^32 를 넘는 값은 낮은 DWORD 만 남는다(2^32+5 는 float32 로 2^32 라 0).
    assert.equal(techBitOf(1e30), 0);
    assert.equal(techBitOf(4294967296), 0);
    assert.equal(techBitOf(4294967040), -256);
});

// 모든 자산 정의의 기본 프레임(+128)이 레코드에 그대로 담기고 내장 타입은 0 이다.
test('타입 로더: 실제 자산의 기본 프레임이 레코드에 담긴다', () => {
    const { types, table } = realCaptureAssets();
    let nonZero = 0;
    table.definitions.forEach((definition, index) => {
        assert.equal(types[70 + index].defaultFrame, definition.defaultFrame);
        if (definition.defaultFrame !== 0) nonZero += 1;
    });
    assert.ok(nonZero > 0);
    for (let number = 0; number < 70; number += 1) assert.equal(types[number].defaultFrame, 0);
});
