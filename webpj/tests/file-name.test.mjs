// 원문 경로 이름 작성기(Format·Device·Name·Directory·Full)와 10칸 출력 고리를 원본 10.78 관측으로 대조한다(WB04-E28, cpppj FileNameTests·FileNameLeafTests·FilePathClientTests(Device) 이식).
//  - file-name-formatter-x86.tsv 352행: 부품 순서 규칙·구분자 치환·접두/연결 문자·예외 뒤 부분 출력과 번호, 현재 칸 256바이트 전체.
//  - file-name-leaf-x86.tsv 64행: extension 0/1·두 구분자·빈/콜론/UNC/선두 dot/255바이트의 반환 위치와 칸 전체. file-name-device-x86.tsv 32행: 장치 접두와 NUL 뒤 원래 FileSpec 보존.
//  - file-directory-x86.tsv 192행(정상 162 + 원문 NULL 읽기 30의 부분 버퍼), file-full-x86.tsv 16행(flag 0/1·+2 물리 입력).
//  - 계약: 열 번째 후속 호출이 같은 칸을 덮는 뷰, 별칭 입력은 선행 clear 뒤 현재 바이트를 읽음, 256바이트 밖 쓰기 거부와 앞선 칸 보존.
// 실제 Client 옵션 저장/VFS 읽기/CD 경로가 같은 고리를 이어 쓰는 검사와 ClientStaticStrings 소유자는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E28: 경로 이름 작성기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { RawFileName } from '../www/core/raw-file-name.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E28)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const formatterRows = fixture('file-name-formatter-x86.tsv');
const leafRows = fixture('file-name-leaf-x86.tsv');
const deviceRows = fixture('file-name-device-x86.tsv');
const directoryRows = fixture('file-directory-x86.tsv');
const fullRows = fixture('file-full-x86.tsv');

// 원문 DWORD 를 읽는다. 원본 바이트 16진을 글자 하나당 바이트 하나의 문자열로 되돌리고 칸 바이트를 16진으로 쓴다. '-' 는 빈 문자열이다. (2026-11-08 WB04-E28)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hexToText = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));
const slotHex = (names) => Buffer.from(names.currentBytes()).toString('hex');
// 첫 NUL 앞까지이다. (2026-11-08 WB04-E28)
const untilNul = (text) => { const end = text.indexOf('\0'); return end < 0 ? text : text.slice(0, end); };

// 기대 결과는 제품 작성 전에 원본 전체 함수로 고정한 출력만 쓴다.
test('FileName.format: 원본10.78 352행(부품 순서·구분자·접두/연결·부분 출력·칸 전체)', { skip: !formatterRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of formatterRows) {
        assert.equal(row.length, 6);
        const names = new RawFileName();
        // 같은 초기 BSS 0 에서 명시 현재 번호까지 빈 호출로 전진한다.
        for (let advance = 0; advance < number(row[1]); advance += 1) names.format([]);
        const parts = row[2] === '-' ? [] : row[2].split(';').map((field) => {
            const colon = field.indexOf(':');
            assert.ok(colon >= 0);
            return { kind: Number(field.slice(0, colon)), text: hexToText(field.slice(colon + 1)) };
        });
        let thrown = false;
        let value = null;
        try { value = names.format(parts, number(row[0])); } catch (error) { if (!(error instanceof TypeError)) throw error; thrown = true; }
        const label = `행 ${count + 1} ${row[0]}/${row[1]}/${row[2]}`;
        assert.equal(thrown, number(row[3]) !== 0, `${label} 예외`);
        assert.equal(names.index, number(row[4]), `${label} 번호`);
        assert.equal(slotHex(names), row[5], `${label} 칸 전체`);
        if (!thrown) {
            assert.equal(value.text, untilNul(hexToText(row[5])), `${label} 값`);
            assert.equal(value.offset, 0);
        }
        count += 1;
    }
    assert.equal(count, 352);
});

// 반환 오프셋과 NUL 뒤 원래 전체 문자열은 독립 CPU 기대값이다.
test('FileName.name: 원본10.78 64행(extension 0/1·두 구분자·bare/빈/콜론/UNC/선두 dot/255바이트)', { skip: !leafRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of leafRows) {
        assert.equal(row.length, 6);
        const names = new RawFileName();
        const input = hexToText(row[2]);
        const result = names.name(input, number(row[1]) !== 0, number(row[0]));
        const label = `행 ${count + 1} ${row[0]}/${row[1]}`;
        assert.equal(names.index, 1, `${label} 번호`);
        assert.equal(result.text, hexToText(row[3]), `${label} 값`);
        assert.equal(result.offset, number(row[4]), `${label} 오프셋`);
        assert.equal(slotHex(names), row[5], `${label} 칸 전체`);
        count += 1;
    }
    assert.equal(count, 64);
});

// 출력이 비어도 NUL 뒤에 남는 원래 FileSpec 과 같은 고리 번호를 원문 그대로 대조한다.
test('FileName.device: 원본10.78 32행(두 구분자·UNC/드라이브/빈/255바이트·칸 전체)', { skip: !deviceRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of deviceRows) {
        assert.equal(row.length, 3);
        const names = new RawFileName();
        const result = names.device(hexToText(row[1]), number(row[0]));
        const expected = hexToText(row[2]);
        const label = `행 ${count + 1} ${row[0]}`;
        assert.equal(names.index, 1, `${label} 번호`);
        assert.equal(slotHex(names), row[2], `${label} 칸 전체`);
        assert.equal(result.text, untilNul(expected), `${label} 값`);
        assert.equal(result.offset, 0);
        count += 1;
    }
    assert.equal(count, 32);
});

// 출력은 소유 문자열이 아니며 열 번째 후속 호출의 선행 clear 와 같은 주소에 덮어써진다.
test('FileName: 열 칸 고리 순환과 입력 별칭의 선행 clear', () => {
    const names = new RawFileName();
    const first = names.format([{ kind: -3, text: 'first' }]);
    assert.deepEqual([first.text, names.index], ['.\\first', 1]);
    // 다른 아홉 칸을 써도 최초 칸의 바이트는 유지된다.
    for (let call = 0; call < 9; call += 1) names.format([]);
    assert.deepEqual([first.text, names.index], ['.\\first', 0]);
    // 원문은 출력 칸을 먼저 0 으로 만든 다음 입력 포인터를 읽는다.
    const aliased = names.format([{ kind: -3, text: first }]);
    assert.deepEqual([aliased.text, aliased.slot === first.slot, aliased.offset === first.offset, names.index], ['.\\', true, true, 1]);
    assert.deepEqual([first.text.charCodeAt(0), first.text.charCodeAt(1), first.text.charCodeAt(2)], [0x2e, 0x5c, 0]);
    // 이름 조회의 내부 뷰도 열 번째 후속 호출에 덮이고 별칭 입력은 비워진 현재 바이트를 읽는다.
    const leaf = new RawFileName();
    const alias = leaf.name('.\\first.ext');
    assert.equal(alias.text, 'first.ext');
    for (let call = 0; call < 9; call += 1) leaf.format([]);
    assert.deepEqual([alias.text, leaf.index], ['first.ext', 0]);
    const cleared = leaf.name(alias);
    assert.deepEqual([cleared.text, leaf.index], ['', 1]);
    assert.ok(leaf.slotBytes(alias.slot).every((byte) => byte === 0));
    assert.throws(() => leaf.name('x'.repeat(256)), RangeError);
    assert.equal(leaf.index, 2);
});

// 원문 지역/출력 배열을 넘는 미정 쓰기는 정상 경로로 절단하지 않는다.
test('FileName: 256바이트 물리 범위 초과는 거부하고 앞선 칸을 보존한다', () => {
    const names = new RawFileName();
    const kept = names.format([{ kind: -3, text: 'kept' }]);
    assert.throws(() => names.format([{ kind: -3, text: 'x'.repeat(256) }]), RangeError);
    assert.deepEqual([names.index, kept.text], [2, '.\\kept']);
    // 입력 256바이트 거부는 현재 칸 clear 뒤, 부품 append 전이다.
    assert.ok(names.currentBytes().every((byte) => byte === 0));
    assert.throws(() => names.format([{ kind: -3, text: 'x'.repeat(254) }]), RangeError);
    assert.deepEqual([names.index, kept.text], [3, '.\\kept']);
    // 선택된 선행 접두는 보존하지만 넘는 부품은 부분 복사하지 않는다.
    const bytes = names.currentBytes();
    assert.deepEqual([bytes[0], bytes[1], bytes[2]], [0x2e, 0x5c, 0]);
    // 디렉터리/장치/전체도 같은 바이트 상한을 거부한다.
    assert.throws(() => names.device('y'.repeat(256)), RangeError);
    assert.throws(() => names.directory('y'.repeat(256)), RangeError);
    assert.throws(() => names.full('y'.repeat(256)), RangeError);
});


// 정상 162행과 원문 NULL 읽기 30행의 부분 버퍼를 구별한다. 기대 경로를 재계산하지 않고 Native 칸 전체 바이트를 읽는다.
test('FileName.directory: 원본10.78 192행(separator·level·255바이트 경계·NULL 읽기 30행)', { skip: !directoryRows && '원본 fixture 없음' }, () => {
    let count = 0;
    let faults = 0;
    for (const row of directoryRows) {
        assert.equal(row.length, 5);
        const names = new RawFileName();
        let fault = false;
        let value = null;
        try { value = names.directory(hexToText(row[2]), Number(row[1]), number(row[0])); } catch (error) { if (!(error instanceof TypeError)) throw error; fault = true; }
        const label = `행 ${count + 1} ${row[0]}/${row[1]}`;
        assert.equal(fault, row[3] === 'read-fault', `${label} 읽기 오류`);
        assert.equal(names.index, 1, `${label} 번호`);
        assert.equal(slotHex(names), row[4], `${label} 칸 전체`);
        if (!fault) {
            assert.equal(value.text, untilNul(hexToText(row[4])), `${label} 값`);
            assert.equal(value.offset, 0);
        }
        faults += fault ? 1 : 0;
        count += 1;
    }
    assert.deepEqual([count, faults], [192, 30]);
});

// flag 0 은 .\ 제거가 아니며 첫 NUL 뒤 +2 물리 주소도 따로 읽는다.
test('FileName.full: 원본10.78 16행(255바이트 입력·상위 바이트·장치/UNC·flag 0/1)', { skip: !fullRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of fullRows) {
        assert.equal(row.length, 3);
        const names = new RawFileName();
        const value = names.full(hexToText(row[0]), number(row[1]) !== 0);
        assert.equal(value.text, untilNul(hexToText(row[2])), `행 ${count + 1} 값`);
        assert.deepEqual([names.index, slotHex(names)], [1, row[2]], `행 ${count + 1} 칸`);
        count += 1;
    }
    assert.equal(count, 16);
});

// 원문의 비정상 접근과 호스트 메모리 범위는 정상 빈 경로로 합성하지 않는다.
test('FileName.directory/full: 별칭 clear·NULL 읽기·범위 초과·접두 건너뜀 계약', () => {
    // 디렉터리: 열 번째 목적지를 첫 출력의 별칭과 같게 만들면 비워진 입력을 읽어 원문 NULL 읽기로 거부한다.
    const names = new RawFileName();
    const first = names.directory('.\\d\\file');
    assert.equal(first.text, '.\\d\\');
    for (let call = 0; call < 9; call += 1) names.format([]);
    assert.throws(() => names.directory(first), TypeError);
    assert.equal(names.index, 1);
    assert.ok(names.currentBytes().every((byte) => byte === 0));
    assert.throws(() => names.directory('x'.repeat(256)), RangeError);
    assert.equal(names.index, 2);
    assert.ok(names.currentBytes().every((byte) => byte === 0));
    // 전체: 같은 방식의 별칭은 빈 값, 256바이트 초과/두 바이트 미만(+2)은 거부하고 번호는 예약된 채이다.
    const full = new RawFileName();
    const kept = full.full('.\\kept');
    for (let call = 0; call < 9; call += 1) full.format([]);
    assert.deepEqual([full.full(kept).text, full.index], ['', 1]);
    assert.throws(() => full.full('x'.repeat(256)), RangeError);
    assert.throws(() => full.full('a', false), TypeError);
    assert.equal(full.index, 3);
    assert.ok(full.currentBytes().every((byte) => byte === 0));
    // 부품 규칙: 루트는 첫 부품, 파일 뒤 디렉터리/파일 없는 확장자는 TypeError 이다.
    const parts = new RawFileName();
    assert.throws(() => parts.format([{ kind: -3, text: 'a' }, { kind: -1, text: 'C' }]), TypeError);
    assert.throws(() => parts.format([{ kind: -3, text: 'a' }, { kind: -2, text: 'd' }]), TypeError);
    assert.throws(() => parts.format([{ kind: -2, text: 'd' }, { kind: -4, text: 'ext' }]), TypeError);
    assert.throws(() => parts.format([{ kind: -9, text: 'x' }]), TypeError);
    // 종료 0 뒤의 부품은 조회하지 않고, 디렉터리/파일/확장자 조합은 원문 접두와 연결 문자를 쓴다.
    assert.equal(parts.format([{ kind: -3, text: 'a' }, { kind: 0, text: 'ignored' }, { kind: -3, text: 'b' }]).text, '.\\a');
    assert.equal(parts.format([{ kind: -2, text: '' }, { kind: -2, text: 'd' }, { kind: -3, text: 'folder\\' }, { kind: -4, text: 'col' }]).text, '.\\d\\folder.col');
});
