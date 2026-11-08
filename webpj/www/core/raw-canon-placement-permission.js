// 일반 타입/패턴 배치의 후보 권한 함수(00462cb0)와 방향 관계 helper(004629e0)를 옮긴 모듈이다(cpppj `o/RawCanonPlacementPermission.cpp/.h`, 10.78).
//  - createCanonPlacementPermissionState(): 그래프 조회 가능(graphReady)·편집기(editor)·방향 동맹 사용(useAlliances)·다른 소유자 허용(allowOtherOwners) 전역과 81칸 DWORD 관계 표(alliances)를 원본 초기값(모두 0)으로 만든다.
//  - new RawCanonPlacementPermission(pool, state, playerAnchor): playerAnchor(owner, type, x, y, xBits, yBits) 는 Player 작업장/그래프 locator 이며 실제 요청자·타입·좌표를 받고 DWORD 를 돌려준다(필수 경계).
//    x/y 는 단정도 값이고 xBits/yBits 는 NaN 페이로드까지 같은 원본 DWORD 비트이다(JS 수는 NaN 비트를 보존하지 못하기 때문에 추가).
//  - candidate(sid, owner): 그래프 비활성이면 0 → 요청자 0 이면 1 → 후보 raw 소유자(BYTE +34)가 관계가 없고 중립이 아니며 다른 소유자 허용이 꺼져 있으면 0 → 아니면 Player 조회 결과(DWORD 비트 그대로)이다.
//  - surrounding(sid, owner): 주변 순회의 바깥 관계 검사(중립도 적용)를 먼저 하고 통과하면 candidate 이다. related(currentOwner, owner): 편집기/동일 소유자/방향 동맹. alliance(currentOwner, owner): 관계 표 직접 읽기이다.
//  - makeCanonPermissionTerrainHooks(pool, permission, hooks): 지형 훅의 후보 권한을 실제 helper 에 연결한다(요청 소유자는 하위 BYTE 를 부호 확장해 전달하며 질의의 원래 DWORD 는 유지한다).
// 10.72(CD)의 소유자 BYTE 위치(+32) 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E149: 일반 배치 권한 이식으로 추가)

// 현재 raw 의 타입 번호·좌표 DWORD·소유자 BYTE 오프셋, 관계 표 칸 수(9×9), 소유자 열 폭이다. (2026-11-09 WB04-E149)
const TYPE = 10;
const X = 14;
const Y = 18;
const OWNER = 34;
const RELATION_COUNT = 81;
const OWNER_COLUMNS = 9;

// 배치 권한이 읽고 쓰는 전역 상태를 원본 초기값으로 만든다. (2026-11-09 WB04-E149)
export function createCanonPlacementPermissionState() {
    return { graphReady: 0, editor: 0, useAlliances: 0, allowOtherOwners: 0, alliances: new Uint32Array(RELATION_COUNT) };
}

// 슬롯의 비정렬 DWORD 를 리틀엔디언으로 읽는다. (2026-11-09 WB04-E149)
function dword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// DWORD 비트를 단정도 float 로 해석한다(산술 변환 없이 비트 해석). (2026-11-09 WB04-E149)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// 후보 권한과 방향 관계의 계층이다. pool(10.78 SidPool)·state·playerAnchor 의 수명은 호출자가 보장하며 전역은 호출 때마다 다시 읽는다. (2026-11-09 WB04-E149)
export class RawCanonPlacementPermission {
    #pool;
    #state;
    #playerAnchor;

    // 실제 Player 조회를 임의 허용으로 대체하지 않도록 필수 경계를 검사한다. (2026-11-09 WB04-E149)
    constructor(pool, state, playerAnchor) {
        if (pool?.edition !== 'patch1078') throw new TypeError('배치 권한은 10.78 SidPool 만 받습니다.');
        if (!state || !(state.alliances instanceof Uint32Array) || state.alliances.length !== RELATION_COUNT) throw new TypeError('배치 권한 관계 표 오류');
        if (typeof playerAnchor !== 'function') throw new TypeError('배치 권한 Player 조회 경계 누락');
        this.#pool = pool;
        this.#state = state;
        this.#playerAnchor = playerAnchor;
    }

    // 연결 시 동일 풀인지 확인할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E149)
    get pool() { return this.#pool; }

    // 최종 소유 관계의 직접 DWORD 읽기이다. 인덱스 산술은 DWORD 로 감기며 표 밖(손상된 소유자)은 진단한다. 편집기/동일 소유자/활성 조건을 붙이지 않는다. (2026-11-09 WB04-E149)
    alliance(currentOwner, owner) {
        const index = (((currentOwner >>> 0) * OWNER_COLUMNS) + (owner >>> 0)) >>> 0;
        if (index >= this.#state.alliances.length) throw new RangeError('배치 권한 관계 표 범위 오류');
        return this.#state.alliances[index] !== 0;
    }

    // 004629e0: 편집기이거나 동일 소유자이면 참, 방향 동맹을 쓰지 않으면 거짓, 아니면 관계 표이다. 중립/다른 소유자 허용은 여기 없다. (2026-11-09 WB04-E149)
    related(currentOwner, owner) {
        if (this.#state.editor || (currentOwner >>> 0) === (owner >>> 0)) return true;
        if (!this.#state.useAlliances) return false;
        return this.alliance(currentOwner, owner);
    }

    // 00462cb0: 그래프 비활성 → 요청자 0 → 현재 raw 소유 관계 → Player 조회 순서이다. 중립/다른 소유자 허용도 관계 조회 이후에 적용한다. 반환은 조회 결과의 DWORD 비트이다. (2026-11-09 WB04-E149)
    candidate(sid, owner) {
        owner >>>= 0;
        if (!this.#state.graphReady) return 0;
        if (!owner) return 1;
        const raw = this.#pool.slot(sid);
        const currentOwner = raw[OWNER];
        if (!this.related(currentOwner, owner) && currentOwner && !this.#state.allowOtherOwners) return 0;
        const xBits = dword(raw, X);
        const yBits = dword(raw, Y);
        return this.#playerAnchor(owner, raw[TYPE], floatOf(xBits), floatOf(yBits), xBits, yBits) >>> 0;
    }

    // MayPlace 주변 순회의 바깥 관계 검사를 먼저 적용한다. 중립 소유자에도 적용하고 다른 소유자 허용은 마지막에 읽는다. (2026-11-09 WB04-E149)
    surrounding(sid, owner) {
        const raw = this.#pool.slot(sid);
        if (!this.related(raw[OWNER], owner) && !this.#state.allowOtherOwners) return 0;
        return this.candidate(sid, owner);
    }
}

// 캡처한 요청자의 원래 DWORD 는 유지하고 helper 인자만 부호 있는 BYTE 계약으로 바꿔 후보 권한 훅만 연결한다. 기존 훅은 값으로 복사해 유지한다. (2026-11-09 WB04-E149)
export function makeCanonPermissionTerrainHooks(pool, permission, hooks = {}) {
    if (pool !== permission.pool) throw new TypeError('배치 권한/지형 SID 풀 불일치');
    return {
        ...hooks,
        candidatePermission: (query, sid) => {
            const owner = ((query.owner << 24) >> 24) >>> 0;
            return permission.candidate(sid, owner) !== 0;
        }
    };
}
