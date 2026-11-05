// 원본10.78의 SID 번호·50바이트 슬롯·자유 목록·삭제 기록을 한 저장소에서 관리한다.
// 근거: cpppj/o/SidPool.cpp의004abb10/004af1d0/004abf60/004abe00. (2026-11-04 WB04-A)

// 패치판 슬롯 크기와 서버/예측 영역의 첫 번호이다. CD10.72 배치는 이 모듈의 범위 밖이다.
const LAYOUT = Object.freeze({ stride: 50, serverFirst: 15000, predictableFirst: 23001 });
// raw 슬롯의 연결 WORD·이전 타입 BYTE·수명 상태 BYTE 위치이다.
const NEXT = 4, TYPE = 10, STATE = 11;
// 일반 할당의 첫 번호와 free/dead/void 상태 비트이다. 0~4와 예측 머리는 예약한다.
const FIRST = 5, FREE = 1, DEAD = 2, VOID = 4;
// 두 자유 목록 머리의 바이트 위치이다. 서버 머리는0번 슬롯+14에 겹쳐 놓인다.
const HEADS = Object.freeze([0, 14]);
// 삭제 기록은 영역마다 최근20쌍의 SID/type DWORD를 유지한다.
const HISTORY_SIZE = 20, HISTORY_WORDS = HISTORY_SIZE * 2;

// JS 경계에서 잘못된 정수/폭을 첫 쓰기 전에 거부한다. 원본의 정수 입력만 전달한다.
function integer(value, minimum, maximum, name) {
    if (!Number.isInteger(value) || value < minimum || value > maximum) throw new RangeError(`${name} 범위 오류`);
    return value;
}

// 원본 SID 공간을 소유한다. 세대 비트나 표시용 별도 객체 번호를 추가하지 않는다.
// slot/bytes/deletions는 독립 읽기 사본, allocated만 현재 슬롯의 쓰기 대여를 제공한다.
export class SidPool {
    // 실제 슬롯 전체와 현재 확보 개수이다. 양수 reset 뒤 이전 쓰기 대여는 무효다.
    #bytes;
    #capacity;
    // 원본의 가감 카운터와 예측 영역의 순차 할당 커서이다.
    #freeCount = 0;
    #predictableCursor = 0;
    // 각 일반 영역의 예약 꼬리와 client/other 순서의 삭제 DWORD 기록이다.
    #tails = new Uint16Array(2);
    #history = [new Uint32Array(HISTORY_WORDS), new Uint32Array(HISTORY_WORDS)];
    // 독립 풀의 서버 값과 세션이 제공하는 같은 {value:boolean} 소유자이다.
    #server;
    #serverOwner;

    // 23003~65535개 슬롯을 확보하고 초기화한다. serverOwner가 있으면 현재 값을 매번 빌린다.
    constructor(capacity = 32768, { server = true, serverOwner = null } = {}) {
        integer(capacity, LAYOUT.predictableFirst + 2, 65535, 'SID 풀 크기');
        if (typeof server !== 'boolean' || (serverOwner !== null &&
            (typeof serverOwner !== 'object' || typeof serverOwner.value !== 'boolean'))) {
            throw new TypeError('SID 풀 서버 권한은 boolean 또는 같은 {value:boolean} 소유자여야 합니다.');
        }
        this.#server = server;
        this.#serverOwner = serverOwner;
        this.#capacity = capacity;
        this.#bytes = new Uint8Array(capacity * LAYOUT.stride);
        this.reset();
    }

    // Fort 로더와 같은10.78 판본을 연결하기 위한 읽기 전용 판본 식별자이다.
    get edition() { return 'patch1078'; }

    // 변경할 수 없는 원본 슬롯/영역 배치를 반환한다.
    get layout() { return LAYOUT; }

    // reset 뒤에도 현재 저장소의 확보 개수를 읽는다.
    get capacity() { return this.#capacity; }

    // 원본 카운터를 읽는다. rebuild의 누적과 예측 반납의 비증가를 보존한다.
    get freeCount() { return this.#freeCount; }

    // 예측 영역 첫 번호에 더할 현재 커서를 읽는다.
    get predictableCursor() { return this.#predictableCursor; }

    // 공유 권한을 복제하지 않고 현재 소유자에서 다시 읽는다.
    get isServer() { return this.#serverOwner === null ? this.#server : this.#serverOwner.value; }

    // capacity>0이면 새 저장소를 먼저 확보하고,0/음수이면 같은 저장소를 초기화한다.
    // 두 삭제 기록은 보존한다. 실제 세션은 기존 Process/UI의 대여를 정리한 뒤 호출해야 한다.
    reset(capacity = 0) {
        integer(capacity, -0x80000000, 0x7fffffff, 'SID 재초기화 인자');
        if (capacity > 0) {
            integer(capacity, LAYOUT.predictableFirst + 2, 65535, 'SID 풀 재확보 크기');
            const replacement = new Uint8Array(capacity * LAYOUT.stride);
            this.#bytes = replacement;
            this.#capacity = capacity;
        }
        this.#bytes.fill(0);
        // 원본은2번부터 free로 표시하며0/1번은 상태0을 유지한다.
        for (let sid = 2; sid < this.#capacity; sid += 1) this.#bytes[sid * LAYOUT.stride + STATE] = FREE;
        // client/server/predictable 세 영역을 각각 끝0으로 닫는다.
        for (let sid = FIRST; sid < this.#capacity; sid += 1) {
            const end = sid < LAYOUT.serverFirst ? LAYOUT.serverFirst :
                (sid < LAYOUT.predictableFirst ? LAYOUT.predictableFirst : this.#capacity);
            this.#writeNext(sid * LAYOUT.stride, sid + 1 === end ? 0 : sid + 1);
        }
        this.#writeNext(HEADS[0], FIRST);
        this.#writeNext(HEADS[1], LAYOUT.serverFirst);
        this.#tails[0] = LAYOUT.serverFirst - 1;
        this.#tails[1] = LAYOUT.predictableFirst - 1;
        this.#freeCount = this.#capacity - FIRST;
        this.#predictableCursor = 1;
    }

    // 상위 월드 초기화가 사용하는80DWORD 정리이다. Pool 바이트/카운터는 보존한다.
    clearDeletionHistory() {
        // 원문처럼other20쌍을 먼저 지우고client20쌍을 지운다.
        for (const index of [1, 0]) this.#history[index].fill(0);
    }

    // flags bit0은 예측, bit1은client이며 예측 분기가 우선한다. 반환값은 원본WORD 번호이다.
    allocate(flags = 0) {
        integer(flags, 0, 0xffffffff, 'SID 할당 플래그');
        let sid;
        if (flags & 1) {
            const head = LAYOUT.predictableFirst * LAYOUT.stride;
            sid = LAYOUT.predictableFirst + this.#predictableCursor;
            if (sid >= this.#capacity || !(this.#bytes[head + STATE] & FREE)) throw new RangeError('예측 SID 소진');
            const offset = sid * LAYOUT.stride;
            if (!(this.#bytes[offset + STATE] & FREE)) throw new Error('예측 SID가 사용 중입니다.');
            if (this.isServer) {
                const next = this.#readNext(offset);
                if (next !== sid + 1 || next >= this.#capacity || !(this.#bytes[next * LAYOUT.stride + STATE] & FREE)) {
                    throw new RangeError('예측 SID 서버 꼬리 소진');
                }
                this.#writeNext(head, next);
            }
            this.#predictableCursor = (this.#predictableCursor + 1) >>> 0;
        } else {
            const client = (flags & 2) !== 0;
            if (!client && !this.isServer) throw new Error('서버 SID 할당 권한이 없습니다.');
            const list = client ? 0 : 1;
            sid = this.#readNext(HEADS[list]);
            // C++의 보호 경계와 같이 마지막 일반 슬롯을 예약하고 소진 때 상태를 보존한다.
            if (!sid || sid === this.#tails[list]) throw new RangeError('일반 SID 소진: 예약 꼬리를 유지합니다.');
            const offset = this.#offset(sid);
            if (!(this.#bytes[offset + STATE] & FREE)) throw new Error('free list SID가 사용 중입니다.');
            const next = this.#readNext(offset);
            if (!next || next >= this.#capacity) throw new Error('free list next 오류');
            this.#writeNext(HEADS[list], next);
        }
        const offset = sid * LAYOUT.stride;
        this.#bytes.fill(0, offset, offset + LAYOUT.stride);
        this.#bytes[offset + STATE] = VOID;
        this.#freeCount = (this.#freeCount - 1) >>> 0;
        return sid;
    }

    // void 할당만 반납한다. 삭제 기록→슬롯 지움/이전 타입→FIFO→상태 순서를 보존한다.
    release(sid) {
        const offset = this.#offset(sid);
        if (sid < FIRST || sid === LAYOUT.predictableFirst || !(this.#bytes[offset + STATE] & VOID) ||
            (this.#bytes[offset + STATE] & FREE)) throw new Error('반납할 SID가 void 할당 객체가 아닙니다.');
        const type = this.#bytes[offset + TYPE];
        const history = this.#history[sid < LAYOUT.serverFirst ? 0 : 1];
        history.copyWithin(2, 0, HISTORY_WORDS - 2);
        history[0] = sid;
        history[1] = type;
        this.#bytes.fill(0, offset, offset + LAYOUT.stride);
        this.#bytes[offset + TYPE] = type;
        if (!this.isServer && sid >= LAYOUT.serverFirst) {
            this.#bytes[offset + STATE] = FREE;
            return;
        }
        if (sid < LAYOUT.predictableFirst) {
            const list = sid < LAYOUT.serverFirst ? 0 : 1;
            this.#writeNext(this.#tails[list] * LAYOUT.stride, sid);
            this.#tails[list] = sid;
            this.#freeCount = (this.#freeCount + 1) >>> 0;
        }
        this.#bytes[offset + STATE] = FREE | DEAD;
    }

    // 서버free 슬롯을 번호순으로 다시 잇는다. freeCount는 재계산하지 않고 원본처럼 더한다.
    rebuildServer() {
        const free = [];
        // 가상 객체 효과 없이 현재raw free 비트만 읽는다.
        for (let sid = LAYOUT.serverFirst; sid < LAYOUT.predictableFirst; sid += 1) {
            if (this.#bytes[sid * LAYOUT.stride + STATE] & FREE) free.push(sid);
        }
        if (free.length === 0) throw new Error('서버 free SID가 없습니다.');
        this.#writeNext(HEADS[1], free[0]);
        // 마지막next는0이며 그 번호는 다음 일반 할당에서도 예약 꼬리로 남는다.
        for (let index = 0; index < free.length; index += 1) {
            this.#writeNext(free[index] * LAYOUT.stride, index + 1 < free.length ? free[index + 1] : 0);
        }
        this.#tails[1] = free[free.length - 1];
        this.#freeCount = (this.#freeCount + free.length) >>> 0;
    }

    // 현재 할당 슬롯을 빌린다. 같은 저장소의raw 바이트이며 양수reset 뒤 다시 빌려야 한다.
    allocated(sid) {
        const offset = this.#offset(sid);
        if (sid < FIRST || sid === LAYOUT.predictableFirst || (this.#bytes[offset + STATE] & FREE)) {
            throw new Error('할당되지 않은 SID 쓰기');
        }
        return this.#bytes.subarray(offset, offset + LAYOUT.stride);
    }

    // 상태와 무관한 물리 슬롯 쓰기 대여이다(free/예약 슬롯 포함, 물리 범위 0..capacity-1). 원본 공간 해제·건물 옆면 몸체가
    // 진단 복귀 뒤 free 슬롯이나 표면 후보를 그대로 읽고 쓰는 경로에만 쓴다(cpppj 의 SquidUnpop/RawBuildingSpace friend 접근과 같음).
    // 일반 객체 쓰기는 allocated 를 쓴다. 양수 reset 뒤에는 다시 빌려야 한다. (2026-11-05 WB04-D1: 공통 공간 해제 이식으로 추가)
    physical(sid) {
        const offset = this.#offset(sid);
        return this.#bytes.subarray(offset, offset + LAYOUT.stride);
    }

    // 읽기 전용span에 해당하는 독립 사본이다. free/예약 슬롯도 물리 범위 안이면 읽는다.
    slot(sid) {
        const offset = this.#offset(sid);
        return this.#bytes.slice(offset, offset + LAYOUT.stride);
    }

    // 검사/저장용 전체 독립 사본이다. 외부 변경으로 자유 목록을 훼손하지 않는다.
    bytes() { return this.#bytes.slice(); }

    // 지정 영역의 최근20개SID/type을 복사한다. 반환값 변경은 실제 삭제 기록을 바꾸지 않는다.
    deletions(client) {
        const history = this.#history[client ? 0 : 1];
        const result = [];
        // SID와이전 타입 DWORD를 같은 순서의 레코드로 만든다.
        for (let index = 0; index < HISTORY_SIZE; index += 1) {
            result.push({ sid: history[index * 2], type: history[index * 2 + 1] });
        }
        return result;
    }

    // client=true는0번 슬롯의머리, false는0번슬롯+14의서버 머리를 읽는다.
    firstFree(client) { return this.#readNext(HEADS[client ? 0 : 1]); }

    // 지정 일반 영역에서 마지막 한 칸으로 예약한 현재 꼬리 번호이다.
    tail(client) { return this.#tails[client ? 0 : 1]; }

    // raw next WORD를호스트 정렬/엔디언과 무관하게 읽는다.
    #readNext(offset) { return this.#bytes[offset + NEXT] | (this.#bytes[offset + NEXT + 1] << 8); }

    // raw next WORD만 낮은BYTE부터 쓰며 인접 payload를 보존한다.
    #writeNext(offset, next) {
        this.#bytes[offset + NEXT] = next & 255;
        this.#bytes[offset + NEXT + 1] = (next >>> 8) & 255;
    }

    // 원본WORD 번호와실제 확보 범위를 검사한 뒤 바이트 위치를 계산한다.
    #offset(sid) {
        integer(sid, 0, this.#capacity - 1, 'SID 번호');
        return sid * LAYOUT.stride;
    }
}
