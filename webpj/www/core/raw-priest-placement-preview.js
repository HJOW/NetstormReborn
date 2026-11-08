// 사제 배치의 기존 요청 계약(타입, x, y, flags, owner, mode)을 일반 타입 미리보기(E152 RawCanonPlacementPreview)에 연결한 모듈이다(cpppj `o/RawPriestPlacementPreview.cpp/.h`, 10.78).
//  - 사제와 일반 배치는 같은 12×12 차단 배열/관계 전역(createCanonPlacementPreviewState)을 공유한다.
//  - new RawPriestPlacementPreview(pool, types, spots, surfaceMap, state, hooks): hooks = { bounds(priestQuery) → 정수 사각형, inspectCollisions(priestQuery, localOwner) → bool } 둘 다 필수이며 비어 있으면 생성 시 거부한다.
//    자료 수명과 지도/타입/판본 계약은 일반 미리보기에서 검사한다. 콜백에는 일반 요청을 원래 사제 여섯 값 인자로 되돌려 전달한다.
//  - inspect(query, localOwner): 사제의 별도 첫 인자는 기존 원본 호출처럼 타입 번호이며 로컬 비교를 다시 수행하지 않는다(접두가 캡처한 값을 사용).
//  - makePriestPlacementPreviewHooks(pool, preview, hooks): 사제 접두의 후반 경계(inspectGeometry)만 실제 미리보기로 바꾸고 기존 모양 조회는 유지한다.
// 10.72(CD)의 선행 linear spot 읽기와 y=254 조기 거부 부재는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E157: 사제 배치 미리보기 이식으로 추가)
import { RawCanonPlacementPreview } from './raw-canon-placement-preview.js';

// 공통 요청에서 사제 API 의 원래 여섯 값 인자를 복구한다. (2026-11-09 WB04-E157)
function priestQuery(query) {
    return { type: query.type, x: query.x, y: query.y, flags: query.flags, owner: query.owner, mode: query.mode };
}

// 사제 배치 미리보기이다. 일반 미리보기에 사제 콜백을 공급하는 얇은 어댑터이다. (2026-11-09 WB04-E157)
export class RawPriestPlacementPreview {
    #preview;

    // 비어 있는 기존 경계는 람다로 가리지 않고 생성 시 진단한 뒤 사제 콜백을 일반 미리보기 콜백으로 감싼다. (2026-11-09 WB04-E157)
    constructor(pool, types, spots, surfaceMap, state, hooks) {
        if (typeof hooks?.bounds !== 'function' || typeof hooks.inspectCollisions !== 'function') throw new TypeError('사제 미리보기 하위 경계 누락');
        const { bounds, inspectCollisions } = hooks;
        this.#preview = new RawCanonPlacementPreview(pool, types, spots, surfaceMap, state, {
            bounds: (query) => bounds(priestQuery(query)),
            inspectCollisions: (query, localOwner) => inspectCollisions(priestQuery(query), localOwner)
        });
    }

    // 연결 시 같은 SID 풀인지 확인할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E157)
    get pool() { return this.#preview.pool; }

    // 사제의 첫 인자는 타입 번호이며 로컬 비교를 다시 수행하지 않는다. (2026-11-09 WB04-E157)
    inspect(query, localOwner) {
        return this.#preview.inspect({ type: query.type, argument: query.type, x: query.x, y: query.y, flags: query.flags, owner: query.owner, mode: query.mode }, localOwner);
    }
}

// 사제 접두와 같은 풀에서만 연결한다. 사제 접두의 후반 경계만 바꾸고 기존 모양 조회는 유지한다. (2026-11-09 WB04-E157)
export function makePriestPlacementPreviewHooks(pool, preview, hooks = {}) {
    if (pool !== preview.pool) throw new TypeError('사제 미리보기/배치의 SID 풀이 다릅니다');
    return { ...hooks, inspectGeometry: (query, localOwner) => preview.inspect(query, localOwner) };
}
