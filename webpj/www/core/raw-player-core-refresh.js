// 현재 Core 의 기술 bit 와 자기 소유 다이스의 count 하한을 0044fc00 순서로 갱신하는 모듈이다(cpppj `o/RawPlayerCoreRefresh.cpp/.h`, 원문 357 BYTE 부모).
// 순서: Core+6(DWORD)을 0 으로 쓴다 → 기술 순회(부모 기술 SID 의 종속 finder 를 Begin, 후보마다 현재 Pool 을 다시 조회해 종류 WORD(+18)를 먼저 읽고 flags(+20)에 4 가 있으면
//   TYPE 의 현재 기술 bit(+1ec) 를 받아 -1 이 아니면 Core+6 에 1<<bit 를 OR — bit 범위 밖은 Ch.cpp 0x15a 진단 뒤에도 하위 5비트로 같은 Core 에 쓴다) → 기술 순회가 끝난 뒤
//   Pool 을 최초 cached 상한(capacity)까지 free(bit0)만 건너뛰며 순회해 유효 관문(현재 capacity 를 읽음)을 통과한 객체 중 TYPE=현재 다이스 TYPE 이고 소유자=현재 owner 인 것의
//   가상 level 을 모아 max(초기 1)를 구하되, 첫 반환이 max 보다 크고 dead(bit1)가 아니면 두 번째 반환으로 max 를 대체한다 → Core+5 가 max-1 보다 작으면 max-1 을 쓴다.
// 하위 장치는 호출 위치에서만 요구하며 사본 상태나 성공 기본값을 추가하지 않는다. 짧은 span/원시 무효 포인터는 선택 접근의 물리 계약 오류(RangeError)로 거부한다.
// hooks: { core() → Uint8Array, technologyParent() → SID, begin(parent) → { current(), next() }, pool() → { bytes, address, capacity }, technologyBit(kind) → signed,
//          playerType(), owner(), level(form: { bytes, address }) → signed, assertion(line) }.
// 근거: cpppj 기준선 1f00a25a, 원본 관측 player-core-refresh-x86.tsv(642행)·player-core-refresh-guard-x86.tsv(20행)(tests/player-core-refresh.test.mjs). 10.78 전용이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E3: Player Core 갱신 이식으로 추가)

// PATCH 10.78 raw 객체 필드의 실제 위치이다: 슬롯 폭 50, TYPE, 상태 flags, 종류 WORD, 목록 flags, 소유자. (2026-11-08 WB04-E3)
const STRIDE = 50, TYPE = 10, FLAGS = 11, KIND = 18, LIST_FLAGS = 20, OWNER = 34;

// 선택해서 도달한 자식만 그 위치에서 필수로 요구한다. (2026-11-08 WB04-E3)
function need(function_, name) {
    if (typeof function_ !== 'function') throw new Error(`현재 Core 갱신의 실제 하위 장치(${name}) 미연결`);
    return function_;
}

// 실제 단일 load 폭만 검사하며 표 끝/NULL 을 임의 값으로 대체하지 않는다. offset 은 uint32 로 감긴다. (2026-11-08 WB04-E3)
function read(bytes, offset, width) {
    const at = offset >>> 0;
    // NULL 포인터(null)는 길이 0 의 물리로 본다.
    if (bytes === null || at > bytes.length || width > bytes.length - at) throw new RangeError('현재 Core 갱신의 실제 load 물리 범위');
    let value = 0;
    // 지정 폭 전체를 원문 레지스터에 캡처한다.
    for (let byte = 0; byte < width; byte += 1) value |= bytes[at + byte] << (byte * 8);
    return value >>> 0;
}

// 같은 store 전체 폭의 host 물리를 확인한 뒤 그 바이트만 쓴다(이웃 Core 필드는 그대로). (2026-11-08 WB04-E3)
function write(bytes, offset, width, value) {
    const at = offset >>> 0;
    if (bytes === null || at > bytes.length || width > bytes.length - at) throw new RangeError('현재 Core 갱신의 실제 store 물리 범위');
    for (let byte = 0; byte < width; byte += 1) bytes[at + byte] = (value >>> (byte * 8)) & 0xff;
}

// 부모가 항상 value 1 로 호출하는 실제 Ch 0042b280 의 범위 진단/마스크 shift/같은 this 쓰기이다. (2026-11-08 WB04-E3)
function setTechnology(core, bit, hooks) {
    if (bit < 0 || bit >= 32) need(hooks.assertion, 'assertion')(0x15a);
    const old = read(core, 6, 4);
    write(core, 6, 4, (old | (1 << (bit & 31))) >>> 0);
}

// 실제 객체 주소와 접근 가능한 그 슬롯의 같은 물리 뷰를 가상 자식에게 넘긴다. (2026-11-08 WB04-E3)
function form(pool, offset) {
    const at = offset >>> 0;
    if (at > pool.bytes.length) throw new RangeError('현재 Core 갱신 가상 객체 물리 범위');
    return { bytes: pool.bytes.subarray(at, at + Math.min(STRIDE, pool.bytes.length - at)), address: (pool.address + at) >>> 0 };
}

// 최초 0040eb40 과 후속 인라인 루프는 1을 증가한 뒤 cached 상한 안에서 free bit0 만 건너뛴다. 현재 capacity 로 최초 cached 상한을 대체하지 않는다. (2026-11-08 WB04-E3)
function advance(state, cachedEnd, pool) {
    state.index = (state.index + 1) | 0;
    while (state.index < cachedEnd) {
        if (!(read(pool.bytes, Math.imul(state.index, STRIDE) + FLAGS, 1) & 1)) return;
        state.index = (state.index + 1) | 0;
    }
}

// 0044fc00: 기술 순회를 먼저 완료하고 같은 현재 소유자의 다이스 count 로 Core+5 하한을 갱신한다. 사용: refreshPlayerCore(hooks). (2026-11-08 WB04-E3)
export function refreshPlayerCore(hooks) {
    write(need(hooks.core, 'core')(), 6, 4, 0);
    const parent = need(hooks.technologyParent, 'technologyParent')();
    const cursor = need(hooks.begin, 'begin')(parent);
    // 독립 Finder 의 현재 후보와 이미 보관한 next 를 사용한다.
    for (;;) {
        const candidate = need(cursor.current, 'cursor.current')();
        if (!candidate) break;
        const pool = need(hooks.pool, 'pool')();
        const offset = Math.imul(candidate, STRIDE) >>> 0;
        const flags = read(pool.bytes, offset + LIST_FLAGS, 1);
        // 원문은 flag 분기 전에도 종류 WORD 를 읽는다.
        const kind = read(pool.bytes, offset + KIND, 2);
        if (flags & 4) {
            const bit = need(hooks.technologyBit, 'technologyBit')(kind) | 0;
            if (bit !== -1) setTechnology(need(hooks.core, 'core')(), bit, hooks);
        }
        need(cursor.next, 'cursor.next')();
    }
    const initial = need(hooks.pool, 'pool')();
    // 실제 iterator 초기 1 과 누적 max 초기 1 이다.
    const cachedEnd = initial.capacity | 0;
    const walk = { index: 1 };
    let maximum = 1;
    advance(walk, cachedEnd, need(hooks.pool, 'pool')());
    let pool = need(hooks.pool, 'pool')();
    // 유효 관문은 현재 capacity 를 읽고 free-skip 만 최초 cached 상한을 유지한다.
    for (;;) {
        const offset = Math.imul(walk.index, STRIDE) >>> 0;
        const address = (pool.address + offset) >>> 0;
        if (address < ((pool.address + STRIDE) >>> 0) || address >= ((pool.address + Math.imul(need(hooks.pool, 'pool')().capacity, STRIDE)) >>> 0)) break;
        if (read(pool.bytes, offset + TYPE, 1) === (need(hooks.playerType, 'playerType')() >>> 0) && read(pool.bytes, offset + OWNER, 1) === (need(hooks.owner, 'owner')() >>> 0)) {
            const first = need(hooks.level, 'level')(form(pool, offset)) | 0;
            pool = need(hooks.pool, 'pool')();
            if (first > maximum && !(read(pool.bytes, offset + FLAGS, 1) & 2)) {
                // 둘째 반환을 다시 max 와 비교하거나 첫째로 대체하지 않는다.
                maximum = need(hooks.level, 'level')(form(pool, offset)) | 0;
                pool = need(hooks.pool, 'pool')();
            }
        }
        advance(walk, cachedEnd, pool);
    }
    const lower = (maximum - 1) | 0;
    const first = read(need(hooks.core, 'core')(), 5, 1);
    if (lower > first) write(need(hooks.core, 'core')(), 5, 1, lower >>> 0);
    else {
        // 원문 else 는 다른 현재 Core 일 수 있는 두 번째 load 와 세 번째 store 이다.
        const selected = read(need(hooks.core, 'core')(), 5, 1);
        write(need(hooks.core, 'core')(), 5, 1, selected);
    }
}
