// 원본 언어 번호/옵션 선택 규칙(cpppj `client/LanguageOptions.cpp`, 원본 004de390/004de410/004de420/004dea40, 호출 435248..43529f)을 옮긴 모듈이다.
//  - detectOriginalLanguage(langid) [004de390]: OS 가 준 LANGID 의 낮은 WORD 로 언어를 고른다. primary ID 7=독일어(3)·10=스페인어(4)·12=프랑스어(2)·17=일본어(5)·22=포르투갈어(6),
//    그 밖은 하위 언어 2(영국식)면 1, 아니면 0 이다.
//  - originalLanguageName(number) [004de410]: 영어 0/1 을 별칭으로 합치지 않은 원문 일곱 이름(+6차 한국어 7)이며 표 밖 번호는 거부한다.
//  - findOriginalLanguage(name) [004de420]: 정확한 C 문자열 비교(ASCII 대소문자도 구별, 첫 NUL 뒤는 보지 않음)로 이름의 번호를 고르고 미등록/빈 이름은 0 이다. 영어는 항상 0 이다.
//  - applyOriginalLanguageOption(detected, current, published, hooks): currentLanguage 를 읽어(원문 128 바이트 지역) 번호를 고르고, 같은 번호면 거짓, 다르면 current 를 먼저 쓴 뒤
//    정규 이름과 raw 이름으로 용어표를 두 번 읽고 published 를 쓴다. 첫 읽기가 실패해도 이미 쓴 current 는 되돌리지 않는다.
// C++ 의 int32 참조는 { value } 상자로 옮겼다(current·published). OS 언어 자동 선택(detectOriginalLanguage)은 웹이 쓰지 않으며(브라우저 언어는 ?lang= 로 고름) 원문 대조용으로 둔다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E272: 4-A client 자료 읽기 계열의 LanguageOptions 이식)

// 6차 한국어: 원문 일곱 번호 뒤에 추가한 클론 전용 언어 번호이다. (2026-11-10 WB04-E272)
export const KOREAN_LANGUAGE_NUMBER = 7;

// 보호 PE 005435b8 의 이름 순서이다. 번호 0/1 은 별칭 합치기 없이 그대로 둔다. (2026-11-10 WB04-E272)
const LANGUAGE_NAMES = Object.freeze(['english', 'english', 'french', 'german', 'spanish', 'japanese', 'portuguese', 'korean']);

// 원문 지역 버퍼 크기(128 바이트)이다. 이름이 이 길이 이상이면 지역 범위를 넘으므로 거부한다. (2026-11-10 WB04-E272)
const NAME_LOCAL_BYTES = 128;

// OS 가 반환한 LANGID 의 낮은 WORD 만 원문과 같은 분기로 해석한다. (2026-11-10 WB04-E272)
export function detectOriginalLanguage(langid) {
    const id = langid & 0xffff;
    switch (id & 0x3ff) {
    case 7: return 3;
    case 10: return 4;
    case 12: return 2;
    case 17: return 5;
    case 22: return 6;
    default: return (id >> 10) === 2 ? 1 : 0;
    }
}

// 유효한 번호의 이름을 읽는다. 무효 번호를 영어로 대체하지 않고 거부한다. (2026-11-10 WB04-E272)
export function originalLanguageName(number) {
    if (!Number.isInteger(number) || number < 0 || number >= LANGUAGE_NAMES.length) throw new RangeError('언어 번호가 표 밖입니다.');
    return LANGUAGE_NAMES[number];
}

// 이름의 번호를 고른다. 첫 NUL 뒤는 보지 않고 대소문자를 구별하며 없으면 0 이다. (2026-11-10 WB04-E272)
export function findOriginalLanguage(name) {
    const nul = name.indexOf('\0');
    const text = nul < 0 ? name : name.slice(0, nul);
    // 표 순서대로 첫 일치를 고른다('english' 는 0 이 먼저).
    for (let index = 0; index < LANGUAGE_NAMES.length; index += 1) if (text === LANGUAGE_NAMES[index]) return index;
    return 0;
}

// 옵션 해석의 언어 선택 부분이다. hooks = { read() → 현재 currentLanguage 문자열(누락은 빈 문자열), load(이름) → 같은 LanguageSpec 으로 용어표를 읽음(부재도 이전 표를 비움) }.
// current/published 는 { value } 상자(int32)이다. 바뀌었으면 true 이다. 사용: applyOriginalLanguageOption(detected, current, published, { read, load }). (2026-11-10 WB04-E272)
export function applyOriginalLanguageOption(detected, current, published, hooks) {
    if (typeof hooks?.read !== 'function') throw new Error('언어 옵션 currentLanguage 읽기 미연결');
    let name = hooks.read();
    const nul = name.indexOf('\0');
    if (nul >= 0) name = name.slice(0, nul);
    if (name.length >= NAME_LOCAL_BYTES) throw new RangeError('언어 옵션 원문 128 바이트 지역 범위');
    let selected = findOriginalLanguage(name);
    if (detected !== selected && selected === 0) selected = detected === 1 ? 1 : 0;
    if ((current.value | 0) === selected) return false;
    // 4dea40 이 먼저 current 를 쓰고 정규 이름을 읽은 뒤 호출자 435220 이 raw 이름을 다시 읽는다. 첫 load 가 실패해도 이미 쓴 current 는 되돌리지 않는다.
    current.value = selected;
    if (typeof hooks.load !== 'function') throw new Error('언어 옵션 실제 용어표 읽기 미연결');
    hooks.load(originalLanguageName(selected));
    hooks.load(name);
    published.value = selected;
    return true;
}
