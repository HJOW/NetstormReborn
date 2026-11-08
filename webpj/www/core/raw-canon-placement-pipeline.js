// 일반 타입/패턴 배치의 모든 복원 모듈을 같은 현재 표면/공통 전역 상태에 연결하는 파이프라인이다(cpppj `o/RawCanonPlacementPipeline.cpp/.h`, 10.78).
//  - createCanonPlacementPipelineState(): 배치(placement)·미리보기(preview)·충돌(collision)·지형(terrain)·권한(permission)·관계(relations)·Player 기준점(anchor)·기하(geometry: patternTypes 8칸 + 진단 장치)·
//    픽셀 모양(shape: 배율) 상태를 원본 초기값으로 한 묶음 만든다.
//  - new RawCanonPlacementPipeline(pool, hash, data, bookkeeping, additional, contained, state): data = { types, frames, shapes, hotspots, spots, islands, regions } 이고 모든 자료/장부/상태는 호출자가 소유해
//    이 객체보다 오래 살아야 한다. 현재 표면(surfaces)과 Player 기준점 지도는 공간 해시의 0단계 배열을 직접 공유한다(hash.view(0)). 모듈은 anchor → permission → surrounding → relations → terrain → geometry → shape → collision →
//    preview → placement 순서로 만들고, 후보/주변/최종 정책은 모두 복원한 실제 모듈에 연결한다.
//  - mayPlace(query): 공통 전역의 중복 표현(편집기·동맹 사용·관계 표·그래프 활성·noIsland 타입 번호)을 현재 값으로 맞춘 뒤 실제 접두부터 최종 반환까지 실행한다.
// 이 모듈은 DOM 을 쓰지 않는다. 10.72(CD) 분기는 범위 밖이다. (2026-11-09 WB04-E154: 일반 배치 파이프라인 이식으로 추가)
import { PlayerPlacementAnchorState, RawPlayerPlacementAnchor, makePlayerAnchorQuery } from './raw-player-placement-anchor.js';
import { RawCanonPixelShape } from './raw-canon-pixel-shape.js';
import { RawCanonPlacement, createPlacementState, makeCanonShapePlacementHooks } from './raw-canon-placement.js';
import { RawCanonPlacementCollision, createCanonPlacementCollisionState, makeCanonGeometryCollisionHooks, makeCanonPlacementCollisionHooks } from './raw-canon-placement-collision.js';
import { RawCanonPlacementGeometry } from './raw-canon-placement-geometry.js';
import { RawCanonPlacementPermission, createCanonPlacementPermissionState, makeCanonPermissionTerrainHooks } from './raw-canon-placement-permission.js';
import { RawCanonPlacementPreview, createCanonPlacementPreviewState, makeCanonGeometryPreviewHooks, makeCanonPlacementPreviewHooks } from './raw-canon-placement-preview.js';
import { RawCanonPlacementRelations, createCanonPlacementRelationsState, makeCanonRelationsTerrainHooks } from './raw-canon-placement-relations.js';
import { RawCanonPlacementSurrounding, makeCanonSurroundingTerrainHooks } from './raw-canon-placement-surrounding.js';
import { RawCanonPlacementTerrain, createCanonPlacementTerrainState, makeCanonTerrainCollisionHooks, makeCanonTerrainGeometryHooks } from './raw-canon-placement-terrain.js';

// 파이프라인이 모듈들에 나눠 주는 상태 묶음을 원본 초기값으로 만든다(픽셀 배율의 초기값은 16×11). (2026-11-09 WB04-E154)
export function createCanonPlacementPipelineState() {
    return {
        placement: createPlacementState(),
        preview: createCanonPlacementPreviewState(),
        collision: createCanonPlacementCollisionState(),
        terrain: createCanonPlacementTerrainState(),
        permission: createCanonPlacementPermissionState(),
        relations: createCanonPlacementRelationsState(),
        anchor: new PlayerPlacementAnchorState(),
        geometry: { patternTypes: Array(8).fill(0), decoderDiagnostics: null },
        shape: { scaleX: 16, scaleY: 11 }
    };
}

// 일반 타입/패턴 배치의 전체 MayPlace 파이프라인이다. 내부 훅이 모듈을 참조하므로 한 번 만든 객체를 그대로 쓴다. (2026-11-09 WB04-E154)
export class RawCanonPlacementPipeline {
    #state;
    #placement;

    // 모든 자료/장부/상태는 호출자가 소유한다. 현재 표면은 해시의 0단계 배열을 직접 공유한다. (2026-11-09 WB04-E154)
    constructor(pool, hash, data, bookkeeping, additional, contained, state) {
        if (!data || !state || typeof hash?.view !== 'function') throw new TypeError('일반 배치 파이프라인 자료/상태 누락');
        const surfaces = hash.view(0);
        // 후보/주변/최종 정책은 모두 복원한 실제 모듈에 연결하고 현재 표면은 해시와 공유한다.
        const anchor = new RawPlayerPlacementAnchor(pool, data.types, surfaces, bookkeeping, additional, contained, state.anchor);
        const permission = new RawCanonPlacementPermission(pool, state.permission, makePlayerAnchorQuery(pool, anchor));
        const surrounding = new RawCanonPlacementSurrounding(pool, data.types, data.frames, data.islands, data.spots, permission);
        const relations = new RawCanonPlacementRelations(pool, data.types, surfaces, data.regions, state.relations, state.placement, permission);
        const terrain = new RawCanonPlacementTerrain(pool, data.types, data.frames, data.islands, state.terrain,
            makeCanonRelationsTerrainHooks(pool, relations, makeCanonSurroundingTerrainHooks(pool, surrounding, makeCanonPermissionTerrainHooks(pool, permission, {}))));
        const geometry = new RawCanonPlacementGeometry(pool, data.types, data.frames, state.geometry);
        const shape = new RawCanonPixelShape(pool, data.types, data.frames, data.shapes, data.hotspots, state.geometry, state.shape);
        const collision = new RawCanonPlacementCollision(pool, hash, data.types, state.collision, state.preview, makeCanonGeometryCollisionHooks(pool, geometry,
            (query) => makeCanonTerrainGeometryHooks(terrain.pool, terrain, query),
            makeCanonTerrainCollisionHooks(pool, terrain, {})));
        const preview = new RawCanonPlacementPreview(pool, data.types, data.spots, surfaces, state.preview,
            makeCanonGeometryPreviewHooks(pool, geometry, makeCanonPlacementCollisionHooks(pool, collision, {})));
        this.#state = state;
        this.#placement = new RawCanonPlacement(pool, data.types, state.placement, makeCanonShapePlacementHooks(pool, shape, makeCanonPlacementPreviewHooks(pool, preview, {})));
    }

    // 서버 건설 요청 어댑터가 같은 풀을 사용하는지 확인하도록 연결된 실제 풀을 돌려준다. (2026-11-09 WB04-E154)
    get pool() { return this.#placement.pool; }

    // 원본 공통 관계/그래프/noIsland 전역이 모듈별로 어긋나지 않게 같은 값을 공급한 뒤 접두부터 최종 반환까지 실행한다. (2026-11-09 WB04-E154)
    mayPlace(query) {
        const state = this.#state;
        state.preview.editor = state.permission.editor;
        state.preview.useAlliances = state.permission.useAlliances;
        state.preview.alliances.set(state.permission.alliances);
        state.anchor.graphReady = state.permission.graphReady;
        state.terrain.noIslandType = state.geometry.patternTypes[3];
        return this.#placement.mayPlace(query);
    }
}
