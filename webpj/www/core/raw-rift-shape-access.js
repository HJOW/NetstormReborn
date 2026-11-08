// 원문 00419850 의 SHP 프레임 접근을 옮긴 모듈이다(cpppj `o/RawRiftShapeAccess.cpp/.h` 의 ReadRiftShapeFrame, 10.78 전용).
// 순서: 프레임이 TYPE 프레임 수(두 배 비트가 있으면 2배, 하위 32비트로 감은 부호 있는 비교) 밖이면 선택된 TYPE 번호 getter 와 오류 상세 writer(00436d40)를 호출하고,
//   범위 밖이거나 SHP 가 없거나 첫 DWORD 가 두 해제 표식이면 원문 진단 `frameCheck( abstract, frame, false )`(..\o\rifttype.h:0xe1, checking 과 무관)를 요청한다.
//   진단이 복귀해도 같은 프레임으로 계속해 선택된 TYPE 의 현재 SHP 에서 저장 프레임의 칸 단위 크기 둘을 읽는다(범위 밖 물리 접근은 그 소비 지점의 호스트 오류).
// TYPE 뷰({token, flags1, frameCount, loaded, header})는 읽는 순간의 현재 필드를 가리킨다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E59: Squid 표시 이식으로 추가)

// 두 배 프레임 TYPE 비트, SHP 해제·미초기화 표식, 원문 진단 식/파일/줄이다. (2026-11-08 WB04-E59)
const DOUBLE_FRAMES = 0x440000;
const SHAPE_FREED = -0x22222223;
const SHAPE_UNINITIALIZED = -0x32323233;
export const RIFT_SHAPE_ASSERTION = Object.freeze({
    expression: 'frameCheck( abstract, frame, false )',
    file: '..\\o\\rifttype.h',
    line: 0xe1
});

// 선택된 실제 장치가 없으면 그 위치에서 거부한다(도달하지 않은 장치는 요구하지 않는다). (2026-11-08 WB04-E59)
function need(device) {
    if (typeof device !== 'function') throw new Error('SHP접근의선택된실제장치미연결');
    return device;
}

// type 은 TYPE 뷰, frame 은 부호 있는 저장 프레임, abstract 는 caller 가 고정한 인자, hooks = { typeNumber(뷰), frameError(번호, 프레임, abstract),
//   assertion(식, 파일, 줄), shape(선택 TYPE 토큰, 저장 프레임) → [폭, 높이] } 이다. 반환은 칸 단위 크기 [폭, 높이]. (2026-11-08 WB04-E59)
export function readRiftShapeFrame(type, frame, abstract, hooks) {
    const count = Math.imul(type.frameCount | 0, (type.flags1 & DOUBLE_FRAMES) ? 2 : 1) | 0;
    const range = frame >= 0 && frame < count;
    if (!range) {
        const number = need(hooks.typeNumber)(type);
        need(hooks.frameError)(number, frame, abstract);
    }
    const header = type.header | 0;
    if (!range || !type.loaded || header === SHAPE_FREED || header === SHAPE_UNINITIALIZED) {
        const { expression, file, line } = RIFT_SHAPE_ASSERTION;
        need(hooks.assertion)(expression, file, line);
    }
    return need(hooks.shape)(type.token, frame);
}
