// 대본 CreateAt 일반 입력(004d84f8..851b)의 이름표와 분배를 옮긴 모듈이다(cpppj `client/CreateAtInput.cpp/.h`, 10.78 전용).
// createAtScriptCode(name): 첫 NUL 앞 이름을 ASCII 대소문자 무시로 'createat' 와 비교해 명령 번호(0x020000e0, 00542a74)를 돌려주고 아니면 0xffffffff 이다(첫 NUL 뒤는 이름으로 해석하지 않음).
// dispatchCreateAtInput(event, editor, mission, create): 수정키를 지운 코드가 CreateAt 이 아니면 false, 키 뗌이거나 편집기도 미션도 아니면 효과 없이 true 이다(Console/정지/로컬 소유자 조건을 새로 붙이지 않음).
//   그 밖에는 두 번째 인자 y 의 전체 DWORD(문자열 토큰)를 실제 현재 런타임 생성 부모 create(token)에 그대로 넘기고 true 이다. 부모의 실패는 성공으로 바꾸지 않고 전파한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E33: CreateAt 일반 입력 이식으로 추가)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';

// 00542a74 의 실제 일반 명령 번호와 수정키 제거 마스크이다. (2026-11-08 WB04-E33)
export const CREATE_AT_COMMAND = 0x020000e0;
const KEY_MASK = 0x03ffffff;

// 원문 이름표의 ASCII 대소문자 비교이다. (2026-11-08 WB04-E33)
export function createAtScriptCode(name) {
    const nul = name.indexOf('\0');
    return asciiLower(nul < 0 ? name : name.slice(0, nul)) === 'createat' ? CREATE_AT_COMMAND : 0xffffffff;
}

// event = { code, x, y }, create(token) 는 현재 런타임 생성 부모이다. 소비하면 true 이다. (2026-11-08 WB04-E33)
export function dispatchCreateAtInput(event, editor, mission, create) {
    if (((event.code & KEY_MASK) >>> 0) !== CREATE_AT_COMMAND) return false;
    if ((event.code & InputCode.release) !== 0 || (!editor && !mission)) return true;
    if (typeof create !== 'function') throw new Error('CreateAt 의 실제 현재 Runtime 생성 부모 미연결');
    create(event.y >>> 0);
    return true;
}
