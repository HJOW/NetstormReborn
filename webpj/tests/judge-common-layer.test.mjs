// 3-E 판정-공통층(WB04-E259)에서 ② 로 재반영한 두 가지를 확인한다: ContentFormHost 의 마지막 현재 가상+50 부착(cpppj AttachCurrent 3f766a97)과
// RawStateSidLists 의 RemoveStateSidList(c9140743). 두 변경 모두 cpppj 에 전용 시험이 없어 C++ 본문의 호출 순서·감김·범위 오류를 손으로 옮겼다.
// 나머지 20단위는 ①(웹 결과 영향 없음) 또는 ③(4-A·4-B·4-D·5-B 연결 때)으로 판정했고 근거는 docs/webpj-judge-common-20261110.md 에 있다. (2026-11-10 WB04-E259)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { ContentFormHost } from '../www/core/content-form-host.js';
import { RawContainedCreate } from '../www/core/contained-create.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { removeStateSidList } from '../www/core/raw-state-sid-lists.js';

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(form-hosts-contract.test.mjs 의 formTypeInputs 와 같다). (2026-11-10 WB04-E259)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}));
// 슬롯 WORD 를 읽는다. (2026-11-10 WB04-E259)
const word = (pool, sid, offset) => get(pool.slot(sid), offset, 2);

// 같은 풀의 공장과 Content 호스트를 만든다. 표면 통지(TYPE 6 = 다리 TYPE 로 취급) 콜백 안에서 호스트/VT 를 바꾸는 장면을 쓴다. (2026-11-10 WB04-E259)
function contentScene(onNotify) {
    const types = formTypeInputs();
    types[90].flags2 = 0x10000;
    const pool = new SidPool(32768, { server: true });
    const factory = new SquidFactory(pool, types);
    const parent = factory.create(90, 2);
    const content = new ContentFormHost(pool, types, () => {}, { state: { inBattle: () => false, checkingLinks: () => false }, captured: () => {} });
    const state = { containedType: () => 6, bridgeType: () => 6 };
    const created = [];
    const create = new RawContainedCreate(pool, state, content.bindCreate(factory, {
        notifySurface: (sid) => { created.push(sid); onNotify({ pool, content, sid, parent }); },
    }));
    return { pool, content, create, parent, created };
}

// 표면 통지가 호스트의 현재 +50 장치를 설치하면 마지막 부착은 호출 순간의 그 장치를 쓴다(BindCreate 시점의 낡은 장치를 쓰지 않는다).
test('ContentFormHost.bindCreate: 통지가 설치한 현재 가상+50 장치로 마지막 부착을 선택한다', () => {
    const calls = [];
    const scene = contentScene(({ content }) => content.bindReattach((form, parent, flags) => calls.push([form, parent, flags])));
    const form = scene.create.create(scene.parent, 80, 7, 0x20, 0x90, 3);
    // 정렬 부착(부모 목록 연결)은 실행되지 않고 선택한 장치만 같은 인자(부착 flags 전체 0x90)로 한 번 호출된다. 앞선 생성/필드 쓰기는 모두 끝난 상태다.
    assert.deepEqual(calls, [[form, scene.parent, 0x90]]);
    assert.deepEqual([word(scene.pool, form, 18), word(scene.pool, form, 12), scene.pool.slot(form)[30], word(scene.pool, form, 20)], [80, 7, 3, 0x20]);
    assert.equal(word(scene.pool, scene.parent, 6), 0);
    assert.equal(word(scene.pool, form, 16), 0);
});

// 장치가 없을 때 현재 VT 가 Content(6)가 아니면(통지가 VT 를 바꾼 경우) 생성/필드 쓰기 뒤에 부착 단계에서 거부한다. TYPE BYTE 는 VT 의 대용으로 쓰지 않는다.
test('ContentFormHost.bindCreate: 장치 없이 현재 VT 가 Content 가 아니면 부착만 거부한다', () => {
    const scene = contentScene(({ pool, sid }) => put(pool.allocated(sid), 0, 0x12345678));
    assert.throws(() => scene.create.create(scene.parent, 80, 7, 0x20, 0x10, 3), /현재 가상\+50 부착 장치 미연결/);
    const [form] = scene.created;
    // 부착 이전의 효과(TYPE/인자/품질/flags 쓰기)는 되돌리지 않고, 정렬 부착은 시작하지 않는다.
    assert.deepEqual([word(scene.pool, form, 18), word(scene.pool, form, 12), scene.pool.slot(form)[30], word(scene.pool, form, 20)], [80, 7, 3, 0x20]);
    assert.equal(word(scene.pool, scene.parent, 6), 0);
});

// 현재 VT 가 Content 이면 기존 정렬 부착으로 부모 목록에 연결된다(독립 호출의 기존 계약 유지).
test('ContentFormHost.bindCreate: 장치 없고 VT 가 Content 면 기존 정렬 부착을 쓴다', () => {
    const scene = contentScene(() => {});
    const form = scene.create.create(scene.parent, 80, 7, 0x20, 0x10, 3);
    assert.equal(word(scene.pool, scene.parent, 6), form);
    assert.equal(word(scene.pool, form, 16), scene.parent);
});

// 목록과 DWORD 배열을 만든다. (2026-11-10 WB04-E259)
function listOf(entries, count) {
    const list = new SquidPostPopList();
    list.entries = [...entries];
    list.count = count;
    return list;
}

// 일치 DWORD 를 모두 세고 나머지를 앞으로 압축한다. 비활성 꼬리(count 이상)는 읽지도 쓰지도 않는다.
test('removeStateSidList: 일치 항목을 모두 제거해 압축하고 꼬리와 용량을 보존한다', () => {
    const list = listOf([5, 7, 5, 9, 5, 0xdead, 0xbeef], 5);
    removeStateSidList(list, 5);
    assert.equal(list.count, 2);
    // 활성 칸은 [7, 9] 로 압축되고, 그 뒤의 옛 값(압축 전 내용)과 비활성 꼬리는 그대로 남는다.
    assert.deepEqual(list.entries, [7, 9, 5, 9, 5, 0xdead, 0xbeef]);
    // 일치가 없으면 아무것도 바뀌지 않는다.
    const same = listOf([1, 2, 3], 3);
    removeStateSidList(same, 4);
    assert.deepEqual([same.entries, same.count], [[1, 2, 3], 3]);
});

// sid 는 DWORD 전체로 비교한다: 음수 입력은 2^32 로 감겨 같은 DWORD 와 일치한다.
test('removeStateSidList: sid 와 항목은 DWORD 로 비교한다', () => {
    const list = listOf([0xffffffff, 1, 0xffffffff], 3);
    removeStateSidList(list, -1);
    assert.deepEqual([list.count, list.entries[0]], [1, 1]);
    // 다른 코드가 부호 있는 수로 저장한 항목(-1)도 같은 DWORD 로 읽혀 일치하고, 일치하지 않는 부호 있는 항목은 DWORD 로 압축된다.
    const signedEntries = listOf([-1, -2, 7], 3);
    removeStateSidList(signedEntries, 0xffffffff);
    assert.deepEqual([signedEntries.count, signedEntries.entries[0], signedEntries.entries[1]], [2, 0xfffffffe, 7]);
});

// 음수 count(원문 signed 비교)는 읽기/쓰기 없이 지나가며 count 도 바뀌지 않는다.
test('removeStateSidList: 음수 count 는 아무 칸도 읽지 않는다', () => {
    const list = listOf([5, 5], 0xffffffff);
    removeStateSidList(list, 5);
    assert.deepEqual([list.count, list.entries], [0xffffffff, [5, 5]]);
});

// count 가 가리키는 칸이 물리 배열 밖이면 그 위치에서 멈춘다. 이미 한 압축 효과와 count 는 되돌리지도 갱신하지도 않는다.
test('removeStateSidList: 물리 범위 밖 칸에서 멈추고 앞선 압축은 남긴다', () => {
    const list = listOf([5, 7], 4);
    assert.throws(() => removeStateSidList(list, 5), RangeError);
    // 첫 칸(5)이 제거로 세어진 뒤 둘째 칸(7)이 앞으로 압축되었고, count 는 마지막에만 줄이므로 그대로다.
    assert.deepEqual([list.entries, list.count], [[7, 7], 4]);
});
