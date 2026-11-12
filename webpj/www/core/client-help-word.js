// 원문 0054db38 `helpWord` 전역 캐시(도움말 항목 문구, 예: "Help", 한국어 "정보")의 소유자를 옮긴 모듈이다(cpppj `client/ClientHelpWord.h/.cpp`, 기준선 1f00a25a 뒤 추가된 새 파일, 3.6절 ③).
// 원문 441300/4413c0 의 갱신 규칙을 그대로 따르는 NULL 가능 문자열 캐시이다: 최초에는 BSS 처럼 null 이고 파일/GUI/Config 를 읽지 않는다.
//  - interpret(configured): 설정값이 null/undefined 이거나 빈 문자열이면 캐시를 건드리지 않고 false 를 돌려준다. 비어 있지 않은 설정값이 있으면, 기존 캐시와 ASCII 대소문자 무시로 같을 때 옛 표기를 유지하고(교체 횟수 불변)
//    true, 다를 때(또는 최초)만 새 문자열로 교체하고 교체 횟수를 하나 올린 뒤 true 를 돌려준다.
//  - get(): 현재 캐시이다(null 은 원문 NULL 포인터이며 빈 문자열과 구별). generation: 실제 교체가 일어난 횟수(같은 표기 유지와 교체를 관측 시험이 구별).
//  - ClientHelpWord.equalsIgnoreCase(left, right): 원문 `_stricmp` 와 같이 ASCII 영문자만 소문자로 접어 비교한다. 한글 등 ASCII 밖 문자는 그대로 비교한다.
// 웹 적응: 원문은 표시 바이트(`EncodeDisplayText`) 문자열을 쓰지만 웹은 JS 문자열을 그대로 쓴다. 한 Client 에서 한 번 만들어 `ClientLocalStateGlobals.helpWord()` 가 보유한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-12 WB04-E335: 4-D ClientLocalStateGlobals 의 하위 소유자로 추가)

// ASCII 영문자만 소문자로 접는다(그 밖의 UTF-16 코드 단위는 그대로). (2026-11-12 WB04-E335)
function foldAscii(code) {
    return code >= 0x41 && code <= 0x5a ? code + 0x20 : code;
}

// helpWord 캐시의 소유자이다. 사용: const cache = new ClientHelpWord(); cache.interpret('Help'); cache.get() === 'Help'. 복사하지 않고 한 객체를 모든 소비자가 공유한다. (2026-11-12 WB04-E335)
export class ClientHelpWord {
    // 현재 캐시 문자열이다(null 은 원문 NULL).
    #value = null;
    // 실제로 새 문자열로 교체한 횟수이다.
    #generation = 0;

    // 현재 캐시를 돌려준다. null 은 원문 NULL 포인터이며 빈 문자열("")과 구별된다. (2026-11-12 WB04-E335)
    get() {
        return this.#value;
    }

    // 실제 교체가 일어난 횟수이다. 같은 표기 유지(교체 없음)와 교체를 관측 검사가 구별하는 용도이다. (2026-11-12 WB04-E335)
    get generation() {
        return this.#generation;
    }

    // 441300/4413c0 의 규칙을 적용한다: 설정값이 없거나 빈 문자열이면 아무것도 바꾸지 않고 false 이다. 있으면 기존 캐시와 대소문자 무시로 같을 때 옛 표기를 유지하고, 다를 때만 교체한다. (2026-11-12 WB04-E335)
    interpret(configured) {
        if (configured === null || configured === undefined || configured === '') return false;
        if (this.#value !== null && ClientHelpWord.equalsIgnoreCase(this.#value, configured)) return true;
        this.#value = String(configured);
        this.#generation += 1;
        return true;
    }

    // 원문 `_stricmp` 와 같은 ASCII 대소문자 무시 비교이다. 길이가 다르면 false 이다. (2026-11-12 WB04-E335)
    static equalsIgnoreCase(left, right) {
        if (left.length !== right.length) return false;
        // 같은 위치의 두 코드 단위를 순서대로 비교한다.
        for (let index = 0; index < left.length; index += 1) {
            if (foldAscii(left.charCodeAt(index)) !== foldAscii(right.charCodeAt(index))) return false;
        }
        return true;
    }
}
