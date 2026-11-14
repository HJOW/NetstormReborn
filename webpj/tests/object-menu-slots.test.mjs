// 객체 메뉴 슬롯 표(OBJECT_MENU_SLOTS)와 VT 조회(objectMenuSlots)를 원본 표의 표본 값·83개 VT 집합(ObjectHitVtables 와 동일)·조회 규칙으로 검증한다.
// 원본 .inc 에서 기계적으로 생성한 표이므로 첫/끝/중간 표본을 손으로 대조하고, 두 독립 생성 표의 VT 집합 일치로 누락/오타를 막는다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E447)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { OBJECT_HIT_VTABLES } from '../www/render/object-hit-vtables.js';
import { OBJECT_MENU_SLOTS, objectMenuSlots } from '../www/render/object-menu-slots.js';

test('표 규모와 불변성: 83개 항목이고 모든 항목과 배열이 동결되며 VT 는 서로 다르고 오름차순이다', () => {
    assert.equal(OBJECT_MENU_SLOTS.length, 83); assert.ok(Object.isFrozen(OBJECT_MENU_SLOTS)); assert.ok(OBJECT_MENU_SLOTS.every((entry) => Object.isFrozen(entry)));
    assert.equal(new Set(OBJECT_MENU_SLOTS.map((entry) => entry.vtable)).size, 83); const sorted = OBJECT_MENU_SLOTS.map((entry) => entry.vtable); assert.deepEqual(sorted, [...sorted].sort((a, b) => a - b));
    for (const entry of OBJECT_MENU_SLOTS) assert.deepEqual(Object.keys(entry), ['vtable', 'populate', 'open']);
});

test('표본: 첫 두 항목·같은 populate/open 를 공유하는 일반 항목·끝 항목의 DWORD 가 원본 표와 같다', () => {
    assert.deepEqual({ ...OBJECT_MENU_SLOTS[0] }, { vtable: 0x00501dd0, populate: 0x004b1270, open: 0x004adb30 }); assert.deepEqual({ ...OBJECT_MENU_SLOTS[1] }, { vtable: 0x00501ea0, populate: 0x0041c210, open: 0x004adb30 });
    for (const index of [2, 3, 4, 5, 6, 7, 8, 9]) assert.deepEqual([OBJECT_MENU_SLOTS[index].populate, OBJECT_MENU_SLOTS[index].open], [0x00420b30, 0x0041e660], String(index));
    assert.equal(OBJECT_MENU_SLOTS[2].vtable, 0x00502370); assert.equal(OBJECT_MENU_SLOTS[9].vtable, 0x00502908);
});

test('ObjectHitVtables 와 같은 83개 VT 집합(순서도 같다)을 가진다', () => {
    assert.deepEqual(OBJECT_MENU_SLOTS.map((entry) => entry.vtable), OBJECT_HIT_VTABLES.map((entry) => entry.table));
});

test('objectMenuSlots: 등록된 VT 는 같은 항목 객체를 돌려주고 부호 있는 32비트 입력도 같은 VT 로 보며 미등록 VT 는 RangeError 다', () => {
    for (const entry of OBJECT_MENU_SLOTS) assert.equal(objectMenuSlots(entry.vtable), entry);
    assert.equal(objectMenuSlots(0x00501dd0).populate, 0x004b1270); assert.equal(objectMenuSlots(0x00501dd0 | 0), OBJECT_MENU_SLOTS[0]); assert.equal(objectMenuSlots(0x100501dd0), OBJECT_MENU_SLOTS[0]);
    for (const vtable of [0, 0x00501dd1, 0x00501dcf, 0xffffffff, -1, 0x00502ffe]) assert.throws(() => objectMenuSlots(vtable), { name: 'RangeError', message: /객체메뉴 VT 미등록/ }, String(vtable));
    assert.throws(() => objectMenuSlots(undefined), RangeError);
});

// cpppj 의 원본 표(기준선 뒤 변경 없음)를 읽어 83개 항목의 세 DWORD 를 전부 대조한다(파일이 없으면 건너뛴다).
const SOURCE = path.join(REPO_ROOT, 'cpppj', 'src', 'client', 'ObjectMenuSlots.inc');
test('원본 ObjectMenuSlots.inc 의 83개 항목 전체와 같다', { skip: !fs.existsSync(SOURCE) }, () => {
    const rows = [...fs.readFileSync(SOURCE, 'utf8').matchAll(/{0x([0-9a-fA-F]+)U,0x([0-9a-fA-F]+)U,0x([0-9a-fA-F]+)U}/g)].map((match) => ({ vtable: parseInt(match[1], 16), populate: parseInt(match[2], 16), open: parseInt(match[3], 16) }));
    assert.equal(rows.length, 83); assert.deepEqual(OBJECT_MENU_SLOTS.map((entry) => ({ ...entry })), rows);
});
