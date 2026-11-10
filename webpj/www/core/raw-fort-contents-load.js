// 원본 004bd130 의 원시 stream 내용물(Contents) 읽기를 옮긴 모듈이다(cpppj `o/RawFortContentsLoad.cpp/.h`, 기준선 1f00a25a 와 같음).
// 읽으면서 즉시 만들고(Create) 필요하면 그 자리에서 중첩 stream 을 진행한다. 별도의 parsed 목록은 만들지 않으며 callback 뒤의 현재 변환표/TYPE/stream 을 반영한다. 앞선 생성·pos 를 되돌리지 않는다.
// 한 항목: 개수 BYTE → (현재 변환표로 바꾼 종류 BYTE, 0 이면 반환 false) → 첫 TYPE flags1 의 QA(BYTE)/QB(signed WORD) 수량 → 두 번째 TYPE 조회의 contentListFlags(0 이면 부모 그릇 flags, 그것도 0 이면 BYTE) →
//   listFlags 에서 0x100 을 지우고 0x10 이 있으면 attach flags 의 8 을 지우고 0x10 을 켠다 → create(parent, kind, quantity, listFlags, attachFlags, 0) → 첫 TYPE flags1 의 container(0x20000)이면 만든 자식을 부모로 재귀.
// 반환 false 는 빈 data/end · 변환 0 · 중첩 false 의 원문 반환이다. 장치 누락/물리 범위 초과는 앞선 효과를 남긴 예외이다. loadingDepth 는 쓰지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// 클론의 분류가 `exclude-edit` 이었으나 일반 Objects 적재(RawFortObjectStream 의 contents 훅, cpppj RawCampaignSession.cpp 3201)도 이 읽기를 쓰므로 `port` 로 바꿔 옮겼다.
// (2026-11-10 WB04-E275: 4-B 내용물 원시 읽기 이식)

// 원본이 진단 뒤 계속하는 두 위치의 번호이다. (2026-11-10 WB04-E275)
export const FORT_CONTENTS_ASSERTION = Object.freeze({ missingTypeConversion: 0x79, wordPastEnd: 0x2e });

// 첫 TYPE flags1 에서 캡처하는 세 원본 비트이다: QA(BYTE 수량), QB(WORD 수량), container(자식 재귀). (2026-11-10 WB04-E275)
const QA = 0x1000;
const QB = 0x2000;
const CONTAINER = 0x20000;

// 도달한 누락 장치만 거부한다. 빈 목록은 선택 TYPE/Create 를 요구하지 않는다. (2026-11-10 WB04-E275)
function need(device) {
    if (typeof device !== 'function') throw new Error('Fort 원시Contents의 실제 경계 미연결');
    return device;
}

// 현재 16 바이트의 DWORD 를 비정렬 리틀 엔디언으로 읽는다. 물리 범위를 넘으면 거부한다. (2026-11-10 WB04-E275)
function dword(bytes, at) {
    if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Fort Contents 현재stream DWORD 물리범위');
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// pos DWORD 만 쓰고 data/end/capacity 는 보존한다. (2026-11-10 WB04-E275)
function setPosition(fields, position) {
    if (fields.length < 8) throw new RangeError('Fort Contents 현재pos 물리범위');
    for (let byte = 0; byte < 4; byte += 1) fields[4 + byte] = (position >>> (byte * 8)) & 255;
}

// 직접 BYTE 는 논리 end 를 검사하지 않고 실제 버퍼의 물리 끝만 거부한다. (2026-11-10 WB04-E275)
function byteAt(bytes, position) {
    if (position >= bytes.length) throw new RangeError('Fort Contents 직접BYTE 물리범위');
    return bytes[position];
}

// 현재 data/pos 에서 하나 읽은 뒤 같은 현재 stream 의 pos 를 진행한다. (2026-11-10 WB04-E275)
function readByte(fields, memory) {
    const position = dword(fields, 4);
    const data = dword(fields, 0);
    const bytes = need(memory.buffer)(data);
    const value = byteAt(bytes, position);
    setPosition(fields, (position + 1) >>> 0);
    return value;
}

// QB: 원시 WORD 를 먼저 읽고 pos+2 를 쓴 뒤 signed pos>end 이면 진단한다. 진단이 data/pos 를 바꿔도 읽은 값은 유지하며 부호 확장한 DWORD 를 돌려준다. (2026-11-10 WB04-E275)
function readWord(fields, memory, hooks) {
    const position = dword(fields, 4);
    const data = dword(fields, 0);
    const bytes = need(memory.buffer)(data);
    if (position > bytes.length || bytes.length - position < 2) throw new RangeError('Fort Contents QB WORD 물리범위');
    const word = bytes[position] | (bytes[position + 1] << 8);
    const end = dword(fields, 8);
    const next = (position + 2) >>> 0;
    setPosition(fields, next);
    if ((next | 0) > (end | 0)) need(hooks.assertion)(FORT_CONTENTS_ASSERTION.wordPastEnd);
    return ((word << 16) >> 16) >>> 0;
}

// 004bd130: 내용물 읽기. parent 는 부모 SID(핸들), stream 은 반환 stream 토큰, flags 는 attach flags, relaxedConversion 이 0 이면 checking 이 켜진 때 변환표의 0 을 진단한다.
// globals = { conversion: 현재 255 바이트 변환표 토큰(읽을 때마다 현재 값 — getter 허용), checking() },
// hooks = { access(token, 작업(fields, memory)), conversionBuffer(token), type(kind) → { flags1, contentListFlags }, parentListFlags(부모), create(parent, kind, quantity, listFlags, attachFlags, quality) → 자식, assertion(번호) }.
// 동기 access 한 번 안에서 모든 원문 재귀를 실행하고 밖에 fields/buffer 를 저장하지 않는다. (2026-11-10 WB04-E275)
export function loadFortContents(parent, stream, flags, relaxedConversion, globals, hooks) {
    let result = false;
    need(hooks.access)(stream, (fields, memory) => {
        // 원본 자식 스택과 같은 독립 프레임이다: 각 재귀 호출의 부모·수정된 attach flags·남은 개수.
        const frames = [];
        // 각 재귀 진입도 현재 data/end 관문을 다시 읽는다.
        const enter = (selected, attachFlags) => {
            if (!dword(fields, 0) || !dword(fields, 8)) return false;
            const count = readByte(fields, memory);
            frames.push({ parent: selected, flags: attachFlags >>> 0, remaining: count });
            return true;
        };
        if (!enter(parent, flags)) return;
        // 한 항목을 읽고 즉시 Create 한 뒤 필요하면 그 자리에서 자식 프레임을 진행한다.
        while (frames.length > 0) {
            const frame = frames[frames.length - 1];
            if (!frame.remaining) {
                frames.pop();
                continue;
            }
            frame.remaining -= 1;
            // 현재 파일 BYTE 를 미리 parsed 타입으로 치환하지 않는다.
            let kind = readByte(fields, memory);
            if (kind < 0xf6) {
                let conversion = globals.conversion >>> 0;
                if (conversion) {
                    if (!relaxedConversion && need(globals.checking)() && !byteAt(need(hooks.conversionBuffer)(conversion), kind)) {
                        need(hooks.assertion)(FORT_CONTENTS_ASSERTION.missingTypeConversion);
                        conversion = globals.conversion >>> 0;
                    }
                    kind = byteAt(need(hooks.conversionBuffer)(conversion), kind);
                }
            }
            if (!kind) return;
            // 첫 TYPE 플래그는 뒤 하위가 TYPE 를 바꾸어도 QA/QB/container 판정에 유지한다.
            const savedFlags = need(hooks.type)(kind).flags1 >>> 0;
            let quantity = 0;
            if (savedFlags & QA) quantity = readByte(fields, memory);
            if (savedFlags & QB) quantity = readWord(fields, memory, hooks);
            // 두 번째 TYPE 조회는 첫 조회와 구별한다.
            let listFlags = need(hooks.type)(kind).contentListFlags >>> 0;
            if (!listFlags) {
                listFlags = need(hooks.parentListFlags)(frame.parent) >>> 0;
                if (!listFlags) listFlags = readByte(fields, memory);
            }
            listFlags = (listFlags & ~0x100) >>> 0;
            if (listFlags & 0x10) frame.flags = ((frame.flags & ~8) | 0x10) >>> 0;
            // 원문이 실제로 Create 한 뒤에만 저장된 container 비트를 본다.
            const child = need(hooks.create)(frame.parent, kind, quantity, listFlags, frame.flags, 0);
            if ((savedFlags & CONTAINER) && !enter(child, frame.flags)) return;
        }
        result = true;
    });
    return result;
}
