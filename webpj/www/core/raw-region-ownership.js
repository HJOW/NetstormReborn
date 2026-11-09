// 지역 소유 투표와 판본별 도장 요청 순서를 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/RawRegionOwnership.cpp/.h`, 10.78 전용; 원본 00455380).
// 추가 목록(additional)과 신전 목록(temples)의 객체 위치를 지역 getter(regionAt)로 지역 번호로 바꿔 질의 좌표의 지역과 같은 객체의 소유자(+34)에 한 표씩 더한다.
//  - RegionOwnershipState: 영향 지역 누적 기록(affected: [{x, y, region}], 좌표는 투표에 쓰지 않음), 월드 다시 그리기 DWORD(revision)와 미니맵 요청 DWORD(dirty).
//    sharedWorldRedraw / sharedMapRevision 에 `{ value }` 상자를 주면 서로 다른 실제 소유자의 현재 DWORD 를 빌린다(독립 입력만 revision·dirty 필드를 읽는다). 월드 다시 그리기는 증가(DWORD 감김), 미니맵은 원문 1 쓰기이다.
//  - RawRegionOwnership(pool, additional, temples, themes, state, hooks): hooks = { regionAt(x, y) → 지역, paint({x, y, owner, theme}), themeMode() → 옵션 정수, typeTheme?(type) → 현재 +0x98 DWORD }.
//    typeTheme 이 없으면 themes(타입별 테마 DWORD 표, 길이 188 이상)를 쓴다. 같은 목록 객체 두 개·필수 훅 누락·테마 표 부족은 구성 때 거부한다.
//    update(x, y, mode = 0): 동률/중립은 소유자 0 으로 두 번 도장(중립 1, 최종 0)하고, 아니면 먼저 소유자 0 으로 도장한 뒤 같은 소유자의 마지막 신전 테마(mode 0 일 때만; 옵션 꺼짐 0, 종류 0→2·1→3·2→1)로 최종 도장한다.
//    도장 뒤 월드 다시 그리기 증가·미니맵 1 쓰기, 영향 지역이 2개 이상이면 mode 10 으로 누적 재투표(지역마다 표를 초기화하지 않음)한다. 좌표 절삭(0 방향)이 int 범위를 벗어나거나 NaN 이면 투표 전에 거부한다.
//    updateNative(x, y, mode = 0): 같은 455380 의 PATCH 몸체로, 투표가 활성 항목 사본이 아니라 현재 signed count 와 현재 저장소(storage)를 항목마다 다시 읽는다(음수 count 는 배열을 읽지 않고, 저장소 물리 밖은 거부하며,
//    표는 DWORD 감김). 좌표 절삭은 실제 도장하는 위치에서 한다. typed update 의 unsigned 전체 목록 선검사와 분리한다.
//  - makeTerrainRegionOwnershipHooks(pool, terrain, hooks): 지역 getter 를 지형(RawCanonPlacementTerrain.regionAt) 앞에 원본 float 편향(0.99999, 비트 0x3f7fff58) snap 으로 연결한다. 다른 풀이면 거부한다.
// outpost 등록/삭제 뒤 지역 통지(MakeOutpostRegionOwnershipHooks)는 RawOutpostLifecycle(멀티플레이 전용, exclude-mp)과 함께 웹 범위 밖이라 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-10 WB04-E249: 3-B RawRegionOwnership 이식으로 추가)

// 슬롯 좌표 float 위치, 소유자(+34)·타입(+10) 위치, 투표 소유자 수(0~8), mode 10(누적 재투표), 최대 SID 이다. (2026-11-10 WB04-E249)
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const TYPE_OFFSET = 10;
const OWNER_COUNT = 9;
const SPECIAL_MODE = 10;
const MAX_SID = 65535;
const THEME_TYPE_COUNT = 188;

// 지형 snap 편향 float 의 정확한 비트(0x3f7fff58)이다. (2026-11-10 WB04-E249)
const SNAP_BIAS = (() => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, 0x3f7fff58, true);
    return view.getFloat32(0, true);
})();

// 슬롯의 float 좌표를 원본 DWORD 비트 그대로 읽는다. (2026-11-10 WB04-E249)
const floatView = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    floatView.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return floatView.getFloat32(0, true);
}

// 유효한 int 범위만 지원하며 x87 처럼 double 합산 뒤 0 방향으로 절삭한다. -0 은 0 으로 바꾼다. (2026-11-10 WB04-E249)
function truncate(value) {
    const result = Math.trunc(value);
    if (!Number.isFinite(result) || result < -0x80000000 || result > 0x7fffffff) throw new RangeError('지역 소유 좌표 절삭 범위 오류');
    return result | 0;
}

// 지역 소유 상태이다. affected 는 영향 지역 누적 기록, revision/dirty 는 독립 입력용 월드 다시 그리기/미니맵 DWORD 이고,
// sharedWorldRedraw/sharedMapRevision 은 실제 서로 다른 소유자의 `{ value }` 상자를 빌릴 때 쓴다. (2026-11-10 WB04-E249)
export class RegionOwnershipState {
    constructor({ affected = [], revision = 0, dirty = 0, sharedWorldRedraw = null, sharedMapRevision = null } = {}) {
        this.affected = affected;
        this.revision = revision >>> 0;
        this.dirty = dirty >>> 0;
        this.sharedWorldRedraw = sharedWorldRedraw;
        this.sharedMapRevision = sharedMapRevision;
    }

    // 현재 월드 다시 그리기 DWORD 이다(빌린 상자가 있으면 그 값). (2026-11-10 WB04-E249)
    get worldRedraw() { return this.sharedWorldRedraw ? this.sharedWorldRedraw.value >>> 0 : this.revision; }

    // 현재 미니맵 요청 DWORD 이다(빌린 상자가 있으면 그 값). (2026-11-10 WB04-E249)
    get mapRevision() { return this.sharedMapRevision ? this.sharedMapRevision.value >>> 0 : this.dirty; }

    // 투표 효과 뒤 현재 월드 다시 그리기 DWORD 를 직접 증가시킨다(DWORD 감김). (2026-11-10 WB04-E249)
    incrementWorldRedraw() {
        if (this.sharedWorldRedraw) this.sharedWorldRedraw.value = (this.sharedWorldRedraw.value + 1) >>> 0;
        else this.revision = (this.revision + 1) >>> 0;
    }

    // 같은 미니맵 DWORD 에는 증가 대신 원문 1 을 쓴다. (2026-11-10 WB04-E249)
    requestMapRefresh() {
        if (this.sharedMapRevision) this.sharedMapRevision.value = 1;
        else this.dirty = 1;
    }
}

export class RawRegionOwnership {
    #pool;
    #additional;
    #temples;
    #themes;
    #state;
    #hooks;

    // 필수 경계와 서로 다른 전역 목록/테마 표를 먼저 확인한다. 풀은 10.78 만 받는다. (2026-11-10 WB04-E249)
    constructor(pool, additional, temples, themes, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('지역 소유는 10.78 SidPool 만 받습니다.');
        if (additional === temples || typeof hooks?.regionAt !== 'function' || typeof hooks.paint !== 'function' || typeof hooks.themeMode !== 'function'
            || (typeof hooks.typeTheme !== 'function' && !(themes?.length >= THEME_TYPE_COUNT))) {
            throw new TypeError('지역 소유 목록/테마/필수 효과 연결 오류');
        }
        this.#pool = pool;
        this.#additional = additional;
        this.#temples = temples;
        this.#themes = themes;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 실제 outpost/terrain 과 같은 풀을 쓰는지 검사하는 참조이다. (2026-11-10 WB04-E249)
    get pool() { return this.#pool; }

    // 현재 TYPE 장치 또는 기존 독립 입력표에서 선택한 원소만 읽는다. (2026-11-10 WB04-E249)
    #theme(type) {
        if (this.#hooks.typeTheme) return this.#hooks.typeTheme(type) >>> 0;
        if (type >= this.#themes.length) throw new RangeError('지역 테마 타입 오류');
        return this.#themes[type] >>> 0;
    }

    // 같은 소유자의 마지막 신전 테마 변환이다: 옵션 꺼짐 0, 종류 0→2, 1→3, 2→1, 그 밖 0. (2026-11-10 WB04-E249)
    #themeFor(kind, enabled) {
        return !enabled ? 0 : kind === 0 ? 2 : kind === 1 ? 3 : kind === 2 ? 1 : 0;
    }

    // 현재 목록 순서로 지역과 소유자를 읽고 같은 표 배열에 누적한다. 목록 크기와 항목은 매 반복 다시 읽고 중복 SID 도 독립 표로 센다. (2026-11-10 WB04-E249)
    #vote(list, x, y, votes, affectedIndex) {
        for (let i = 0; i < list.items().length; i += 1) {
            const number = list.items()[i];
            if (number > MAX_SID) throw new RangeError('지역 투표 SID 범위 오류');
            const raw = this.#pool.slot(number);
            const region = this.#hooks.regionAt(coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET));
            const target = affectedIndex !== null ? this.#affectedAt(affectedIndex).region : this.#hooks.regionAt(x, y);
            if (region !== target) continue;
            const owner = this.#pool.slot(number)[OWNER_OFFSET];
            if (owner >= votes.length || votes[owner] === 0x7fffffff) throw new RangeError('지역 투표 소유자/표 개수 오류');
            votes[owner] += 1;
        }
    }

    // 영향 지역 기록의 한 항목이다. 범위 밖이면 거부한다. (2026-11-10 WB04-E249)
    #affectedAt(index) {
        const entry = this.#state.affected[index];
        if (!entry) throw new RangeError('지역 소유 영향 목록 범위 오류');
        return entry;
    }

    // 최대 표 소유자와 동률 여부를 원문 순서로 고른다: 더 큰 최대가 나오면 앞선 동률을 해제한다. (2026-11-10 WB04-E249)
    #tally(votes) {
        let owner = 0;
        let best = 0;
        let tied = false;
        for (let i = 0; i < votes.length; i += 1) {
            if (votes[i] > best) { best = votes[i]; owner = i; tied = false; } else if (votes[i] === best) tied = true;
        }
        return { owner, tied };
    }

    // 00455380: 동률/중립과 mode 10 누적 재투표를 처리한다. 좌표 절삭은 투표 전에 한다. (2026-11-10 WB04-E249)
    update(x, y, mode = 0) {
        const state = this.#state;
        const special = mode === SPECIAL_MODE;
        const paintX = truncate(x);
        const paintY = truncate(y);
        const votes = new Int32Array(OWNER_COUNT);
        const paint = (owner, theme) => this.#hooks.paint({ x: paintX, y: paintY, owner, theme });
        if (special) {
            // 지역마다 표를 초기화하지 않으며 중복 지역도 다시 누적한다.
            for (let index = 0; index < state.affected.length; index += 1) {
                this.#vote(this.#additional, x, y, votes, index);
                this.#vote(this.#temples, x, y, votes, index);
            }
            mode = 0;
        } else {
            state.affected.length = 0;
            this.#vote(this.#additional, x, y, votes, null);
            this.#vote(this.#temples, x, y, votes, null);
        }
        let { owner, tied } = this.#tally(votes);
        let theme = 0;
        if (tied || owner === 0) {
            paint(1, 0);
            state.affected.length = 0;
            owner = 0;
        } else {
            if (mode === 0) {
                // 지역과 무관하게 같은 소유자의 마지막 신전 테마가 남는다.
                for (let i = 0; i < this.#temples.items().length; i += 1) {
                    const number = this.#temples.items()[i];
                    if (number > MAX_SID) throw new RangeError('지역 테마 SID 범위 오류');
                    const raw = this.#pool.slot(number);
                    if (raw[OWNER_OFFSET] !== owner) continue;
                    const kind = this.#theme(raw[TYPE_OFFSET]);
                    theme = this.#themeFor(kind, this.#hooks.themeMode());
                }
            }
            paint(0, 0);
        }
        paint(owner, theme);
        state.incrementWorldRedraw();
        state.requestMapRefresh();
        if (!special && state.affected.length > 1) this.update(x, y, SPECIAL_MODE);
    }

    // 현재 저장소 한 항목을 읽는다. 선택한 항목의 물리만 검사하며 용량/전체 count/다음 항목은 미리 읽지 않는다. (2026-11-10 WB04-E249)
    #nativeValue(list, index) {
        const storage = list.storage();
        if (index >= storage.length) throw new RangeError('지역 Native 목록의 현재 DWORD 물리 부족');
        return storage[index] >>> 0;
    }

    // 원문 signed 현재 count 가 0 이하면 배열을 읽지 않고, 각 지역 조회 콜백 뒤 현재 owner 만 표에 더한다(표는 DWORD 감김). (2026-11-10 WB04-E249)
    #voteNative(list, x, y, votes, affectedIndex) {
        // 원문 각 반복의 현재 count/현재 포인터를 사용한다.
        for (let index = 0; index < (list.readCount() | 0); index += 1) {
            const number = this.#nativeValue(list, index);
            if (number > MAX_SID) throw new RangeError('지역 Native 투표의 실제 DWORD SID 물리 범위');
            const raw = this.#pool.slot(number);
            const region = this.#hooks.regionAt(coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET));
            const target = affectedIndex !== null ? this.#affectedAt(affectedIndex).region : this.#hooks.regionAt(x, y);
            if (region !== target) continue;
            const owner = this.#pool.slot(number)[OWNER_OFFSET];
            if (owner >= votes.length) throw new RangeError('지역 Native 투표의 현재 owner 물리 범위');
            votes[owner] = (votes[owner] + 1) | 0;
        }
    }

    // 같은 455380 의 PATCH 도장/재투표이며 typed update 의 unsigned 전체 목록 선검사와 분리한다. (2026-11-10 WB04-E249)
    updateNative(x, y, mode = 0) {
        const state = this.#state;
        const special = mode === SPECIAL_MODE;
        const votes = new Int32Array(OWNER_COUNT);
        // 좌표 절삭은 실제 도장을 선택한 위치에서만 수행한다.
        const paint = (owner, theme) => this.#hooks.paint({ x: truncate(x), y: truncate(y), owner, theme });
        if (special) {
            // 지역별 표를 다시 초기화하지 않고 현재 영향 목록을 순회한다.
            for (let index = 0; index < state.affected.length; index += 1) {
                this.#voteNative(this.#additional, x, y, votes, index);
                this.#voteNative(this.#temples, x, y, votes, index);
            }
            mode = 0;
        } else {
            state.affected.length = 0;
            this.#voteNative(this.#additional, x, y, votes, null);
            this.#voteNative(this.#temples, x, y, votes, null);
        }
        let { owner, tied } = this.#tally(votes);
        let theme = 0;
        if (tied || !owner) {
            paint(1, 0);
            state.affected.length = 0;
            owner = 0;
        } else {
            if (!mode) {
                // 현재 신전 signed 개수와 포인터를 매 항목 다시 선택한다.
                for (let index = 0; index < (this.#temples.readCount() | 0); index += 1) {
                    const number = this.#nativeValue(this.#temples, index);
                    if (number > MAX_SID) throw new RangeError('지역 Native 테마의 실제 DWORD SID 물리 범위');
                    const raw = this.#pool.slot(number);
                    if (raw[OWNER_OFFSET] !== owner) continue;
                    const kind = this.#theme(raw[TYPE_OFFSET]);
                    theme = this.#themeFor(kind, this.#hooks.themeMode());
                }
            }
            paint(0, 0);
        }
        paint(owner, theme);
        state.incrementWorldRedraw();
        state.requestMapRefresh();
        // 실제 후행 영향 목록이 여러 개면 같은 owner/모듈로 누적 재투표한다.
        if (!special && state.affected.length > 1) this.updateNative(x, y, SPECIAL_MODE);
    }
}

// 기존 지역 getter 앞에 원본 0.99999 float 편향 snap 을 연결한다: double 정밀도로 편향을 더해 0 쪽으로 절삭한 정수를 float 에 저장하고 그 정수로 지형에 묻는다. 다른 풀이면 거부한다. (2026-11-10 WB04-E249)
export function makeTerrainRegionOwnershipHooks(pool, terrain, hooks) {
    if (pool !== terrain?.pool) throw new TypeError('지역 소유/지형 SID 풀이 다릅니다');
    return {
        ...hooks,
        regionAt: (x, y) => {
            const sx = Math.fround(truncate(x + SNAP_BIAS));
            const sy = Math.fround(truncate(y + SNAP_BIAS));
            return terrain.regionAt(truncate(sx), truncate(sy));
        },
    };
}
