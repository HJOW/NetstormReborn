// Early Mission 1(튜토리얼 1)의 실제 단계 조건을 공유 미션 게이트의 Advance 에 전달하는 모듈이다(cpppj `o/RawTutorialOne.cpp/.h`, PATCH 004c3a20, 10.78 전용).
//  - handle(event): 이벤트에서 modifier/release 상위 비트를 지운 명령(`event & 0x03ffffff`)과 현재 단계(signed char, 공유 단계 전역)로 한 분기만 실행한다.
//    'A': F4 명령(homeCommand)이거나 다리 부류(4) 제작 수가 0 보다 크면 Advance, reclaim 명령이면 1 을 반환한다(나머지는 0). 'B': 다리 제작 수 > 7 이면 Advance. 'C': > 18 이면 Advance 뒤 간헐천 생성(Advance 반환과 무관).
//    'D': 현재 플레이어 신전의 Graph 를 한 번만 구한 뒤 최근 생성 간헐천 목록(최대 5개)을 현재 signed count 만큼 순서대로 보며 Graph 가 같은 항목마다 Advance(목록 중간에 count 가 바뀌어도 매 조건에서 다시 읽음).
//    'E'/'F': 전투 중이고 현재 SP(단정도)가 200/600 이상이면 Advance. 그 밖의 단계는 아무것도 하지 않는다(NaN 은 어느 비교도 거짓).
//  - 단락 평가·중복 Advance·SP 비교 순서를 원본대로 유지한다. UI 프레임 게이트·건설·수확 몸체 자체는 호출자가 연결한다.
// state = { stage, localPlayer, battleShown, homeCommand, reclaimCommand }(미션 게이트/클라이언트의 현재 전역을 읽고 쓰는 속성, 호출 중 바뀐 값도 다시 읽음), players = 같은 9×0xb4 raw Player 표(Uint8Array),
// geysers = 00595094/98 의 최근 생성 최대 5개 목록(SquidPostPopList, 전체 간헐천 목록과 다르다), hooks = { builtForGenus(4), graphFor(sid), money(owner) → float, advance(), spawnGeysers() }.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E237: Early1 단계 판정기 이식으로 추가)

// 원본이 이벤트의 modifier/release 상위 비트를 제거하는 마스크, 제작 수를 합산할 다리 부류, 실제 Player 기록 폭과 현재 신전 SID DWORD 위치, 단계 SP 기준이다. (2026-11-10 WB04-E237)
const COMMAND_MASK = 0x03ffffff;
const BRIDGE_GENUS = 4;
const PLAYER_BYTES = 0xb4;
const TEMPLE_OFFSET = 0x7c;
const PLAYER_COUNT = 9;
const SP_STAGE_E = 200;
const SP_STAGE_F = 600;

// 비정렬 Player DWORD 를 little endian 으로 읽는다. (2026-11-10 WB04-E237)
function readPlayer(bytes, offset) {
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// Early1 의 PATCH 004c3a20 전체 분기이다. (2026-11-10 WB04-E237)
export class RawTutorialOne {
    // 미션 게이트/클라이언트의 현재 전역과 같은 Player 표·최근 간헐천 목록·실제 집계/Graph/SP/단계/생성 경계이다. 상태와 목록은 복제하지 않는다.
    #state;
    #players;
    #geysers;
    #hooks;

    // 실제 저장소의 폭과 필수 효과를 확인하며 단계/목록/게임 상태를 변경하지 않는다. (2026-11-10 WB04-E237)
    constructor(state, players, geysers, hooks = {}) {
        if (!state || players?.length !== PLAYER_COUNT * PLAYER_BYTES || !geysers || ['builtForGenus', 'graphFor', 'money', 'advance', 'spawnGeysers'].some((name) => typeof hooks[name] !== 'function')) {
            throw new TypeError('튜토리얼1 실제 Player/목록/하위 연결 오류');
        }
        this.#state = state;
        this.#players = players;
        this.#geysers = geysers;
        this.#hooks = hooks;
    }

    // 같은 프레임에서 단계가 바뀌더라도 원본 switch 가 선택한 한 분기만 실행한다. 'A' 에서 reclaim 명령만 1 을 반환한다. (2026-11-10 WB04-E237)
    handle(event) {
        const state = this.#state;
        const hooks = this.#hooks;
        const command = (event & COMMAND_MASK) >>> 0; // A 분기의 두 비교가 쓰는 같은 입력이다.
        switch (state.stage) {
        case 0x41: // 'A'
            if (command === (state.homeCommand >>> 0) || hooks.builtForGenus(BRIDGE_GENUS) > 0) hooks.advance();
            if (command === (state.reclaimCommand >>> 0)) return 1;
            break;
        case 0x42: // 'B'
            if (hooks.builtForGenus(BRIDGE_GENUS) > 7) hooks.advance();
            break;
        case 0x43: // 'C'
            if (hooks.builtForGenus(BRIDGE_GENUS) > 18) { hooks.advance(); hooks.spawnGeysers(); }
            break;
        case 0x44: { // 'D'
            const player = state.localPlayer >>> 0;
            if (player >= PLAYER_COUNT) throw new RangeError('튜토리얼1 현재 Player 범위');
            const temple = readPlayer(this.#players, player * PLAYER_BYTES + TEMPLE_OFFSET); // 간헐천 순회 전에 신전 Graph 를 한 번만 구한다.
            const graph = hooks.graphFor(temple); // Graph 0/254 도 원본은 그대로 비교한다.
            // 매 조건에서 현재 signed count 를 확인하고 모든 일치 항목에 Advance 를 요청한다.
            for (let index = 0; index < (this.#geysers.readCount() | 0); index += 1) {
                const entries = this.#geysers.storage();
                if (index >= entries.length) throw new RangeError('튜토리얼1 간헐천 목록 물리 범위');
                if (hooks.graphFor(entries[index]) === graph) hooks.advance();
            }
            break;
        }
        case 0x45: // 'E'
            if (state.battleShown && hooks.money(state.localPlayer) >= SP_STAGE_E) hooks.advance();
            break;
        case 0x46: // 'F'
            if (state.battleShown && hooks.money(state.localPlayer) >= SP_STAGE_F) hooks.advance();
            break;
        default: break;
        }
        return 0;
    }
}
