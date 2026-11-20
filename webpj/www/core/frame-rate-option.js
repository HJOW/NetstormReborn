// 옵션 메뉴 Frame Rate(프레임 상한) 선택지와 Tell 대상 해석을 옮긴 모듈이다(cpppj `client/FrameRateOption.cpp/.h` 후순위 F1, 원본 기본 상한 `DAT_005318d8 = 0x4b` = 75).
// 사용:
//   normalizeFrameRate(configured) → 설정 maxFPS 값(없으면 0)을 1~1000 이면 그대로, 아니면 원본 기본 75 로 바꾼다. 1~1000 사이의 임의 값도 수동 설정으로 인정한다.
//   parseFrameRateTarget(target) → "Patch1101.SetFrameRate(60)" 같은 Tell 대상에서 선택지(60·75·120)에 있는 정수만 돌려주고 그 밖은 null 이다(대소문자 무시, 접두·닫는 괄호가 정확해야 한다).
//   frameRateChoices(configured) → 옵션 메뉴 항목 { label, number, checked } 목록이다(원본 기본 75 에는 " (Original)" 을 붙이고 현재 상한에 체크한다).
//   frameDue(now, last, rate) → rAF 시각(ms)이 마지막 처리 시각에서 상한 간격(1000/rate, 표시 주기 흔들림 허용 1.5ms)만큼 지났으면 true(그 프레임을 처리) 이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1094: 원본 75 FPS 상한과 60·120 선택을 웹 프레임 루프에 적용하기 위해 추가)

// 옵션 메뉴가 제공하는 프레임 상한 선택지와 원본 기본값이다. (2026-11-20 WB04-E1094)
export const FRAME_RATE_CHOICES = Object.freeze([60, 75, 120]);
export const ORIGINAL_FRAME_RATE = 75;
// Tell 대상의 접두이다. 소문자로 비교한다. (2026-11-20 WB04-E1094)
const TARGET_PREFIX = 'patch1101.setframerate(';
// 표시 주기(rAF) 시각이 흔들려도 상한 근처의 프레임을 버리지 않게 하는 허용치(ms)이다. 60Hz 표시에서 75 상한(13.33ms)이 매 프레임 처리되도록 둔다. (2026-11-20 WB04-E1094)
const JITTER_MS = 1.5;

// 설정 값을 상한(fps)으로 바꾼다. 0·범위 밖·정수가 아닌 값은 원본 기본이다. (2026-11-20 WB04-E1094)
export function normalizeFrameRate(configured) {
    return Number.isInteger(configured) && configured >= 1 && configured <= 1000 ? configured : ORIGINAL_FRAME_RATE;
}

// Tell 대상에서 선택지 안의 프레임 상한만 읽는다. 숫자는 10진 숫자만 받고 부호·공백·소수는 거부한다. (2026-11-20 WB04-E1094)
export function parseFrameRateTarget(target) {
    const lower = String(target ?? '').toLowerCase();
    if (!lower.startsWith(TARGET_PREFIX) || !lower.endsWith(')')) return null;
    const digits = lower.slice(TARGET_PREFIX.length, -1);
    if (!/^[0-9]+$/u.test(digits)) return null;
    const value = Number(digits);
    return FRAME_RATE_CHOICES.includes(value) ? value : null;
}

// 옵션 메뉴 항목 목록을 만든다. configured 는 설정 maxFPS 값이다. (2026-11-20 WB04-E1094)
export function frameRateChoices(configured) {
    const current = normalizeFrameRate(configured);
    // 선택지마다 항목을 만든다(원본 기본에는 Original 표시를 붙인다).
    return FRAME_RATE_CHOICES.map((rate) => ({ label: `${rate} FPS${rate === ORIGINAL_FRAME_RATE ? ' (Original)' : ''}`, number: rate, checked: rate === current }));
}

// 이번 rAF 프레임을 처리할지 정한다. last 가 null 이면(첫 프레임) 항상 처리한다. (2026-11-20 WB04-E1094)
export function frameDue(now, last, rate) {
    if (last === null || last === undefined) return true;
    return now - last >= 1000 / rate - JITTER_MS;
}
