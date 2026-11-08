// 원문 004ad500 의 표시 부모(Squid 표시 요청)를 옮긴 모듈이다(cpppj `o/RawSquidDisplayParent.cpp/.h` 의 RunRawSquidDisplayParent, 10.78 전용).
// 순서: 진입 때 raw 프레임(+36)을 한 번 읽어 고정한다(진단이 raw 프레임을 바꿔도 지역값은 유지). 현재 checking 이 비0 이면 현재 TYPE 뷰를 조회해 프레임 범위를 검사하고,
//   범위 밖이면 현재 checking 을 다시 읽어 비0 일 때 오류 상세(frameError)를 기록한 뒤, 범위 밖이거나 SHP 가 없으면 원문 846 진단(frameCheck)을 요청한다. 진단이 복귀하면 새 TYPE 뷰를 조회해
//   범위 안이고 SHP 가 있으면 mark(TYPE 번호, 고정 프레임, 현재 x·y 비트, 선택 비트<<4)를 호출하고, 범위 밖이고 checking 이 비0 이면 오류 상세만 기록한다.
// TYPE 뷰({token, flags1, frameCount, loaded, header})는 호출 순간의 현재 필드를 읽는 속성으로 받는다(같은 레코드를 빌리는 getter 도 허용). 선택하지 않은 장치는 요구하지 않고 필요한 장치가 없으면 빈 성공 대신 오류를 낸다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E58: 객체 표시 부모 이식으로 추가)

// 슬롯 길이, 프레임·좌표 필드 위치, 두 배 프레임 TYPE 비트, SHP 해제 표식, 원문 진단 식/파일/줄이다. (2026-11-08 WB04-E58)
const SLOT_BYTES = 50;
const TYPE_OFFSET = 10;
const FRAME_OFFSET = 36;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const EXTRA_OFFSET = 40;
const DOUBLE_FRAME_FLAGS = 0x440000;
const FREED_SHAPE_A = -0x22222223;
const FREED_SHAPE_B = -0x32323233;
export const SQUID_DISPLAY_ASSERTION = Object.freeze({
    expression: 'getTypeStruct()->frameCheck( isAbstract(), theFrame, true )',
    file: '\\Ns\\O\\Squid.cpp',
    line: 0x846
});

// 선택된 실제 장치가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E58)
function need(device) {
    if (typeof device !== 'function') throw new Error('Squid 표시 부모의 선택된 실제 장치 미연결');
    return device;
}

// raw 의 정확한 4바이트를 리틀엔디언 부호 없는 DWORD 로 읽는다(비정렬 허용). (2026-11-08 WB04-E58)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 프레임이 호출 순간 TYPE 프레임 수 안인지 본다. 두 배 비트가 있으면 개수를 2배로 하되 하위 32비트로 감은 뒤 부호 있는 수로 비교한다. (2026-11-08 WB04-E58)
function frameInRange(type, frame) {
    const count = Math.imul(type.frameCount | 0, (type.flags1 & DOUBLE_FRAME_FLAGS) ? 2 : 1) | 0;
    return frame >= 0 && frame < count;
}

// null SHP 와 두 해제 표식은 프레임 범위와 별개로 검사한다. (2026-11-08 WB04-E58)
function shapePresent(type) {
    const header = type.header | 0;
    return Boolean(type.loaded) && header !== FREED_SHAPE_A && header !== FREED_SHAPE_B;
}

// TYPE 번호 조회 전에 abstract 비트를 고정한 뒤 실제 세 인자 오류 상세 writer 를 호출한다. 두 장치 사이에 checking 을 다시 검사하지 않는다. (2026-11-08 WB04-E58)
function report(raw, type, frame, hooks) {
    const abstract = raw[EXTRA_OFFSET] & 1;
    const number = need(hooks.typeNumber)(type);
    need(hooks.frameError)(number, frame, abstract);
}

// raw 는 물리 50바이트 슬롯(Uint8Array), checking() 은 현재 checking DWORD getter, hooks = { type(번호) → TYPE 뷰, typeNumber(뷰) → 번호,
//   frameError(번호, 프레임, abstract), assertion(식, 파일, 줄), mark(번호, 프레임, x비트, y비트, 선택표식) }. 호출 동안 raw 와 반환 TYPE 필드의 수명은 유지되어야 한다. (2026-11-08 WB04-E58)
export function runRawSquidDisplayParent(raw, checking, hooks) {
    if (!(raw instanceof Uint8Array) || raw.length !== SLOT_BYTES) throw new Error('Squid 표시 부모의 물리50BYTE 슬롯 필요');
    if (typeof checking !== 'function') throw new TypeError('Squid 표시 부모의 checking 공급자는 함수여야 합니다.');
    // 소비 위치마다 현재 checking 의 모든 비트를 다시 읽는 비0 관문이다(불리언 getter 도 허용).
    const active = () => (Number(checking()) >>> 0) !== 0;
    const frame = readDword(raw, FRAME_OFFSET) | 0;
    if (active()) {
        const type = need(hooks.type)(raw[TYPE_OFFSET]);
        const range = frameInRange(type, frame);
        if (!range && active()) report(raw, type, frame, hooks);
        if (!range || !shapePresent(type)) {
            const { expression, file, line } = SQUID_DISPLAY_ASSERTION;
            need(hooks.assertion)(expression, file, line);
        }
    }
    const type = need(hooks.type)(raw[TYPE_OFFSET]);
    if (frameInRange(type, frame)) {
        if (shapePresent(type)) need(hooks.mark)(raw[TYPE_OFFSET], frame, readDword(raw, X_OFFSET), readDword(raw, Y_OFFSET), ((raw[EXTRA_OFFSET] >> 2) & 1) << 4);
    } else if (active()) report(raw, type, frame, hooks);
}
