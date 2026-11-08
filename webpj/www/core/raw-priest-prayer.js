// 사제 기도 입력(4940b0)·수신(493f80)·605 진행(494580)을 옮긴 모듈이다(cpppj `o/RawPriestPrayer.cpp/.h`, 10.78 전용).
//  - command(부모, 명령, 인자): 명령의 상위 6비트를 뺀 값이 기도(0x030000de)이면 packed 인자의 상위 WORD 를 희망 종류로 하여, 이미 첫 605 Regular 가 있거나 현재 방향이 10..14 이면 요청하지 않고,
//    그 밖에는 8바이트 패킷(1,0x6d,8,0,부모 WORD,종류 WORD)을 대상 9 로 요청하고 항상 1 을 반환한다. 공황(0x03000103)이면 공황 명령을 먼저 실행하고, 나머지는 부모 처리기에 위임한다.
//  - receive(패킷): 희망 종류(+42)를 쓰고 → 상태 비트 3(void/dead)이면 끝 → 이미 605 가 있거나 방향 10..14 이면 끝 → 권한자는 이동 프로세스 종류를 조회해 이동 Form 을 제거하고 같은 패킷을 배포 → 예약 성공 시 605(payload 1) 공유 Regular 등록 → 의식 단계 4.
//  - tick(부모, 개수, payload): 개수(+46)를 쓰고 → 이동/정지 조회 → 마비이거나 이동 Form 이 있으면 0 → 개수가 현재 희망 종류의 기도 횟수와 정확히 같을 때만 → 권한자이고 첫 mask1/168 내용물이 없으면 현재 내용물 삭제 후 희망 종류 생성 → 기본 프레임 지정 후 0, 같지 않으면 payload 그대로 반환.
// 각 원본 분기 위치에서 현재 권한을 다시 읽으며(boss 는 getter), 선택되지 않은 하위 효과는 요구하지 않고 도달한 누락만 진단한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E66: 사제 기도 이식으로 추가)

// 기도 Regular 사건 번호(605), 운명의 종류(168), 기도 명령/공황 명령 번호, 슬롯의 TYPE/상태/희망 종류/개수 위치와 방향 범위이다. (2026-11-08 WB04-E66)
export const PRIEST_PRAYER_EVENT = 605;
const DEVASTATION = 168;
const PRAY_COMMAND = 0x030000de;
const PANIC_COMMAND = 0x03000103;
const COMMAND_MASK = 0x03ffffff;
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const KIND_OFFSET = 42;
const COUNT_OFFSET = 46;
const PRAYING_FIRST = 10;
const PRAYING_LAST = 14;
const TYPE_COUNT = 188;

// 선택되지 않은 경계는 요구하지 않으며 도달한 누락만 오류로 드러낸다. (2026-11-08 WB04-E66)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('사제 기도의 실제 하위 미연결');
    return hook;
}

// pool 은 10.78 SidPool, types 는 188칸 { prayTime, defaultFrame } 표(참조), boss 는 현재 권한을 돌려주는 getter, hooks 는 하위 효과 묶음이다:
//   hasEvent(sid, 사건)→bool, direction(sid)→signed, movementType(종류)→수, movement(sid, 종류, 순번, 제거)→이동 Form SID(0=없음), request(대상, 패킷), broadcast(패킷), reserveShared()→bool,
//   addShared(sid, 사건, payload), ritual(sid, 단계), immobile(sid)→bool, containedType(sid, mask, 종류, 순번)→수, discard(sid), create(sid, 종류, 인자, flags, 부착 flags, 품질), setFrame(sid, 프레임, flags), panic(sid), parent(sid, 명령, 인자)→수. (2026-11-08 WB04-E66)
export class RawPriestPrayer {
    // 같은 실제 사제/내용물/Form 을 보관하는 풀, 현재 TYPE 의 기도 표, 현재 권한 getter, 외부 효과이다.
    #pool;
    #types;
    #boss;
    #hooks;

    // 판본/타입 표 크기를 확인한다. 하위 효과는 도달할 때만 요구한다. (2026-11-08 WB04-E66)
    constructor(pool, types, boss, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || typeof boss !== 'function') throw new TypeError('사제 기도 PATCH 풀/188TYPE 필요');
        this.#pool = pool;
        this.#types = types;
        this.#boss = boss;
        this.#hooks = hooks ?? {};
    }

    // 같은 풀 연결을 검사하는 풀이다. (2026-11-08 WB04-E66)
    get pool() {
        return this.#pool;
    }

    // 현재 물리 슬롯의 리틀엔디언 필드를 요청한 폭으로 읽는다. (2026-11-08 WB04-E66)
    #read(sid, offset, width = 4) {
        const raw = this.#pool.physical(sid);
        let result = 0;
        // 요청한 폭의 낮은 바이트부터 조합한다.
        for (let index = 0; index < width; index += 1) result += raw[offset + index] * 2 ** (8 * index);
        return result;
    }

    // 예약/free 슬롯에도 원본처럼 현재 DWORD 를 쓴다(할당 필터를 추가하지 않음). (2026-11-08 WB04-E66)
    #write(sid, offset, value) {
        const raw = this.#pool.physical(sid);
        // 정확히 현재 DWORD 의 낮은 바이트부터 쓴다.
        for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
    }

    // 실제 타입 표 밖의 손상 입력은 명시 오류이다. (2026-11-08 WB04-E66)
    #type(kind) {
        if (kind >= this.#types.length) throw new RangeError('사제 기도 현재 TYPE 범위');
        return this.#types[kind];
    }

    // 기도 입력(4940b0)이다. 기도 명령만 요청/1 반환으로 소비하고 나머지는 부모 처리기로 넘긴다. (2026-11-08 WB04-E66)
    command(parent, code, parameter) {
        const command = (code & COMMAND_MASK) >>> 0;
        const hooks = this.#hooks;
        if (command === PRAY_COMMAND) {
            const kind = (parameter >>> 16) & 0xffff;
            if (!need(hooks.hasEvent)(parent, PRIEST_PRAYER_EVENT)) {
                const direction = need(hooks.direction)(parent) | 0;
                if (direction < PRAYING_FIRST || direction > PRAYING_LAST) {
                    need(hooks.request)(9, Uint8Array.of(1, 0x6d, 8, 0, parent & 0xff, (parent >>> 8) & 0xff, kind & 0xff, (kind >>> 8) & 0xff));
                }
            }
            return 1;
        }
        if (command === PANIC_COMMAND) need(hooks.panic)(parent);
        return need(hooks.parent)(parent, code >>> 0, parameter >>> 0) >>> 0;
    }

    // 기도 요청 수신(493f80)이다. 패킷의 부모/희망 종류 WORD 를 읽는다. (2026-11-08 WB04-E66)
    receive(packet) {
        if (packet.length < 8) throw new RangeError('기도6d 부모/종류 WORD 부족');
        const parent = packet[4] | (packet[5] << 8);
        const kind = packet[6] | (packet[7] << 8);
        const hooks = this.#hooks;
        this.#write(parent, KIND_OFFSET, kind);
        if (this.#read(parent, STATE_OFFSET, 1) & 3) return;
        if (need(hooks.hasEvent)(parent, PRIEST_PRAYER_EVENT)) return;
        const direction = need(hooks.direction)(parent) | 0;
        if (direction >= PRAYING_FIRST && direction <= PRAYING_LAST) return;
        if (this.#boss()) {
            const movement = need(hooks.movementType)(this.#read(parent, TYPE_OFFSET, 1));
            need(hooks.movement)(parent, movement, 0, true);
            need(hooks.broadcast)(packet);
        }
        if (need(hooks.reserveShared)()) need(hooks.addShared)(parent, PRIEST_PRAYER_EVENT, 1);
        need(hooks.ritual)(parent, 4);
    }

    // 605 진행(494580)이다. 반환은 다음 예약 payload 이거나(기도 횟수 불일치) 0(종료)이다. (2026-11-08 WB04-E66)
    tick(parent, count, payload) {
        const hooks = this.#hooks;
        this.#write(parent, COUNT_OFFSET, count >>> 0);
        const movementType = need(hooks.movementType)(this.#read(parent, TYPE_OFFSET, 1));
        const movement = need(hooks.movement)(parent, movementType, 0, false);
        if (need(hooks.immobile)(parent) || movement) return 0;
        if ((count | 0) !== this.#type(this.#read(parent, KIND_OFFSET)).prayTime) return payload;
        if (this.#boss() && !need(hooks.containedType)(parent, 1, DEVASTATION, 0)) {
            need(hooks.discard)(parent);
            need(hooks.create)(parent, this.#read(parent, KIND_OFFSET), 0, 1, 0, 0);
        }
        need(hooks.setFrame)(parent, this.#type(this.#read(parent, TYPE_OFFSET, 1)).defaultFrame, 0);
        return 0;
    }
}
