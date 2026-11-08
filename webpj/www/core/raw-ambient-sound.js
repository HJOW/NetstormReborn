// 주변음 부모(004180f0)와 그 기한 재예약(004180d0)을 옮긴 모듈이다(cpppj `o/RawAmbientSound.cpp/.h`, 10.78 전용). 게임 정지/음향 장치와 별개인 Client 현재값(벽시각)을 읽는다.
//  - stepAmbientSound(globals, random, hooks): 벽시각을 한 번 읽어 기한이 미래이거나 비순서(NaN)면 그것만 하고 돌아간다. 기한이 지났으면 벽시각을 한 번 더 읽어 `현재 + 120.0` 을 기한에 저장한 뒤(난수보다 먼저),
//    강제 값이 signed 로 0 이하일 때만 게임 난수(3000)/1000 으로 종류(0 천둥·1 독수리·2 고래)를 새로 고른다(범위 밖 종류는 그대로 보존하며 아무것도 재생하지 않음).
//    천둥은 `thunderQuietDistant.wav` 를 재생한 뒤 소리 요청이 돌아온 다음 현재 모달 토큰을 다시 읽어 0 일 때만 번개 섬광(0)을 요청한다. 독수리/고래는 게임 난수(20000)-10000 을 좌우 값으로 `eagleScream.wav`/`whale.wav` 를 재생한다.
//  - resetAmbientSoundDeadline(globals, wallSeconds): 현재 벽시각 + 10.0 을 기한에 저장한다(State/Edit 가 쓰는 같은 기한이며 게임 시계나 이전 프레임 캐시를 쓰지 않음).
// globals = { deadline(double, State/Edit 와 공유하는 같은 기한), kind(int32), forced(int32, 원문 최초 0) } 이며 읽기/쓰기는 항상 현재 값에 한다(프레임 사본을 만들지 않음). 훅 = { wallSeconds(), sound(이름, 반복, 음량, 좌우, 우선순위, 한도), currentDialog(), flash(밝기) } 이다.
// 도달한 실제 장치만 요구하며 선행 효과는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E77: 주변음 이식으로 추가)

// 주변음 재예약 간격(초)과 State 지연(초), 종류별 음향 이름과 난수 범위이다. (2026-11-08 WB04-E77)
const AMBIENT_INTERVAL = 120.0;
const STATE_DELAY = 10.0;
const KIND_RANGE = 3000;
const KIND_DIVISOR = 1000;
const PAN_RANGE = 20000;
const PAN_CENTER = 10000;
const THUNDER_SOUND = 'thunderQuietDistant.wav';
const EAGLE_SOUND = 'eagleScream.wav';
const WHALE_SOUND = 'whale.wav';

// 도달하지 않은 장치는 요구하지 않고 도달했을 때 연결 여부를 확인한다. (2026-11-08 WB04-E77)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('주변음 현재 장치 미연결');
    return hook;
}

// 현재 벽시각 + 10 으로 기한을 다시 예약한다. (2026-11-08 WB04-E77)
export function resetAmbientSoundDeadline(globals, wallSeconds) {
    globals.deadline = need(wallSeconds)() + STATE_DELAY;
}

// 미래/비순서 기한이면 첫 시계 조회만 수행한다. 재예약은 두 번째 조회 뒤 난수보다 먼저 저장한다. (2026-11-08 WB04-E77)
export function stepAmbientSound(globals, random, hooks) {
    // 시계 callback 이 바꾼 현재 기한을 그 뒤에 조회한다.
    const now = need(hooks.wallSeconds)();
    if (!(globals.deadline <= now)) return;
    globals.deadline = need(hooks.wallSeconds)() + AMBIENT_INTERVAL;
    if (globals.forced <= 0) globals.kind = Math.floor(random.next(KIND_RANGE) / KIND_DIVISOR) | 0;
    if (globals.kind === 0) {
        need(hooks.sound)(THUNDER_SOUND, 0, 0, 0, 0, 0);
        // 소리 요청이 복귀한 뒤 현재 모달 토큰을 다시 읽는다.
        if (!need(hooks.currentDialog)()) need(hooks.flash)(0);
    } else if (globals.kind === 1 || globals.kind === 2) {
        const pan = (random.next(PAN_RANGE) | 0) - PAN_CENTER;
        need(hooks.sound)(globals.kind === 1 ? EAGLE_SOUND : WHALE_SOUND, 0, 0, pan, 0, 0);
    }
}
