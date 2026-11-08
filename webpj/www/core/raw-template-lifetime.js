// Template9(캠페인/미션 템플릿 9칸)의 실제 소유/빌림 분기와 20바이트 헤더 초기화·반납 순서(004bd2e0/004bf690/004bfb00)를 옮긴 모듈이다(cpppj `o/RawTemplateLifetime.cpp/.h`, 10.78 전용).
// 헤더 20바이트 = [0] manager 포인터(불투명 핸들), [4] 빌림 DWORD, 나머지는 보존 필드이다. 큰 소멸/할당 반납 효과는 훅이며 소멸 뒤 헤더 포인터를 다시 읽지 않는다(처음 저장한 같은 manager 를 반납).
//  - initializeTemplateHeader(header): 이전 포인터를 반납하지 않고 정확한 20바이트를 0 으로 쓴다(다른 폭은 원본 밖 쓰기 대신 거부).
//  - releaseTemplateHeader(header, hooks): manager 포인터가 비영이고 빌림 DWORD 가 0 인(소유한) 헤더만 destroy → release 한 뒤, 포인터가 0 이어도 20바이트를 비운다.
//  - clearTemplateHeaders(headers, hooks): 9칸(180바이트)을 차례로 훑으며 비영 포인터인 칸만 반납한다. 포인터 0 인 칸의 stale 필드는 보존하고, 앞 칸 소멸 효과가 바꾼 뒤 칸의 현재 포인터/빌림 값을 순서대로 읽는다.
// hooks = { destroy(manager), release(manager) } 는 반납이 필요한 지점에서만 요구한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E123: RawTemplateLifetime 이식으로 추가)

// 원본 Template 한 기록과 전역 배열의 실제 폭이다. (2026-11-08 WB04-E123)
const HEADER_BYTES = 20;
const TEMPLATE_COUNT = 9;

// 비정렬 포인터/빌림 DWORD 를 같은 바이트에서 읽는다. (2026-11-08 WB04-E123)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 004bd2e0: 다른 폭은 원본 밖 쓰기 대신 명시적으로 거부한다. 사용: initializeTemplateHeader(header20). (2026-11-08 WB04-E123)
export function initializeTemplateHeader(header) {
    if (header.length !== HEADER_BYTES) throw new RangeError('Template 실제 20BYTE 헤더 누락');
    header.fill(0);
}

// 004bf690: 소멸이 헤더 포인터/빌림 값을 바꾸어도 처음 저장한 같은 manager 를 free 한다. 사용: releaseTemplateHeader(header20, { destroy, release }). (2026-11-08 WB04-E123)
export function releaseTemplateHeader(header, hooks) {
    if (header.length !== HEADER_BYTES) throw new RangeError('Template 실제 20BYTE 반납 헤더 누락');
    const manager = read(header, 0);
    if (manager && !read(header, 4)) {
        if (typeof hooks?.destroy !== 'function' || typeof hooks.release !== 'function') throw new Error('Template 실제 소멸/할당 반납 미연결');
        hooks.destroy(manager);
        hooks.release(manager);
    }
    initializeTemplateHeader(header);
}

// 004bfb00: 빈 포인터는 부가 DWORD 를 지우지 않는 전역 정리의 자연 반환이다. 사용: clearTemplateHeaders(headers180, hooks). (2026-11-08 WB04-E123)
export function clearTemplateHeaders(headers, hooks) {
    if (headers.length !== TEMPLATE_COUNT * HEADER_BYTES) throw new RangeError('Template 실제 9개 헤더 누락');
    // 앞 소멸 효과가 바꾼 뒤 칸의 현재 포인터와 빌림 값을 순서대로 읽는다.
    for (let slot = 0; slot < TEMPLATE_COUNT; slot += 1) {
        const header = headers.subarray(slot * HEADER_BYTES, (slot + 1) * HEADER_BYTES);
        if (read(header, 0)) releaseTemplateHeader(header, hooks);
    }
}
