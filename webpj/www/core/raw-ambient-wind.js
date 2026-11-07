// Client 004dd330 의 공유 바람(연소 입자의 가속에 쓰는 X 바람, Y 바람, 각도)을 기존 게임 난수/각도 정규화와 연결하는 모듈이다(cpppj `o/RawAmbientWind.cpp/.h`, StepAmbientWind 의 10.78 경로, 004dd2b0).
// 매 프레임: X 바람과 Y 바람에 각각 double 0.9 를 곱해 float 로 저장(감쇠) → 게임 난수 next(100)이 10 이상이면 끝 → 아니면 next(65536)으로 잡음(-0.06..0.06, 곱셈 순서 value × (1/65536) × 폭 + 최소)을 만들어
//   X 바람에 더하고 float 로 저장 → atan2(-Y, X) × 57.295827908797776(라디안→도)을 float 로 저장 → Boardcoord 각도 정규화(normalizeBoardAngle, 진단 포함)의 결과를 float 로 각도에 저장한다.
// 나머지 프레임에서는 각도와 두 번째 난수를 보존한다. 시간 배율/재시드/독립 난수를 추가하지 않는다. 입자와 Client 가 같은 장기 바람 객체를 직접 갱신한다. 이 모듈은 DOM 을 쓰지 않는다.
// 주의: atan2 는 JS Math.atan2 이다(원본 x87 fpatan 결과를 float 로 저장하는 경계에서 대조). (2026-11-08 WB04-E17: 공유 바람 이식으로 추가)
import { normalizeBoardAngle } from './raw-board-angle.js';

// 실제 PE 의 감쇠 double, 라디안→도 환산 double, 잡음 범위 float 상수이다. (2026-11-08 WB04-E17)
const DAMPING = 0.9;
const DEGREES_PER_RADIAN = 57.295827908797776;
const NOISE_MINIMUM = Math.fround(-0.06);
const NOISE_MAXIMUM = Math.fround(0.06);

// 바람 상태의 초기값을 만든다(실제 PE: X 0, Y 1, 각도 0). 소비자는 이 객체를 직접 읽고 쓴다. (2026-11-08 WB04-E17)
export function createAmbientWindState() {
    return { x: 0, y: 1, angle: 0 };
}

// 바람 한 프레임을 갱신한다. state = { x, y, angle } (float 값), random 은 GameRandom, diagnostics = { checking(), assertion } 이다. (2026-11-08 WB04-E17)
export function stepAmbientWind(state, random, diagnostics = {}) {
    state.x = Math.fround(state.x * DAMPING);
    state.y = Math.fround(state.y * DAMPING);
    if (random.next(100) >= 10) return;
    const value = random.next(65536);
    const width = Math.abs(NOISE_MAXIMUM - NOISE_MINIMUM);
    const offset = value * (1.0 / 65536.0) * width + NOISE_MINIMUM;
    state.x = Math.fround(offset + state.x);
    const degrees = Math.fround(Math.atan2(-state.y, state.x) * DEGREES_PER_RADIAN);
    state.angle = Math.fround(normalizeBoardAngle(degrees, diagnostics));
}
