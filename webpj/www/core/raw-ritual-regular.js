// Rune·Dais 정기(Regular) 사건 분배의 공용 분기 본문이다(cpppj `o/RawRitualRegular.cpp/.h` 중 Rune/Dais 부분, 10.78 전용).
// 장기 Native(RawDaisRegular)와 독립 typed 입력이 같은 원문 분기 순서를 실행하도록 분기만 함수로 분리했다. 큰 의식 본문(진행·희생)은 호출자가 주는 훅이다.
//  - runRuneRegular(44a3a0): 사건 0x304 에서만 같은 소유자 다이스 검색 → 발견하면 전체 진행 → 룬 자기 삭제(flags 0x48)를 하고 -1 을 돌려준다. 그 밖의 사건은 payload 그대로 돌려준다.
//  - runDaisHandle(44b150): 0x305(프레임 +1 의 단계 비교·프레임 지정) / 0x306(희생 진행) / 0x307(count 가 0 이 아니면 의식 실행) / 그 밖은 공통 Damageable 로 보낸다.
//  - runAltarRegular(44a5e0): 제단 20(저항 진행)·21(저항 확인과 자동 해방)·22(역방향 진행)·23(무동작)·24(희생 진행과 최종 붕괴)를 분배하고 그 밖은 공통 Damageable 로 보낸다.
// 임의의 TYPE/free/dead 선검사를 덧붙이지 않는다. 제단 정기는 RawAltarSacrifice 가 훅(조회·난수·예약·음향)을 연결해 호출한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E367: 7-P3 RawDaisRegular 이식의 선행 분기로 추가, 같은 날 WB04-E368 에서 제단 정기 runAltarRegular 추가)
import { pathDirection } from './raw-path-animation.js';

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

// 제단 정기의 현재 저항 전역이다: 다음 주기에 더하는 지연 초(00532698)와 원문 난수/1000 과 비교하는 탈출 확률 정수(0053269c)이다. 세션에서 다시 정하지 않는다.
// 사용: const state = new AltarSacrificeState(); state.escapePercent = 40; (2026-11-13 WB04-E368)
export class AltarSacrificeState {
    // 다음 주기 덧셈 값(float32 3.0)이다.
    struggleSeconds = 3.0;
    // 난수(0..99999)/1000 의 몫과 비교하는 탈출 확률 정수이다.
    escapePercent = 40;
}

// 제단 정기에서 쓰는 float32 상수와 소리 선택표, 최종 삭제 flags 이다. (2026-11-13 WB04-E368)
const ALTAR_FINISHED_DELAY = 3.0;
const ALTAR_STRUGGLE_PAYLOAD = Math.fround(0.08);
const ALTAR_SACRIFICE_DESTROY = 0x400000;
const ALTAR_CHAINS = Object.freeze(['chainRattle23.wav', 'chainRattle24.wav', 'chainRattle25.wav', 'chainRattle26.wav', 'chainRattle27.wav']);
const ALTAR_STRUGGLES = Object.freeze(['priestStruggleFade01.wav', 'priestStruggleFade02.wav', 'priestStruggleFade03.wav']);
// 동맹 인덱스 상한(플레이어 1..8)이다. (2026-11-13 WB04-E368)
const ALTAR_PLAYER_COUNT = 8;

// DWORD 비트를 float32 로 읽는다. (2026-11-13 WB04-E368)
function altarFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 제단 정기 분기(44a5e0)이다. pool 은 10.78 SidPool, frames 는 188칸 FrameCodeTable, state 는 AltarSacrificeState 이다.
//  - hooks = { dais(altar)→다이스, priest(dais)→포획 사제 Form, boss()→bool, alliance(원래 owner, owner)→DWORD, release(altar), escapeFrame(dais, count), destroy(sid, flags),
//    advance(altar, delta, flags)→구간 끝 여부, frame(sid, frame, flags), reserveShared()→bool, addShared(sid, event, payload), randomInt(low, high), randomFloat(low, high),
//    sound(x, y, name, flags), removeAnimation(altar), damageable(sid, event, count, payload)→지연 }. 도달한 훅만 요구한다.
//  - 20/22: 제단이 방향 14(희생 중)일 때만 ±1 프레임 진행(역방향이 구간 끝에 닿으면 +1 로 되돌림)하고 아니면 0. 21: 자동 해방(권한·포획 사제·동맹이면 해방 후 -1) → 방향 15(해방됨)면 payload →
//    난수로 20/22 를 다시 예약(0.08)하고 소리 → 다음 지연(난수 0..2 + 저항 초). 23: payload. 24: 다이스 방향 10(빈 프레임)이면 권한 삭제(0x400000) 또는 payload,
//    진행하는 동안 이탈 프레임/붕괴 소리, 구간 끝이면 포획 삭제·애니메이션 제거·폭발 소리·다이스 K/제단 M 프레임 후 3.0.
// 사용: runAltarRegular(pool, frames, altar, 24, 0, 0.12, state, hooks). (2026-11-13 WB04-E368)
export function runAltarRegular(pool, frames, altar, event, count, payload, state, hooks) {
    // 원문 유효 주소 범위(1..capacity-1)만 판정한다.
    const valid = (sid) => sid > 0 && sid < pool.capacity;
    // 선택 시점의 현재 두 좌표를 읽는다.
    const position = (sid) => { const raw = pool.slot(sid); return [altarFloat(raw, 14), altarFloat(raw, 18)]; };
    // 현재 TYPE/프레임의 방향 코드이다(TYPE 이 표 밖이면 거부).
    const direction = (sid) => {
        const raw = pool.slot(sid);
        if (raw[10] >= frames.length) throw new RangeError('제단 현재 방향 TYPE 물리 범위');
        return pathDirection(frames[raw[10]], (raw[36] | (raw[37] << 8) | (raw[38] << 16) | (raw[39] << 24)) | 0);
    };
    // 저장한 대상의 현재 TYPE 에서 글자 구간의 첫 번호이다.
    const firstFrame = (sid, letter) => {
        const type = pool.slot(sid)[10];
        if (type >= frames.length) throw new RangeError('제단 현재 frame TYPE 물리 범위');
        return frames[type].run(letter).first;
    };
    let dais = need(hooks.dais)(altar);
    let center = [0, 0];
    const hasDais = valid(dais);
    if (hasDais) {
        const point = position(dais);
        center = [Math.fround(point[0] - ALTAR_FINISHED_DELAY), Math.fround(point[1] - ALTAR_FINISHED_DELAY)];
    }
    if (event === 21) {
        const captured = need(hooks.priest)(dais);
        // 권한이 있고 포획 사제가 있으며 원래 owner 와 다이스 owner 가 동맹이면 자동 해방하고 더 예약하지 않는다.
        if (need(hooks.boss)() && valid(captured)) {
            const oldOwner = pool.slot(captured)[12] & 127;
            const owner = pool.slot(dais)[34];
            if (oldOwner > ALTAR_PLAYER_COUNT || owner > ALTAR_PLAYER_COUNT) throw new RangeError('제단 자동 해방 동맹 인덱스');
            if (need(hooks.alliance)(oldOwner, owner)) {
                need(hooks.release)(altar);
                return -1;
            }
        }
        if (direction(altar) === 15) return payload;
        let nextEvent = 20;
        if (Math.trunc(need(hooks.randomInt)(0, 100000) / 1000) < state.escapePercent) {
            nextEvent = 22;
            need(hooks.advance)(altar, -1, 0);
        }
        if (need(hooks.reserveShared)()) need(hooks.addShared)(altar, nextEvent, ALTAR_STRUGGLE_PAYLOAD);
        if (hasDais && direction(dais) !== 10) {
            const chain = need(hooks.randomInt)(0, 5);
            if (chain < 0 || chain >= ALTAR_CHAINS.length) throw new RangeError('제단 chain 난수 범위');
            need(hooks.sound)(center[0], center[1], ALTAR_CHAINS[chain], 0);
            const struggle = Math.trunc(need(hooks.randomInt)(0, 3000) / 1000);
            if (struggle < 0 || struggle >= ALTAR_STRUGGLES.length) throw new RangeError('제단 struggle 난수 범위');
            need(hooks.sound)(center[0], center[1], ALTAR_STRUGGLES[struggle], 0);
        }
        return Math.fround(need(hooks.randomFloat)(0, 2) + Math.fround(state.struggleSeconds));
    }
    if (event === 20 || event === 22) {
        if (direction(altar) !== 14) return 0;
        const delta = event === 20 ? 1 : -1;
        if (need(hooks.advance)(altar, delta, 0)) {
            if (delta === -1) need(hooks.advance)(altar, 1, 0);
            return 0;
        }
        return payload;
    }
    if (event === 23) return payload;
    if (event !== 24) return need(hooks.damageable)(altar, event, count, payload);
    dais = need(hooks.dais)(altar);
    if (!valid(dais)) throw new Error('제단24 원본의 다이스0 프레임읽기 경계');
    if (direction(dais) === 10) {
        if (need(hooks.boss)()) {
            need(hooks.destroy)(dais, ALTAR_SACRIFICE_DESTROY);
            return -1;
        }
        return payload;
    }
    if (!need(hooks.advance)(altar, 1, 0)) {
        if (count < 3 || Math.trunc(need(hooks.randomInt)(0, 100000) / 1000) < 50) {
            if (valid(dais)) need(hooks.escapeFrame)(dais, count);
        }
        if (need(hooks.randomFloat)(0, 1) < payload) {
            const point = position(altar);
            need(hooks.sound)(point[0], point[1], 'altarBurnCollapse.wav', 0);
        }
        return payload;
    }
    dais = need(hooks.dais)(altar);
    if (!valid(dais)) return payload;
    if (need(hooks.boss)()) {
        const captured = need(hooks.priest)(dais);
        if (captured) need(hooks.destroy)(captured, 8);
    }
    need(hooks.removeAnimation)(altar);
    const point = position(altar);
    need(hooks.sound)(point[0], point[1], 'explosion.wav', 0);
    need(hooks.frame)(dais, firstFrame(dais, 'K'), 0);
    need(hooks.frame)(altar, firstFrame(altar, 'M'), 0);
    return ALTAR_FINISHED_DELAY;
}
