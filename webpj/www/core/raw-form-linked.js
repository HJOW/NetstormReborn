// 원본 10.78 Form::isRobust(004ae080)의 부모 체인 전체 검사와 ProcessForm 의 실제 부모 getter 를 옮긴 모듈이다.
//  - checkRawFormChain: 부모 +6 머리에서 +4 next 를 끝까지 따라가며 자기 번호가 있는지 본다. 자기를 찾은 뒤에도 끝까지 범위를 검사하고(논리 capacity 이상이면 false),
//    TYPE/상태 필터나 첫 발견의 조기 성공을 추가하지 않는다. 원문이 끝나지 않는 순환은 웹에서 명시 오류로 멈춘다.
//  - checkRawFormLinked: 현재 가상 부모 getter 를 한 번 부른 뒤 그 시점의 현재 풀로 검사한다(getter 가 풀을 다시 확보해도 새 저장소를 읽는다).
//  - readRawProcessFormParent: ProcessForm VT(0x512098)의 +0 getter 는 상태/TYPE 검사 없이 +16 WORD 만 돌려준다. 다른 VT 는 거부한다.
// 근거: cpppj `o/RawFormLinked.cpp/.h`(72012e83), 원본 관측 `cpppj/tests/fixtures/form-linked-parent-x86.tsv`(120행, tests/form-hosts.test.mjs).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-06 WB04-D5a: Content 호스트의 Linked 소비자와 함께 추가)
import { SQUID_FORM_VTABLES } from './squid-vtables.js';

// 슬롯 WORD 를 읽는다(정렬 무관). (2026-11-06 WB04-D5a)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// ProcessForm 의 실제 부모 getter 이다(+16 WORD 전체, 상태/TYPE 무관). 미복원 VT 는 임의 부모로 대신하지 않고 거부한다. (2026-11-06 WB04-D5a)
export function readRawProcessFormParent(pool, form) {
    const raw = pool.slot(form);
    const table = (word(raw, 0) | (word(raw, 2) << 16)) >>> 0;
    if (table !== SQUID_FORM_VTABLES.process) throw new Error('Form 진단의 미복원 부모 가상 getter');
    return word(raw, 16);
}

// 명시 물리 바이트(bytes)·슬롯 폭(stride)·논리 범위(capacity)에서 parent 체인에 form 이 있는지 검사한다. 바이트는 읽기만 한다. (2026-11-06 WB04-D5a)
export function checkRawFormChain(bytes, stride, capacity, form, parent) {
    const slots = Math.floor(bytes.length / stride);
    if (stride < 8 || capacity > 65535 || capacity > slots) throw new RangeError('Form 연결 검사의 실제 풀 물리 범위');
    // 부모/체인 슬롯의 물리 범위만 확인한다.
    const slot = (sid) => {
        if (sid >= slots) throw new RangeError('Form 연결 검사의 부모 물리 범위');
        return bytes.subarray(sid * stride, (sid + 1) * stride);
    };
    let current = word(slot(parent), 6);
    let found = false;
    const visited = new Set();
    // 자기를 찾아도 나머지 체인의 범위/끝을 검사한다.
    while (current) {
        if (current >= capacity) return false;
        if (visited.has(current)) throw new Error('Form 연결 검사의 원문 순환 호스트 중단');
        visited.add(current);
        if (current === form) found = true;
        current = word(slot(current), 4);
    }
    return found;
}

// 현재 가상 부모 getter(parent(form) → SID)를 한 번 읽고 그 뒤의 현재 풀 바이트로 전체 체인을 검사한다. (2026-11-06 WB04-D5a)
export function checkRawFormLinked(pool, form, parent) {
    if (typeof parent !== 'function') throw new TypeError('Form 연결 검사의 현재 부모 가상 장치 미연결');
    const selected = parent(form);
    return checkRawFormChain(pool.bytes(), pool.layout.stride, pool.capacity, form, selected);
}
