// 원문 004d53a0의 초기 명령 등록과 flash/desc/cheatorama 콜백을 실제 Client 장치에서 분리해 옮긴다(cpppj client/RawIssueCommands.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 같은 명령표와 공유 치트 DWORD 를 쓰는 초기 명령 계층이며, 명령에서 설정 파일이나 게임 모드를 임의로 바꾸지 않는다. 치트 비밀번호를 Config 에 저장하지 않는다.
// 직접 호출과 실제 Chat 분배기가 같은 본체를 쓴다. 원본 관측 fixture(issue-commands 122행)는 E467 의 dialog-native.test.mjs 가 직접 대조한다.
// 사용: registerIssueCommands((name, callback, visible) => table.add(name, callback, visible)); const result = invokeIssueCommand(0x4d5340, '/data', cheatBox, hooks) (알려진 콜백이면 0, 아니면 null).
// (2026-11-14 WB04-E437: RawMenuControls 선행 초기 점 명령 이식)
import { configParseLong } from '../assets/config.js';
import { readRawTextToken } from '../core/raw-text-token.js';

// 원문 콜백 번호(004d5300/004d5340/004d5360)이다. 등록표와 분배기가 같은 값을 쓴다.
export const ISSUE_FLASH_COMMAND = 0x4d5300;
export const ISSUE_DESCRIPTION_COMMAND = 0x4d5340;
export const ISSUE_CHEAT_COMMAND = 0x4d5360;
// 004d5360 이 유일하게 005cab30 에 저장하는 치트 값이다.
export const ISSUE_CHEAT_VALUE = 0x845fed;

// 호출하지 않는 MP/음향 장치를 미리 요구하지 않는다. 도달한 장치만 검사한다.
function need(fn) {
    if (typeof fn !== 'function') throw new Error('초기 점 명령의 선택 장치 미연결');
    return fn;
}

// 네 실제 등록 호출을 같은 순서로 수행한다: flash, flash(중복 두 번을 제거하지 않는다), desc(표시 1), cheatorama(표시 1). 기존 count/표시 포인터의 부수 효과는 등록기가 소유한다.
// registration(name, callback, visible) 은 매 호출마다 요구한다(중간에 던지면 앞 등록은 남는다).
export function registerIssueCommands(registration) {
    need(registration)('flash', ISSUE_FLASH_COMMAND, 0);
    need(registration)('flash', ISSUE_FLASH_COMMAND, 0);
    need(registration)('desc', ISSUE_DESCRIPTION_COMMAND, 1);
    need(registration)('cheatorama', ISSUE_CHEAT_COMMAND, 1);
}

// 알려진 세 콜백은 원문 결과 0 을 돌려주고 다른 콜백은 null(nullopt)로 다음 분배기에 넘긴다. argument 는 null/undefined(없음) 또는 바이트 문자열이며 빈 문자열은 "있음"이다.
// cheat 는 { value } 상자(원문 005cab30 DWORD 참조)이며 cheatorama 가 값이 맞을 때만 쓴다. hooks = { flash(밝기), sound(이름), description(-1, argument|null, 0) }.
// 인자 부재·첫 토큰·atol 뒤 실제 효과 순서를 보존한다: flash 는 flash 장치(밝기) → sound(thunderQuietDistant.wav), desc 는 description, cheatorama 는 인자 없음이면 TypeError.
export function invokeIssueCommand(callback, argument, cheat, hooks) {
    const given = argument !== null && argument !== undefined;
    const code = callback >>> 0;
    if (code === ISSUE_FLASH_COMMAND) {
        need(hooks.flash)(given ? configParseLong(argument) : 0);
        need(hooks.sound)('thunderQuietDistant.wav');
        return 0;
    }
    if (code === ISSUE_DESCRIPTION_COMMAND) {
        need(hooks.description)(-1, given ? argument : null, 0);
        return 0;
    }
    if (code === ISSUE_CHEAT_COMMAND) {
        if (!given) throw new TypeError('cheatorama 원문의 실제 인자 포인터 없음');
        // 첫 토큰(기본 구분자)의 atol 이 정확히 치트 값일 때만 공유 DWORD 에 저장한다.
        if ((configParseLong(readRawTextToken(argument).text) >>> 0) === ISSUE_CHEAT_VALUE) cheat.value = ISSUE_CHEAT_VALUE;
        return 0;
    }
    return null;
}
