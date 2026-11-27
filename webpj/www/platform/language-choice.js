// 옵션 Language의 영어/한국어/일본어 선택·Tell 대상 해석·저장 선택 읽기를 맡는다.
// 웹 저장 번호는0 자동·1 영어·2 한국어·3 일본어다. 선택 뒤 페이지를 다시 불러 언어 묶음을 적용한다.
// languageChoices(current)는 현재 언어만 자국어 이름으로, 나머지는영어로 표시해 준비되지 않은 글꼴을 요구하지 않는다.
// parseLanguageTarget(target)는Patch1101.SetLanguage(1..3)를 읽고 languageOfChoice(number)는 언어 키를 돌려준다.
// savedLanguage(storage)는 netstorm.settings의 유효한 명시 선택만 반환하며 저장소 오류/자동은null이다.
import { SETTINGS_STORAGE_KEY } from './settings.js';

// 선택지 표이다. 번호는 설정 값이며 label 은 각 언어 자신의 표기이다. (2026-11-20 WB04-E1095)
const CHOICES = Object.freeze([
    Object.freeze({ number: 1, language: 'english', label: 'English' }),
    Object.freeze({ number: 2, language: 'korean', label: '한국어', englishLabel: 'Korean' }),
    Object.freeze({ number: 3, language: 'japanese', label: '日本語', englishLabel: 'Japanese' }),
]);
// Tell 대상의 접두(소문자)이다. (2026-11-20 WB04-E1095)
const TARGET_PREFIX = 'patch1101.setlanguage(';

// 옵션 메뉴 항목을 만든다. current는 현재 언어('english'|'korean'|'japanese')다. (2026-11-20 WB04-E1095)
export function languageChoices(current) {
    // 현재 언어의 글꼴로 보장되는 자국어 이름만 쓰고 다른 언어는 영어 이름을 쓴다.
    return CHOICES.map((choice) => ({ label: choice.language === current ? choice.label : (choice.englishLabel ?? choice.label), number: choice.number, checked: choice.language === current }));
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

// 웹 저장 선택1/2/3과 원본 엔진 언어0/7/5는 구별한다. AI·입력·메뉴는 원본 번호를 받는다.
export function originalLanguageNumber(language) { return language === 'korean' ? 7 : language === 'japanese' ? 5 : 0; }
