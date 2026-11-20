// 일반 전투 참가자 한 명의 Territory 를 실제 월드에 배치하는 부모를 옮긴 모듈이다(cpppj `o/RawBattleIsland.cpp/.h`, 원본 10.78 0046d9b0 → 0046da60 → 0046e2c0). 파일 수신·State 전환은 소유하지 않는다.
// 사용: new RawBattleIsland(pool, world, islands, frames, puzzlePieceType, hooks).build(owner).
//  - hooks = { assertion(줄), territory(owner, output120), placement(owner, record) → { left, top, right, bottom }, region(owner, island), generate(state, island), boss() → bool,
//    objects(owner, firstTerritory, endTerritory), graph(reset), builderAssertion(종류), decoderDiagnostics }.
// 순서: owner 0 이면 진단(0x10f, 복귀하면 계속) → 현재 Template Territory 20×6바이트 읽기 → 레코드 플래그 바이트(+1)의 0x04 비트가 켜진 첫 영역(없으면 0번) 선택 → 그 영역의 Player 별 배치 범위
// → RawIslandBuilder.place(record, 선택 번호, 위치 무시) → 현재 Player+0x78 에 섬 번호 쓰기 → 지형 생성 → 권한이면 그 영역 한 개의 객체 적재 → Graph 전체 재구성(0). 실패 이전 효과를 되돌리지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1104: 12차 스커미시 시작 섬 이식)
import { RawIslandBuilder } from './raw-island-builder.js';

// 선택된 하위 장치만 필요한 위치에서 요구한다. (2026-11-20 WB04-E1104)
function need(value, label) {
    if (typeof value !== 'function') throw new TypeError(`전투섬 생성의 ${label} 미연결`);
    return value;
}
// Territory 한 기록의 바이트 수와 한 Template 의 영역 수이다. (2026-11-20 WB04-E1104)
const RECORD_BYTES = 6;
const TERRITORY_COUNT = 20;

// 전투섬 부모이다. 생성자는 풀 동일성과 10.78 판본을 확인한다(CD 10.72 분기는 웹 범위 밖). (2026-11-20 WB04-E1104)
export class RawBattleIsland {
    #pool;
    #world;
    #islands;
    #frames;
    #puzzlePieceType;
    #hooks;

    constructor(pool, world, islands, frames, puzzlePieceType, hooks) {
        if (islands?.pool !== pool || pool?.edition !== 'patch1078') throw new TypeError('전투섬의 실제 10.78 Pool 불일치');
        this.#pool = pool;
        this.#world = world;
        this.#islands = islands;
        this.#frames = frames;
        this.#puzzlePieceType = puzzlePieceType;
        this.#hooks = hooks;
    }

    // 참가자 owner 의 전투 영역을 월드에 배치한다. 첫 0x04 플래그 영역 또는 0번을 쓴다. (2026-11-20 WB04-E1104)
    build(owner) {
        const hooks = this.#hooks;
        const state = { bounds: { left: -1, top: -1, right: -1, bottom: -1 }, owner: owner >>> 0, decoderDiagnostics: hooks.decoderDiagnostics ?? null };
        if (!state.owner) need(hooks.assertion, '진단')(0x10f);
        const records = new Uint8Array(RECORD_BYTES * TERRITORY_COUNT);
        need(hooks.territory, 'Territory 읽기')(state.owner, records);
        let selected = 0;
        // 레코드의 플래그 바이트(+1)에서 0x04 비트가 켜진 첫 영역을 찾고 없으면 원문처럼 0번을 유지한다.
        for (let index = 0; index < TERRITORY_COUNT; index += 1) {
            if (records[index * RECORD_BYTES + 1] & 4) { selected = index; break; }
        }
        const record = records.subarray(selected * RECORD_BYTES, (selected + 1) * RECORD_BYTES);
        state.bounds = need(hooks.placement, 'Player 배치')(state.owner, record);
        const builder = new RawIslandBuilder(this.#world, this.#world, this.#islands, this.#frames, this.#puzzlePieceType, state, need(hooks.builderAssertion, 'builder 진단'));
        const island = builder.place(record, selected, true);
        need(hooks.region, '섬 번호 기록')(state.owner, island);
        need(hooks.generate, '지형 생성')(state, island | 0);
        if (need(hooks.boss, '권한 조회')()) need(hooks.objects, '객체 적재')(state.owner, selected, selected + 1);
        need(hooks.graph, 'Graph 재구성')(false);
    }
}
