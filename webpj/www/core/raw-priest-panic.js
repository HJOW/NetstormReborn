// 사제 공황 이동(00491db0)의 임시 TYPE 속도 신전 복귀와 같은 영역 임의 이동을 옮긴 모듈이다(cpppj `o/RawPriestPanic.cpp/.h`, 10.78 전용).
// 순서: 현재 좌표의 섬 신전을 조회 → 신전이 있고 현재 HP 가 최대 HP 보다 작고 신전과 사제의 소유자가 같으면 현재 TYPE 의 속도를 두 배로 하여 신전 경로를 시작하고 속도를 절삭한 원래 정수로 되돌린다(경로가 생기면 끝)
//   → 현재 좌표가 섬 밖이면 영역 검사를 생략하고, 아니면 ±20 임의 후보 좌표(난수 둘, 현재 좌표에 더함)가 같은 영역일 때까지 반복하여 첫 후보에서 경로를 시작한다(실패해도 반환).
// 각 하위 효과 뒤 위치/소유자/HP/TYPE 를 다시 읽으며 도달한 하위 누락은 진단한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E66: 사제 공황 이식으로 추가)

// 슬롯의 TYPE/좌표/HP/소유자 위치와 속도 표 크기, CRT 정수 절삭이 허용하는 signed 범위이다. (2026-11-08 WB04-E66)
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const HP_OFFSET = 26;
const OWNER_OFFSET = 34;
const TYPE_COUNT = 188;
const INT_LOW = -2147483648;
const INT_HIGH = 2147483648;

// 선택된 실제 하위 누락을 빈 성공으로 숨기지 않는다. (2026-11-08 WB04-E66)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('사제 panic 실제 하위 미연결');
    return hook;
}

// 원본 CRT 의 정상 signed DWORD 절삭 범위를 확인하고 0 방향으로 절삭한다(정수 변환이라 음의 0 이 남지 않도록 | 0 으로 정규화). (2026-11-08 WB04-E66)
function speedInteger(value) {
    if (!Number.isFinite(value) || value < INT_LOW || value >= INT_HIGH) throw new RangeError('사제 panic 속도 CRT 정상범위');
    return Math.trunc(value) | 0;
}

// 비트 보존을 위해 정렬되지 않은 슬롯 좌표를 단정도로 읽는다. (2026-11-08 WB04-E66)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    // 네 바이트를 리틀엔디언으로 옮긴다.
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, speeds 는 188칸 { speed } 현재 TYPE 속도 표(경로 생성기가 같은 표를 관측; 참조), hooks = { temple(x, y)→신전 SID, maximum(sid)→최대 HP, target(actor, 신전)→경로(0=없음), land(x, y)→bool,
//   region(x, y)→지역 번호, random(low, high)→수, point(actor, x, y)→경로 } 이다. (2026-11-08 WB04-E66)
export class RawPriestPanic {
    // 효과 뒤 위치/소유자/HP/TYPE 를 다시 읽는 풀, 현재 속도 표, 큰 하위 효과이다.
    #pool;
    #speeds;
    #hooks;

    // 판본과 속도 표 크기를 확인한다. 하위 효과는 도달할 때만 요구한다. (2026-11-08 WB04-E66)
    constructor(pool, speeds, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(speeds) || speeds.length !== TYPE_COUNT) throw new TypeError('사제 panic PATCH/188TYPE 필요');
        this.#pool = pool;
        this.#speeds = speeds;
        this.#hooks = hooks ?? {};
    }

    // 같은 풀 연결을 검사하는 풀이다. (2026-11-08 WB04-E66)
    get pool() {
        return this.#pool;
    }

    // 현재 슬롯 필드를 요청한 폭으로 읽는다. (2026-11-08 WB04-E66)
    #read(sid, offset, width = 4) {
        const raw = this.#pool.physical(sid);
        let result = 0;
        // 요청한 BYTE 폭만 조합한다.
        for (let index = 0; index < width; index += 1) result += raw[offset + index] * 2 ** (8 * index);
        return result;
    }

    // 현재 좌표를 단정도로 읽는다. (2026-11-08 WB04-E66)
    #position(sid) {
        const raw = this.#pool.physical(sid);
        return [floatAt(raw, X_OFFSET), floatAt(raw, Y_OFFSET)];
    }

    // 491db0 전체 순서를 실행하며 첫 실제 임의 경로 시도 뒤에는 실패해도 반환한다. (2026-11-08 WB04-E66)
    run(priest) {
        const hooks = this.#hooks;
        const [startX, startY] = this.#position(priest);
        const temple = need(hooks.temple)(startX, startY);
        if (temple) {
            const hp = this.#read(priest, HP_OFFSET) | 0;
            if (hp < need(hooks.maximum)(priest) && this.#read(temple, OWNER_OFFSET, 1) === this.#read(priest, OWNER_OFFSET, 1)) {
                const kind = this.#read(priest, TYPE_OFFSET, 1);
                if (kind >= this.#speeds.length) throw new RangeError('사제 panic 현재TYPE 범위');
                const type = this.#speeds[kind];
                const saved = speedInteger(type.speed);
                type.speed = Math.fround(type.speed + type.speed);
                const path = need(hooks.target)(priest, temple);
                type.speed = Math.fround(saved);
                if (path) return;
            }
        }
        const [landX, landY] = this.#position(priest);
        const direct = !need(hooks.land)(landX, landY);
        // 원본은 영역이 맞는 첫 실제 경로 시도까지 임의 위치를 계속 고른다.
        for (;;) {
            const [x, y] = this.#position(priest);
            const candidateX = Math.fround(need(hooks.random)(-20, 20) + x);
            const candidateY = Math.fround(need(hooks.random)(-20, 20) + y);
            let valid = direct;
            if (!valid && need(hooks.land)(candidateX, candidateY)) {
                const candidateRegion = need(hooks.region)(candidateX, candidateY) >>> 0;
                const [currentX, currentY] = this.#position(priest);
                const currentRegion = need(hooks.region)(currentX, currentY) >>> 0;
                valid = candidateRegion === currentRegion;
            }
            if (valid) {
                need(hooks.point)(priest, candidateX, candidateY);
                return;
            }
        }
    }
}
