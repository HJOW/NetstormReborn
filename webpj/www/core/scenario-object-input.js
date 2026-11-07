// 시나리오/객체 메뉴 공통 명령 다섯 개(MakeMine·InflictDamage·DestroyPiece·TurnToObelisk·Meltdown)를 현재 월드 객체의 가상 6c 입력에 전달하는 입력 분배 모듈이다(cpppj `client/ScenarioObjectInput.cpp/.h`, 10.78 전용, 원문 4d6eca..4d6f0d).
// 메뉴 수명과 분리된 일반 입력이다. 수정키를 지운 코드가 다섯 명령이 아니면 false, 키 뗌이면 효과 없이 true 이다.
// 대상 SID 는 y 의 낮은 WORD 이며 0 이면 소비만 한다. 0 이 아니면 objectInput(대상 SID, 사건 코드 전체)을 호출하고(없으면 거부) 그 반환값은 다음 일반 입력에 쓰지 않는다. 메뉴·현재 선택·소유자는 추가 실행 조건이 아니다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E27: 시나리오 객체 일반 입력 이식으로 추가)
import { InputCode } from './input-queue.js';
import { asciiLower } from '../assets/config.js';

// 수정키 제거 마스크이다. (2026-11-08 WB04-E27)
const KEY_MASK = 0x03ffffff;
// 실제 이름표의 다섯 항목(소문자)이다. 친절한 별칭이나 시나리오 오자는 더하지 않는다. (2026-11-08 WB04-E27)
const OBJECT_COMMANDS = new Map([
    ['makemine', 0x030000af],
    ['inflictdamage', 0x030000e2],
    ['destroypiece', 0x03000015],
    ['turntoobelisk', 0x030000ce],
    ['meltdown', 0x030000f8],
]);

// 이름을 ASCII 대소문자 무시로 조회하고 미등록이면 0xffffffff 이다. (2026-11-08 WB04-E27)
export function scenarioObjectScriptCode(name) {
    return OBJECT_COMMANDS.get(asciiLower(name)) ?? 0xffffffff;
}

// 메뉴 버튼의 명령 허용과 일반 입력 분배가 같은 표를 쓴다. 같은 번호의 입력 플래그만 제거한다. (2026-11-08 WB04-E27)
export function isScenarioObjectCommand(command) {
    const value = (command & KEY_MASK) >>> 0;
    for (const code of OBJECT_COMMANDS.values()) if (value === code) return true;
    return false;
}

// event = { code, x, y }, objectInput(sid, 사건 코드) 는 현재 객체의 가상 6c 이다. 소비하면 true 이다. (2026-11-08 WB04-E27)
export function dispatchScenarioObjectInput(event, objectInput) {
    if (!isScenarioObjectCommand(event.code)) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    const target = (event.y >>> 0) & 0xffff;
    if (target) {
        if (typeof objectInput !== 'function') throw new Error('시나리오 객체 명령의 현재 가상 입력 미연결');
        objectInput(target, event.code >>> 0);
    }
    // 원문 4d6f09 는 객체 반환값을 읽지 않고 사건의 y 를 다시 읽는다.
    return true;
}
