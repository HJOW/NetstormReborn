// C++ DestroyPatch/ReleasePatch의 진단 정상 복귀 지점13개와 효과 뒤 현재 상태 재조회를 검사한다. 손상 입력은 격리된 풀에만 넣는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidForms } from '../www/core/squid-forms.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';
import { put } from './raw-support.mjs';

// 실제 풀·삭제기이며 가상 효과는 깊이 완료와 호출 순서만 기록한다. 기본 루트는 void 자산으로 공간 해제를 요구하지 않는다.
function scene(serverSid = false) {
    const authority = { value: true }, pool = new SidPool(24000, { serverOwner: authority }), types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, name: 'unit\0tail', cost: 0 }));
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types), root = pool.allocate(serverSid ? 0 : 2), events = [], lines = [];
    pool.allocated(root)[10] = 74;
    const result = { pool, types, destroy, root, authority, events, lines, assertion: () => {} };
    destroy.bindPatchDiagnostics({ assertion: (expression, file, line) => { lines.push(line); events.push(`assert:${line}`); result.assertion(expression, file, line); }, trace: (level, text) => { assert.equal(level, 0x27); events.push(text); } });
    result.hooks = { selected: () => 0, emit: (event) => { events.push(event.effect); if (event.effect === 'preDestroy') destroy.completePreDestroy(); if (event.effect === 'postDestroy') destroy.completePostDestroy(); } };
    return result;
}

test('DestroyPatch: free 진단은 dead 표시 뒤이며 복귀 후 현재 free에 따라 반환/계속한다', () => {
    // 같은 원래 free 입력에서 진단 장치의 현재 상태 수정만 달리한다.
    for (const repair of [false, true]) {
        const s = scene(); s.pool.physical(s.root)[11] = 1;
        s.assertion = () => { assert.equal(s.pool.slot(s.root)[11], 3); if (repair) s.pool.physical(s.root)[11] = 6; };
        s.destroy.destroy(s.root, 0, s.hooks); assert.deepEqual(s.lines, [0x24d]);
        assert.equal(s.events.includes('preDestroy'), repair); assert.equal(s.pool.slot(s.root)[11], 3);
        if (repair) assert.equal(s.pool.deletions(true)[0].sid, s.root);
    }
});

test('DestroyPatch: 권한 진단 복귀 뒤 원본처럼 반환하고 서버 객체의 dead는 남는다', () => {
    const s = scene(true); s.authority.value = false; s.destroy.destroy(s.root, 0, s.hooks);
    assert.deepEqual(s.lines, [0x269]); assert.equal(s.pool.slot(s.root)[11], 6); assert.equal(s.events.length, 1);
});

test('DestroyPatch: pre/post 깊이 불일치를 알린 뒤 다음 효과와 반납을 계속한다', () => {
    const s = scene(); s.hooks.emit = (event) => s.events.push(event.effect);
    s.assertion = (_expression, _file, line) => { if (line === 0x276) s.destroy.completePreDestroy(); if (line === 0x2a9) s.destroy.completePostDestroy(); };
    s.destroy.destroy(s.root, 0, s.hooks);
    assert.deepEqual(s.lines, [0x276, 0x2a9]); assert.equal(s.events[0], `destroying unit ${s.root}\n`);
    assert.deepEqual([s.destroy.preDepth, s.destroy.postDepth, s.pool.slot(s.root)[11]], [0, 0, 3]);
});

test('DestroyPatch: 종속 free/contained 진단과 free 재진단은 원래 순서이며 next를 먼저 보존한다', () => {
    const s = scene(), child = s.pool.allocate(2); put(s.pool.physical(s.root), 6, child, 2); s.pool.physical(child)[10] = 75; s.pool.physical(child)[11] = 1;
    s.assertion = (_expression, _file, line) => { if (line === 0x280) put(s.pool.physical(child), 4, child, 2); };
    s.destroy.destroy(s.root, 0, s.hooks);
    assert.deepEqual(s.lines, [0x27d, 0x27e, 0x280]);
    assert.ok(s.events.includes('releaseDependent')); assert.ok(!s.events.includes('destroyDependent')); assert.equal(s.pool.slot(s.root)[11], 3);
});

test('DestroyPatch: pre에서 풀린 루트는 진단 복귀 후 반환하며 물리 SID0은 유효성 진단 후 계속한다', () => {
    const freed = scene(), emit = freed.hooks.emit;
    freed.hooks.emit = (event) => { emit(event); if (event.effect === 'preDestroy') freed.pool.physical(freed.root)[11] |= 1; };
    freed.destroy.destroy(freed.root, 0, freed.hooks); assert.deepEqual(freed.lines, [0x28f]); assert.ok(!freed.events.includes('postDestroy'));
    const zero = scene(); zero.pool.physical(0).fill(0); zero.pool.physical(0)[10] = 74; zero.pool.physical(0)[11] = 4;
    zero.destroy.destroy(0, 0, zero.hooks); assert.deepEqual(zero.lines, [0x2a2]); assert.equal(zero.pool.slot(0)[11], 3);
});

test('ReleasePatch: void/free 진단 뒤 현재 TYPE를 기록하고 꼬리 진단 복귀도 FIFO 반납을 완료한다', () => {
    const s = scene(); s.pool.physical(s.root)[11] = 1;
    s.pool.releasePatch(s.root, (_expression, _file, line) => { s.lines.push(line); if (line === 0x15d) s.pool.physical(s.root)[10] = 75; });
    assert.deepEqual(s.lines, [0x15c, 0x15d]); assert.deepEqual(s.pool.deletions(true)[0], { sid: s.root, type: 75 });
    const tail = scene(), sid = tail.pool.tail(true), count = tail.pool.freeCount;
    tail.pool.releasePatch(sid, (_expression, file, line) => { tail.lines.push(line); if (line === 0x123) assert.equal(file, 'c:\\ns\\o\\squid.h'); });
    assert.deepEqual(tail.lines, [0x15c, 0x15d, 0x123]); assert.equal(tail.pool.freeCount, count + 1); assert.equal(tail.pool.slot(sid)[11], 3);
});

test('DestroyPatch: 현재 Form VT로 Unpop을 고르고 새 void 결과 검사를 더하지 않는다', () => {
    const s = scene(), emit = s.hooks.emit; let unpopped = 0;
    s.hooks.emit = (event) => { emit(event); if (event.effect === 'preDestroy') { const raw = s.pool.physical(s.root); raw[11] &= ~4; put(raw, 0, new SquidForms(s.pool).vtable(6)); } };
    s.hooks.unpopForm = () => { unpopped += 1; }; s.destroy.destroy(s.root, 0, s.hooks);
    assert.equal(unpopped, 1); assert.deepEqual(s.lines, [0x15c]); assert.equal(s.pool.slot(s.root)[11], 3);
});

test('DestroyPatch Lifecycle: !isForm 진단 복귀가 바꾼 현재 TYPE로 후속 장부를 처리한다', () => {
    const s = scene(), state = new SquidDeletionState(); s.pool.physical(s.root)[10] = 6;
    const lifecycle = new SquidDestroyLifecycle(s.pool, s.types, { graphsEnabled: false, suppressed: true, totalCost: 0 }, state, s.destroy);
    s.assertion = (_expression, _file, line) => { if (line === 0x771) s.pool.physical(s.root)[10] = 74; };
    s.destroy.destroy(s.root, 0, lifecycle.hooks()); assert.deepEqual(s.lines, [0x771]);
    assert.equal(s.pool.deletions(true)[0].type, 74); assert.deepEqual([s.destroy.preDepth, s.destroy.postDepth], [0, 0]);
});
