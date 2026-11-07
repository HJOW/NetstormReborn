// 대본/메뉴 DetonateBomb 일반 입력(원문 4d6eca..4da477)의 분배를 옮긴 모듈이다(cpppj `client/DetonateInput.cpp/.h`, 10.78 전용).
// dispatchDetonateInput(event, objectInput): 수정키를 지운 코드가 DetonateBomb(0x03000074, 5428c8)이 아니면 false, 키 뗌이면 효과 없이 true 이다. 대상은 y 의 낮은 WORD 이며 0 이면 소비만 한다.
//   0 이 아니면 objectInput(대상 SID, 사건 코드 전체)을 호출한다(없으면 거부). 현재 선택/소유자/메뉴 체인을 새 조건으로 쓰지 않고, 객체 가상 6c 의 반환은 원문처럼 무시한다(종료 예외나 임의 5c 요청으로 바꾸지 않음).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E33: DetonateBomb 일반 입력 이식으로 추가)
import { InputCode } from './input-queue.js';

// 원본 5428c8 DetonateBomb 의 초기 토큰과 수정키 제거 마스크이다. (2026-11-08 WB04-E33)
export const DETONATE_BOMB_COMMAND = 0x03000074;
const KEY_MASK = 0x03ffffff;

// event = { code, x, y }, objectInput(sid, 사건 코드) 는 현재 객체의 가상 6c 이다. 소비하면 true 이다. (2026-11-08 WB04-E33)
export function dispatchDetonateInput(event, objectInput) {
    if (((event.code & KEY_MASK) >>> 0) !== DETONATE_BOMB_COMMAND) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    const target = (event.y >>> 0) & 0xffff;
    if (target) {
        if (typeof objectInput !== 'function') throw new Error('Detonate 의 실제 가상 입력 미연결');
        objectInput(target, event.code >>> 0);
    }
    return true;
}
