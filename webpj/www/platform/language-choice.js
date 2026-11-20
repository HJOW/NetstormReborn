// 옵션 메뉴 Language(언어 선택) 선택지와 Tell 대상 해석, 저장된 선택 읽기를 맡는 모듈이다(cpppj 6차 한국어의 `compatibilityLanguages`·SetLanguage 에 대응하는 웹 판본).
// 웹은 언어 묶음(한국어 글꼴·번역표)을 시작할 때 적재하므로 선택을 설정(`language`: 0 자동, 1 English, 2 한국어)에 저장한 뒤 페이지를 다시 불러 적용한다.
// 사용:
//   languageChoices(current) → 옵션 메뉴 항목 { label, number, checked } 목록(항목 이름은 번역하지 않고 현재 언어에 체크한다. 한국어 항목은 한국어 글꼴이 있는 한국어 화면에서만 '한국어'이고 영어 화면에서는 한글을 그릴 수 없어 'Korean' 이다).
//   parseLanguageTarget(target) → Tell 대상 `Patch1101.SetLanguage(n)`(n 은 1 또는 2, 대소문자 무시)에서 선택 번호, 아니면 null.
//   languageOfChoice(number) → 'english' | 'korean' | null.
//   savedLanguage(storage) → 저장소의 netstorm.settings 에 남은 선택('english'|'korean')이며 없거나 자동(0)이면 null. 저장소 오류는 무시한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1095: 웹에서 URL 매개변수 없이 언어를 고르게 하려고 추가)
import { SETTINGS_STORAGE_KEY } from './settings.js';

// 선택지 표이다. 번호는 설정 값이며 label 은 각 언어 자신의 표기이다. (2026-11-20 WB04-E1095)
const CHOICES = Object.freeze([
    Object.freeze({ number: 1, language: 'english', label: 'English' }),
    Object.freeze({ number: 2, language: 'korean', label: '한국어', englishLabel: 'Korean' }),
]);
// Tell 대상의 접두(소문자)이다. (2026-11-20 WB04-E1095)
const TARGET_PREFIX = 'patch1101.setlanguage(';

// 옵션 메뉴 항목을 만든다. current 는 현재 언어('english'|'korean')이다. (2026-11-20 WB04-E1095)
export function languageChoices(current) {
    // 영어 화면에는 한글 글꼴(한국어 묶음)이 없으므로 한국어 항목의 이름은 영어 표기를 쓴다.
    return CHOICES.map((choice) => ({ label: current === 'korean' ? choice.label : (choice.englishLabel ?? choice.label), number: choice.number, checked: choice.language === current }));
}

// 선택 번호의 언어 이름이다. 알 수 없으면 null 이다. (2026-11-20 WB04-E1095)
export function languageOfChoice(number) {
    return CHOICES.find((choice) => choice.number === number)?.language ?? null;
}

// Tell 대상에서 선택 번호만 읽는다. 10진 숫자만 받고 선택지에 없는 번호는 거부한다. (2026-11-20 WB04-E1095)
export function parseLanguageTarget(target) {
    const lower = String(target ?? '').toLowerCase();
    if (!lower.startsWith(TARGET_PREFIX) || !lower.endsWith(')')) return null;
    const digits = lower.slice(TARGET_PREFIX.length, -1);
    if (!/^[0-9]+$/u.test(digits)) return null;
    const number = Number(digits);
    return languageOfChoice(number) === null ? null : number;
}

// 저장소에 남은 언어 선택을 읽는다. 저장소가 없거나 손상됐거나 자동이면 null 이다. (2026-11-20 WB04-E1095)
export function savedLanguage(storage) {
    try {
        const text = storage?.getItem?.(SETTINGS_STORAGE_KEY);
        if (!text) return null;
        return languageOfChoice(JSON.parse(text)?.values?.language);
    } catch {
        return null;
    }
}
