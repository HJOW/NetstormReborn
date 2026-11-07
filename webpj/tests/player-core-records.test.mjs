// Player Core 장부(9×16 BYTE)와 별도 Core 10 BYTE 초기화를 원본 PE 관측으로 대조한다(WB04-E3, cpppj PlayerCoreRecordsTests 이식).
//  - player-core-records-x86.tsv 192행: 장부 앞 64/뒤 48 BYTE 인접 물리를 포함한 256 BYTE 창, 별도 지역 Core 16 BYTE, Core 와 무관한 색 표 44 BYTE 를 전체 대조한다.
//    mode 0 initializePlayerCore, 1 writePlayerCoreMetadata, 2 initializePlayerCoreRecords, 3 lastKnown 주소의 Core 초기화, 4 owner 색 표 6관측은 귀속 감사로만 읽는다(Core 제품이 아님).
//  - 짧은 물리 뷰는 앞선 store 만 남기고 먼저 전체 10/14 BYTE 를 검사하지 않으며, metadata 는 입력값 복제와 padding 2 를 보존하고 정상 장부 밖 주소를 선택 접근에서만 거부한다.
// (2026-11-08 WB04-E3: Player Core 장부 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { initializePlayerCore, initializePlayerCoreRecords, writePlayerCoreMetadata } from '../www/core/raw-player-core-records.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E3)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'player-core-records-x86.tsv'));

// 미작성 인접 BYTE/padding 도 모두 대조하는 소문자 16진 문자열이다(NUL 에서 자르지 않음). (2026-11-08 WB04-E3)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 별도 owner 색 표 6관측은 귀속 감사로만 읽고 Core 관련 186관측의 전체 물리를 대조한다.
test('Player Core 장부: 독립 PE 186관측과 색 표 6관측 분리', { skip: !rows && '원본 fixture 없음' }, () => {
    let total = 0;
    let compared = 0;
    let colorAudit = 0;
    for (const values of rows) {
        total += 1;
        assert.equal(values.length, 7);
        const input = values[1].split(',');
        const mode = Number(input[0]);
        const seed = Number(input[1]);
        const owner = Number(BigInt(input[2]) & 0xffffffffn);
        const subscriber = Number(BigInt(input[3]) & 0xffffffffn);
        const coreSeed = Number(input[4]);
        const records = new Uint8Array(256);
        const core = new Uint8Array(16);
        const colors = new Uint8Array(44);
        const argument = new Uint8Array(10);
        // Native 의 장부 앞 64/뒤 48 인접 물리도 같고, 별도 지역 Core 와 Core 와 무관한 표는 따로 유지한다.
        for (let byte = 0; byte < records.length; byte += 1) records[byte] = (seed + byte * 17) & 0xff;
        for (let byte = 0; byte < core.length; byte += 1) core[byte] = (seed + byte * 13) & 0xff;
        for (let byte = 0; byte < colors.length; byte += 1) colors[byte] = (seed + byte * 7) & 0xff;
        for (let byte = 0; byte < argument.length; byte += 1) argument[byte] = (coreSeed + byte * 23) & 0xff;
        if (mode === 0) initializePlayerCore(core);
        else if (mode === 1) writePlayerCoreMetadata({ bytes: records, origin: 64 }, owner, subscriber, argument);
        else if (mode === 2) initializePlayerCoreRecords({ bytes: records, origin: 64 });
        else if (mode === 3) initializePlayerCore(records.subarray(208));
        else if (mode === 4) {
            // 별도 43b540 을 Core 제품에서 구현했다고 주장하지 않는다.
            colorAudit += 1;
            assert.equal(values[5].startsWith('O0:4:0;O4:4:1;'), true);
            continue;
        } else assert.fail('Core 장부 진입 모드');
        compared += 1;
        assert.equal(hex(records), values[2], `행 ${total} records`);
        assert.equal(hex(core), values[3], `행 ${total} core`);
        assert.equal(hex(colors), values[4], `행 ${total} unrelatedColors`);
    }
    assert.deepEqual([total, compared, colorAudit], [192, 186, 6]);
});

// 선택된 단일 store 폭 부족에서 앞선 store 만 유지하며 먼저 전체 10/14 BYTE 를 검사하지 않는다.
test('Player Core 장부: 짧은 물리 뷰는 순서대로 앞선 store 만 남긴다', () => {
    // Core 의 4/4/2 경계와 그 사이 짧은 물리를 별도 호스트 계약으로 검증한다.
    for (let length = 0; length <= 10; length += 1) {
        const bytes = new Uint8Array(16).fill(0xa5);
        let rejected = false;
        try {
            initializePlayerCore(bytes.subarray(0, length));
        } catch (error) {
            assert.ok(error instanceof RangeError);
            rejected = true;
        }
        assert.equal(rejected, length < 10, `길이 ${length}`);
        const zeroEnd = length >= 10 ? 10 : length >= 8 ? 8 : length >= 4 ? 4 : 0;
        // 성공한 store 뒤의 물리는 그대로다.
        for (let byte = 0; byte < bytes.length; byte += 1) assert.equal(bytes[byte], byte < zeroEnd ? 0 : 0xa5, `길이 ${length} byte ${byte}`);
    }
    const records = new Uint8Array(32).fill(0xa5);
    assert.throws(() => initializePlayerCoreRecords({ bytes: records.subarray(0, 13), origin: 0 }), RangeError);
    // 첫 Core 완료 전에 subId 0 이 앞당겨지지 않는다.
    assert.deepEqual([records[0], records[3], records[4], records[11], records[12]], [0xa5, 0xa5, 0, 0, 0xa5]);
    records.fill(0xa5);
    assert.throws(() => initializePlayerCoreRecords({ bytes: records.subarray(0, 14), origin: 0 }), RangeError);
    // 첫 record 완료 효과는 다음 자식 중단에도 남는다.
    assert.deepEqual([records.subarray(0, 14).every((byte) => byte === 0), records[14], records[16]], [true, 0xa5, 0xa5]);
});

// metadata 는 입력값 복제와 padding 2 를 보존하며 정상 장부 밖 주소를 선택 접근에서만 거부한다.
test('Player Core 장부: metadata 는 padding 을 보존하고 입력 Core 를 복제한다', () => {
    const records = new Uint8Array(144).fill(0xa5);
    const core = Uint8Array.of(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    writePlayerCoreMetadata({ bytes: records, origin: 0 }, 8, 0x12345678, core);
    core.fill(0);
    assert.deepEqual([records[128], records[131], records[132], records[141], records[142], records[143]], [0x78, 0x12, 1, 10, 0xa5, 0xa5]);
    const before = records.slice();
    // 원문 owner 관문이 아닌 호스트 물리 144 BYTE 의 계약이다.
    assert.throws(() => writePlayerCoreMetadata({ bytes: records, origin: 0 }, 9, 0, core), RangeError);
    assert.deepEqual(records, before);
    // owner 주소는 32비트로 감긴다: 0x10000004×16 은 64 로 감겨 물리 끝이라 선택 접근에서 거부되고, 0x10000003×16 은 48 로 감겨 물리 안에 쓴다.
    const wide = new Uint8Array(64).fill(0xa5);
    assert.throws(() => writePlayerCoreMetadata({ bytes: wide, origin: 0 }, 0x10000004, 7, Uint8Array.of(9, 0, 0, 0, 0, 0, 0, 0, 0, 0)), RangeError);
    assert.ok(wide.every((byte) => byte === 0xa5));
    writePlayerCoreMetadata({ bytes: wide, origin: 0 }, 0x10000003, 7, Uint8Array.of(9, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    assert.deepEqual([wide[48], wide[52]], [7, 9]);
});
