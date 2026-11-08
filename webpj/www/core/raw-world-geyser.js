// 10.78 전체 세계 간헐천 생성 부모(00486440)의 장식·난수·예산·분할/전체 요청 순서를 옮긴 모듈이다(cpppj `o/RawWorldGeyser.cpp/.h`, 10.78 전용).
// 같은 실제 Session 의 옵션과 건설 몸체를 조립하는 생성 부모이며 로컬 생성 하나로 대신하지 않는다. 별도 설정 사본을 두지 않고 현재 battle/옵션/타입을 효과마다 다시 읽는다.
//  - generate(): 로그("Make Local Geysers\n") → 로컬 생성(00486360) → 전투이고 옵션 9 가 0 이 아니면 장식 → predictableGeysers(기본 1)가 0 이면 벽시각 시드로 난수 → 예산·밀도·간격으로 요청량 계산 → 옵션 18 에 따른 분할/전체 요청.
//    · 장식(Local 뒤 현재 장식 타입 4 개로 여섯 항목 표를 한 번 고정): 벽시각 시드 난수로 `옵션9 × 50`(반복마다 현재 옵션 9 를 다시 읽음)번 `표[난수(6)]` 타입을 고르고 Monument(네 번째 장식 타입)는 열 번째 초과 요청을 건너뛰며 전체 범위(whole)에 배치(decorate)한다.
//    · 예산: 기본 예산 b, 밀도 d(−1/0/1/2)에서 보정 `(밀도 0.1/0.2 가산 +1) × b` 를 ftol 절삭, 간격 s(3/4/6)에서 크기 가산(−0.2/−0.1/+0.1)을 더해 다시 절삭한 값을 /2/4 한 corner, edge = corner+1, center = edge/2.
//    · 옵션 18 == 2: 아홉 칸(3×3 분할)을 차례로 중앙 center·네 모서리 corner·네 변 edge 로 요청한다(중앙은 허용 지형 0). 분할을 시작한 뒤 옵션이 바뀌어도 아홉 요청을 끝낸다.
//    · 옵션 18 == 1: 밀도 getter 를 한 번 호출(결과 미사용)한 뒤 밀도 −1→b/2, 0→b, 1→(b/2)×3, 2→b<<1 로 수량을 정하고 half = 수량/2 로 전체 범위에 한 번(허공 인수=demo, 지형 인수=demo==0), 나머지(예산−half)를 허공만으로 한 번 더 요청한다.
//  - worldGeyserDensity(selection): 옵션 0~3 을 밀도 −1/0/1/2 로 읽고 표 밖은 오류이다. timeSeed(seconds): 벽시각 시드 `(value>>4) + value×33`(value = 시각×1013 의 ftol 하위 DWORD, DWORD 감김)이며 RawFortFinishOwner::TimeSeed 와 같은 식이다.
// state = { battle(live), options(87바이트, 9/18 을 반복 중 재조회), decorationTypes(live 4개) }, hooks = { log, local, configInt(key, default), wallSeconds, budget, density, spacing, demo, whole, partition(cell, grid),
// decorate(random, rectangle, region, quantity, type, force), populate(quantity, random, rectangle, { excludePlayers, allowLand, allowVoid }) }. 도달한 하위가 없으면 앞선 효과를 유지한 채 오류이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E107: 세계 간헐천 생성 부모 이식으로 추가)
import { GameRandom } from './random.js';

// 원본 세계 시드와 Monument 최대 요청 수, 옵션 9 의 반복 배율, 시각 배율/계수, ftol 경계이다. (2026-11-08 WB04-E107)
const PREDICTABLE_SEED = 0xfed;
const SPECIAL_LIMIT = 10;
const DECORATIONS_PER_OPTION = 50;
const TIME_SCALE = 1013;
const SEED_FACTOR = 33;
const INT64_BOUNDARY = 2 ** 63;
const OPTION_BYTES = 87;

// 실제 도달한 하위가 없으면 앞선 효과를 유지한 채 실패한다. (2026-11-08 WB04-E107)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('세계 간헐천 생성의 실제 하위 미연결');
    return hook;
}

// x87 CRT 의 signed64 절삭 하위 DWORD(signed32)를 반환한다. (2026-11-08 WB04-E107)
function truncate(value) {
    if (!Number.isFinite(value) || value < -INT64_BOUNDARY || value >= INT64_BOUNDARY) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 벽시각 시드: SAR 의 음수 반올림과 DWORD 감김을 그대로 유지하며 글로벌 게임 난수 시드를 바꾸지 않는다. (2026-11-08 WB04-E107)
export function timeSeed(wallSeconds) {
    const value = truncate(wallSeconds * TIME_SCALE);
    return ((value >> 4) + Math.imul(value, SEED_FACTOR)) >>> 0;
}

// 밀도와 간격은 서로 다른 옵션이며 현재 BYTE 를 각각 조회한다. 원문 0041ca40 의 실제 스택 표이다. (2026-11-08 WB04-E107)
export function worldGeyserDensity(selection) {
    const values = [-1, 0, 1, 2];
    if (!(selection >= 0 && selection < values.length)) throw new RangeError('세계 간헐천 밀도의 원문 표 범위');
    return values[selection];
}

// 전체 생성 부모이다. (2026-11-08 WB04-E107)
export class RawWorldGeyser {
    // 실제 현재 전역(battle/options/decorationTypes)을 빌리며 생성 시작의 값으로 고정하지 않는다. (2026-11-08 WB04-E107)
    #state;
    // 실제 도달한 원문 하위만 호출하는 같은 Session 장치이다. (2026-11-08 WB04-E107)
    #hooks;

    // 고정 판본/물리 폭만 확인하고 하위는 선택된 지점에서 요구한다. (2026-11-08 WB04-E107)
    constructor(edition, state, hooks = {}) {
        if (edition !== 'patch1078' || state.options.length !== OPTION_BYTES) throw new Error('세계 간헐천 부모의 PATCH 87BYTE 계약 오류');
        this.#state = state;
        this.#hooks = hooks;
    }

    // 전체 부모는 boss/fort 관문을 새로 추가하지 않는다. 각 실제 Populate 가 현재 권한을 판정한다. (2026-11-08 WB04-E107)
    generate() {
        const hooks = this.#hooks;
        const state = this.#state;
        need(hooks.log)('Make Local Geysers\n');
        need(hooks.local)();
        const random = new GameRandom(PREDICTABLE_SEED);
        if (state.battle && state.options[9] !== 0) {
            // Local 뒤 여섯 선택 항목만 한 번 고정한다.
            const types = [state.decorationTypes[0], state.decorationTypes[0], state.decorationTypes[0], state.decorationTypes[1], state.decorationTypes[2], state.decorationTypes[3]];
            const decoration = new GameRandom(0);
            decoration.setState(timeSeed(need(hooks.wallSeconds)()));
            let special = 0;
            // 배치 하위가 바꾼 현재 옵션 9 로 다음 반복 상한을 읽는다.
            for (let index = 0; index < state.options[9] * DECORATIONS_PER_OPTION; index += 1) {
                const type = types[decoration.next(6)];
                if (type === state.decorationTypes[3]) {
                    special += 1;
                    if (special > SPECIAL_LIMIT) continue;
                }
                const rectangle = need(hooks.whole)();
                need(hooks.decorate)(decoration, rectangle, 0, 1, type, 1);
            }
        }
        if (need(hooks.configInt)('predictableGeysers', 1) === 0) random.setState(timeSeed(need(hooks.wallSeconds)()));
        const base = need(hooks.budget)();
        const density = need(hooks.density)();
        const original = base;
        // ROM 00501960/00505de0/00500460 의 밀도 가산이다.
        const densityAddition = density === 1 ? 0.1 : density === 2 ? 0.2 : 0.0;
        const adjusted = truncate((densityAddition + 1.0) * original);
        const spacing = need(hooks.spacing)();
        // ROM 005066b8/005066d0/00501960/00500460 의 크기 가산이다.
        const sizeAddition = spacing === 3 ? -0.2 : spacing === 4 ? -0.1 : spacing === 6 ? 0.1 : 0.0;
        const corner = Math.trunc(Math.trunc(truncate(sizeAddition * original + adjusted) / 2) / 4);
        const edge = (corner + 1) | 0;
        const center = Math.trunc(edge / 2);
        if (state.options[18] === 2) {
            // 원본은 분할 시작을 선택한 뒤 옵션 18 이 바뀌어도 아홉 요청을 끝낸다.
            for (let cell = 1; cell <= 9; cell += 1) {
                const middle = cell === 5;
                const cornerCell = cell === 1 || cell === 3 || cell === 7 || cell === 9;
                const rectangle = need(hooks.partition)(cell, 3);
                need(hooks.populate)(middle ? center : cornerCell ? corner : edge, random, rectangle, { excludePlayers: 1, allowLand: middle ? 0 : 1, allowVoid: 1 });
            }
        }
        if (state.options[18] !== 1) return;
        need(hooks.density)();
        let quantity = 0;
        if (need(hooks.density)() === -1) quantity = Math.trunc(need(hooks.budget)() / 2);
        else if (need(hooks.density)() === 0) quantity = need(hooks.budget)();
        else if (need(hooks.density)() === 1) quantity = Math.imul(Math.trunc(need(hooks.budget)() / 2), 3);
        else if (need(hooks.density)() === 2) quantity = (need(hooks.budget)() << 1) | 0;
        else quantity = need(hooks.budget)();
        const half = Math.trunc(quantity / 2);
        let rectangle = need(hooks.whole)();
        const allowVoid = need(hooks.demo)();
        const allowLand = need(hooks.demo)() === 0 ? 1 : 0;
        need(hooks.populate)(half, random, rectangle, { excludePlayers: 1, allowLand, allowVoid });
        rectangle = need(hooks.whole)();
        const remaining = (need(hooks.budget)() - half) | 0;
        need(hooks.populate)(remaining, random, rectangle, { excludePlayers: 1, allowLand: 0, allowVoid: 1 });
    }
}
