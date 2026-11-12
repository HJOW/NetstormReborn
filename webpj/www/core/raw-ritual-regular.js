// Rune·Dais 정기(Regular) 사건 분배의 공용 분기 본문이다(cpppj `o/RawRitualRegular.cpp/.h` 중 Rune/Dais 부분, 10.78 전용).
// 장기 Native(RawDaisRegular)와 독립 typed 입력이 같은 원문 분기 순서를 실행하도록 분기만 함수로 분리했다. 큰 의식 본문(진행·희생)은 호출자가 주는 훅이다.
//  - runRuneRegular(44a3a0): 사건 0x304 에서만 같은 소유자 다이스 검색 → 발견하면 전체 진행 → 룬 자기 삭제(flags 0x48)를 하고 -1 을 돌려준다. 그 밖의 사건은 payload 그대로 돌려준다.
//  - runDaisHandle(44b150): 0x305(프레임 +1 의 단계 비교·프레임 지정) / 0x306(희생 진행) / 0x307(count 가 0 이 아니면 의식 실행) / 그 밖은 공통 Damageable 로 보낸다.
// 임의의 TYPE/free/dead 선검사를 덧붙이지 않는다. 제단 정기(runAltarRegular)는 RawAltarSacrifice 단위에서 이어서 옮긴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E367: 7-P3 RawDaisRegular 이식의 선행 분기로 추가)

// 룬 정기의 소멸 사건·룬 자기 삭제 flags·다이스 단계 사건 번호이다. (2026-11-13 WB04-E367)
const RUNE_EVENT = 0x304;
const RUNE_DESTROY_FLAGS = 0x48;
const DAIS_STAGE_EVENT = 0x305;
const DAIS_SACRIFICE_EVENT = 0x306;
const DAIS_RITUAL_EVENT = 0x307;
// 프레임 DWORD 위치(+36, 부호 있음)이다. (2026-11-13 WB04-E367)
const FRAME_OFFSET = 36;
// 305 가 같은 단계에 머무를 때 돌려주는 다음 지연(float32 0.1)이다. (2026-11-13 WB04-E367)
const STAGE_HOLD_DELAY = Math.fround(0.1);

// 호출 중 장치가 바뀌어도 선택한 함수를 복귀까지 유지하도록 필수 훅을 꺼낸다(없으면 거부). (2026-11-13 WB04-E367)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('의식 정기의 선택된 실제 효과 미연결');
    return hook;
}

// 룬 정기 분기이다. hooks = { findDais(rune) → 다이스 SID(0 없음), progress(dais), destroy(rune, flags) }.
// 사용: runRuneRegular(pool, rune, event, payload, hooks). 304 가 아닌 사건은 훅을 하나도 부르지 않고 payload 를 그대로 돌려준다.
// 304: 다이스 검색 → 유효 SID(풀 범위 안)면 진행 → 룬 삭제(flags 0x48) → -1(더 이상 예약하지 않음). (2026-11-13 WB04-E367)
export function runRuneRegular(pool, rune, event, payload, hooks) {
    if (event !== RUNE_EVENT) return payload;
    const dais = need(hooks.findDais)(rune);
    // 발견한 다이스가 풀 범위 안일 때만 전체 진행 본문을 실행한다.
    if (dais && dais < pool.capacity) need(hooks.progress)(dais);
    need(hooks.destroy)(rune, RUNE_DESTROY_FLAGS);
    return -1;
}

// 다이스 정기 분기이다. hooks = { stage(dais, frame) → 단계, frame(dais, frame, flags), sacrificeAdvance(dais) → 지연, ritual(dais), damageable(sid, event, count, payload) → 지연 }.
// 사용: runDaisHandle(pool, dais, event, count, payload, hooks). 305: 저장한 현재 프레임 f 와 f+1(int32 감김)의 단계를 f+1 먼저, f 나중 순서로 조회하고 f+1 을 지정한 뒤
//   두 단계가 같으면 0.1, 다르면 0 을 돌려준다. 306: 희생 진행 후단의 지연. 307: count 가 0 이 아니면 의식 실행 후 0, 0 이면 공통 Damageable. (2026-11-13 WB04-E367)
export function runDaisHandle(pool, dais, event, count, payload, hooks) {
    if (event === DAIS_STAGE_EVENT) {
        const raw = pool.slot(dais);
        // 프레임 DWORD 를 부호 있게 읽고 다음 프레임은 int32 로 감긴다.
        const frame = (raw[FRAME_OFFSET] | (raw[FRAME_OFFSET + 1] << 8) | (raw[FRAME_OFFSET + 2] << 16) | (raw[FRAME_OFFSET + 3] << 24)) | 0;
        const next = (frame + 1) | 0;
        // 원문 선언 순서대로 다음 프레임의 단계를 먼저, 현재 프레임의 단계를 나중에 조회한다.
        const nextStage = need(hooks.stage)(dais, next);
        const stage = need(hooks.stage)(dais, frame);
        need(hooks.frame)(dais, next, 0);
        return stage === nextStage ? STAGE_HOLD_DELAY : 0;
    }
    if (event === DAIS_SACRIFICE_EVENT) return need(hooks.sacrificeAdvance)(dais);
    if (event === DAIS_RITUAL_EVENT && count) {
        need(hooks.ritual)(dais);
        return 0;
    }
    return need(hooks.damageable)(dais, event, count, payload);
}
