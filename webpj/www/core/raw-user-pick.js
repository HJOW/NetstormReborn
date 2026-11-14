// 원본 Userinput.cpp 4d5840 의 사용자 후보 선택(클릭/우클릭/이동 커서가 공유)을 같은 현재 물리 풀에서 실행한다(cpppj `client/RawUserPick.cpp/.h`, 기준선 1f00a25a, 10.78 전용).
// 순서는 정지 관문 → 월드 변환 → 검색 시작(반경은 metamap 일 때만 8/8, 아니면 4/7) → 후보 순환(물리 범위만 확인, free/dead 필터 추가 없음) → 소유자·이동·깊이·좌표 순위 → 가상 hit → 마지막 1.6 근접 이동 후보 교체이다.
// 객체 상태는 쓰지 않는다. 도달한 경계에서 하위 훅이 연결되지 않았으면 그 위치에서 Error 이다.
// 사용: const pick = new RawUserPick(pool, types, hooks); const sid = pick.pick(x, y, force);
// (2026-11-15 WB04-E495: 5-B 사용자 선택 이식으로 추가)

// 슬롯 오프셋: TYPE BYTE(+10), 좌표 float(+14/+18), 소유자 BYTE(+34), signed 깊이 BYTE(+35), 상태 BYTE(+40)다.
const TYPE_AT = 10; const X_AT = 14; const Y_AT = 18; const OWNER_AT = 34; const DEPTH_AT = 35; const MARK_AT = 40;
// 이동 가능 flags2 비트, 건물 숨김 flags2 마스크(원문 0x90564A00), 추상/매몰 거부 비트와 비선택 flags1 비트다.
const MOVABLE = 0x30000; const HIDDEN_MASK = 2421574144; const ABSTRACT = 0x2000000; const UNSELECTABLE = 0x8000000;
// 숨김 건물 TYPE 번호 두 개(164, 84)와 마지막 교체를 허락하지 않는 flags2 비트(표면/다리/edgefarm)다.
const HIDDEN_TYPE_A = 164; const HIDDEN_TYPE_B = 84; const SURFACE = 0x80006;
// 큰 후보 번호 상한(WORD)과 초기 깊이 값이다.
const MAX_SID = 65535; const INITIAL_DEPTH = 9999;

// 호출 경로에 도달한 하위 훅이 없으면 선행 효과를 감추지 않고 실패한다(C++ logic_error → Error).
function need(hook) {
    if (typeof hook !== 'function') throw new Error('선택기의 도달한 원본 하위 미연결');
    return hook;
}

// 현재 raw 네 바이트를 비정렬 little endian float32 로 읽는다(주변 필드를 읽거나 정렬을 가정하지 않는다).
function position(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset + offset, 4).getFloat32(0, true);
}

// 현재 풀·188 TYPE 표·현재 장치 훅으로 구성하는 사용자 후보 선택기이다.
// hooks = { paused, localOwner, metamap, hideBuildings, project(x, y) → [x, y](float), begin([x, y], radius1, radius2) → 첫 SID, next() → 다음 SID, center(sid) → [x, y], hit(sid, x, y) → bool } 이다.
export class RawUserPick {
    // 풀(물리 슬롯)·TYPE 표·훅 묶음이다.
    #pool; #types; #hooks;

    // 선택/장치 효과 없이 같은 참조만 설치한다.
    constructor(pool, types, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('원본4d5840 선택기는 10.78 전용');
        this.#pool = pool; this.#types = types; this.#hooks = hooks;
    }

    // 현재 슬롯의 TYPE 레코드를 매번 다시 조회한다(후속 콜백이 TYPE 바이트를 바꿀 수 있다). 범위 밖이면 RangeError 이다.
    #type(sid) {
        const number = this.#pool.physical(sid)[TYPE_AT];
        if (number >= this.#types.length) throw new RangeError('선택기의 현재 TYPE 범위');
        return this.#types[number];
    }

    // 화면 좌표 (x, y)에서 가장 우선인 후보 SID 를 돌려준다(없으면 0). force 는 비선택/추상 필터만 우회하며 숨김/매몰 필터와 다르다.
    pick(x, y, force = 0) {
        if (need(this.#hooks.paused)()) return 0;
        const point = need(this.#hooks.project)(x, y); const meta = need(this.#hooks.metamap)() !== 0; // 원문은 반경을 만들 때만 metamap 을 읽는다.
        let candidate = need(this.#hooks.begin)(point, meta ? 8 : 4, meta ? 8 : 7) >>> 0;
        // 원래 초기값이며 근접 거리는 float 저장 경계를 보존한다.
        let best = 0; let near = 0; let bestDepth = INITIAL_DEPTH; let bestX = 0; let bestY = 0; let nearest = Math.fround(1.6); let bestOwned = false; let bestMobile = false;
        // 후보 사슬 끝(0 또는 물리 범위 밖)까지 순환한다. free/dead/자산 여부를 새 필터로 추가하지 않고 물리 범위만 검사한다.
        while (candidate > 0 && candidate < this.#pool.capacity) {
            if (candidate > MAX_SID) throw new RangeError('선택기의 WORD SID 범위');
            const sid = candidate;
            const owned = this.#pool.physical(sid)[OWNER_AT] === (need(this.#hooks.localOwner)() >>> 0); // 소유 이동 여부는 후보 진입 시점 값이다.
            const mobile = owned && (this.#type(sid).flags2 & MOVABLE) !== 0;
            const type = this.#type(sid); const raw = this.#pool.physical(sid); // 같은 현재 TYPE/extra 의 원본 거부 관문이다.
            const hidden = need(this.#hooks.hideBuildings)() !== 0 && ((type.flags2 & HIDDEN_MASK) !== 0 || raw[TYPE_AT] === HIDDEN_TYPE_A || raw[TYPE_AT] === HIDDEN_TYPE_B);
            if (!hidden && (force || (!(type.flags2 & ABSTRACT) && !(type.flags1 & UNSELECTABLE) && (!(raw[MARK_AT] & 0x10) || raw[OWNER_AT] === (need(this.#hooks.localOwner)() >>> 0))))) {
                if (this.#type(sid).flags2 & MOVABLE) { // 가상 hit 실패와 무관하게 가까운 이동 후보를 먼저 기억한다.
                    const center = need(this.#hooks.center)(sid);
                    const dy0 = point[1] - center[1]; const dx0 = point[0] - center[0]; const distance = Math.sqrt(dy0 * dy0 + dx0 * dx0);
                    if (distance < nearest) { nearest = Math.fround(distance); near = sid; }
                }
                const current = this.#pool.physical(sid);
                const px = position(current, X_AT); const py = position(current, Y_AT); // 원래 저장된 float 차이이며 raw 좌표를 중심으로 바꾸지 않는다.
                const dy = Math.fround(py - bestY); const dx = Math.fround(px - bestX);
                const dz = (current[DEPTH_AT] << 24 >> 24) - bestDepth; // signed BYTE 깊이 차이다.
                let allowed = owned || !bestOwned; // 자기 소유 hit 은 다른 소유자보다 우선한다.
                if (allowed && owned === bestOwned) {
                    if (mobile && !bestMobile) allowed = true;
                    else if (!mobile && bestMobile) allowed = false;
                    else if (dz >= 0 && (dz !== 0 || (dy <= 0 && (dy !== 0 || dx <= 0)))) allowed = false;
                }
                if (allowed && need(this.#hooks.hit)(sid, x, y)) { // 순위를 통과한 현재 후보에만 +b0 를 호출한다.
                    bestY = Math.fround(dy + bestY); bestDepth += dz; bestX = Math.fround(dx + bestX); best = sid; bestOwned = owned; bestMobile = mobile;
                }
            }
            candidate = need(this.#hooks.next)() >>> 0;
        }
        if ((!best || (this.#type(best).flags2 & SURFACE)) && near) best = near; // 표면/다리/edgefarm 의 hit 보다 가까운 이동 유닛을 마지막에 고른다.
        return best;
    }
}
