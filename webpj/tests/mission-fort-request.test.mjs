// 미션 Fort 요청(mission-fort-request.js)과 Fort 원문 이미지 값(mission-fort-source.js)을 원본 10.78 관측과 cpppj 시험, 웹 계약으로 확인한다.
//  - 원본 관측: mission-fort-request-x86.tsv 144행(네 이름 슬롯 × 요청 전체 144BYTE 양옆 이웃 8BYTE × Fort 선택 뒤/복제 뒤 현재값 변형 3종)의 요청 후 160BYTE·순번·Zone 512BYTE·복제 86BYTE·효과 호출 순서를 전부 대조한다.
//  - cpppj 시험 이식: MissionFortRequestTests 2건(Native144 · OwnedImageAndCurrentFields).
//  - 웹 계약: 이름의 첫 NUL 절단, 주소 길이/형식 거부, 도달한 효과 누락의 부분 쓰기, Zone 캡처 거부 조건(소비됨·중복·예약 크기·슬롯 수 범위), 지연 로드 취소 순서, MissionFortSource 값 검증.
//  - 제외 확인: `WriteCurrentGuideFortRequest` 는 Edit 전용(호출자 UberEditor.inc 하나)이라 옮기지 않았음을 모듈 내보내기로 확인한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E337)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import * as requestModule from '../www/core/mission-fort-request.js';
import { captureMissionFortZone, createMissionFortLoadRequest, releaseMissionFortLoad, writeMissionFortRequest } from '../www/core/mission-fort-request.js';
import { createMissionFortFileSelection, createMissionFortSourceImage } from '../www/core/mission-fort-source.js';
import { makeLocalMissionZone } from '../www/core/local-mission-transport.js';

// 원본 관측 행이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-12 WB04-E337)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-fort-request-x86.tsv'));

// 바이트를 소문자 16진 문자열로 바꾼다. (2026-11-12 WB04-E337)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
// 소문자 16진 문자열을 바이트로 바꾼다. (2026-11-12 WB04-E337)
const unhex = (text) => Uint8Array.from(text.match(/../gu) ?? [], (pair) => Number.parseInt(pair, 16));
// 문자열의 각 글자를 한 바이트로 보는 ASCII 바이트이다(원문 이름은 평면 ASCII). (2026-11-12 WB04-E337)
const asciiBytes = (text) => Uint8Array.from(text, (ch) => ch.charCodeAt(0));
// 예외가 나는지 본다. (2026-11-12 WB04-E337)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

test('원본 관측 144행: 요청 전체 160BYTE·순번·Zone 512BYTE·복제 86BYTE·호출 순서가 모두 일치한다', { skip: ROWS === null }, () => {
    assert.equal(ROWS.length, 144);
    // 각 행은 새 물리 블록과 현재 외부 전역으로 원본과 같은 입력을 받는다.
    for (const row of ROWS) {
        assert.equal(row.length, 7);
        const input = JSON.parse(row[0]);
        const address = unhex(input[1]);
        const port = { value: input[2] >>> 0 };
        const count = { value: input[3] >>> 0 };
        const name = input[4];
        const variant = input[5];
        const result = unhex(row[1]);
        const next = new Uint32Array(36);
        // 원문 144BYTE 를 DWORD 저장소로 읽고 양쪽 이웃 8BYTE 는 그대로 둔다.
        for (let index = 0; index < next.length; index += 1) next[index] = get(result, 8 + index * 4);
        const request = createMissionFortLoadRequest();
        let trace = '';
        let storage = '';
        const hooks = {
            selectFort(chosen) {
                trace = `Fort,${hex(asciiBytes(chosen))}`;
                // 변형 1: 큰 Fort 선택이 현재 주소·포트·순번을 바꾼다.
                if (variant === 1) {
                    address.set(unhex('414243444546474849'));
                    port.value = 12345;
                    count.value = 0xfffffffe;
                }
            },
            duplicate(packet) {
                trace += ';Allocate,86';
                // 변형 2: 복제 효과가 순번·포트·요청 DWORD 를 바꾼다.
                if (variant === 2) {
                    count.value = 0xffffffff;
                    port.value = 9876;
                    next[0x60 / 4] = 0x76543210;
                }
                // 관찰기 확정 전의 선언 길이는 4 이고 확정 뒤에는 15 + 슬롯 수 × 71 이다.
                assert.equal(get(packet.storage, 2, 2), 4);
                assert.equal(captureMissionFortZone(request, packet), 1);
                assert.equal(get(packet.storage, 2, 2), 86);
                storage = hex(packet.storage);
                return 0x12200000;
            },
        };
        writeMissionFortRequest(name, { next, sequence: count, localAddress: address, port: () => port.value }, hooks);
        // 전체 DWORD 를 주변 8BYTE 와 함께 같은 160BYTE 출력으로 직렬화한다.
        for (let index = 0; index < next.length; index += 1) put(result, 8 + index * 4, next[index]);
        assert.equal(hex(result), row[2]);
        assert.equal(String(count.value), row[3]);
        assert.equal(storage, row[4]);
        assert.equal(hex(request.zone), row[5]);
        assert.equal(trace, row[6]);
    }
});

test('cpppj MissionFortRequest_OwnedImageAndCurrentFields: 복제 이미지는 단독 소유이고 중복 캡처를 거부한다', () => {
    const request = createMissionFortLoadRequest();
    request.next.fill(0xa5a5a5a5);
    const address = Uint8Array.from([0x7a, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x2c, 0x2a, 0]);
    const sequence = { value: 0 };
    const hooks = {
        selectFort(chosen) { request.fort = chosen; },
        duplicate(packet) {
            // 복제 직전에는 상태 7·현재 포트가 이미 쓰였고 +0x5c 의 윗 3BYTE 는 stale 이다.
            assert.equal(request.next[0], 7);
            assert.equal(request.next[31], 6799);
            assert.equal(request.next[23] >>> 8, 0xa5a5a5);
            return captureMissionFortZone(request, packet);
        },
    };
    writeMissionFortRequest('Tutorial1', { next: request.next, sequence, localAddress: address, port: { value: 6799 } }, hooks);
    assert.equal(request.fort, 'Tutorial1');
    assert.equal(request.next[32], 1);
    assert.equal(sequence.value, 1);
    assert.equal(request.zone.length, 86);
    assert.equal(request.consumed, false);
    const extra = makeLocalMissionZone();
    assert.ok(throws(() => captureMissionFortZone(request, extra), Error));
    assert.equal(request.zone.length, 86);
    // 복제본은 패킷 저장소와 별개 버퍼이다(스택 패킷 수명이 끝나도 바이트가 남는다).
    assert.notEqual(request.zone.buffer, extra.storage.buffer);
});

test('이름은 첫 NUL 앞만 Fort 선택에 전달한다', () => {
    const next = new Uint32Array(36);
    const names = [];
    const hooks = { selectFort(chosen) { names.push(chosen); }, duplicate() { return 1; } };
    const globals = { next, sequence: { value: 0 }, localAddress: new Uint8Array(9), port: 1 };
    writeMissionFortRequest('Abc\0Def', globals, hooks);
    writeMissionFortRequest('\0Hidden', globals, hooks);
    writeMissionFortRequest('NoNul', globals, hooks);
    assert.deepEqual(names, ['Abc', '', 'NoNul']);
});

test('주소는 9BYTE 이상 Uint8Array, 블록은 36DWORD, 순번은 상자여야 하며 거부는 아무것도 쓰지 않는다', () => {
    const next = new Uint32Array(36).fill(0x11111111);
    const hooks = { selectFort() { throw new Error('선택되지 않아야 함'); }, duplicate() { return 1; } };
    const before = Array.from(next);
    assert.ok(throws(() => writeMissionFortRequest('A', { next, sequence: { value: 0 }, localAddress: new Uint8Array(8), port: 1 }, hooks), RangeError));
    assert.ok(throws(() => writeMissionFortRequest('A', { next, sequence: { value: 0 }, localAddress: [0, 0, 0, 0, 0, 0, 0, 0, 0], port: 1 }, hooks), RangeError));
    assert.ok(throws(() => writeMissionFortRequest('A', { next: new Uint32Array(35), sequence: { value: 0 }, localAddress: new Uint8Array(9), port: 1 }, hooks), TypeError));
    assert.ok(throws(() => writeMissionFortRequest('A', { next: new Uint32Array(37), sequence: { value: 0 }, localAddress: new Uint8Array(9), port: 1 }, hooks), TypeError));
    assert.ok(throws(() => writeMissionFortRequest('A', { next: new Uint16Array(36), sequence: { value: 0 }, localAddress: new Uint8Array(9), port: 1 }, hooks), TypeError));
    assert.ok(throws(() => writeMissionFortRequest('A', { next, sequence: 0, localAddress: new Uint8Array(9), port: 1 }, hooks), TypeError));
    assert.deepEqual(Array.from(next), before);
});

test('도달한 효과의 누락은 앞선 쓰기만 남기고 거부하며 순번을 늘리지 않는다', () => {
    const address = Uint8Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]);
    // 선택 효과가 없으면 아무것도 쓰지 않는다.
    let next = new Uint32Array(36).fill(0x22222222);
    let sequence = { value: 5 };
    assert.ok(throws(() => writeMissionFortRequest('A', { next, sequence, localAddress: address, port: 77 }, { duplicate() { return 1; } })));
    assert.deepEqual(Array.from(next), new Array(36).fill(0x22222222));
    assert.equal(sequence.value, 5);
    // 복제 효과가 없으면 선택 뒤 주소·상태·포트까지만 쓰고 핸들과 순번은 그대로이다.
    next = new Uint32Array(36).fill(0x33333333);
    sequence = { value: 5 };
    assert.ok(throws(() => writeMissionFortRequest('A', { next, sequence, localAddress: address, port: 77 }, { selectFort() {} })));
    assert.equal(next[0x54 / 4], 0x04030201);
    assert.equal(next[0x58 / 4], 0x08070605);
    assert.equal(next[0x5c / 4], 0x33333309);
    assert.equal(next[0], 7);
    assert.equal(next[0x7c / 4], 77);
    assert.equal(next[0x80 / 4], 0x33333333);
    assert.equal(sequence.value, 5);
});

test('순번은 32비트로 감기고 복제가 돌려준 핸들은 그대로 쓴다', () => {
    const next = new Uint32Array(36);
    const sequence = { value: 0xffffffff };
    writeMissionFortRequest('A', { next, sequence, localAddress: new Uint8Array(9), port: { value: 0x80000001 } }, { selectFort() {}, duplicate() { return 0xfedcba98; } });
    assert.equal(sequence.value, 0);
    assert.equal(next[0x7c / 4], 0x80000001);
    assert.equal(next[0x80 / 4], 0xfedcba98);
});

test('Zone 캡처: 길이는 15 + 첫 슬롯 수 × 71 이고 소비됨·중복·예약 크기·슬롯 수 범위는 거부한다', () => {
    // 정상: 슬롯 1개 → 86BYTE.
    const request = createMissionFortLoadRequest();
    const packet = makeLocalMissionZone();
    assert.equal(captureMissionFortZone(request, packet), 1);
    assert.equal(request.zone.length, 86);
    assert.equal(get(packet.storage, 2, 2), 86);
    // 복제 뒤에 원래 패킷 저장소를 바꿔도 요청이 소유한 이미지는 변하지 않는다.
    const snapshot = hex(request.zone);
    packet.storage.fill(0x5a);
    assert.equal(hex(request.zone), snapshot);
    // 슬롯 2·4개는 선언 길이 WORD 의 낮은 BYTE 가 128 이상(157)이거나 높은 BYTE 가 비영(299)인 경계이다.
    for (const [slots, length] of [[2, 157], [4, 299], [0, 15], [7, 512]]) {
        const sized = makeLocalMissionZone();
        sized.storage[14] = slots;
        const sizedRequest = createMissionFortLoadRequest();
        assert.equal(captureMissionFortZone(sizedRequest, sized), 1);
        assert.equal(get(sized.storage, 2, 2), length);
        assert.equal(sizedRequest.zone.length, length);
    }
    // 슬롯 7개는 정확히 512BYTE 로 허용되고 8개는 예약 밖이라 범위 오류이다(복제는 만들지 않는다).
    const seven = makeLocalMissionZone();
    seven.storage[14] = 7;
    const sevenRequest = createMissionFortLoadRequest();
    assert.equal(captureMissionFortZone(sevenRequest, seven), 1);
    assert.equal(sevenRequest.zone.length, 512);
    const eight = makeLocalMissionZone();
    eight.storage[14] = 8;
    const eightRequest = createMissionFortLoadRequest();
    assert.ok(throws(() => captureMissionFortZone(eightRequest, eight), RangeError));
    assert.equal(eightRequest.zone.length, 0);
    // 이미 소비된 요청과 512BYTE 가 아닌 패킷은 거부한다.
    const consumed = createMissionFortLoadRequest();
    consumed.consumed = true;
    assert.ok(throws(() => captureMissionFortZone(consumed, makeLocalMissionZone()), Error));
    assert.ok(throws(() => captureMissionFortZone(createMissionFortLoadRequest(), { storage: new Uint8Array(511), bytes() { return new Uint8Array(0); } }), Error));
    assert.ok(throws(() => captureMissionFortZone(createMissionFortLoadRequest(), { storage: new Uint8Array(513), bytes() { return new Uint8Array(0); } }), Error));
});

test('빈 요청 값은 144BYTE·순번 0·빈 이름/Zone·미소비·starting 없음이다', () => {
    const request = createMissionFortLoadRequest();
    assert.equal(request.next.length, 36);
    assert.ok(request.next instanceof Uint32Array);
    assert.equal(request.sequence, 0);
    assert.equal(request.fort, '');
    assert.equal(request.zone.length, 0);
    assert.equal(request.consumed, false);
    assert.equal(request.starting, null);
});

test('지연 로드 취소는 LoadMissionFort → LoadMissionWorld 순서이고 요청 참조를 놓는다', () => {
    const calls = [];
    const box = { value: createMissionFortLoadRequest() };
    releaseMissionFortLoad({ cancel(name) { calls.push(name); } }, box);
    assert.deepEqual(calls, ['LoadMissionFort', 'LoadMissionWorld']);
    assert.equal(box.value, null);
    // 이미 비어 있어도 취소 두 번은 그대로 실행한다.
    releaseMissionFortLoad({ cancel(name) { calls.push(name); } }, box);
    assert.equal(calls.length, 4);
    assert.ok(throws(() => releaseMissionFortLoad({}, { value: null }), TypeError));
});

test('Edit 전용 Guide Fort 요청은 옮기지 않았다', () => {
    assert.equal(requestModule.writeCurrentGuideFortRequest, undefined);
});

test('MissionFortSource 값: 이미지는 Uint8Array, 이름은 문자열, 읽기는 함수여야 한다', () => {
    const image = Uint8Array.of(1, 2, 3);
    const source = createMissionFortSourceImage(image, './data/d/Tutorial1.fort');
    assert.equal(source.image, image);
    assert.equal(source.fileName, './data/d/Tutorial1.fort');
    assert.ok(throws(() => createMissionFortSourceImage([1, 2], 'x'), TypeError));
    assert.ok(throws(() => createMissionFortSourceImage(image, 5), TypeError));
    let reads = 0;
    const selection = createMissionFortFileSelection('A.fort', () => { reads += 1; return image; });
    // 선택만으로는 읽지 않고 호출할 때만 같은 파일을 읽는다.
    assert.equal(reads, 0);
    assert.equal(selection.read(), image);
    assert.equal(reads, 1);
    assert.equal(selection.fileName, 'A.fort');
    assert.ok(throws(() => createMissionFortFileSelection(1, () => image), TypeError));
    assert.ok(throws(() => createMissionFortFileSelection('A', null), TypeError));
});
