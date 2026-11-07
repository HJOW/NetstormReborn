// 언어 변경 요청(004dea70 SetLanguage)과 그 일반 입력/알림 분배를 옮긴 모듈이다(cpppj `client/LanguageRequest.cpp/.h`, `client/LanguageOptions.cpp`의 이름표, 10.78 전용). HTML·시나리오와 현재 Client 가 같은 언어 요청을 공유한다.
//  - requestOriginalLanguage(requested, current, refresh, hooks): 요청 번호가 현재 언어 번호(signed → DWORD 비트)와 같으면 문자열 표/설정/큐/카운터 어느 것도 읽거나 바꾸지 않고 끝낸다. 다르면 set('currentLanguage', 언어 이름)이 동기 Config 변경으로
//    현재 언어 번호/용어표를 실제 갱신하게 한 뒤, 복귀 뒤 현재 LanguageChanged 명령(changed())을 읽어 queue(명령, 0, 0)을 예약하고 refresh.value 를 1 올린다(DWORD 감김). 예외가 나면 그때까지의 설정/큐 효과를 되돌리지 않고 뒤 카운터도 올리지 않는다.
//  - dispatchSetLanguage(event, request): SetLanguage(0x020000a5, 542988) 일반 입력의 뗌이 아닐 때 두 번째 DWORD 를 request(번호)에 전달한다(첫 DWORD 는 원문 부모가 쓰지 않고 수정키도 막지 않음). 뗌은 소비만 한다.
//  - dispatchLanguageChanged(event, alreadyRouted, route): LanguageChanged(0x020000a6, 54298c) 알림을 이미 원문 Gump 경로로 전달했다면 중복 없이 소비만 하고, 아니면 같은 실제 트리의 route(event)로 전달한다. 뗌 표시를 지우지 않는다.
//  - originalLanguageName(number): 원문 이름표 순서의 언어 이름('english','english','french','german','spanish','japanese','portuguese','korean')이며 번호 0/1 은 별칭을 합치지 않는다. 표 밖은 RangeError 이다.
//    마지막 korean(7)은 원본에 없는 클론 추가 언어(6차 한국어 지원)이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E34: 언어 요청 이식으로 추가)
import { InputCode } from './input-queue.js';

// 보호 PE 542988/54298c 이름표의 초기 명령과 수정키 제거 마스크, 언어 이름표(005435b8 순서 + 클론 korean)이다. (2026-11-08 WB04-E34)
export const SET_LANGUAGE_COMMAND = 0x020000a5;
export const LANGUAGE_CHANGED_COMMAND = 0x020000a6;
const KEY_MASK = 0x03ffffff;
const LANGUAGE_NAMES = Object.freeze(['english', 'english', 'french', 'german', 'spanish', 'japanese', 'portuguese', 'korean']);

// 선택되지 않은 장치는 요구하지 않고 원문의 도달 경계에서만 거부한다. (2026-11-08 WB04-E34)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('언어 요청의 실제 장치 미연결');
    return hook;
}

// 유효한 원문 인덱스만 읽으며 무효 번호를 영어로 대체하지 않는다. (2026-11-08 WB04-E34)
export function originalLanguageName(number) {
    if (!Number.isInteger(number) || number < 0 || number >= LANGUAGE_NAMES.length) throw new RangeError('언어 번호 범위 오류');
    return LANGUAGE_NAMES[number];
}

// hooks = { set(키, 값), changed() → 현재 LanguageChanged 명령, queue(명령, 첫째, 둘째) }, refresh = { value } 카운터 보관 객체이다. (2026-11-08 WB04-E34)
export function requestOriginalLanguage(requested, current, refresh, hooks) {
    if ((requested >>> 0) === (current >>> 0)) return;
    need(hooks.set)('currentLanguage', originalLanguageName(requested));
    const command = need(hooks.changed)();
    need(hooks.queue)(command, 0, 0);
    refresh.value = (refresh.value + 1) >>> 0;
}

// 일반 입력의 뗌 관문 뒤 두 번째 DWORD 를 요청 번호로 전달한다. (2026-11-08 WB04-E34)
export function dispatchSetLanguage(event, request) {
    if (((event.code & KEY_MASK) >>> 0) !== SET_LANGUAGE_COMMAND) return false;
    if ((event.code & InputCode.release) === 0) need(request)(event.y >>> 0);
    return true;
}

// Chat 의 기존 처리기는 뗌 표시도 원문처럼 해석하므로 여기서 알림을 지우지 않는다. (2026-11-08 WB04-E34)
export function dispatchLanguageChanged(event, alreadyRouted, route) {
    if (((event.code & KEY_MASK) >>> 0) !== LANGUAGE_CHANGED_COMMAND) return false;
    if (!alreadyRouted) need(route)(event);
    return true;
}
