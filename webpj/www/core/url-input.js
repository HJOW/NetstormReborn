// 원문 URL 명령(4d7b09..4d7b1e)의 현재 문자열을 기존 4a00c0 링크 분배기에 전달하는 일반 입력 모듈이다(cpppj `client/UrlInput.cpp/.h`, 10.78 전용).
// Guide/Gump 뒤 명령만 받는다. 수정키를 제거한(0x03ffffff) 코드가 URL 명령 번호(kUrlCommand = 0x0200005b, 00542e60 URL → 00542864 전역의 초기값)가 아니면 소비하지 않는다(false).
// 키 뗌은 소비하되 효과가 없다(true). 그 밖에는 두 번째 DWORD(event.y 의 32비트 전체 비트)를 문자열 주소 토큰으로 해석해 text(토큰)로 현재 문자열을 얻고 activate(문자열)로 링크를 활성화한다(true).
// 모드(Editor/missionPost)는 조회하지 않는다. 링크 정책을 새로 만들지 않으며 문자열/분배기 장치가 없으면 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E20: URL 일반 입력 이식으로 추가)
import { InputCode } from './input-queue.js';

// URL 명령 번호와 수정키 제거 마스크이다. (2026-11-08 WB04-E20)
export const URL_COMMAND = 0x0200005b;
const KEY_MASK = 0x03ffffff;

// event = { code, x, y }, text(token) → 문자열, activate(문자열). (2026-11-08 WB04-E20)
export function dispatchUrlInput(event, text, activate) {
    if (((event.code & KEY_MASK) >>> 0) !== URL_COMMAND) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    if (typeof text !== 'function' || typeof activate !== 'function') throw new Error('URL 일반 입력의 현재 문자열/링크 분배기 미연결');
    activate(text(event.y >>> 0));
    return true;
}
