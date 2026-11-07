// 원본 502370 폭발 계열(Point Blast)의 가상 bc 자격(41e9c0)과 가상 c0 효과(41ea30)를 옮긴 모듈이다(cpppj `o/RawBombBlast.cpp/.h`, 10.78 전용).
//  - handles(parent): 부모의 현재 가상 표가 실제 502370 인 것만 처리한다(타입 이름으로 넓히지 않는다).
//  - mayTarget(target): 대상 TYPE genus 에 표면(4)이 있으면 번호 0 은 참, 아니면 현재 프레임 코드의 보호 비트(0x40)가 꺼져 있을 때 참이다(프레임 범위 밖은 거부). 표면이 아니면 성장 중 비 방해 TYPE(150)은 참,
//    그 밖에는 HP 를 보유(dais 추상 genus 0x400000 이면 extra 비트 1, 아니면 flags1 0x10)하고 허용 genus 0x80074200 이 하나라도 있을 때 참이다. 현재 HP 값은 읽지 않는다.
//  - apply(parent, target): 대상이 부모의 +12 WORD(운반체)이면 아무것도 하지 않는다. 표면이면 부모 캡처 중심(RawBombTargets.center)과 대상 발자국 중심 거리가 `반경 + 발자국/2 − 1` 이내(비순서 포함)이면
//    삭제(flags 0x100048), 밖이고 권한이 있으면 끝 칸 변환(edge). 표면이 아니면 피해 전에 대상 중심을 캡처하고, 사제 TYPE(158)이 부모와 동맹이면 끝, 성장 중 비 방해(150)는 권한이 있을 때 삭제(flags 0x8100008),
//    그 밖은 피해 1000(부모 owner·부모 SID)을 준 뒤 캡처한 중심에 ground 효과를 낸다. 추가 free/dead/HP 값 관문은 넣지 않고, 효과 뒤 바뀐 TYPE/좌표/권한/가상 표는 원본 위치에서 다시 읽는다.
// 상태 { boss, growingRainBlockerType(150), priestType(158), alliances(81) } 는 현재 값을 읽는 살아 있는 객체이다. 훅 { radius, damage, destroy, ground, edge } 는 도달한 지점에서만 요구한다(없으면 TypeError). DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E54: RawBombBlast 이식으로 추가)

// 슬롯 필드 위치와 개수 상수, 가상 표, genus 비트, 삭제 flags, 피해량이다. (2026-11-08 WB04-E54)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const CARRIER_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
const TYPE_COUNT = 188;
const OWNER_COUNT = 9;
const BLAST_TABLE = 0x502370;
const SURFACE_GENUS = 4;
const DAIS_GENUS = 0x400000;
const HAS_HP_FLAG = 0x10;
const ALLOWED_GENUS = 0x80074200;
const PROTECTED_FRAME = 0x40;
const SURFACE_DESTROY = 0x100048;
const GROWING_DESTROY = 0x8100008;
const BLAST_DAMAGE = 1000;

// 선택한 큰 하위의 누락을 무동작 성공으로 바꾸지 않는다. (2026-11-08 WB04-E54)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('Bomb 폭발 실제 하위 미연결');
    return hook;
}

// 폭발 계열 실행기이다. 사용: new RawBombBlast(pool, types, frames, coordinates, state, { radius, damage, destroy, ground, edge }). coordinates 는 같은 풀의 RawBombTargets 이다. (2026-11-08 WB04-E54)
export class RawBombBlast {
    #pool;
    #types;
    #frames;
    #coordinates;
    #state;
    #hooks;

    // 다른 판본/풀의 좌표 계산과 프레임을 섞지 않는다. 훅은 생성 시점의 사본을 쓴다. (2026-11-08 WB04-E54)
    constructor(pool, types, frames, coordinates, state, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || frames?.length !== TYPE_COUNT || coordinates?.pool !== pool || typeof state !== 'object' || state === null) {
            throw new TypeError('Bomb 폭발 판본/TYPE/Pool 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#coordinates = coordinates;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 같은 실제 월드의 저장소인지 확인하는 풀 참조이다. (2026-11-08 WB04-E54)
    get pool() {
        return this.#pool;
    }

    // 현재 물리 필드를 원본 폭으로 읽는다(주변 필드로 폭을 넓히지 않는다). (2026-11-08 WB04-E54)
    #read(sid, offset, width = 4) {
        const raw = this.#pool.slot(sid);
        let value = 0;
        // 낮은 바이트부터 요청한 원본 폭을 조합한다.
        for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
        return value;
    }

    // 효과 뒤 바뀐 현재 TYPE 를 범위 검사해 읽는다. (2026-11-08 WB04-E54)
    #type(sid) {
        const kind = this.#read(sid, TYPE_OFFSET, 1);
        if (kind >= this.#types.length) throw new RangeError('Bomb 폭발 현재 TYPE 범위');
        return this.#types[kind];
    }

    // 4adc60: 현재 HP 값을 읽지 않고 HP 를 보유하는 형태인지 판단한다(dais 추상이면 extra 비트 1). (2026-11-08 WB04-E54)
    #hasHitPoints(sid) {
        const type = this.#type(sid);
        return (type.flags2 & DAIS_GENUS) ? (this.#read(sid, EXTRA_OFFSET, 1) & 1) !== 0 : (type.flags1 & HAS_HP_FLAG) !== 0;
    }

    // 4adea0: 현재 발자국 중심이며 보드 제한을 추가하지 않는다. 비표면 피해 효과의 위치도 호출 전에 이 중심으로 캡처한다. (2026-11-08 WB04-E54)
    #targetCenter(target) {
        const type = this.#type(target);
        const raw = this.#pool.slot(target);
        return [Math.fround(floatAt(raw, X_OFFSET) - type.footX * 0.5), Math.fround(floatAt(raw, Y_OFFSET) - type.footY * 0.5)];
    }

    // 유효 owner 0..8 만 실제 동맹 배열을 조회한다. (2026-11-08 WB04-E54)
    #allied(first, second) {
        if (first >= OWNER_COUNT || second >= OWNER_COUNT) throw new RangeError('Bomb 폭발 동맹 owner 범위');
        return this.#state.alliances[first * OWNER_COUNT + second] !== 0;
    }

    // 실제 주소로만 이 계열을 분배한다. (2026-11-08 WB04-E54)
    handles(parent) {
        return this.#read(parent, 0) === BLAST_TABLE;
    }

    // 41e9c0: protected 표면은 제외하고 비표면은 특별 대상 또는 HP 보유/허용 genus 를 요구한다. (2026-11-08 WB04-E54)
    mayTarget(target) {
        const genus = this.#type(target).flags2;
        if (genus & SURFACE_GENUS) {
            // 422560 의 0번 조기 반환이다.
            if (!target) return true;
            const kind = this.#read(target, TYPE_OFFSET, 1);
            const frame = this.#read(target, FRAME_OFFSET) | 0;
            const codes = this.#frames[kind];
            if (frame < 0 || frame >= codes.length) throw new RangeError('Bomb 보호 프레임 범위');
            return !(codes[frame].flags & PROTECTED_FRAME);
        }
        if (this.#read(target, TYPE_OFFSET, 1) === this.#state.growingRainBlockerType) return true;
        return this.#hasHitPoints(target) && (genus & ALLOWED_GENUS) !== 0;
    }

    // 41ea30: 자격 호출과 독립된 실제 효과 몸체이다. 추가 free/dead/HP 값 관문을 넣지 않는다. (2026-11-08 WB04-E54)
    apply(parent, target) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (target === this.#read(parent, CARRIER_OFFSET, 2)) return;
        const genus = this.#type(target).flags2;
        if (genus & SURFACE_GENUS) {
            const center = this.#coordinates.center(parent);
            const victimCenter = this.#targetCenter(target);
            // 가상 b4 이전에 현재 발자국을 float 로 저장한다.
            const halfWidth = Math.fround(this.#type(target).footX * 0.5);
            const radius = need(hooks.radius)(parent, this.#read(parent, 0)) | 0;
            const sum = Math.fround(radius + halfWidth);
            const dx = center[0] - victimCenter[0];
            const dy = center[1] - victimCenter[1];
            const distance = Math.sqrt(dx * dx + dy * dy);
            const limit = sum - 1.0;
            // 원본 unordered 분기도 안쪽 삭제로 들어간다.
            if (!(limit < distance)) need(hooks.destroy)(target, this.#read(target, 0), SURFACE_DESTROY);
            else if (state.boss) need(hooks.edge)(target, center[0], center[1]);
            return;
        }
        const center = this.#targetCenter(target);
        const kind = this.#read(target, TYPE_OFFSET, 1);
        if (kind === state.priestType && this.#allied(this.#read(parent, OWNER_OFFSET, 1), this.#read(target, OWNER_OFFSET, 1))) return;
        if (kind === state.growingRainBlockerType) {
            if (state.boss) need(hooks.destroy)(target, this.#read(target, 0), GROWING_DESTROY);
        } else {
            need(hooks.damage)(target, this.#read(target, 0), BLAST_DAMAGE, this.#read(parent, OWNER_OFFSET, 1), parent);
        }
        need(hooks.ground)(center[0], center[1]);
    }
}

// 정렬되지 않은 float 4바이트를 읽는다. (2026-11-08 WB04-E54)
const floatView = new DataView(new ArrayBuffer(4));
function floatAt(raw, offset) {
    floatView.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return floatView.getFloat32(0, true);
}
