// 지면/다리 행 묶음 수집기 render/tile-group-collection.js 를 원본 10.78 관측(tilegroup-x86.tsv originals 353행)과 cpppj `TileGroupCollectionTests.cpp` 의 계약·공통 제출 연결로 검사한다.
//  - 관측 한 행 = 완성 입력 한 건(가시 사각형 float 비트 네 개·행 한도·화면 높이·초기 해시 선택, 해시 0단계의 희소 칸, 완성 raw 슬롯 50바이트)과 그 결과(헤더 수·배경 덮임·연속 항목 3072바이트·헤더 1024바이트)이다.
//    헤더는 첫 항목의 위치(연속 배열 안 바이트 오프셋 + 격리 기준 0x15001000)와 개수 상자 값을 직렬화한다. 정상 반환 때 해시 선택은 2 단계이고 입력 풀과 해시 0단계는 수집 전후 같아야 하며
//    모든 항목 토큰은 `object()` 가 같은 풀 SID 로 되돌린다. CD(originalCD)·추가판(original1037) 행은 웹이 10.78 만 지원하므로 사용하지 않는다.
//  - cpppj 시험 이식: PatchNative(위 353행)·RowLimitsAndEditionRows(패치 부분: strict 한도·실패 시 선택/배경 보존·49개 행·화면 높이 0 의 상한 6)·TokenContractsAndLifetime·CommonSubmissionIntegration(실제 raw SID/해시의 행 순서를 공통 CRT 정렬과 kind 1 분배까지 연결).
//    `ObjectCollection_CommonSubmissionIntegration`(UI·지면과의 공통 제출)은 GumpCollection 단위 뒤에 닫는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E394)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { collectDrawEntries, dispatchDrawEntries, sortRenderEntries } from '../www/render/draw-submission.js';
import { TileGroupCollection, bindTileGroupCollection } from '../www/render/tile-group-collection.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E394)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'tilegroup-x86.tsv'));
// 기계어 관찰의 가상 포인터 토큰 기준(헤더·멤버·풀)과 전체 출력 폭이다. 호스트 주소가 아니다. (2026-11-13 WB04-E394)
const HEADERS = 0x15000000;
const MEMBERS = 0x15001000;
const POOL = 0x15030000;
const MEMBERS_SIZE = 96 * 16;
const HEADERS_SIZE = 64 * 8;
const hex = (bytes) => Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex');
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
const throwsOf = (action) => { try { action(); } catch { return true; } return false; };

// 호스트 연결 검사용 raw 위치/깊이 입력을 실제 슬롯에 공급한다. (2026-11-13 WB04-E394)
function place(pool, sid, x, y, depth) {
    const raw = pool.physical(sid);
    put(raw, 14, floatToBits(x));
    put(raw, 18, floatToBits(y));
    put(raw, 35, depth & 255, 1);
}

test('TileGroupCollection_PatchNative: 원본 10.78 관측 353행의 연속 항목·헤더·배경 덮임·해시 선택과 입력 보존이 모두 일치한다', { skip: rows === null }, () => {
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    let replayed = 0;
    // 각 입력의 완성 슬롯과 희소 해시, 원본의 현재 선택 단계를 공급한다.
    for (const row of rows) {
        if (row[0] !== 'originals') continue;
        const config = row[2].split(',').map(Number);
        hash.reset();
        hash.select(config[6]);
        // y/x 의 선형 인덱스는 해시 입력 위치이며 기대 방문 범위를 계산하지 않는다.
        if (row[3] !== '-') {
            for (const entry of row[3].split(';')) {
                const [index, value] = entry.split(',').map(Number);
                hash.setCell(0, index % 256, Math.floor(index / 256), value);
            }
        }
        // 타입/state/next 와 미사용 바이트까지 원본 슬롯의 완성 입력을 복사한다.
        for (const slot of row[4].split(';')) {
            const [sid, bytes] = slot.split('=');
            const raw = unhex(bytes);
            assert.equal(raw.length, pool.layout.stride);
            pool.physical(Number(sid)).set(raw);
        }
        const inputPool = pool.bytes();
        const inputHash = hash.entries(0);
        const collector = new TileGroupCollection(hash, pool, { screenHeight: config[5], objectTokenBase: POOL, groupTokenBase: HEADERS });
        const covered = { value: false };
        const view = { left: bitsToFloat(config[0]), top: bitsToFloat(config[1]), right: bitsToFloat(config[2]), bottom: bitsToFloat(config[3]) };
        const groups = collector.collect(view, config[4], covered);
        const entries = collector.entries();
        const members = new Uint8Array(MEMBERS_SIZE).fill(0xa5);
        const headers = new Uint8Array(HEADERS_SIZE).fill(0xa5);
        assert.ok(entries.length <= members.length);
        members.set(entries);
        // 실제 보기의 오프셋만 격리 포인터로 직렬화한다. 헤더 개수/경계/멤버 결과는 계산하지 않는다.
        groups.forEach((group, index) => {
            put(headers, index * 8, MEMBERS + (group.entries.byteOffset - entries.byteOffset));
            put(headers, index * 8 + 4, group.count.value);
            assert.equal(collector.group(group.token).entries.byteOffset, group.entries.byteOffset);
        });
        // 모든 실제 객체 토큰을 같은 풀의 슬롯 번호로 되돌릴 수 있어야 한다.
        for (let at = 0; at < entries.length; at += 16) {
            const token = new DataView(entries.buffer, entries.byteOffset + at, 16).getUint32(8, true);
            assert.equal(collector.object(token), (token - POOL) / pool.layout.stride);
        }
        const same = groups.length === Number(row[5]) && covered.value === (row[6] === '1') && hex(members) === row[7] && hex(headers) === row[8];
        assert.ok(same, `TileGroup 행 ${row[1]}: headers=${groups.length} covered=${covered.value}`);
        assert.equal(hash.selected, 2);
        assert.equal(row[9], '2,4,64');
        // 120만 바이트급 배열은 원소 비교 대신 바이트 버퍼 비교로 입력 보존을 확인한다.
        assert.ok(Buffer.from(pool.bytes().buffer).equals(Buffer.from(inputPool.buffer)), '입력 풀 보존');
        assert.ok(Buffer.from(hash.entries(0).buffer).equals(Buffer.from(inputHash.buffer)), '입력 해시 보존');
        replayed += 1;
    }
    assert.equal(replayed, 353);
});

test('TileGroupCollection_RowLimitsAndEditionRows: 행 한계는 strict/행별이며 정상 종료만 해시 선택과 배경 결과를 갱신하고 헤더 수 상한은 화면 높이로 정한다', () => {
    const patch = new SidPool(24000);
    const hash = new SquidHash();
    const collector = new TileGroupCollection(hash, patch);
    hash.setCell(0, 2, 2, 5);
    hash.setCell(0, 3, 2, 5);
    hash.setCell(0, 2, 3, 5);
    hash.setCell(0, 3, 3, 5);
    const covered = { value: true };
    assert.equal(collector.collect({ left: 2, top: 2, right: 3, bottom: 3 }, 3, covered).length, 2);
    assert.equal(collector.entries().length, 4 * 16);
    covered.value = true;
    assert.ok(throwsOf(() => collector.collect({ left: 2, top: 2, right: 3, bottom: 3 }, 2, covered)));
    assert.equal(hash.selected, 0);
    assert.equal(covered.value, true);
    // 실패 전 부분 출력(첫 행의 두 항목)이 남는다.
    assert.equal(collector.entries().length, 2 * 16);
    hash.reset();
    assert.equal(collector.collect({ left: 2, top: 2, right: 3, bottom: 3 }, 0, covered).length, 0);
    assert.equal(hash.selected, 2);
    assert.equal(covered.value, false);
    // 49개 행은 패치판에서 유효하다(화면 높이 768 → 상한 75).
    for (let y = 0; y < 49; y += 1) hash.setCell(0, 1, y, 5);
    assert.equal(collector.collect({ left: 0, top: 0, right: 2, bottom: 48 }, 2, covered).length, 49);
    hash.reset();
    const short = new TileGroupCollection(hash, patch, { screenHeight: 0 });
    // 화면 높이 0 의 패치 상한은 6이다. 5개는 성공하고 6개는 실패한다.
    for (let y = 0; y < 5; y += 1) hash.setCell(0, 1, y, 5);
    assert.equal(short.collect({ left: 0, top: 0, right: 2, bottom: 4 }, 2, covered).length, 5);
    hash.setCell(0, 1, 5, 5);
    assert.ok(throwsOf(() => short.collect({ left: 0, top: 0, right: 2, bottom: 5 }, 2, covered)));
});

test('TileGroupCollection_TokenContractsAndLifetime: 잘못된 상태·토큰 범위를 거부하고 새 수집 때 이전 헤더를 지운다', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    const collector = new TileGroupCollection(hash, pool);
    const covered = { value: false };
    assert.throws(() => new TileGroupCollection(hash, pool, { screenHeight: -1 }), RangeError);
    assert.throws(() => new TileGroupCollection(hash, pool, { screenHeight: 768, objectTokenBase: 0, groupTokenBase: 1 }), RangeError);
    assert.throws(() => new TileGroupCollection(hash, pool, { screenHeight: 768, objectTokenBase: 0xffff0000 }), RangeError);
    assert.throws(() => collector.object(0), RangeError);
    assert.throws(() => collector.object(51), RangeError);
    assert.throws(() => collector.group(0x80000001), RangeError);
    hash.setCell(0, 2, 2, 5);
    const first = collector.collect({ left: 2, top: 2, right: 2, bottom: 2 }, 2, covered)[0];
    const token = new DataView(first.entries.buffer, first.entries.byteOffset, 16).getUint32(8, true);
    assert.equal(collector.object(token), 5);
    hash.reset();
    assert.equal(collector.collect({ left: 2, top: 2, right: 2, bottom: 2 }, 2, covered).length, 0);
    assert.equal(collector.entries().length, 0);
    assert.throws(() => collector.group(first.token), RangeError);
    hash.setCell(0, 2, 2, 65535);
    assert.ok(throwsOf(() => collector.collect({ left: 2, top: 2, right: 2, bottom: 2 }, 2, covered)));
});

test('TileGroupCollection_CommonSubmissionIntegration: 실제 raw SID/해시의 행 순서를 공통 CRT 정렬과 kind 1 내부 sprite 제출까지 연결한다', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    const collector = new TileGroupCollection(hash, pool);
    place(pool, 5, 30, 20, 2);
    place(pool, 6, 10, 15, 100);
    place(pool, 7, 50, 40, 10);
    hash.setCell(0, 2, 2, 5);
    hash.setCell(0, 3, 2, 6);
    hash.setCell(0, 2, 3, 7);
    const collection = {};
    const dispatch = {};
    bindTileGroupCollection(collection, dispatch, collector);
    const request = { screen: { left: 0, top: 0, right: 0, bottom: 0 }, visible: { left: 2, top: 2, right: 3, bottom: 3 }, unclipped: { left: 0, top: 0, right: 0, bottom: 0 }, dirtyFlags: 0, groupRowLimit: 3 };
    const work = new Uint8Array(8 * 16).fill(0xa5);
    const result = collectDrawEntries(request, work, collection);
    assert.equal(result.count, 2);
    assert.equal(result.backgroundCovered, false);
    assert.equal(hash.selected, 2);
    const entries = work.subarray(0, 2 * 16);
    sortRenderEntries(entries);
    const drawn = [];
    // 행 내부의 높은 깊이 100도 순서를 다시 정렬하지 않고 SID 5 뒤에 그린다.
    dispatch.sprite = (entry) => {
        drawn.push(collector.object(new DataView(entry.buffer, entry.byteOffset, 16).getUint32(8, true)));
        assert.equal(entry[14], 1);
        assert.equal(entry[15], 0);
    };
    dispatchDrawEntries(entries, {}, dispatch);
    assert.deepEqual(drawn, [7, 5, 6]);
    assert.equal(work[15], 0xa5);
    assert.equal(work[31], 0xa5);
    drawn.length = 0;
    dispatchDrawEntries(entries, { hideGroups: true, debugAfterEach: false }, dispatch);
    assert.deepEqual(drawn, []);
    request.dirtyFlags = 4;
    hash.select(1);
    assert.equal(collectDrawEntries(request, work, collection).count, 0);
    assert.equal(hash.selected, 1);
});

// ---- 변형 시험 보강: 상태 검증 경계·토큰 간격·기본 토큰의 양수성·NaN 좌표·전체 맵 용량·헤더 상한 (2026-11-13 WB04-E394) ----

test('TileGroupCollection 상태 검증 경계: 화면 높이 상한·토큰 기준의 정수/범위·두 영역의 맞닿음과 겹침을 따로 확인한다', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    const objectSize = pool.capacity * pool.layout.stride;
    // 화면 높이는 정수이고 0 이상 `INT_MAX − 66` 이하만 받는다.
    assert.doesNotThrow(() => new TileGroupCollection(hash, pool, { screenHeight: 0 }));
    assert.doesNotThrow(() => new TileGroupCollection(hash, pool, { screenHeight: 2147483647 - 66 }));
    assert.throws(() => new TileGroupCollection(hash, pool, { screenHeight: 2147483647 - 65 }), RangeError);
    assert.throws(() => new TileGroupCollection(hash, pool, { screenHeight: 1.5 }), RangeError);
    // 토큰 기준은 32비트 부호 없는 정수이다: 소수·음수·2^32 이상은 거부한다.
    assert.throws(() => new TileGroupCollection(hash, pool, { objectTokenBase: 0.5 }), RangeError);
    assert.throws(() => new TileGroupCollection(hash, pool, { objectTokenBase: -1 }), RangeError);
    assert.throws(() => new TileGroupCollection(hash, pool, { objectTokenBase: 2 ** 32 }), RangeError);
    assert.throws(() => new TileGroupCollection(hash, pool, { groupTokenBase: 0.5 }), RangeError);
    assert.throws(() => new TileGroupCollection(hash, pool, { groupTokenBase: -1 }), RangeError);
    assert.throws(() => new TileGroupCollection(hash, pool, { groupTokenBase: 2 ** 32 }), RangeError);
    // 객체 영역 끝이 정확히 2^32 이면 허용하고 한 칸 넘으면 거부한다(묶음 영역은 0 에서 시작해 겹치지 않도록 객체를 뒤에 둔다).
    assert.doesNotThrow(() => new TileGroupCollection(hash, pool, { objectTokenBase: 2 ** 32 - objectSize, groupTokenBase: 0 }));
    assert.throws(() => new TileGroupCollection(hash, pool, { objectTokenBase: 2 ** 32 - objectSize + 1, groupTokenBase: 0 }), RangeError);
    // 묶음 영역 끝이 2^32 이면 허용하고 한 칸 넘으면 거부한다(묶음 영역 256개 × 8바이트).
    assert.doesNotThrow(() => new TileGroupCollection(hash, pool, { objectTokenBase: 0, groupTokenBase: 2 ** 32 - 2048 }));
    assert.throws(() => new TileGroupCollection(hash, pool, { objectTokenBase: 0, groupTokenBase: 2 ** 32 - 2047 }), RangeError);
    // 두 영역은 정확히 맞닿는 것은 허용하고 한 칸이라도 겹치면 거부한다(두 방향 모두).
    assert.doesNotThrow(() => new TileGroupCollection(hash, pool, { objectTokenBase: 0, groupTokenBase: objectSize }));
    assert.throws(() => new TileGroupCollection(hash, pool, { objectTokenBase: 0, groupTokenBase: objectSize - 1 }), RangeError);
    assert.doesNotThrow(() => new TileGroupCollection(hash, pool, { objectTokenBase: 2048, groupTokenBase: 0 }));
    assert.throws(() => new TileGroupCollection(hash, pool, { objectTokenBase: 2047, groupTokenBase: 0 }), RangeError);
});

test('TileGroupCollection 토큰: 기본 묶음 기준은 부호 없는 0x80000000 이고 헤더 토큰은 8 간격이며, 객체/묶음 토큰의 각 거부 조건을 따로 확인한다', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    const collector = new TileGroupCollection(hash, pool);
    const covered = { value: false };
    // 새 수집기의 항목은 비어 있다.
    assert.equal(collector.entries().length, 0);
    hash.setCell(0, 1, 1, 5);
    hash.setCell(0, 1, 2, 6);
    hash.setCell(0, 1, 3, 7);
    const groups = collector.collect({ left: 1, top: 1, right: 1, bottom: 3 }, 4, covered);
    // 기본 기준 0x80000000 에서 8 간격이며 양수 토큰이다. 각 토큰은 그대로 해당 헤더를 되돌린다.
    assert.deepEqual(groups.map((group) => group.token), [0x80000000, 0x80000008, 0x80000010]);
    groups.forEach((group) => assert.equal(collector.group(group.token), group));
    // 객체 토큰은 SID × 50 이다(기본 기준 0).
    const first = new DataView(groups[0].entries.buffer, groups[0].entries.byteOffset, 16).getUint32(8, true);
    assert.equal(first, 5 * 50);
    // 묶음 토큰의 거부 조건: 정수가 아님·기준 미만·8 의 배수가 아님·현재 헤더 수 밖.
    assert.throws(() => collector.group(1.5), RangeError);
    assert.throws(() => collector.group(0x7ffffff8), RangeError);
    assert.throws(() => collector.group(0x80000004), RangeError);
    assert.throws(() => collector.group(0x80000018), RangeError);
    assert.throws(() => collector.group(NaN), RangeError);
    // 기준이 8 의 배수가 아니어도 기준에서 뺀 값이 8 의 배수인 토큰이 맞다.
    const odd = new TileGroupCollection(hash, pool, { objectTokenBase: 0, groupTokenBase: 0x15000002 });
    const oddGroups = odd.collect({ left: 1, top: 1, right: 1, bottom: 2 }, 4, covered);
    assert.equal(oddGroups.length, 3);
    assert.deepEqual(oddGroups.map((group) => group.token), [0x15000002, 0x1500000a, 0x15000012]);
    oddGroups.forEach((group) => assert.equal(odd.group(group.token), group));
    assert.throws(() => odd.group(0x15000008), RangeError);
    assert.throws(() => odd.group(0x15000006), RangeError);
    // 객체 토큰의 거부 조건: 정수가 아님·기준 미만·stride 배수가 아님·SID 0·용량 이상(마지막 SID 는 허용).
    const based = new TileGroupCollection(hash, pool, { objectTokenBase: 1000, groupTokenBase: 0x80000000 });
    assert.equal(based.object(1000 + 50), 1);
    assert.equal(based.object(1000 + 50 * 23999), 23999);
    assert.throws(() => based.object(1000 + 50 * 24000), RangeError);
    assert.throws(() => based.object(1000), RangeError);
    assert.throws(() => based.object(999), RangeError);
    assert.throws(() => based.object(1000 + 51), RangeError);
    assert.throws(() => based.object(1000.5), RangeError);
    assert.throws(() => based.object(NaN), RangeError);
});

test('TileGroupCollection 좌표 경계: NaN/무한대 우측 경계는 정수 indefinite 의 칸 0 에서 한 칸 넓히고, 큰 float32 좌표는 하위 DWORD 칸을 쓴다', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    const collector = new TileGroupCollection(hash, pool);
    const covered = { value: false };
    // 칸 0·1·2 에 하나씩 둔다.
    for (let x = 0; x < 3; x += 1) hash.setCell(0, x, 0, 5 + x);
    // 수집된 항목의 SID 목록이다(객체 토큰 = SID × 50).
    const sids = (view) => collector.collect(view, 1000, covered).flatMap((group) => Array.from({ length: group.count.value }, (_, index) => new DataView(group.entries.buffer, group.entries.byteOffset + index * 16, 16).getUint32(8, true) / 50));
    // NaN 은 칸 0 이고 오른쪽/아래는 한 칸 넓히므로 칸 0·1 만 훑는다(범위 밖 무한대도 같다).
    assert.deepEqual(sids({ left: 0, top: 0, right: NaN, bottom: 0 }), [5, 6]);
    assert.deepEqual(sids({ left: 0, top: 0, right: Infinity, bottom: 0 }), [5, 6]);
    assert.deepEqual(sids({ left: 0, top: 0, right: -Infinity, bottom: 0 }), [5, 6]);
    // 2^63 이상의 float32 도 indefinite 이다(하위 DWORD 0).
    assert.deepEqual(sids({ left: 0, top: 0, right: 2 ** 63, bottom: 0 }), [5, 6]);
    // 2^33+2^10 은 float32 로 정확하고 하위 DWORD 1024 라 255 로 제한된다(오른쪽 255 → 우측 확장도 255).
    for (let x = 3; x < 256; x += 1) hash.setCell(0, x, 0, 1);
    const wide = sids({ left: 0, top: 0, right: 2 ** 33 + 2 ** 10, bottom: 0 });
    assert.equal(wide.length, 256);
    assert.deepEqual(wide.slice(0, 4), [5, 6, 7, 1]);
    // 큰 음수 좌표는 하위 DWORD 가 음수이면 0 으로 제한된다: -(2^33+2^10) → 하위 DWORD -1023.
    assert.deepEqual(sids({ left: -(2 ** 33) - 2 ** 10, top: 0, right: 0, bottom: 0 }), [5, 6]);
    // 0.5 올림 경계: 1.5 는 칸 2(+0.5 후 절삭)이고 왼쪽은 한 칸 줄이므로 칸 1..3, 1.4 는 칸 1 이므로 칸 0..2 이다.
    assert.deepEqual(sids({ left: 1.5, top: 0, right: 1.5, bottom: 0 }), [6, 7, 1]);
    assert.deepEqual(sids({ left: 1.4, top: 0, right: 1.4, bottom: 0 }), [5, 6, 7]);
});

test('TileGroupCollection 용량과 헤더 상한: 맵 전체를 채워도 연속 저장소가 모자라지 않고, 헤더 상한은 화면 높이 ÷ 11 + 6 이다', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    const covered = { value: false };
    // 맵 전체(256×256)를 SID 5 로 채우고 화면 높이를 크게 해 헤더 256개를 허용한다.
    for (let y = 0; y < 256; y += 1) for (let x = 0; x < 256; x += 1) hash.setCell(0, x, y, 5);
    const collector = new TileGroupCollection(hash, pool, { screenHeight: 4000 });
    const groups = collector.collect({ left: 0, top: 0, right: 255, bottom: 255 }, 300, covered);
    assert.equal(groups.length, 256);
    assert.equal(collector.entries().length, 256 * 256 * 16);
    assert.equal(groups[255].count.value, 256);
    assert.equal(covered.value, true);
    // 헤더 상한: 화면 높이 11 → 6+1=7 개 미만. 6개는 허용하고 7번째는 거부한다.
    hash.reset();
    const short = new TileGroupCollection(hash, pool, { screenHeight: 11 });
    for (let y = 0; y < 7; y += 1) hash.setCell(0, 1, y, 5);
    assert.equal(short.collect({ left: 1, top: 0, right: 1, bottom: 4 }, 4, covered).length, 6);
    assert.ok(throwsOf(() => short.collect({ left: 1, top: 0, right: 1, bottom: 5 }, 4, covered)));
    // 화면 높이 10 은 나눗셈이 0 이라 상한이 6 이다(헤더 6개째에서 거부하므로 5개까지 허용).
    const ten = new TileGroupCollection(hash, pool, { screenHeight: 10 });
    assert.equal(ten.collect({ left: 1, top: 0, right: 1, bottom: 3 }, 4, covered).length, 5);
    assert.ok(throwsOf(() => ten.collect({ left: 1, top: 0, right: 1, bottom: 4 }, 4, covered)));
});
