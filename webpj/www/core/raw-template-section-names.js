// 원문 004bccb0 의 Client 전역 005c8aa4 lazy 이름표 생성만 옮긴 모듈이다(cpppj `o/RawTemplateSectionNames.cpp/.h`, 10.78 전용).
// 현재 표 토큰(current)이 비0 이면 하위 장치 없이 그대로 반환한다. 비0 이 아니면: 512 BYTE 새 할당(allocate) → 현재 전역에 토큰 공개 → 그 토큰의 물리(buffer)를 얻고, 토큰이 0 이면 원문 004bcced 의 NULL 복사로 RangeError.
//   기본 135 문자('/Subscriber/State/Mission/CoreData/Chaff/Badges/Territory/TypeNames/CompressedData/Technology/Money/Deck/Reserved2/Reserved3/Reserved4/') + NUL 을 앞에서부터 쓰고,
//   index 0..19 마다 서식(format(index), 'Terr%02d' + NUL)을 현재 NUL 뒤에 붙이고 '/'+NUL 을 붙이며(매번 현재 전역 표를 다시 조회), 마지막에 '/' 한 번 더 붙인다(총 277 BYTE). 표 끝의 미정 235 BYTE 는 채우지 않는다.
// 물리 쓰기가 부족하면 RangeError 로 거부하며 이미 쓴 앞 BYTE 는 보존한다. 반환 토큰은 Client 전역과 같은 현재 값이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E19: Template 구역 이름표 이식으로 추가)

// 실제 operator new 크기와 정상 신규표에서 실제로 쓰는 BYTE 수이다. (2026-11-08 WB04-E19)
export const TEMPLATE_SECTION_NAMES_CAPACITY = 512;
export const TEMPLATE_SECTION_NAMES_WRITTEN_BYTES = 277;

// 원문 기본 135 문자 문자열이다. (2026-11-08 WB04-E19)
const BASE_NAMES = '/Subscriber/State/Mission/CoreData/Chaff/Badges/Territory/TypeNames/CompressedData/Technology/Money/Deck/Reserved2/Reserved3/Reserved4/';

// 기존표 반환에는 하위를 요구하지 않고 도달한 장치만 요구한다. (2026-11-08 WB04-E19)
function need(device) {
    if (typeof device !== 'function') throw new Error('Template 이름표의 선택된 실제 하위 미연결');
    return device;
}

// 이미 쓴 앞 BYTE 는 물리 실패 때도 보존한다. (2026-11-08 WB04-E19)
function write(bytes, at, value) {
    if (at >= bytes.length) throw new RangeError('Template 이름표 실제 쓰기 범위');
    bytes[at] = value;
}

// 같은 현재 표의 최초 NUL 위치이다. (2026-11-08 WB04-E19)
function end(bytes) {
    const at = bytes.indexOf(0);
    if (at < 0) throw new RangeError('Template 이름표 현재 NUL 누락');
    return at;
}

// 처음 읽은 NUL 포함 길이만 현재 끝에 붙인다. (2026-11-08 WB04-E19)
function append(bytes, text) {
    const length = end(text) + 1;
    const at = end(bytes);
    for (let index = 0; index < length; index += 1) write(bytes, at + index, text[index]);
}

// globals = { current }(Client 전역 005c8aa4 의 토큰), hooks = { allocate(512) → 토큰/0, buffer(토큰) → 같은 물리 뷰, format(index) → 'Terr%02d'+NUL 바이트 }. (2026-11-08 WB04-E19)
export function getTemplateSectionNames(globals, hooks) {
    if (globals.current) return globals.current;
    globals.current = need(hooks.allocate)(TEMPLATE_SECTION_NAMES_CAPACITY) >>> 0;
    const bytes = need(hooks.buffer)(globals.current);
    if (!globals.current) throw new RangeError('원문 004bcced 의 NULL 이름표 복사');
    // 원문 0x22 DWORD 의 136 BYTE 를 같은 순서로 복사한다. 할당 나머지는 읽거나 쓰지 않는다.
    for (let at = 0; at < BASE_NAMES.length; at += 1) write(bytes, at, BASE_NAMES.charCodeAt(at));
    write(bytes, BASE_NAMES.length, 0);
    const slash = Uint8Array.of(0x2f, 0);
    // 각 Terr 서식은 ASCII 숫자 두 자리이며 실제 표의 현재 NUL 뒤로 붙인다.
    for (let index = 0; index < 20; index += 1) {
        const name = need(hooks.format)(index);
        append(need(hooks.buffer)(globals.current), name);
        append(need(hooks.buffer)(globals.current), slash);
    }
    append(need(hooks.buffer)(globals.current), slash);
    return globals.current;
}
