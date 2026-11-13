// Client 의 ColorSort 입력 소유자 client-color-sort-input.js(ClientColorSortInput·colorSortLogicalInput)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - colorsort-fixed-input-x86.tsv 의 6행(제어 639 기본 53비트 3행 + 895 확장 64비트 3행): 같은 최초 논리 RGB 가 화면 팔레트 변경과 무관하게 고정 주소/값으로 유지되고,
//    현재 빨강 번호로 rebuild 한 아홉 표 256바이트가 기본 53비트 관측과 같다(64비트 행은 명도 표(인덱스 8)만 원본에서 다르고 나머지 여덟 표는 같으며 이를 섞지 않는다).
//  - 계약: 명시 capture 만 입력을 바꾸고 미캡처/미생성은 거부, 독립 입력 fallback 규칙, 실제 primary 의 주소/전체 BYTE 유지, 전용 소유자 표가 저장된 명도를 직접 읽음, 누락/잘못된 초기 물리 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E374: 5-A ClientColorSortInput 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { COLOR_SORT_PRIMARY_BYTES } from '../www/core/raw-color-sort-primary.js';
import { ClientColorSortInput, colorSortLogicalInput } from '../www/core/client-color-sort-input.js';
import { buildSpecialOwnerColorMaps, buildSpecialOwnerColorMapsFromLightness } from '../www/assets/owner-colors.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E374)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'colorsort-fixed-input-x86.tsv'));

// 16진 문자열을 바이트 배열로 읽는다. (2026-11-13 WB04-E374)
const bytes = (hex) => Uint8Array.from(Buffer.from(hex, 'hex'));
// 리틀 엔디언 RGBX 256개를 원문 입력으로만 읽는다(정규화 없음). (2026-11-13 WB04-E374)
function logicalOf(hex) {
    const raw = Buffer.from(hex, 'hex');
    assert.equal(raw.length, 1024);
    return Uint32Array.from({ length: 256 }, (_, index) => raw.readUInt32LE(index * 4));
}
// 서로 다른 회색 256개와 유지되는 예약 BYTE 를 가진 명시 입력이다. (2026-11-13 WB04-E374)
const grayPalette = () => Uint32Array.from({ length: 256 }, (_, index) => (0xa5000000 | (index * 0x010101)) >>> 0);

test('ColorSortFixedInput: 원본 10.78 여섯 관측의 아홉 표와 고정 초기 입력 주소/값이 화면 입력 변경과 무관하게 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const owner = new ClientColorSortInput();
    let same = null;
    let initial = null;
    let normal53 = 0;
    let extended64 = 0;
    let different64 = 0;
    let nativeHash53 = '';
    let nativeHash64 = '';
    let count = 0;
    // 6관측을 모두 읽고 각각 지정 정밀도 계약을 검사한다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        const source = logicalOf(row[3]);
        const currentScreen = logicalOf(row[4]);
        if (!owner.captured) {
            owner.capture(source);
            initial = Uint32Array.from(source);
            same = owner.logical;
        }
        assert.ok(owner.logical === same && Buffer.compare(Buffer.from(initial.buffer), Buffer.from(owner.logical.buffer)) === 0);
        const maps = owner.rebuild(Number(row[2]));
        const expected = row[6].split(':');
        assert.equal(expected.length, 9);
        // 64비트 명도 표만 원문에서 별도 차이가 난다.
        for (let index = 0; index < 9; index += 1) {
            const native = bytes(expected[index]);
            assert.equal(native.length, 256);
            const equal = Buffer.compare(maps[index], native) === 0;
            if (row[0] === '639' || index !== 8) assert.ok(equal, `행 ${count} 표 ${index}`);
            else { assert.ok(!equal, `행 ${count} 64비트 표 8`); different64 += 1; }
        }
        if (row[0] === '639') {
            normal53 += 1;
            if (!nativeHash53) nativeHash53 = row[5];
            assert.equal(nativeHash53, row[5]);
        } else {
            assert.equal(row[0], '895');
            extended64 += 1;
            if (!nativeHash64) nativeHash64 = row[5];
            assert.equal(nativeHash64, row[5]);
        }
        // 호출자의 현재 화면 입력을 바꿔도 초기 소유자에 동기화하지 않는다.
        currentScreen.fill(0x12345678);
        assert.ok(owner.logical === same && Buffer.compare(Buffer.from(initial.buffer), Buffer.from(owner.logical.buffer)) === 0);
        count += 1;
    }
    assert.ok(count === 6 && normal53 === 3 && extended64 === 3 && different64 === 3 && nativeHash53 !== nativeHash64);
});

test('ColorSortFixedInput: 명시 capture 만 입력을 바꾸고 미캡처/미생성을 빈 현재 RGB 성공으로 바꾸지 않는다', () => {
    const owner = new ClientColorSortInput();
    assert.equal(owner.captured, false);
    assert.equal(owner.mapsReady, false);
    assert.throws(() => owner.logical, /미캡처/u);
    assert.throws(() => owner.rebuild(0), /미캡처/u);
    const first = Uint32Array.from({ length: 256 }, (_, index) => index * 0x010101);
    owner.capture(first);
    const same = owner.logical;
    first.fill(0xffffff);
    // 호출자 값 변경은 복사한 초기 입력에 전파하지 않는다.
    assert.ok(owner.logical === same && owner.logical[17] === 0x111111);
    assert.throws(() => owner.maps, /미생성/u);
    assert.equal(owner.mapsReady, false);
    const maps = owner.rebuild(0x123);
    // 현재 빨강의 하위 BYTE 만 읽는다.
    assert.equal(maps[1][17], 0x23);
    assert.ok(owner.maps === maps && owner.mapsReady);
    owner.capture(first);
    // 명시 새 생성 위치에서만 입력이 바뀌고 표는 다시 만들어야 한다.
    assert.ok(owner.logical === same && owner.logical[17] === 0xffffff);
    assert.equal(owner.mapsReady, false);
    assert.throws(() => owner.maps, /미생성/u);
    // 독립 검사만 명시한 현재 입력 fallback 을 쓰고 실제 Client 는 고정 입력 주소를 유지한다.
    assert.ok(colorSortLogicalInput([], first) === first);
    assert.ok(colorSortLogicalInput(null, first) === first);
    assert.ok(colorSortLogicalInput(owner.logical, first) === same);
    assert.throws(() => colorSortLogicalInput(first.subarray(0, 255), first), TypeError);
    // 길이가 256 이 아닌 capture 입력도 거부한다.
    assert.throws(() => owner.capture(first.subarray(0, 255)), TypeError);
    assert.throws(() => owner.capture(null), TypeError);
});

test('ClientColorSortPrimary: 실제 primary 가 화면 입력과 무관하게 같은 주소/전체 BYTE 를 유지하고 constructor 가 쓰지 않은 padding 을 그대로 소유한다', () => {
    const owner = new ClientColorSortInput();
    const logical = grayPalette();
    const allocation = new Uint8Array(COLOR_SORT_PRIMARY_BYTES).fill(0xa7);
    owner.captureWithPrimary(logical, allocation);
    const primary = owner.primary;
    const before = Uint8Array.from(primary.bytes);
    const address = primary.bytes;
    // 화면 입력(현재 팔레트)이 바뀌어도 primary 는 같은 객체/바이트이다.
    const screen = Uint32Array.from(logical);
    screen[17] = 0x00c15b21;
    assert.ok(owner.primary === primary && primary.bytes === address);
    assert.equal(Buffer.compare(Buffer.from(before), Buffer.from(primary.bytes)), 0);
    assert.equal(Buffer.compare(Buffer.from(logical.buffer), Buffer.from(owner.logical.buffer)), 0);
    // 각 56바이트 기록의 +52..55 는 constructor 가 쓰지 않은 초기 물리 그대로이다.
    for (let index = 0; index < 256; index += 1) assert.ok(primary.bytes.subarray(index * 56 + 52, index * 56 + 56).every((byte) => byte === 0xa7), `기록 ${index}`);
    // 호출자 할당 배열은 복사해 쓰므로 이후 변경은 전파하지 않는다.
    allocation.fill(0);
    assert.ok(primary.bytes.subarray(52, 56).every((byte) => byte === 0xa7));
});

// primary 의 저장 명도(+8 double)를 명시 변경한다(RGB 와 다른 필드는 쓰지 않는다). (2026-11-13 WB04-E374)
function storeLightness(primary, index, value) {
    new DataView(primary.bytes.buffer, primary.bytes.byteOffset, primary.bytes.byteLength).setFloat64(index * 56 + 8, value, true);
}

test('ClientColorSortPrimary: 전용 소유자 표는 기존 결과를 유지하고 저장된 명도만 바꾸면 현재 저장값을 직접 읽는다', () => {
    const owner = new ClientColorSortInput();
    const logical = grayPalette();
    owner.captureWithPrimary(logical, new Uint8Array(COLOR_SORT_PRIMARY_BYTES).fill(0x63));
    const primary = owner.primary;
    const lightOf = () => Float64Array.from({ length: 256 }, (_, index) => primary.lightness(index));
    // 모든 전용 타입에서 저장 명도 입력과 RGB 재계산 입력의 결과가 같다.
    for (const type of [111, 141, 137, 94, 97]) assert.deepEqual(buildSpecialOwnerColorMapsFromLightness(lightOf(), type), buildSpecialOwnerColorMaps(logical, type), `타입 ${type}`);
    // RGB 는 그대로 두고 L 만 별도 입력으로 만든다.
    for (let index = 0; index < 256; index += 1) storeLightness(primary, index, 0.0);
    storeLightness(primary, 86, 0.75);
    storeLightness(primary, 7, 0.75);
    // 모든 소유자가 1..8 의 명시 후보를 쓴다: 동률 없이 후보 7(shade 6)만 저장 L 과 일치한다.
    const candidates = new Array(72).fill(0);
    for (let row = 1; row < 9; row += 1) for (let shade = 0; shade < 8; shade += 1) candidates[row * 8 + shade] = shade + 1;
    const raw = buildSpecialOwnerColorMapsFromLightness(lightOf(), 111, candidates);
    const legacy = buildSpecialOwnerColorMaps(logical, 111, candidates);
    assert.ok(raw[0][86] === 86 && raw[1][17] === 17 && JSON.stringify(raw) !== JSON.stringify(legacy));
    // RGB 재계산이면 이 기대를 통과할 수 없다.
    for (let row = 1; row < 9; row += 1) assert.ok(raw[row][86] === 7 && legacy[row][86] === 8, `행 ${row}`);
    assert.equal(Buffer.compare(Buffer.from(logical.buffer), Buffer.from(owner.logical.buffer)), 0);
});

test('ClientColorSortPrimary: 실제 raw 를 요구하고 재캡처 실패가 이전 raw 소비자를 깨뜨리지 않으며 정확 0x3800 이 아닌 입력을 거부한다', () => {
    const owner = new ClientColorSortInput();
    const logical = grayPalette();
    // 독립 logical 캡처는 실제 raw 를 만들지 않는다.
    owner.capture(logical);
    assert.throws(() => owner.primary, /primary 미연결/u);
    owner.captureWithPrimary(logical);
    assert.ok(owner.primary.constructed && owner.captured);
    const retained = owner.primary;
    // 기존 소비자가 보유한 이전 raw 는 참조 수명까지 유효하다.
    owner.capture(logical);
    assert.throws(() => owner.primary, /primary 미연결/u);
    assert.ok(retained.constructed);
    // 정확 0x3800 이 아닌 입력은 실패하고 captured 도 거짓이다.
    assert.throws(() => owner.captureWithPrimary(logical, new Uint8Array(8)), TypeError);
    assert.equal(owner.captured, false);
    assert.throws(() => owner.primary, /primary 미연결/u);
    assert.ok(retained.constructed);
    assert.equal(owner.mapsReady, false);
    // 소유자 색표 저장소는 입력 캡처/재생성과 무관하게 같은 객체이다.
    const tables = owner.ownerTables;
    owner.captureWithPrimary(logical);
    assert.ok(owner.ownerTables === tables);
});

test('ClientColorSortInput: 아홉 표 배열 길이와 첫 표·명도 표·항등 표가 번호 0 부터 비검정 팔레트에서도 독립 구현과 같다', () => {
    // 색 0 이 검정이 아닌 합성 팔레트(원본 관측의 색 0 은 검정이라 0번 처리 누락이 가려진다).
    const logical = Uint32Array.from({ length: 256 }, (_, index) => ((index * 37) & 255) | ((((index * 91) + 13) & 255) << 8) | ((((index * 53) + 7) & 255) << 16));
    // 색 0 을 밝은 주황으로 두고 같은 색을 뒤 번호(7)에 복제한다: 0번 처리가 빠지면 0번 후보의 거리와 두 색의 최근접이 달라진다.
    logical[0] = 0x003264c8;
    logical[7] = logical[0];
    assert.notEqual(logical[0], 0);
    const owner = new ClientColorSortInput();
    owner.capture(logical);
    const maps = owner.rebuild(77);
    assert.equal(maps.length, 9);
    // 독립 구현: 색마다 RGB(0..1)와 명도를 만들고 1.6배 RGB 의 L1 최근접과 (명도+0.2)/3 의 명도 최근접을 번호 순서로 찾는다.
    const normalize = 1.0 / 255.0;
    const rgb = Array.from(logical, (value) => [(value & 255) * normalize, ((value >>> 8) & 255) * normalize, ((value >>> 16) & 255) * normalize]);
    const light = rgb.map(([red, green, blue]) => (Math.max(red, green, blue) + Math.min(red, green, blue)) * 0.5);
    for (let index = 0; index < 256; index += 1) {
        const target = rgb[index].map((value) => value * 1.600000023841858);
        let best = 1e6;
        let bestIndex = 0;
        let bestLight = 1e6;
        let bestLightIndex = 0;
        // 후보를 번호 순서로 엄격 비교한다(RGB 최솟값은 float32 로 저장).
        for (let candidate = 0; candidate < 256; candidate += 1) {
            const distance = Math.abs(target[0] - rgb[candidate][0]) + Math.abs(target[1] - rgb[candidate][1]) + Math.abs(target[2] - rgb[candidate][2]);
            if (distance < best) { best = Math.fround(distance); bestIndex = candidate; }
            const lightDistance = Math.abs((light[index] + 0.20000000298023224) * 0.3333333333333333 - light[candidate]);
            if (lightDistance < bestLight) { bestLight = lightDistance; bestLightIndex = candidate; }
        }
        const reserved = index >= 228 && index <= 245;
        assert.equal(maps[0][index], reserved || index >= 254 ? index : bestIndex, `첫 표 ${index}`);
        assert.equal(maps[8][index], index >= 254 ? index : bestLightIndex, `명도 표 ${index}`);
        assert.equal(maps[3][index], index, `항등 표 ${index}`);
    }
    // 파괴 표는 현재 빨강의 하위 BYTE, 글자 그림자 표는 0 이다.
    assert.ok(maps[1].every((value) => value === 77));
    assert.ok(maps[5].every((value) => value === 0));
    // 다시 rebuild 해도 같은 표 객체를 갱신한다.
    assert.ok(owner.rebuild(200) === maps && maps[1].every((value) => value === 200));
});
