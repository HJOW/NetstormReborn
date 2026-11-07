// 공통 Squid 의 작은 상태 setter 네 가지를 검사한다(WB04-E2): 레벨(RawSquidLevel), 섬 번호(setSquidRegion), 표면 WORD(setRawSquidWord), 현재 HP(setRawSquidHitPoints).
//  - 레벨: squid-level-x86.tsv 10.78 행(has/level/진단 횟수/전체 슬롯, cpppj CampaignMotion_ReplaysActualLevelNativeBothEditions 의 10.78 부분).
//  - 섬 번호: 현재 genus 분기(미지원 genus 진단·다리 region 진단·다리 noRegion 은 조용히 반환·섬 WORD 저장)와 진단 장치 부재의 거부.
//  - 표면 WORD/HP: 저장 폭과 이웃 필드 보존, 다리 통지 부재가 앞선 저장을 되돌리지 않음.
// 기대값은 fixture 와 cpppj 소스의 계약이다. (2026-11-08 WB04-E2: 공통 작은 setter 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawSquidLevel, makeSquidLevelAssertion } from '../www/core/raw-squid-level.js';
import { SQUID_REGION_ASSERT, getSquidRegion, setSquidRegion } from '../www/core/raw-squid-region.js';
import { setRawSquidHitPoints, setRawSquidWord } from '../www/core/raw-squid-word.js';

// 레벨 원본 관측 행(10.78만)이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E2)
const levelRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'squid-level-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-08 WB04-E2)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 입력 SID 50까지 로컬 할당한 풀이다. (2026-11-08 WB04-E2)
function poolWithTarget(server = true) {
    const pool = new SidPool(32768, { server });
    for (let sid = 5; sid <= 50; sid += 1) assert.equal(pool.allocate(2), sid);
    return pool;
}

// 새 독립 PE 관찰 입력을 같은 현재 슬롯의 전체 BYTE 와 직접 대조한다: 행은 [판본, state, group, n, 진단 변경, has, level, assertions, 전체 슬롯]이다.
test('RawSquidLevel: 원본10.78 레벨 has/get/set 행(진단 복귀와 전체 슬롯)', { skip: !levelRows && '원본 fixture 없음' }, () => {
    let cases = 0;
    for (const row of levelRows) {
        assert.equal(row.length, 9);
        const pool = poolWithTarget();
        const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        const sid = 50;
        const raw = pool.allocated(sid);
        raw.fill(0);
        raw[10] = 82;
        raw[11] = Number(row[1]);
        let assertions = 0;
        const level = new RawSquidLevel(pool, types, makeSquidLevelAssertion((expression, file, line) => {
            assert.deepEqual([expression, file, line], ['0 <= n && n < 8', '\\Ns\\O\\Squid.cpp', 0x579]);
            assertions += 1;
            // 진단 장치가 정상 복귀하기 전에 현재 상태 BYTE 를 바꾸면 저장은 그 현재 값을 읽는다.
            if (Number(row[4])) pool.allocated(sid)[11] = 0x14;
        }));
        assert.equal(level.has(sid), Number(row[5]) !== 0, `행 ${cases}`);
        level.set(sid, Number(row[3]));
        assert.deepEqual([level.get(sid), assertions, hex(pool.slot(sid))], [Number(row[6]), Number(row[7]), row[8]], `행 ${cases}: ${row.slice(1, 5).join(',')}`);
        cases += 1;
    }
    assert.equal(cases, levelRows.length);
    assert.ok(cases >= 40);
});

// 레벨 계약: 범위 밖(signed 음수 포함)은 진단 뒤에도 하위 3비트를 저장하고, 하위 5비트를 보존하며, 현재 TYPE 이 표 밖이면 거부한다. 진단 장치가 없으면 생성부터 거부한다.
test('RawSquidLevel: 범위 진단 뒤 저장 계속·하위 5비트 보존·TYPE 범위 거부', () => {
    const pool = poolWithTarget();
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const raw = pool.allocated(50);
    raw[10] = 82;
    raw[11] = 0x1d;
    const lines = [];
    const level = new RawSquidLevel(pool, types, makeSquidLevelAssertion((expression, file, line) => lines.push(line)));
    level.set(50, 5);
    assert.deepEqual([pool.slot(50)[11], level.get(50), lines.length], [0xbd, 5, 0]);
    level.set(50, 0xfffffffd);
    assert.deepEqual([pool.slot(50)[11], lines], [0xbd, [0x579]], '음수 n 은 진단 뒤 하위 3비트(5)만 저장');
    level.set(50, 9);
    assert.deepEqual([pool.slot(50)[11], lines.length], [0x3d, 2], '8 이상도 진단 뒤 하위 3비트(1)');
    raw[10] = 200;
    assert.throws(() => level.has(50), /TYPE 범위/);
    assert.throws(() => new RawSquidLevel(pool, types, null), TypeError);
    assert.throws(() => makeSquidLevelAssertion(null)(), /진단 장치 미연결/);
});

// 섬 번호: genus 분기와 진단 복귀.
test('setSquidRegion: 현재 genus 분기와 진단 복귀', () => {
    const pool = poolWithTarget();
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const raw = pool.allocated(50);
    raw.fill(0);
    raw[10] = 82;
    put(raw);
    const seen = [];
    const diagnostic = (kind) => seen.push(kind);
    // 미지원 genus: 진단 뒤 저장하지 않는다(기존 WORD 보존).
    setSquidRegion(pool, types, 50, 0x1234, 0xffff, diagnostic);
    assert.deepEqual([seen, get(pool.slot(50), 8, 2)], [[SQUID_REGION_ASSERT.unsupportedGenus], 0x5566]);
    // 섬 genus(0x02): WORD 하위 16비트만 저장하고 이웃 바이트를 보존한다.
    types[82].flags2 = 2;
    setSquidRegion(pool, types, 50, 0x12345678, 0xffff, diagnostic);
    assert.deepEqual([get(pool.slot(50), 8, 2), pool.slot(50)[7], pool.slot(50)[10], seen.length], [0x5678, 0x77, 82, 1]);
    // 다리 genus(0x04): region 이 noRegion 이면 조용히 반환하고, 다르면 진단 뒤 기존 WORD 를 보존한다.
    types[82].flags2 = 4;
    setSquidRegion(pool, types, 50, 0xffff, 0xffff, diagnostic);
    assert.deepEqual([get(pool.slot(50), 8, 2), seen.length], [0x5678, 1]);
    setSquidRegion(pool, types, 50, 7, 0xffff, diagnostic);
    assert.deepEqual([get(pool.slot(50), 8, 2), seen.at(-1)], [0x5678, SQUID_REGION_ASSERT.bridgeRegion]);
    // 진단 경계가 없으면 그 분기에서만 거부하고, 타입 번호가 표 밖이면 범위 오류이다.
    assert.throws(() => setSquidRegion(pool, types, 50, 7, 0xffff), /진단 경계 누락/);
    assert.doesNotThrow(() => setSquidRegion(pool, types, 50, 0xffff, 0xffff));
    raw[10] = 200;
    assert.throws(() => setSquidRegion(pool, types, 50, 1, 0xffff, diagnostic), RangeError);
});

// 섬 번호 getter(기준선 뒤 cpppj 9cb0547f 반영): 미지원 genus 는 진단 뒤에도 noRegion, 다리는 raw WORD 를 읽지 않고 noRegion, 섬은 WORD 를 돌려준다.
test('getSquidRegion: genus 분기와 진단 복귀', () => {
    const pool = poolWithTarget();
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const raw = pool.allocated(50);
    raw.fill(0);
    raw[10] = 82;
    put(raw);
    const seen = [];
    assert.equal(getSquidRegion(pool, types, 50, 0xffff, (kind) => seen.push(kind)), 0xffff);
    assert.deepEqual(seen, [SQUID_REGION_ASSERT.unsupportedGenus]);
    types[82].flags2 = 2;
    assert.equal(getSquidRegion(pool, types, 50, 0xffff), 0x5566);
    types[82].flags2 = 0x01000000;
    assert.equal(getSquidRegion(pool, types, 50, 0xffff), 0x5566, '3x3 섬 genus');
    types[82].flags2 = 4;
    assert.equal(getSquidRegion(pool, types, 50, 0x7fff), 0x7fff, '다리는 noRegion');
    types[82].flags2 = 0;
    assert.throws(() => getSquidRegion(pool, types, 50, 0xffff), /진단 미연결/);
    raw[10] = 200;
    assert.throws(() => getSquidRegion(pool, types, 50, 0xffff), RangeError);
});

// 슬롯의 섬 WORD(+8)에 0x5566 과 이웃 표식(+7=0x77)을 놓는다. (2026-11-08 WB04-E2)
function put(raw) {
    raw[7] = 0x77;
    raw[8] = 0x66;
    raw[9] = 0x55;
}

// 표면 WORD: 낮은 BYTE 부터 저장하고 현재 다리 TYPE 이면 통지한다. 통지 부재는 앞선 저장을 되돌리지 않고 그 위치에서 거부한다.
test('setRawSquidWord: WORD 저장 뒤 현재 다리 TYPE 이면 통지한다', () => {
    const pool = poolWithTarget();
    const raw = pool.allocated(50);
    raw[10] = 82;
    raw[11] = 0;
    raw[14] = 0x99;
    const notified = [];
    setRawSquidWord(pool, 50, 0xabcd, 83, (sid) => notified.push(sid));
    assert.deepEqual([get(pool.slot(50), 12, 2), pool.slot(50)[14], notified], [0xabcd, 0x99, []]);
    setRawSquidWord(pool, 50, 0x1234, 82, (sid) => notified.push(sid));
    assert.deepEqual([get(pool.slot(50), 12, 2), notified], [0x1234, [50]]);
    // 통지 부재는 앞선 저장을 되돌리지 않는다.
    assert.throws(() => setRawSquidWord(pool, 50, 0x4321, 82, null), /통지 미연결/);
    assert.equal(get(pool.slot(50), 12, 2), 0x4321);
    // 물리 슬롯이면 free 슬롯에도 쓴다(패치판은 free/contained 여부로 선행 거부하지 않음).
    pool.physical(60)[11] = 1;
    setRawSquidWord(pool, 60, 0x0f0f, 83, null);
    assert.equal(get(pool.slot(60), 12, 2), 0x0f0f);
});

// 현재 HP: 낮은 바이트부터 DWORD 만 쓰고 옆 필드를 보존하며, 할당되지 않은 슬롯은 거부한다.
test('setRawSquidHitPoints: HP DWORD 만 쓰고 이웃 필드를 보존한다', () => {
    const pool = poolWithTarget();
    const raw = pool.allocated(50);
    raw.fill(0xee);
    setRawSquidHitPoints(pool, 50, -2);
    assert.deepEqual([...pool.slot(50).subarray(24, 32)], [0xee, 0xee, 0xfe, 0xff, 0xff, 0xff, 0xee, 0xee]);
    setRawSquidHitPoints(pool, 50, 91);
    assert.equal(get(pool.slot(50), 26, 4), 91);
    assert.throws(() => setRawSquidHitPoints(pool, 51, 1), /할당되지 않은/);
});
