// 의식 보상 선택 일반 입력 세 개(UpgradeAltar·GetNewTech·RequestStormpower)의 요청 작성과 송신(원문 004d82ed/004d8351/004d83b7)을 정적으로 옮긴 모듈이다(cpppj `client/RitualChoiceInput.cpp/.h`, 10.78 전용). 즉시 기술 지급과 분리해 원문 6b 요청으로 전송한다.
// constructRitualChoicePacket(packet): 0040af50 의 최초 헤더 4바이트만 쓴다: [0]=1(버전), [1]=0x6b(메시지), [2..3]=길이 10. 10바이트 미만이면 RangeError. 본문/뒤쪽 바이트는 보존한다.
// dispatchRitualChoiceInput(event, hooks): 수정키를 뗀 코드(상위 입력 표시 제외, 임의 low BYTE 키는 명령이 아님)가 세 명령이 아니면 false, 뗌은 장치 조회 없이 소비한다. 누름은 10바이트 패킷을 먼저 construct 로 작성한 뒤(대상 진단보다 앞) 분기한다:
//   UpgradeAltar(0x020000d4)는 대상 = y 하위 WORD, 종류 = daisType(). GetNewTech(0x020000d5)/RequestStormpower(0x020000f9)는 현재 제단(dais())이 0 이면 assertion(0x82f/0x838) 뒤 다시 읽고 계속하며,
//   대상 = dais() 하위 WORD, 종류 = GetNewTech 는 y 전체 DWORD, RequestStormpower 는 nuggetType(). 패킷 +4 에 대상 WORD, +6 에 종류 DWORD 를 쓰고 send(9, 패킷)이다.
// 이 경로는 원문 관측 fixture 가 없는 정적 복원이다(cpppj 도 테스트/통신 실행을 추가하지 않음). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E35: 의식 선택 입력 정적 복원으로 추가)
import { InputCode } from './input-queue.js';

// 명령 초기값(00542a44/48/00542ad8)과 수정키 제거 마스크, 대상 진단 줄(GetNewTech/RequestStormpower)이다. (2026-11-08 WB04-E35)
export const UPGRADE_ALTAR_COMMAND = 0x020000d4;
export const GET_NEW_TECH_COMMAND = 0x020000d5;
export const REQUEST_STORMPOWER_COMMAND = 0x020000f9;
const KEY_MASK = 0x03ffffff;
const GET_NEW_TECH_ASSERTION = 0x82f;
const STORMPOWER_ASSERTION = 0x838;
const PACKET_BYTES = 10;

// 미선택 장치에 성공값을 공급하지 않는다. (2026-11-08 WB04-E35)
function required(hook) {
    if (typeof hook !== 'function') throw new Error('의식 선택 입력의 실제 장치 미연결');
    return hook;
}

// 낮은 바이트부터 지정 폭만 쓴다(SID 는 WORD, 기술은 DWORD). (2026-11-08 WB04-E35)
function put(bytes, at, value, count) {
    for (let index = 0; index < count; index += 1) bytes[at + index] = (value >>> (8 * index)) & 0xff;
}

// 임의 low BYTE 키를 명령으로 취급하지 않는다. (2026-11-08 WB04-E35)
export function isRitualChoiceCommand(code) {
    return code === GET_NEW_TECH_COMMAND || code === REQUEST_STORMPOWER_COMMAND || code === UPGRADE_ALTAR_COMMAND;
}

// PE 0052ee64=6b / 65=1 / 68=WORD 10 이며 BYTE 0 은 버전이다. (2026-11-08 WB04-E35)
export function constructRitualChoicePacket(packet) {
    if (packet.length < PACKET_BYTES) throw new RangeError('의식 선택 요청 10BYTE 부족');
    packet[0] = 0;
    packet[1] = 0;
    put(packet, 2, 0, 2);
    packet[1] = 0x6b;
    put(packet, 2, PACKET_BYTES, 2);
    packet[0] = 1;
}

// hooks = { dais(), nuggetType(), daisType(), assertion(줄), construct(패킷), send(대상, 패킷) } 이다. 소비하면 true 이다. (2026-11-08 WB04-E35)
export function dispatchRitualChoiceInput(event, hooks) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (!isRitualChoiceCommand(code)) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    const packet = new Uint8Array(PACKET_BYTES);
    // 생성자는 대상 진단보다 먼저 실행한다.
    required(hooks.construct)(packet);
    let dais;
    let kind;
    if (code === UPGRADE_ALTAR_COMMAND) {
        dais = (event.y >>> 0) & 0xffff;
        kind = required(hooks.daisType)() >>> 0;
    } else {
        if (!required(hooks.dais)()) required(hooks.assertion)(code === GET_NEW_TECH_COMMAND ? GET_NEW_TECH_ASSERTION : STORMPOWER_ASSERTION);
        dais = required(hooks.dais)() & 0xffff;
        kind = code === GET_NEW_TECH_COMMAND ? event.y >>> 0 : required(hooks.nuggetType)() >>> 0;
    }
    put(packet, 4, dais, 2);
    put(packet, 6, kind, 4);
    required(hooks.send)(9, packet);
    return true;
}
