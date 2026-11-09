// Badges 전역 장부 raw-fort-badges-globals.js(CRT 구성 004fcbe0·count reset 0047d860·소유자)를 원본 10.78 관측과 cpppj `FortBadgesGlobalsTests` 계약으로 대조한다.
//  - 원본 관측 fort-badges-globals-x86.tsv 32행(crt 8·reset 8·crt-reset 8·reset-crt 8): 초기 프로필 0..7(0, 0xff, 0xa5, 증가, 감소, 곱셈, DWORD 패턴, 1..255 순환)의 6160 BYTE 장부에
//    관측과 같은 순서로 두 함수를 적용한 전체 장부가 원본 출력과 한 바이트도 다르지 않다(첫 기록 +4..+11 의 8 BYTE 가 입력 그대로 남는 것 포함).
//  - 계약: 소유자의 주소 보존(같은 배열)과 count 외 6048 BYTE 불변 reset, 짧거나 긴 배열 거부 후 입력 불변.
// 실제 MetaLoader·getter·writer 연결 시험(cpppj 의 세 번째 시험)은 RawFortMetaLoad 가 미이식이라 그 단위를 옮길 때 대조한다. (2026-11-10 WB04-E225: Badges 전역 장부 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import {
    FORT_BADGES_COUNT_OFFSET, FORT_BADGES_GLOBALS_BYTES, FORT_BADGES_RECORD_BYTES, FORT_BADGES_RECORD_COUNT,
    RawFortBadgesGlobals, initializeFortBadgesRecords, resetFortBadgeCounts,
} from '../www/core/raw-fort-badges-globals.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다. (2026-11-10 WB04-E225)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fort-badges-globals-x86.tsv'));

// 원본에 공급한 명시 프로필만 만들고 출력의 기대 초기화는 계산하지 않는다. cpppj `BadgeGlobalsInitial` 과 같다. (2026-11-10 WB04-E225)
function initialProfile(profile) {
    const bytes = new Uint8Array(FORT_BADGES_GLOBALS_BYTES);
    for (let index = 0; index < bytes.length; index += 1) { // type/count/padding 도 같은 초기 BYTE 패턴으로 채운다.
        switch (profile) {
        case 0: break;
        case 1: bytes[index] = 255; break;
        case 2: bytes[index] = 0xa5; break;
        case 3: bytes[index] = index & 0xff; break;
        case 4: bytes[index] = (255 - index) & 0xff; break;
        case 5: bytes[index] = (index * 37 + 19) & 0xff; break;
        case 6: bytes[index] = (((Math.imul(Math.trunc(index / 4), 0x10003) + 0x80000000) >>> 0) >>> ((index % 4) * 8)) & 0xff; break;
        default: assert.equal(profile, 7); bytes[index] = 1 + (index % 255); break;
        }
    }
    return bytes;
}

// 16진수 문자열을 바이트 배열로 푼다. (2026-11-10 WB04-E225)
function unhex(hex) {
    assert.equal(hex.length % 2, 0);
    return Uint8Array.from(hex.match(/../g) ?? [], (pair) => parseInt(pair, 16));
}

// 예외 종류를 확인한다. (2026-11-10 WB04-E225)
function throwsKind(action, kind) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

test('Badges 전역 장부: 원본 10.78 32입력의 초기 0/비0 전체 장부와 두 함수 순서가 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let compared = 0;
    for (const row of rows) {
        assert.equal(row.length, 10);
        const bytes = initialProfile(Number(row[2]));
        for (const stage of row[1].split('-')) { // 원본과 같은 명시 두 함수 순서만 적용한다.
            if (stage === 'crt') initializeFortBadgesRecords(bytes);
            else { assert.equal(stage, 'reset'); resetFortBadgeCounts(bytes); }
        }
        assert.equal(row[3], '1', row[0]);
        assert.deepEqual(bytes, unhex(row[4]), `${row[0]} (${row[1]}, 프로필 ${row[2]})`);
        compared += 1;
    }
    assert.equal(compared, 32);
});

test('Badges 전역 장부 계약: 새 소유자는 BSS 0 → CRT 한 번이며 배열 주소를 보존하고 reset 은 count 외 6048 BYTE 를 바꾸지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const first = new RawFortBadgesGlobals();
    const second = new RawFortBadgesGlobals();
    const view = first.records();
    assert.equal(first.records(), view);
    assert.notEqual(second.records(), view);
    assert.equal(view.length, 6160);
    // 최초 BSS 0 → CRT 결과는 프로필 0 의 crt 행과 같다.
    assert.deepEqual(view, unhex(rows[0][4]));
    // 무늬로 채운 뒤 reset 하면 count 4 BYTE 외 모든 바이트가 그대로이다.
    view.set(initialProfile(7));
    const previous = Uint8Array.from(view);
    first.resetCounts();
    for (let index = 0; index < view.length; index += 1) {
        assert.equal(view[index], index % FORT_BADGES_RECORD_BYTES >= FORT_BADGES_COUNT_OFFSET ? 0 : previous[index], `index ${index}`);
    }
    assert.equal(first.records(), view);
    // count 위치는 기록마다 216..219 이며 기록 수는 28 이다.
    assert.equal(FORT_BADGES_RECORD_COUNT * FORT_BADGES_RECORD_BYTES, 6160);
    // 짧은/긴 장부는 효과 전에 거부하고 입력을 바꾸지 않는다. 바이트 배열이 아닌 값도 거부한다.
    const shortBytes = new Uint8Array(6159).fill(0xa5);
    const longBytes = new Uint8Array(6161).fill(0xa5);
    assert.ok(throwsKind(() => initializeFortBadgesRecords(shortBytes), RangeError));
    assert.ok(throwsKind(() => resetFortBadgeCounts(longBytes), RangeError));
    assert.ok(throwsKind(() => initializeFortBadgesRecords([]), TypeError));
    assert.ok(shortBytes.every((value) => value === 0xa5) && longBytes.every((value) => value === 0xa5));
});
