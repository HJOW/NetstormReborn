// 생산 칸 조각 반환 004468b0을 같은 contained 생성기·게임 난수·Regular에 연결한다(cpppj client/RawCursorReturnPiece.cpp/.h, 10.78).
// 사용: new RawCursorReturnPiece(contained, state, random, hooks).returnPiece(flags, source, quality).
// productTimers/fortMode/unitRate가 있으면 생성/할당 효과 뒤 현재 값을 다시 읽는다. 상태와 훅의 실제 소유자는 이 몸체보다 오래 살아야 한다.
// 이 모듈은 DOM을 쓰지 않는다. (2026-11-14 WB04-E399: 커서 전역·미리보기 표시의 6단계 선행)
import { selectBridgePattern } from './raw-canon-type.js';

// 일반 전투의 세 소멸 시간과 요새 모드의 원본 float32 비트값이다.
const LIFETIMES = Object.freeze([10, 5, 1]);
const FORT_LIFETIME = new DataView(Uint8Array.of(0x17, 0xb7, 0xd1, 0x38).buffer).getFloat32(0, true);

// 현재 선택 TYPE와 다리 TYPE, 생산 칸 유지·요새·소멸 시간 옵션이다.
export class CursorReturnPieceState {
    // 원본 cursorType와 다리 TYPE 번호다. 현재 전역은 각 반환 시점에 읽는다.
    type = 0; bridgeType = 82;
    // 유지 옵션·요새 모드 DWORD와 일반 전투 옵션 BYTE다.
    keepPieces = 0; fort = 0; lifetimeIndex = 0;
}

// 실제 contained 생성과 난수·타이머를 조립하며 별도 조각/시간 저장소를 만들지 않는다.
export class RawCursorReturnPiece {
    // 생성기·같은 현재 상태·게임 난수·타이머/설정 장치다.
    #contained; #state; #random; #hooks;

    // 두 타이머 경계는 생성 때 확인하고, 설정 getter는 독립 상태 입력을 지원하기 위해 선택으로 둔다.
    constructor(contained, state, random, hooks = {}) {
        if (typeof hooks.allocateTimer !== 'function' || typeof hooks.createTimer !== 'function') throw new TypeError('조각 반환 타이머 경계 누락');
        this.#contained = contained; this.#state = state; this.#random = random; this.#hooks = hooks;
    }

    // 로컬 배치·GUI·Regular 어댑터가 동일 풀을 사용하는지 확인한다.
    get pool() { return this.#contained.pool; }

    // 다리는 난수 한 번과 원본 가중 패턴을 사용한다. 일반 조각은 품질을 signed DWORD 감소 뒤 0으로 제한한다.
    returnPiece(flags, source, quality) {
        const s = this.#state; const h = this.#hooks;
        source >>>= 0; quality >>>= 0; flags = (flags & ~0x100) >>> 0;
        if (!source || source >= this.pool.capacity) throw new RangeError('조각 반환 부모 SID 범위 오류');
        if ((s.type >>> 0) === (s.bridgeType >>> 0)) {
            const pattern = selectBridgePattern(this.#random.next(10000) | 0);
            this.#contained.create(source, s.type >>> 0, pattern >>> 0, flags, 0x10, 0);
            return;
        }
        if (((h.productTimers ? h.productTimers() : s.keepPieces) >>> 0) === 0) flags = (flags | 0x100) >>> 0;
        const decreased = (quality - 1) >>> 0;
        const sid = this.#contained.create(source, s.type >>> 0, 0, flags, 0x10, (decreased | 0) < 0 ? 0 : decreased);
        if (((h.productTimers ? h.productTimers() : s.keepPieces) >>> 0) === 0 && h.allocateTimer(0x28)) {
            const fort = h.fortMode ? h.fortMode() : s.fort;
            let duration = FORT_LIFETIME;
            if (!(fort >>> 0)) {
                const index = (h.unitRate ? h.unitRate() : s.lifetimeIndex) & 0xff;
                if (index >= LIFETIMES.length) throw new RangeError('조각 반환 소멸 시간 번호 오류');
                duration = LIFETIMES[index];
            }
            h.createTimer(sid, 0, duration);
        }
    }
}

// 같은 풀의 실제 ProcessHost.addRegular를 타이머 생성에 연결한다. 할당 성공/실패 장치는 호출자가 유지한다.
export function makeCursorReturnPieceHooks(pool, host, hooks = {}) {
    if (host?.pool !== pool) throw new TypeError('조각 반환/프로세스 SID 풀이 다릅니다');
    return { ...hooks, createTimer: (sid, event, duration) => host.addRegular(sid, event, duration) };
}

// 로컬 건설과 GUI 내려놓기의 동일한 반환 인수를 실제 몸체에 연결한 새 묶음이다.
export function makeReturnPieceHooks(pool, body, hooks = {}) {
    if (body?.pool !== pool) throw new TypeError('조각 반환 SID 풀이 다릅니다');
    return { ...hooks, returnPiece: (flags, source, quality) => body.returnPiece(flags, source, quality) };
}
