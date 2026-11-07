// 원본 10.78 패치판의 분산 SP 저장소(ScrambledSpStore: 0040eba0 초기화·0040ec50 읽기·0040edb0 쓰기·00491680 회전)를 옮긴 모듈이다.
//  - 소유자(0~8)별 SP 의 32비트 값은 32개 배열(각 9 DWORD, 원본 operator_new(0x24))에 흩어 둔다. 값의 k번째 비트는 배열 permutation[k] 의
//    소유자 DWORD 의 (offset+k)&31 번째 비트이며, 쓰기 때마다 같은 DWORD 의 다른 위치에 게임 난수 잡음 5개를 OR 한다.
//  - 초기화는 재시드 → offset(난수 100) → 항등 순열·추적 offset 쓰기 → 32번 교환(난수 32) → 다시 항등 → 32개 배열 확보 순서로 게임 난수를 소비한다.
//  - 회전은 offset 을 signed DWORD 로 3 늘리고(9600 초과면 한 번 9600을 뺌) 모든 DWORD 를 왼쪽으로 3비트 돌린다. 저장 값은 변하지 않는다.
// 게임 난수(004558c0)는 random.js 의 GameRandom 을 같은 객체로 빌린다(SP 쓰기·공격·생성이 같은 스트림을 소비).
// 근거: cpppj `o/SpStore.cpp/.h`(기준선 1f00a25a). 원본 관측: `cpppj/tests/fixtures/reward-x86.tsv`(초기 SP 쓰기 뒤 난수 상태·배열 Adler 열)·
//       `sp-add-bits-x86.tsv`(SP 가산 504행). 검사는 tests/squid-reward.test.mjs 에 있다. CD 10.72 는 SP 를 전역 float 으로 두므로 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-07 WB04-D5b: SquidReward 의 패치판 SP 저장 장치로 추가)
import { GameRandom } from './random.js';

// 비트 수·소유자 수와 초기화 offset 상한·순열/잡음 상한·칸당 잡음 수·회전 증가량/상한이다(원본 push 0x64·0x20, edi=5, +3·9600). (2026-11-07 WB04-D5b)
const BITS = 32, OWNERS = 9;
const OFFSET_LIMIT = 100, BIT_LIMIT = 32, NOISE_PER_BIT = 5;
const ROTATE_STEP = 3, OFFSET_WRAP = 9600;

// 소유자 번호가 0~8 정수인지 확인한다. 원본은 9 이상이면 힙 밖을 읽고 쓰므로 상태를 바꾸기 전에 거부한다. (2026-11-07 WB04-D5b)
function checkOwner(owner) {
    if (!Number.isInteger(owner) || owner < 0 || owner >= OWNERS) throw new RangeError('SP 저장소 소유자 번호는 0~8이어야 합니다.');
}

// 32비트 값의 왼쪽 순환이다. (2026-11-07 WB04-D5b)
function rotl(value, count) {
    return ((value << count) | (value >>> (32 - count))) >>> 0;
}

// 패치판 분산 SP 저장소이다(cpppj ScrambledSpStore).
// rng 는 같은 GameRandom, reseed() 는 004559d0 시각 재시드가 만든 새 난수 상태(DWORD)를 돌려주는 함수다.
// allocation 은 초기화되지 않은 힙의 DWORD 채움값(기본 0) 또는 각 36바이트 할당 입력을 채우는 함수(bytes: Uint8Array(36))다.
// 사용: const store = new ScrambledSpStore(game, () => seed, 0xcdcdcdcd); store.set(1, floatBits); store.get(1). (2026-11-07 WB04-D5b)
export class ScrambledSpStore {
    // 비트 수(배열 개수)와 소유자 수이다. 배열 k 의 owner 번째 DWORD 가 pools[k*9+owner] 이다.
    static BITS = BITS;
    static OWNERS = OWNERS;

    // 같은 게임 난수·재시드 함수·채움값/할당 입력이다.
    #rng;
    #reseed;
    #fill = 0;
    #allocation = null;
    // 초기화 여부(DAT_0054db24)·비트 위치 기준(00568af8)·빌린 사제 추적 offset(0054d4e0) 소유자이다.
    #initialized = false;
    #offset = 0;
    #tracking = null;
    // 실제 WORD 순열(00531890)과 32×9 DWORD 분산 배열, 첫 배열 확보 여부(00594fd0 의 NULL 조건)이다.
    #permutation = new Int16Array(BITS);
    #pools = new Uint32Array(BITS * OWNERS);
    #firstPool = false;

    // 재시드 함수가 없으면 첫 초기화에서 오류다(생성 때는 받기만 한다). allocation 이 함수면 각 할당 입력을 순서대로 받는다.
    constructor(rng, reseed, allocation = 0) {
        if (!(rng instanceof GameRandom)) throw new TypeError('SP 저장소는 같은 GameRandom 을 빌려야 합니다.');
        if (reseed !== null && reseed !== undefined && typeof reseed !== 'function') throw new TypeError('SP 저장소 재시드는 함수여야 합니다.');
        this.#rng = rng;
        this.#reseed = reseed ?? null;
        if (typeof allocation === 'function') this.#allocation = allocation;
        else {
            if (!Number.isInteger(allocation) || allocation < 0 || allocation > 0xffffffff) throw new RangeError('SP 저장소 채움값은 DWORD 여야 합니다.');
            this.#fill = allocation >>> 0;
        }
    }

    // 0054d4e0 을 소유한 외부 사제 상태({ value: signed DWORD })를 첫 초기화 전에 연결한다. 다른 소유자 재연결·늦은 연결은 거부한다.
    bindTrackingOffset(tracking) {
        if (!tracking || typeof tracking !== 'object' || !('value' in tracking)) throw new TypeError('SP 추적 offset 소유자는 { value } 여야 합니다.');
        if (this.#initialized || (this.#tracking && this.#tracking !== tracking)) throw new Error('SP 추적 offset 의 늦은 연결/다른 소유자 오류');
        this.#tracking = tracking;
    }

    // 0040eba0 이후인지(DAT_0054db24)이다.
    get initialized() { return this.#initialized; }

    // 0040eba0: 재시드 → 실제 offset → 항등/추적 offset → 32번 순열 교환 → 다시 항등 → 실제 배열 준비 순서다.
    initialize() {
        if (!this.#reseed) throw new Error('SP 저장소 재시드 콜백 누락');
        const rng = this.#rng;
        rng.setState(this.#reseed() >>> 0);
        this.#offset = rng.next(OFFSET_LIMIT);
        // 첫 항등 순열을 만든 뒤 같은 추적 offset 전역에도 현재 offset 을 쓴다.
        for (let index = 0; index < BITS; index += 1) this.#permutation[index] = index;
        if (this.#tracking) this.#tracking.value = this.#offset | 0;
        // 실제 32번 교환을 수행한다(난수 소비 순서 보존).
        for (let index = 0; index < BITS; index += 1) {
            const other = rng.next(BIT_LIMIT);
            const value = this.#permutation[index];
            this.#permutation[index] = this.#permutation[other];
            this.#permutation[other] = value;
        }
        // 교환 결과를 저장에 쓰기 전에 다시 항등 순열로 덮어쓴다.
        for (let index = 0; index < BITS; index += 1) this.#permutation[index] = index;
        // 각 할당이 정상 반환한 뒤에만 같은 행을 보관한다. 뒤의 실패가 앞선 행을 되돌리지 않는다.
        for (let index = 0; index < BITS; index += 1) {
            const input = new Uint32Array(OWNERS);
            if (this.#allocation) {
                const bytes = new Uint8Array(OWNERS * 4);
                this.#allocation(bytes);
                const view = new DataView(bytes.buffer);
                // 36바이트를 리틀엔디언 DWORD 9개로 읽는다.
                for (let owner = 0; owner < OWNERS; owner += 1) input[owner] = view.getUint32(owner * 4, true);
            } else input.fill(this.#fill);
            this.#pools.set(input, index * OWNERS);
            if (index === 0) this.#firstPool = true;
        }
        this.#initialized = true;
    }

    // 0040ec50: 초기화 전이면 먼저 초기화하고 소유자의 32비트 값을 모은다.
    get(owner) {
        checkOwner(owner);
        if (!this.#initialized) this.initialize();
        let bits = 0;
        // 값의 32비트를 낮은 비트부터 하나씩 모은다.
        for (let k = 0; k < BITS; k += 1) {
            const mask = (1 << ((this.#offset + k) & 31)) >>> 0;
            if (this.#pools[this.#permutation[k] * OWNERS + owner] & mask) bits = (bits | (1 << k)) >>> 0;
        }
        return bits >>> 0;
    }

    // 0040edb0: 초기화 전이면 먼저 초기화하고 값 비트를 쓴 뒤, 칸마다 실제 비트를 뺀 위치에 난수 잡음 5개를 OR 한다.
    set(owner, bits) {
        checkOwner(owner);
        if (!Number.isInteger(bits) || bits < -0x80000000 || bits > 0xffffffff) throw new RangeError('SP 저장 값은 32비트 정수여야 합니다.');
        if (!this.#initialized) this.initialize();
        const value = bits >>> 0;
        // 값의 32비트를 낮은 비트부터 하나씩 기록한다.
        for (let k = 0; k < BITS; k += 1) {
            const index = this.#permutation[k] * OWNERS + owner;
            const mask = (1 << ((this.#offset + k) & 31)) >>> 0;
            let word = this.#pools[index];
            word = ((value >>> k) & 1) ? (word | mask) >>> 0 : (word & ~mask) >>> 0;
            // 잡음은 실제 비트를 제외한 위치에만 들어간다.
            for (let n = 0; n < NOISE_PER_BIT; n += 1) word = (word | ((1 << this.#rng.next(BIT_LIMIT)) & ~mask)) >>> 0;
            this.#pools[index] = word;
        }
    }

    // 00491680: 필요하면 초기화하고 offset 을 signed DWORD 로 3 늘린다(9600 초과면 한 번 뺌). 모든 32×9 DWORD 를 왼쪽으로 3비트 돌린다.
    rotate() {
        if (!this.#initialized) this.initialize();
        const advanced = (this.#offset + ROTATE_STEP) >>> 0;
        this.#offset = (advanced | 0) > OFFSET_WRAP ? (advanced - OFFSET_WRAP) >>> 0 : advanced;
        // 현재 순열의 각 배열을 소유자 0..8 순으로 같은 물리 DWORD 에서 회전한다.
        for (const index of this.#permutation) {
            // 한 배열의 9 DWORD 를 순서대로 회전한다.
            for (let owner = 0; owner < OWNERS; owner += 1) {
                const at = index * OWNERS + owner;
                this.#pools[at] = rotl(this.#pools[at], ROTATE_STEP);
            }
        }
    }

    // 진단용 비트 위치 기준(00568af8 의 현재값)이다.
    get offset() { return this.#offset; }

    // 진단용 분산 배열의 독립 사본(배열 k 의 owner 번째 DWORD 가 [k*9+owner])이다.
    pools() { return this.#pools.slice(); }

    // 진단용 WORD 순열의 독립 사본이다.
    permutation() { return this.#permutation.slice(); }

    // 첫 분산 배열이 확보됐는지(00594fd0 이 NULL 이 아닌지)이다.
    get firstPoolReady() { return this.#firstPool; }

    // 빌린 추적 offset 의 현재값이다. 미연결이면 임의 초기값 대신 오류다.
    get trackingOffset() {
        if (!this.#tracking) throw new Error('SP 추적 offset 의 실제 소유자 연결 누락');
        return this.#tracking.value;
    }
}
