// 일반 중앙섬(owner 0·원점 배치)을 만드는 부모를 옮긴 모듈이다(cpppj `o/RawCentralIsland.cpp/.h`, 원본 10.78 0046e140). 생성·중심 기록·거주 구역·군도 분리를 한 순서로 묶는다.
// 사용: new RawCentralIsland(pool, world, islands, frames, puzzlePieceType, globals, hooks).build(record) — record 는 Zone 6바이트 영역 기록(패턴|회전, 플래그, 위치, 씨앗 …)이다.
//  - globals = { battle(), boss(), terrainEnabled(), archipelago(), position: Float32Array(2), unassignedTerritory() }: 현재 전투 모드·권한(boss)·terrainEnabled·archipelago 전역 getter,
//    장기 중심 X/Y 를 쓰는 float32 두 칸(00568afc/00568b00), 현재 invalidRegion(127, 00540a30; owner 0 과 구별한다).
//  - hooks = { assertion(식, 줄), placement(record) → { left, top, right, bottom }, generate(state, island), residence(island), partition(), graph(reset), builderAssertion(종류), decoderDiagnostics }.
// 순서: 진단 4개(inBattleMode·위치 0·빈 섬 목록·owner 0; 진단 뒤 복귀하면 현재 상태로 계속한다) → 원점 배치 범위 → 중심 기록(청크 ×16 의 DWORD 감김 float32 두 값의 평균) → RawIslandBuilder.place(record, 현재 invalidRegion, 위치 무시)
// → 지형 생성 → 권한이고 terrainEnabled 가 0 일 때만 거주 구역 → archipelago 가 0 이 아니면 군도 분리 → Graph 전체 재구성(0). 앞선 효과는 실패해도 되돌리지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1104: 12차 스커미시 중앙섬 이식)
import { RawIslandBuilder } from './raw-island-builder.js';

// 선택된 하위 장치만 필요한 위치에서 요구한다. (2026-11-20 WB04-E1104)
function need(value, label) {
    if (typeof value !== 'function') throw new TypeError(`중앙섬 생성의 ${label} 미연결`);
    return value;
}
// 청크 번호를 셀 경계 float32 로 바꾼다(0040d900: DWORD 감김 16배 뒤 float32 저장). (2026-11-20 WB04-E1104)
const centralCell = (chunk) => Math.fround((chunk << 4) | 0);

// 일반 중앙섬 부모이다. 생성자는 10.78 판본과 풀 동일성만 확인하고 모든 자료는 build 동기 호출보다 오래 살아야 한다. (2026-11-20 WB04-E1104)
export class RawCentralIsland {
    #pool;
    #world;
    #islands;
    #frames;
    #puzzlePieceType;
    #globals;
    #hooks;

    constructor(pool, world, islands, frames, puzzlePieceType, globals, hooks) {
        if (pool?.edition !== 'patch1078' || islands?.pool !== pool) throw new TypeError('중앙섬의 실제 10.78 Pool 불일치');
        this.#pool = pool;
        this.#world = world;
        this.#islands = islands;
        this.#frames = frames;
        this.#puzzlePieceType = puzzlePieceType;
        this.#globals = globals;
        this.#hooks = hooks;
    }

    // 값으로 전달된 실제 Zone 6바이트 영역 기록으로 일반 중앙섬을 만든다. (2026-11-20 WB04-E1104)
    build(record) {
        const globals = this.#globals, hooks = this.#hooks;
        const state = { bounds: { left: -1, top: -1, right: -1, bottom: -1 }, owner: 0, decoderDiagnostics: hooks.decoderDiagnostics ?? null };
        if (!globals.battle()) need(hooks.assertion, '진단')('inBattleMode', 0xea);
        if (record[2]) need(hooks.assertion, '진단')('terr.xChunkCoord == 0 && terr.yChunkCoord == 0', 0xeb);
        if (this.#islands.count) need(hooks.assertion, '진단')('islandList.getTotal() == 0', 0xec);
        if (state.owner) need(hooks.assertion, '진단')('playerId == INVALID_PLAYER_ID', 0xed);
        state.bounds = need(hooks.placement, '원점 배치')(record);
        // 장기 중심은 배치 범위의 두 청크 경계 float32 값을 double 로 더해 절반으로 한 float32 이다.
        globals.position[0] = Math.fround((centralCell(state.bounds.right) + centralCell(state.bounds.left)) * 0.5);
        globals.position[1] = Math.fround((centralCell(state.bounds.bottom) + centralCell(state.bounds.top)) * 0.5);
        const builder = new RawIslandBuilder(this.#world, this.#world, this.#islands, this.#frames, this.#puzzlePieceType, state, need(hooks.builderAssertion, 'builder 진단'));
        const island = builder.place(record, globals.unassignedTerritory() >>> 0, true); // 00540a30 의 현재 127 을 쓰며 builder owner 0 으로 대신하지 않는다.
        need(hooks.generate, '지형 생성')(state, island | 0);
        if (globals.boss() && !globals.terrainEnabled()) need(hooks.residence, '거주 구역')(island | 0);
        if (globals.archipelago()) need(hooks.partition, '군도 분리')();
        need(hooks.graph, 'Graph 재구성')(false);
    }
}
