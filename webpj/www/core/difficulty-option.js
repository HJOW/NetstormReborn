// 옵션 메뉴 Difficulty(난이도) 선택지와 Tell 대상 해석을 맡는 모듈이다(cpppj 11차 난이도의 `DifficultyMenuChoices`·`Patch1101.SetDifficulty(1|2|3)`에 대응). 계산 자체는 `ai-difficulty.js` 가 맡는다.
// 사용:
//   difficultyChoices(configured) → 옵션 메뉴 항목 { label, number, checked } 목록: Easy(1)·Normal(2)·Hard(3). 설정 값이 없거나 범위 밖이면 보통에 체크한다. label 은 번역 키이다.
//   parseDifficultyTarget(target) → Tell 대상 `Patch1101.SetDifficulty(n)`(n 은 1·2·3, 대소문자 무시)에서 난이도 번호, 아니면 null.
// 선택은 설정 `difficulty` 에 저장되고 새 미션의 적 AI 생성 때 읽힌다(core/campaign-client-services.js). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1096: 11차 난이도 옵션 메뉴 추가)
import { AiDifficulty, aiDifficultyFromValue } from './ai-difficulty.js';

// 선택지 표(번역 키와 번호)이다. (2026-11-20 WB04-E1096)
const CHOICES = Object.freeze([
    Object.freeze({ label: 'Easy', number: AiDifficulty.Easy }),
    Object.freeze({ label: 'Normal', number: AiDifficulty.Normal }),
    Object.freeze({ label: 'Hard', number: AiDifficulty.Hard }),
]);
// Tell 대상의 접두(소문자)이다. (2026-11-20 WB04-E1096)
const TARGET_PREFIX = 'patch1101.setdifficulty(';

// 현재 설정 값으로 체크한 선택지 목록을 만든다. (2026-11-20 WB04-E1096)
export function difficultyChoices(configured) {
    const current = aiDifficultyFromValue(configured);
    return CHOICES.map((choice) => ({ label: choice.label, number: choice.number, checked: choice.number === current }));
}

// Tell 대상에서 난이도 번호만 읽는다. 10진 숫자만 받고 1~3 밖은 거부한다. (2026-11-20 WB04-E1096)
export function parseDifficultyTarget(target) {
    const lower = String(target ?? '').toLowerCase();
    if (!lower.startsWith(TARGET_PREFIX) || !lower.endsWith(')')) return null;
    const digits = lower.slice(TARGET_PREFIX.length, -1);
    if (!/^[0-9]+$/u.test(digits)) return null;
    const number = Number(digits);
    return CHOICES.some((choice) => choice.number === number) ? number : null;
}
