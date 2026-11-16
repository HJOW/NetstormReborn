// 원본004bcea0의 내용물 직렬화 부모다(225553df, WB04-E629).
// 사용: saveFortContents(parent, streamToken, hooks). 각 Begin의 finder/current와 최초 개수 BYTE 주소를 보관한다.
// 명시 프레임으로 깊이 우선 순서/자식 복귀 뒤 부모 Next를 유지하며 순환은 손상으로 거부한다.

// 실제 선택된 용량/목록 플래그 진단이다. 진단 복귀 뒤 앞선 부분 쓰기를 되돌리지 않는다.
export const FORT_CONTENT_SAVE_ASSERTION = Object.freeze({ writeCapacity: 'WriteCapacity', listFlags: 'ListFlags' });
// 도달한 외부 장치만 그 위치에서 요구한다.
function need(device) { if (typeof device !== 'function') throw new Error('Fort 내용물 저장의 실제 경계 미연결'); return device; }
// 비정렬 원문 필드의 지정 폭만 읽으며 실제 물리 밖을 성공 값으로 채우지 않는다.
function read(bytes, offset, width = 4) {
    if (offset > bytes.length || width > bytes.length - offset) throw new RangeError('Fort 내용물 저장의 원문 필드 범위');
    let value = 0;
    // 낮은 BYTE부터 원문 레지스터의 같은 폭으로 조합한다.
    for (let byte = 0; byte < width; byte++) value |= bytes[offset + byte] << (byte * 8);
    return value >>> 0;
}
// 같은 stream의 지정 DWORD만 쓰고 인접 필드를 보존한다.
function put(bytes, offset, value) {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('Fort 내용물 저장의 stream 필드 범위');
    new DataView(bytes.buffer, bytes.byteOffset + offset, 4).setUint32(0, value >>> 0, true);
}
// 용량 진단 없이 pos/end를 먼저 진행하고 당시 data+offset의 개수 BYTE를0으로 쓴다.
function reserveCount(stream, hooks) {
    let count;
    need(hooks.access)(stream, (fields, memory) => {
        count = { data: read(fields, 0), offset: read(fields, 4) }; const next = (count.offset + 1) >>> 0;
        put(fields, 4, next); if ((read(fields, 8) | 0) < (next | 0)) put(fields, 8, next);
        const buffer = need(memory.buffer)(count.data); if (count.offset >= buffer.length) throw new RangeError('Fort 내용물 저장의 개수 BYTE 물리 범위'); buffer[count.offset] = 0;
    });
    return count;
}
// 현재 data가 바뀌어도 원래 개수 주소의 BYTE를 증가시킨다. 256번째 항목은0으로 감긴다.
function increment(count, stream, hooks) {
    need(hooks.access)(stream, (_fields, memory) => { const buffer = need(memory.buffer)(count.data); if (count.offset >= buffer.length) throw new RangeError('Fort 내용물 저장의 해제된 개수 위치'); buffer[count.offset]++; });
}
// 캡처한 값만 BYTE/WORD로 쓴다. 진단 뒤 data/pos와 출력 뒤 pos/end는 각각 현재 칸을 다시 읽는다.
function append(value, width, stream, hooks) {
    need(hooks.access)(stream, (fields, memory) => {
        if ((read(fields, 12) | 0) < ((read(fields, 4) + width) | 0)) need(hooks.assertion)(FORT_CONTENT_SAVE_ASSERTION.writeCapacity);
        const buffer = need(memory.buffer)(read(fields, 0)); const position = read(fields, 4);
        if (position > buffer.length || width > buffer.length - position) throw new RangeError('Fort 내용물 저장의 실제 출력 범위');
        // 인라인 원문 폭만 낮은 BYTE부터 기록한다.
        for (let byte = 0; byte < width; byte++) buffer[position + byte] = value >>> (byte * 8);
        const next = (read(fields, 4) + width) >>> 0; put(fields, 4, next); if ((read(fields, 8) | 0) < (next | 0)) put(fields, 8, next);
    });
}
// 실제 개수/robust/현재 종류·QA/목록 플래그/중첩 순서로 같은 stream에 쓴다.
export function saveFortContents(parent, stream, hooks) {
    const frames = []; const active = new Set();
    // 진입 당시 부모와 개수 주소를 고정하고 Begin마다 독립 cursor 수명을 보관한다.
    const enter = (selected) => {
        if (active.has(selected)) throw new Error('Fort 내용물 저장의 순환 부모'); active.add(selected);
        const count = reserveCount(stream, hooks); const cursor = need(hooks.begin)(selected); frames.push({ parent: selected, count, cursor });
    };
    enter(parent);
    // 각 자식 목록이 끝난 뒤 부모의 보관한 Next를 한 번만 호출한다.
    while (frames.length) {
        const frame = frames.at(-1); const candidate = need(frame.cursor.current)();
        if (!candidate || candidate >= need(hooks.poolLimit)()) { active.delete(frame.parent); frames.pop(); if (frames.length) need(frames.at(-1).cursor.next)(); continue; }
        if (need(hooks.robust)(candidate)) {
            increment(frame.count, stream, hooks); const kind = read(need(hooks.form)(need(frame.cursor.current)()), 0x12, 2); const flags = need(hooks.type)(kind).flags1 >>> 0;
            append(kind, 1, stream, hooks);
            if (flags & 0x1000) append(read(need(hooks.form)(need(frame.cursor.current)()), 0xc, 1), 1, stream, hooks);
            if (flags & 0x2000) append(read(need(hooks.form)(need(frame.cursor.current)()), 0xc, 2), 2, stream, hooks);
            let required = need(hooks.type)(kind).contentListFlags >>> 0; if (!required) required = need(hooks.parentType)(frame.parent).containerListFlags >>> 0;
            if (!required) append(read(need(hooks.form)(need(frame.cursor.current)()), 0x14, 1), 1, stream, hooks);
            else if (((read(need(hooks.form)(need(frame.cursor.current)()), 0x14, 2) & required) >>> 0) !== required) need(hooks.assertion)(FORT_CONTENT_SAVE_ASSERTION.listFlags);
            if (flags & 0x20000) { const nested = need(frame.cursor.current)(); enter(nested); continue; }
        }
        need(frame.cursor.next)();
    }
}
