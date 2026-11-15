// 11차 난이도 조절의 순수 계산이다: 적 인공지능 설정(600바이트)에 쉬움/어려움 배율을 곱한다(cpppj `o/AiDifficulty.cpp/.h`). 원본 10.78 에는 난이도 개념이 없으므로 보통(기본)은 설정을 한 비트도 바꾸지 않는다.
// 사용:
//   aiDifficultyFromValue(value) → 저장값 1 쉬움, 3 어려움, 그 밖(0/누락/범위 밖 포함)은 보통(2).   aiDifficultyToValue(level) → 설정 파일 정수(역함수).
//   applyAiDifficulty(settings, level) → 600바이트 Uint8Array 의 새 사본을 돌려준다(입력은 바꾸지 않음). 보통이면 같은 바이트의 사본이고, 쉬움은 +0x220 다리 그리기 주기·+0x224 물건 갱신 주기·+0x228 이동 간격(단정도)에
//     4 배, 어려움은 0.5 배를 곱하며(0 이하/비유한 주기는 그대로), 쉬움만 +0x234 채취 유닛 수(signed)가 1 이상일 때 절반(내림, 최소 1)으로 줄인다. 크기가 600바이트가 아니면 거부한다.
//   sanitizeAiForcedEnemy(settings, owner) → 10차 NB10: 미션이 정한 강제 적(+0x240)이 소유자 범위(1..8) 밖이거나 자기 자신이면 0(자동 선택)으로 바꾼 사본을, 바꿀 것이 없으면 null 을 돌려준다.
// 설정 안의 AI 값 위치는 원문 AI 설정 파서(004c2f30 대)가 쓰는 오프셋을 따른다. 이 모듈은 DOM·설정 파일·세션에 의존하지 않는다.
// 원본 관측은 없다(원본에 난이도가 없음). cpppj `AiDifficultyTests.cpp` 의 다섯 시험을 tests/ai-difficulty.test.mjs 가 옮겨 대조한다. (2026-11-15 WB04-E554: 11차 난이도 계산 AiDifficulty 이식)

// 난이도 단계이다. 값은 설정 파일(options.cfg)의 difficulty 값과 같고, 없거나 범위 밖이면 보통이다. (2026-11-15 WB04-E554)
export const AiDifficulty = Object.freeze({ Easy: 1, Normal: 2, Hard: 3 });
// 설정 파일에 저장하는 난이도 키 이름이다(옵션 메뉴와 스커미시 화면이 공유한다). (2026-11-15 WB04-E554)
export const DIFFICULTY_CONFIG_KEY = 'difficulty';
// 쉬움에서 주기에 곱하는 배율(느려짐)과 어려움에서 곱하는 배율(빨라짐)이다. (2026-11-15 WB04-E554)
export const EASY_CYCLE_FACTOR = 4;
export const HARD_CYCLE_FACTOR = 0.5;
// AI 설정의 float 주기 필드 오프셋(ai%sBridgeDrawRate/StuffRefreshRate/TimeBetweenMoves), 채취 유닛 수(ai%sCollectors)·강제 적(ai%sEnemy) 오프셋과 설정 전체 크기이다. (2026-11-15 WB04-E554)
const CYCLE_OFFSETS = Object.freeze([0x220, 0x224, 0x228]);
const COLLECTORS_OFFSET = 0x234;
const FORCED_ENEMY_OFFSET = 0x240;
const SETTINGS_BYTES = 600;

// 설정이 정확히 600바이트인지 확인하고 호스트 정렬과 무관한 DataView 를 만든다. (2026-11-15 WB04-E554)
function copyOf(settings, message) {
    if (settings.length !== SETTINGS_BYTES) throw new RangeError(message);
    const copy = new Uint8Array(settings);
    return { copy, view: new DataView(copy.buffer) };
}

// 저장된 정수(0/누락 포함)를 난이도로 바꾼다: 1 은 쉬움, 3 은 어려움, 그 밖의 모든 값은 보통이다. (2026-11-15 WB04-E554)
export function aiDifficultyFromValue(value) {
    const normalized = value >>> 0;
    if (normalized === AiDifficulty.Easy) return AiDifficulty.Easy;
    if (normalized === AiDifficulty.Hard) return AiDifficulty.Hard;
    return AiDifficulty.Normal;
}

// 난이도를 설정 파일에 저장하는 정수로 바꾼다(열거값 그대로, aiDifficultyFromValue 의 역함수). (2026-11-15 WB04-E554)
export function aiDifficultyToValue(level) { return level >>> 0; }

// 600바이트 AI 설정의 사본에 난이도를 적용해 돌려준다. 입력은 바꾸지 않는다. (2026-11-15 WB04-E554)
export function applyAiDifficulty(settings, level) {
    const { copy, view } = copyOf(settings, 'AI 난이도: 설정은 600바이트여야 한다');
    if (level === AiDifficulty.Normal) return copy;
    const factor = level === AiDifficulty.Easy ? EASY_CYCLE_FACTOR : HARD_CYCLE_FACTOR;
    // 세 주기 필드를 차례로 곱한다. 0 이하/비유한 값(예: 다리 주기 0)은 배율로 바꿀 수 없어 그대로 둔다.
    for (const offset of CYCLE_OFFSETS) {
        const value = view.getFloat32(offset, true);
        if (Number.isFinite(value) && value > 0) view.setFloat32(offset, Math.fround(value * factor), true);
    }
    // 쉬움만 채취 유닛 수를 줄이며 원래 0 이하이면(채취 유닛 없음 설계) 건드리지 않는다.
    if (level === AiDifficulty.Easy) {
        const collectors = view.getInt32(COLLECTORS_OFFSET, true);
        if (collectors > 0) view.setInt32(COLLECTORS_OFFSET, Math.max(1, Math.trunc(collectors / 2)), true);
    }
    return copy;
}

// 강제 적 번호가 1..8 밖이거나 자기 자신이면 0 으로 정리한 사본을 돌려준다. 0(자동 선택)과 정상 값은 바꿀 것이 없어 null 이다. (2026-11-15 WB04-E554)
export function sanitizeAiForcedEnemy(settings, owner) {
    if (settings.length !== SETTINGS_BYTES) throw new RangeError('AI 설정은 600바이트여야 한다');
    // 일반 배열 입력도 받으므로 바이트 배열로 바꿔 읽는다(복사 없이 Uint8Array 는 그대로 쓴다).
    const bytes = settings instanceof Uint8Array ? settings : Uint8Array.from(settings);
    const forced = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(FORCED_ENEMY_OFFSET, true);
    // 정상이거나 자동 선택이면 바꾸지 않는다.
    if (forced === 0 || (forced <= 8 && forced !== (owner >>> 0))) return null;
    const { copy, view } = copyOf(settings, 'AI 설정은 600바이트여야 한다');
    view.setUint32(FORCED_ENEMY_OFFSET, 0, true);
    return copy;
}
