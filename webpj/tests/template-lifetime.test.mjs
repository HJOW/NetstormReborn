// Template9 20바이트 헤더 raw-template-lifetime.js(initialize/release/clear)를 원본 10.78 관측 240행과 cpppj 계약으로 대조한다.
//  - 원본 관측: template-lifetime-v2-x86.tsv — 입력(종류 "A" 전역 정리 / "S" 단일 반납 / 그 밖 초기화, 선택 칸, 소멸·free 직전 변이 종류)과 전체 196바이트(앞 8 + 헤더 9칸 180 + 뒤 8)로
//    정리 후 전체 196바이트와 소멸(D)/free(F) 직전 전체 바이트 호출 순서가 같다. 소멸·free 효과 뒤 헤더의 포인터/빌림 값이 바뀌어도 처음 저장한 manager 를 반납한다.
//  - 계약: 다른 폭의 헤더/배열은 거부하고, 필요한 효과가 없으면 반납 지점에서 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E123: Template 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { clearTemplateHeaders, initializeTemplateHeader, releaseTemplateHeader } from '../www/core/raw-template-lifetime.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E123)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'template-lifetime-v2-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

test('TemplateLifetime: 전역/단일/초기화와 소멸·free 직전 전체 헤더가 독립 240행과 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 240);
    // 각 행은 같은 전체 196바이트 입력과 현재 효과 변이를 사용한다.
    rows.forEach((row, index) => {
        assert.equal(row.length, 4);
        const input = row[0].slice(1, -1).split(',');
        assert.equal(input.length, 6);
        const bytes = Uint8Array.from(Buffer.from(row[1], 'hex'));
        const headers = bytes.subarray(8, 188);
        const selected = Number(input[4]);
        const variant = Number(input[5]);
        const trace = [];
        // 실제 경계 직전 상태를 관측한다. 고정 힙 입력 주소는 원문의 외부 불투명 handle 이다.
        const record = (kind, handle) => trace.push(`${kind},${handle},${hex(bytes)}`);
        const effects = {
            destroy: (handle) => {
                record('D', handle);
                const slot = (handle - 0x12100000) / 0x1000;
                if (variant === 1) {
                    put(headers, slot * 20 + 4, 0x87654321);
                    if (slot < 8) put(headers, (slot + 1) * 20 + 4, 1);
                }
            },
            release: (handle) => {
                record('F', handle);
                const slot = (handle - 0x12100000) / 0x1000;
                if (variant === 2) {
                    put(headers, slot * 20, 0xdead0000);
                    put(headers, slot * 20 + 16, 0x76543210);
                    if (slot < 8) put(headers, (slot + 1) * 20, 0);
                }
            }
        };
        if (input[0] === '"A"') clearTemplateHeaders(headers, effects);
        else if (input[0] === '"S"') releaseTemplateHeader(headers.subarray(selected * 20, selected * 20 + 20), effects);
        else initializeTemplateHeader(headers.subarray(selected * 20, selected * 20 + 20));
        assert.equal(hex(bytes), row[2], `Template 수명 행 ${index} 바이트`);
        assert.equal(trace.length ? trace.join(';') : '-', row[3], `Template 수명 행 ${index} 호출`);
    });
});

test('TemplateLifetime: 다른 폭 거부·미연결 효과 거부·빈 포인터의 stale 필드 보존', () => {
    assert.throws(() => initializeTemplateHeader(new Uint8Array(19)), RangeError);
    assert.throws(() => releaseTemplateHeader(new Uint8Array(21), {}), RangeError);
    assert.throws(() => clearTemplateHeaders(new Uint8Array(179), {}), RangeError);
    // 소유한(포인터 비영, 빌림 0) 헤더는 반납 지점에서 효과가 필요하다(없으면 거부, 헤더 불변).
    const owned = new Uint8Array(20).fill(0xaa);
    put(owned, 0, 0x12100000);
    put(owned, 4, 0);
    const before = owned.slice();
    assert.throws(() => releaseTemplateHeader(owned, {}), Error);
    assert.deepEqual(owned, before);
    // 빌린 헤더(빌림 DWORD 비영)는 소멸 없이 비우고, 빈 포인터 칸의 stale 필드는 전역 정리에서 보존한다.
    const borrowed = new Uint8Array(20).fill(0xbb);
    put(borrowed, 0, 0x12100000);
    put(borrowed, 4, 1);
    releaseTemplateHeader(borrowed, {});
    assert.ok(borrowed.every((value) => value === 0));
    const headers = new Uint8Array(180);
    put(headers, 2 * 20 + 8, 0x7391);
    clearTemplateHeaders(headers, {});
    assert.equal(headers[2 * 20 + 8], 0x91);
});
