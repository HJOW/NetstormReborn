// 소유자 색표 수명 owner-color-table-state.js(setBannerOption·initializeOwnerColorTables·OwnerColorTableState)를 원본 10.78 관측과 계약으로 대조한다.
//  - banner-option-x86.tsv 의 2,066행(operation init 16·write 2,050): 시작 입력(ready·enabled·redraw·존재 마스크·배너 타입 다섯·연속 요청·skip)을 주고 실행해
//    끝의 ready·enabled·redraw DWORD, 188칸 현재 포인터 표 전체, 생성(standard/special 과 오프셋 double 비트·소스 목록)/반납(free) 효과 순서가 같다. 연속 요청 사이에 포인터/설정을 다시 만들지 않는다.
//  - 계약: 장기 저장소가 최초 등록 전에는 설정 해석이 무시되고 초기화 뒤 토큰 배열 주소가 바뀌지 않으며, 켜고 끌 때 97·94 표와 배너 표가 만들어지고 반납되고, 토큰으로 같은 표를 빌리고, 잘못된 입력은 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E374: 5-A OwnerColorTableState 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { RawColorSortPrimary, COLOR_SORT_PRIMARY_BYTES } from '../www/core/raw-color-sort-primary.js';
import { OwnerColorTableState, defaultOwnerColorTableTypes, initializeOwnerColorTables, setBannerOption } from '../www/core/owner-color-table-state.js';

// 원본 관측 행이다(첫 줄은 열 이름). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E374)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'banner-option-x86.tsv'))?.filter((row) => row[0] !== 'name') ?? null;

// 타입 번호가 고정 별칭 토큰이 된다(native 명시 할당 장치와 같은 규칙). (2026-11-13 WB04-E374)
const token = (type) => 0x100000 + type * 2304;
// double 의 원시 비트를 부호 없는 십진 문자열로 만든다(관측 trace 와 같은 표기). (2026-11-13 WB04-E374)
function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}

test('BannerOption: 원본 10.78 banner-option 2,066행의 끝 DWORD·188칸 포인터·생성/반납 효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 입력 9열을 먼저 공급하고 실행 뒤 관측 6열을 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 15);
        count += 1;
        const globals = { ready: Number(row[2]), enabled: Number(row[3]), redraw: Number(row[4]), tables: new Array(188).fill(0) };
        globals.tables[111] = token(111);
        const types = defaultOwnerColorTableTypes();
        const typeInput = row[6].split(',').map(Number);
        assert.equal(typeInput.length, 5);
        // 현재 타입 전역과 존재 마스크를 입력한다.
        typeInput.forEach((type, index) => {
            types.banner[index] = type;
            if (Number(row[5]) & (1 << index)) globals.tables[type] = token(type);
        });
        const trace = [];
        const hooks = {
            standard: (type) => { trace.push(`standard:${type}`); globals.tables[type] = token(type); },
            special: (type, offset, sources) => { trace.push(`special:${type}:${doubleBits(offset)}:${sources.join(',')}`); globals.tables[type] = token(type); },
            release: (pointer) => { trace.push(`free:${pointer}`); },
        };
        // 연속 요청 사이에 현재 포인터/설정을 재생성하지 않는다.
        for (const request of row[7].split(',')) {
            if (row[1] === 'init') initializeOwnerColorTables(globals, Number(row[8]), types, hooks);
            else setBannerOption(globals, Number(request), types, hooks);
        }
        const where = `행 ${count} ${row[0]}`;
        assert.equal(globals.ready, Number(row[9]), `${where} ready`);
        assert.equal(globals.enabled, Number(row[10]), `${where} enabled`);
        assert.equal(globals.redraw, Number(row[11]), `${where} redraw`);
        assert.equal(globals.tables.join(','), row[12], `${where} 포인터`);
        assert.equal(trace.length ? trace.join(';') : '-', row[13], `${where} 효과`);
        assert.ok(Number(row[14]) > 0);
    }
    assert.equal(count, 2066);
});

// 회색 팔레트로 만든 primary 이다(명도 전용 표가 읽는 저장 명도 입력). (2026-11-13 WB04-E374)
function grayPrimary() {
    const primary = new RawColorSortPrimary(new Uint8Array(COLOR_SORT_PRIMARY_BYTES));
    const rgb = Uint32Array.from({ length: 256 }, (_, index) => index | (index << 8) | (index << 16));
    primary.construct(new Uint8Array(rgb.buffer));
    return primary;
}

test('OwnerColorTableState: 최초 등록 전 설정 해석은 무시되고 초기화 뒤 토큰 배열 주소와 표가 켜기/끄기를 거쳐 유지된다', () => {
    const owner = new OwnerColorTableState();
    const redraw = { value: 0 };
    owner.setEnabled(0, redraw);
    assert.equal(owner.enabled, 1);
    assert.equal(owner.ready, false);
    assert.equal(redraw.value, 0);
    assert.throws(() => owner.tokens, /최초 등록 전/u);
    owner.initialize(grayPrimary(), 188, 82, redraw);
    const tokens = owner.tokens;
    assert.ok(owner.ready);
    assert.equal(redraw.value, 1);
    assert.equal(owner.enabled, 1);
    // 일반 표(다리 82 포함)와 전용 표(111·141·137)는 만들어지고 배너 표(94·97·85·71·128)는 없다.
    for (const type of [82, 111, 141, 137]) assert.notEqual(tokens[type], 0, `타입 ${type}`);
    for (const type of [94, 97, 85, 71, 128]) assert.equal(tokens[type], 0, `타입 ${type}`);
    // 배너를 끄면(0) 지면 97·받침 윗면 94 표가 생기고 다시 그리기가 한 번 늘어난다.
    owner.setEnabled(0, redraw);
    assert.equal(redraw.value, 2);
    assert.equal(owner.enabled, 0);
    assert.notEqual(tokens[94], 0);
    assert.notEqual(tokens[97], 0);
    const color = owner.color(97, 2);
    assert.equal(owner.colorToken(tokens[97] + 512), color);
    // 같은 값이면 아무 것도 하지 않아 표 주소도 그대로이다.
    owner.setEnabled(0, redraw);
    assert.equal(redraw.value, 2);
    assert.equal(owner.color(97, 2), color);
    assert.equal(owner.tokens, tokens);
    // 다시 켜면 97·94 표가 반납되고 일반/전용 표는 그대로이다.
    owner.setEnabled(1, redraw);
    assert.equal(redraw.value, 3);
    assert.equal(owner.enabled, 1);
    assert.equal(tokens[94], 0);
    assert.equal(tokens[97], 0);
    assert.notEqual(tokens[82], 0);
    assert.notEqual(tokens[111], 0);
    assert.equal(owner.tokens, tokens);
});

test('OwnerColorTableState: 조회·토큰 판독·최초 등록의 잘못된 입력을 거부하고 다시 그리기 DWORD 는 unsigned 로 감긴다', () => {
    const owner = new OwnerColorTableState();
    const redraw = { value: 0xffffffff };
    const primary = grayPrimary();
    // 필수 물리가 없거나 타입 수가 모자라면 등록하지 않는다.
    assert.throws(() => owner.initialize(null, 188, 82, redraw), TypeError);
    assert.throws(() => owner.initialize(new RawColorSortPrimary(new Uint8Array(COLOR_SORT_PRIMARY_BYTES)), 188, 82, redraw), TypeError);
    assert.throws(() => owner.initialize(primary, 170, 82, redraw), TypeError);
    assert.equal(owner.ready, false);
    owner.initialize(primary, 188, 82, redraw, 1);
    // 다시 그리기 DWORD 는 0xffffffff + 1 = 0 으로 감긴다(최초 등록의 setBannerOption 호출).
    assert.equal(redraw.value, 0);
    // skip 이 참이면 일반 표 앞 둘(82·155)은 만들지 않는다.
    assert.equal(owner.tokens[82], 0);
    assert.equal(owner.tokens[155], 0);
    assert.notEqual(owner.tokens[158], 0);
    // 최초 등록은 한 번뿐이다.
    assert.throws(() => owner.initialize(primary, 188, 82, redraw), TypeError);
    // 조회 계약: 표 밖/없는 표/색 번호 9 이상/정렬 어긋난 토큰/기준 아래 토큰은 거부한다.
    assert.throws(() => owner.color(188, 0), RangeError);
    assert.throws(() => owner.color(94, 0), RangeError);
    assert.throws(() => owner.color(158, 9), RangeError);
    assert.ok(owner.color(158, 8));
    assert.throws(() => owner.colorToken(0x100000 + 158 * 2304 + 1), RangeError);
    assert.throws(() => owner.colorToken(0xfffff), RangeError);
    assert.equal(owner.colorToken(0x100000 + 158 * 2304), owner.color(158, 0));
    assert.equal(owner.colorToken(0x100000 + 158 * 2304 + 8 * 256), owner.color(158, 8));
});

test('OwnerColorTableState: 전용 표는 primary 가 저장한 명도를 읽고 소스/오프셋 계약을 어기면 거부한다', () => {
    const redraw = { value: 0 };
    const owner = new OwnerColorTableState();
    owner.initialize(grayPrimary(), 188, 82, redraw);
    // 항등 행(중립 0)과 소유자 행의 소스 색은 정렬된 표이다.
    assert.equal(owner.color(111, 0)[17], 17);
    assert.equal(owner.color(111, 1).length, 256);
    // 일반 띠 표는 228..245 만 바꾸고 나머지는 항등이다.
    const standard = owner.color(158, 3);
    for (let index = 0; index < 228; index += 1) assert.equal(standard[index], index);
    assert.ok(standard.slice(228, 246).some((value, offset) => value !== 228 + offset));
    // 지원하지 않는 전용 소스·잘못된 오프셋은 같은 훅 계약에서 거부한다(내부 훅은 setEnabled 경로로만 닿으므로 함수로 직접 검사).
    const globals = { ready: 1, enabled: 1, redraw: 0, tables: new Array(188).fill(0) };
    assert.throws(() => setBannerOption(globals, 0, defaultOwnerColorTableTypes(), {}), /장치 미연결/u);
    assert.throws(() => setBannerOption({ ...globals, tables: [] }, 5, defaultOwnerColorTableTypes(), {}), RangeError);
    assert.throws(() => initializeOwnerColorTables({ ready: 0, enabled: 1, redraw: 0, tables: new Array(188).fill(0) }, 0, defaultOwnerColorTableTypes(), {}), /장치 미연결/u);
});

test('OwnerColorTableState: 기본 TYPE 전역 값·최소 타입 수 171 경계·토큰 배열 길이·살아 있는 토큰 변경 반영·배너 범위와 토큰 정렬 경계를 지킨다', () => {
    // 보호 10.78 의 현재 TYPE 전역 값은 원본 표 그대로이다.
    assert.deepEqual(defaultOwnerColorTableTypes(), {
        banner: [85, 71, 128, 97, 94],
        standard: [82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126],
        special: [111, 141, 137],
    });
    // 타입 수 171 은 허용하고(정확히 최소), 그 길이만큼의 토큰/표를 만든다. 170 은 이미 거부된다.
    const redraw = { value: 0 };
    const owner = new OwnerColorTableState();
    owner.initialize(grayPrimary(), 171, 82, redraw);
    assert.equal(owner.tokens.length, 171);
    assert.notEqual(owner.tokens[170], 0);
    const big = new OwnerColorTableState();
    big.initialize(grayPrimary(), 188, 82, { value: 0 });
    assert.equal(big.tokens.length, 188);
    // 반환된 토큰 배열은 살아 있는 보기이다: 원소를 0 으로 쓰면 그 표 조회가 거부되고, 표가 반납된 타입에 임의 토큰을 써도 표가 없으면 거부된다.
    const live = big.tokens;
    const kept = live[158];
    live[158] = 0;
    assert.throws(() => big.color(158, 0), RangeError);
    live[158] = kept;
    assert.ok(big.color(158, 0));
    live[94] = 0x100000 + 94 * 2304;
    assert.throws(() => big.color(94, 0), RangeError);
    live[94] = 0;
    // 색 번호 8 까지 허용하고 9 부터 거부한다.
    assert.ok(big.color(158, 8));
    assert.throws(() => big.color(158, 9), RangeError);
    // 토큰 기준값과 정확히 같은 값은 정렬 검사를 통과해 타입 0 조회 계약에서 거부된다(정렬 문구가 아님).
    assert.throws(() => big.colorToken(0x100000), { name: 'RangeError', message: /조회 계약/u });
    assert.throws(() => big.colorToken(0x100001), { name: 'RangeError', message: /토큰 정렬/u });
    // 배너 타입이 포인터 표 길이와 같으면(범위 밖) 거부하고 바로 앞 번호는 허용한다.
    const types = defaultOwnerColorTableTypes();
    types.banner[0] = 5;
    assert.throws(() => setBannerOption({ ready: 1, enabled: 0, redraw: 0, tables: new Array(5).fill(0) }, 1, types, {}), { name: 'RangeError', message: /배너/u });
    types.banner = [0, 1, 2, 3, 4];
    setBannerOption({ ready: 1, enabled: 0, redraw: 0, tables: [0, 0, 0, 0, 0] }, 1, types, {});
    types.banner = [0, 1, 2, 3, 5];
    assert.throws(() => setBannerOption({ ready: 1, enabled: 0, redraw: 0, tables: [0, 0, 0, 0, 0] }, 1, types, {}), RangeError);
});
