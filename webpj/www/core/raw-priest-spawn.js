// 사제 해방의 자리 탐색과 생성 호출, Carrier 검사 분기(0044b2e0)를 옮긴 모듈이다(cpppj `o/RawPriestSpawn.cpp/.h`, 10.78 전용).
// 순서: 좌표에 float32 0.99999 를 더해 절삭한 칸에서 시작하여 북→동→남→서 나선(한 변 길이 1,1,2,2,…,11,11; 총 132개 후보)으로 이동한다. 범위 밖(가장자리 0/255 포함) 후보도 같은 방향으로 한 칸 이동하며 예산을 소비한다.
//   후보 칸의 spot 이 0 이면 배치 검사 없이, 아니면 배치 검사가 참이고 spot 에 0x10 이 없을 때만 채택하여 → 생성 → WORD 전체 복사 → 현재 새 자산 타입이 다리 타입이면 표면 알림 → 소유자(하위 7비트) 지정 → Pop → Carrier 검사 래퍼(genus 0x30000 비트가 있으면 가상 +0xcc 요청, 없으면 원본 assert).
//   132개 후보를 모두 소진하면 현재 debug 전역이 켜져 있을 때 원본 진단으로 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E68: 사제 생성 이식으로 추가)

// 슬롯의 TYPE/WORD 위치, Carrier genus 비트와 소유자 마스크, 첫 자산 타입 번호, 지도 크기, 스냅 bias(float32 0.99999)이다. (2026-11-08 WB04-E68)
const TYPE_OFFSET = 10;
const WORD_OFFSET = 12;
const CARRIER = 0x30000;
const OWNER_MASK = 0x7f;
const FIRST_ASSET_TYPE = 70;
const SIDE = 256;
const CELLS = SIDE * SIDE;
const SNAP_BIAS = Math.fround(0.99999);

// 원본 8방향 표의 0/2/4/6번(북→동→남→서) 이동량, 나선 단계 범위(단계 2~23 에서 길이 단계/2)이다. (2026-11-08 WB04-E68)
const DX = Object.freeze([0, 1, 0, -1]);
const DY = Object.freeze([-1, 0, 1, 0]);
const FIRST_STAGE = 2;
const STAGE_END = 24;
const INT_LOW = -2147483648;
const INT_HIGH = 2147483647;

// 사제 생성 처리 중 읽는 전역이다: 다리 타입(005411a0; WORD 쓰기 뒤 표면 알림 대상, 하위 효과가 바꿀 수 있음)과 모든 자리 탐색 실패 때 원본 진단을 요구하는 checking(005e4794 의 전체 DWORD, 실제 값을 빌리는 sharedChecking getter)이다. (2026-11-08 WB04-E68)
export class PriestSpawnState {
    bridgeType = 82;
    checkingPlacement = 1;
    sharedChecking = null;

    // 실패 시점의 현재 checking 비트를 다시 읽는다(비영만 켜짐). (2026-11-08 WB04-E68)
    isChecking() {
        return (this.sharedChecking ? this.sharedChecking() >>> 0 : this.checkingPlacement >>> 0) !== 0;
    }
}

// x87 의 넓은 덧셈 뒤 CRT 절삭을 보존하고 비유한/정수 범위 밖 입력을 거부한다. (2026-11-08 WB04-E68)
function snap(coordinate) {
    const value = coordinate + SNAP_BIAS;
    if (!Number.isFinite(value) || value < INT_LOW || value > INT_HIGH) throw new RangeError('사제 생성 스냅 좌표 범위 오류');
    return Math.trunc(value) | 0;
}

// pool 은 10.78 SidPool, types 는 188칸 TYPE 표(참조), spots 는 65536칸 spot 지도(참조), state 는 PriestSpawnState,
//   hooks = { mayPlace({ type, x, y, flags, owner, mode })→bool, create(type, flags)→SID, setOwner(sid, 소유자), pop(sid, x, y, flags), notifySurface(sid), checkCarrier(sid) } 이며 모두 필요하다. (2026-11-08 WB04-E68)
export class RawPriestSpawn {
    // 객체 슬롯 풀, 타입 표, spot 지도, 전역, 하위 효과이다.
    #pool;
    #types;
    #spots;
    #state;
    #hooks;

    // 판본별 타입 표 길이·공간 크기·필수 경계를 구성 때 확인한다. (2026-11-08 WB04-E68)
    constructor(pool, types, spots, state, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || !(spots instanceof Uint8Array) || spots.length !== CELLS
            || ['mayPlace', 'create', 'setOwner', 'pop', 'notifySurface', 'checkCarrier'].some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('사제 생성 자료/하위 효과 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#spots = spots;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 종속 해방과 같은 풀인지 확인하는 풀이다. (2026-11-08 WB04-E68)
    get pool() {
        return this.#pool;
    }

    // 실패해도 호출자에게 반환하지 않으며 패치는 현재 debug 전역에 따라 진단한다. word 는 하위 16비트만 쓰고 소유자와 배치 검사에는 하위 7비트만 전달한다. (2026-11-08 WB04-E68)
    spawn(x, y, type, word) {
        if (type < FIRST_ASSET_TYPE || type >= this.#types.length) throw new RangeError('사제 생성 타입 범위 오류');
        let cellX = snap(x);
        let cellY = snap(y);
        const wordValue = word & 0xffff;
        const owner = wordValue & OWNER_MASK;
        let direction = 0;
        // 나선의 한 변 길이는 1,1,2,2,...,11,11 이다.
        for (let stage = FIRST_STAGE; stage < STAGE_END; stage += 1) {
            // 범위 밖 후보도 같은 방향으로 한 칸 이동하며 탐색 예산을 소비한다.
            for (let step = 0; step < (stage >> 1); step += 1) {
                if (cellX > 0 && cellY > 0 && cellX < SIDE - 1 && cellY < SIDE - 1) {
                    const spot = this.#spots[cellY * SIDE + cellX];
                    const candidateX = Math.fround(cellX);
                    const candidateY = Math.fround(cellY);
                    if (!spot || (this.#hooks.mayPlace({ type, x: candidateX, y: candidateY, flags: 0, owner, mode: 0 }) && !(spot & 0x10))) {
                        const created = this.#hooks.create(type, 0);
                        const raw = this.#pool.allocated(created);
                        raw[WORD_OFFSET] = wordValue & 0xff;
                        raw[WORD_OFFSET + 1] = wordValue >>> 8;
                        if (raw[TYPE_OFFSET] === this.#state.bridgeType) this.#hooks.notifySurface(created);
                        this.#hooks.setOwner(created, owner);
                        this.#hooks.pop(created, candidateX, candidateY, 0);
                        this.#checkCarrier(created);
                        return;
                    }
                }
                cellX += DX[direction];
                cellY += DY[direction];
            }
            direction = (direction + 1) % DX.length;
        }
        if (this.#state.isChecking()) throw new Error('사제 생성 자리 탐색 실패: 원본 debug assert');
    }

    // Carrier 검사 래퍼의 genus 분기는 실제 구현이며 가상 +0xcc 몸체만 외부에 남긴다. Pop 뒤의 현재 새 자산 타입으로 판정한다. (2026-11-08 WB04-E68)
    #checkCarrier(sid) {
        const type = this.#pool.allocated(sid)[TYPE_OFFSET];
        if (type >= this.#types.length) throw new RangeError('사제 Carrier 검사 타입 범위 오류');
        if (this.#types[type].flags2 & CARRIER) this.#hooks.checkCarrier(sid);
        else throw new Error('사제 Carrier 검사 genus 누락: 원본 assert');
    }
}

// 같은 풀의 종속 해방에서 사제 생성/자리 탐색 경계만 실제 몸체로 연결한다. 다른 일반 자산 해방 효과는 보존한다. (2026-11-08 WB04-E68)
export function makePriestSpawnHooks(pool, spawn, hooks) {
    if (spawn.pool !== pool) throw new TypeError('사제 생성/종속 해방의 SID 풀이 다릅니다');
    return { ...hooks, spawnPriest: (x, y, type, word) => { spawn.spawn(x, y, type, word); } };
}
