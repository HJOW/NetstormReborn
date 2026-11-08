// 실제 100개/32바이트 섬 목록(IslandList 00568b60)과 현재 owner 변경 효과를 같은 pool/해시/목록/Player 상태에서 실행하는 모듈이다(cpppj `o/RawIslandRuntime.cpp/.h`, 10.78: 패치 0046edf0..0046f320).
//  - 저장소: 4바이트 count DWORD + 100개 × 32바이트 기록(+0 신전 DWORD·+4 존재 DWORD·+8 현재 owner DWORD·+12 지역 BYTE·+13 원래 owner BYTE·+16..+28 경계 float 네 개). createRawIslandStorage()는 3204바이트 장기 배열을 만든다.
//    resetRawIslandRecords(records): 100행의 지정 필드(DWORD 3개·BYTE 2개·float 4개)와 마지막 count 만 쓴다(padding 2바이트와 다른 필드는 보존, 크기가 3204가 아니면 쓰기 전에 거부).
//  - createFortFinishOwnerState(): 섬/Fort 소유자 처리가 공유하는 모드 상태 { chaffMode, special, fort, playerRegions[9](초기 127), invalidRegion(127), residenceType(89) } 이다(이후 RawFortFinishOwner 가 같은 구조를 쓴다).
//  - new RawIslandRuntime(pool, hash, records, modes, residences, worldRedraw, mapRevision, hooks, worldExtent = 256): worldRedraw/mapRevision 은 실제 공유 DWORD 를 빌리는 { value } 상자(0059a8b0 은 owner 변경 때 증가, 패치 005949d4 는 1 저장),
//    hooks = { setResidenceOwner(sid, owner)(거주 구역 가상 owner 실행, 같은 raw 풀 갱신), assertion('MissingIsland' | 'InvalidBounds')(원본 진단 뒤 원문처럼 계속) } 둘 다 필수이다.
//  - reset()/count/exists(i)/add(owner, territory)/delete(i)/matches(i, owner, territory)/temple(i)·setTemple/owner(i)/bounds(i)·setBounds·boundsBits/bytes/pool/bindPlayerRecords(players): 원본 getter/setter 이다.
//    존재하지 않는 섬의 getter/setter 는 진단 뒤 현재 존재를 다시 읽어 계속한다(bounds getter 는 물리 100개 안에서 원본처럼 읽는다). count 는 삭제로 줄이지 않고 빈 구간을 재사용하지 않는다.
//  - setOwner(i, owner): 허용 모드(Fort 이면서 special 이 아니면 즉시 반환)에서 owner → worldRedraw 증가 → mapRevision=1 → 현재 경계 검사(진단)→ 현재 등록 거주 구역 목록(매 반복 count/SID/type/좌표/표면 WORD 재조회)에서 경계 안의 거주 구역에 표면 owner 전파 → Player 첫 지역(127 이면 이 섬 번호) 순서이다.
//    bindPlayerRecords 로 실제 9 Player BYTE(0xb4 간격, +0x78 첫 지역 DWORD)를 공유하면 modes.playerRegions 사본은 쓰지 않는다.
// 10.72(CD)의 조용한 getter/setter·bounds unordered 조건·Player 간격 0xac 는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E161: 섬 런타임 이식으로 추가)
import { rawPathSurfaceAt } from './raw-path-surface.js';

// 기록의 stride, 기록 수, 저장소 크기, Reset 이 대입하는 −10000 float 비트(0xc61c4000), Player 미할당 첫 지역, Player 기록 크기/첫 지역 위치, 거주 구역 owner 위치이다. (2026-11-09 WB04-E161)
const STRIDE = 32;
const RECORDS = 100;
export const ISLAND_STORAGE_BYTES = 4 + RECORDS * STRIDE;
const UNSET_BOUNDS = 0xc61c4000;
const UNASSIGNED_FIRST_REGION = 127;
const PLAYER_STRIDE = 0xb4;
const PLAYER_FIRST_REGION = 0x78;
const OWNER = 34;

// 섬 목록의 3204바이트 장기 배열(최초 호스트 0 입력)을 만든다. (2026-11-09 WB04-E161)
export function createRawIslandStorage() { return new Uint8Array(ISLAND_STORAGE_BYTES); }

// 섬/Fort 소유자 처리가 공유하는 모드 상태를 원본 초기값으로 만든다. (2026-11-09 WB04-E161)
export function createFortFinishOwnerState() {
    return { chaffMode: false, special: false, fort: false, playerRegions: Array(9).fill(UNASSIGNED_FIRST_REGION), invalidRegion: UNASSIGNED_FIRST_REGION, residenceType: 89 };
}

// 실제 필드의 낮은 바이트부터 쓰며 정렬·구조체 padding 을 가정하지 않는다. (2026-11-09 WB04-E161)
function writeField(records, offset, value, width = 4) {
    for (let byte = 0; byte < width; byte += 1) records[offset + byte] = (value >>> (byte * 8)) & 0xff;
}

// Delete 와 전체 Reset 은 같은 한 기록 몸체를 쓰며 물리 밖 입력은 쓰기 전에 거부한다. (2026-11-09 WB04-E161)
function resetRecord(records, island) {
    if (records.length !== ISLAND_STORAGE_BYTES || island < 0 || island >= RECORDS) throw new RangeError('Island Reset 실제 3204BYTE/기록 범위 오류');
    const record = 4 + island * STRIDE;
    writeField(records, record, 0);
    writeField(records, record + 4, 0);
    writeField(records, record + 8, 0);
    writeField(records, record + 12, 255, 1);
    writeField(records, record + 13, 0, 1);
    writeField(records, record + 20, UNSET_BOUNDS);
    writeField(records, record + 16, UNSET_BOUNDS);
    writeField(records, record + 28, UNSET_BOUNDS);
    writeField(records, record + 24, UNSET_BOUNDS);
}

// 0046edf0: Session 없는 cleanup 도 같은 물리의 100행 지정 필드와 마지막 count 를 직접 초기화한다(padding 2바이트는 유지). (2026-11-09 WB04-E161)
export function resetRawIslandRecords(records) {
    if (records.length !== ISLAND_STORAGE_BYTES) throw new RangeError('Island Reset 실제 3204BYTE 범위 오류');
    for (let island = 0; island < RECORDS; island += 1) resetRecord(records, island);
    writeField(records, 0, 0);
}

// MSVC signed char 를 같은 비트의 signed DWORD 로 확장한다. (2026-11-09 WB04-E161)
const signedByte = (value) => (value << 24) >> 24;

// DWORD 비트를 단정도 float 로 해석한다. (2026-11-09 WB04-E161)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 변경 가능한 100개 섬 기록과 owner 변경 효과 계층이다. 참조의 수명은 호출자가 보장한다. (2026-11-09 WB04-E161)
export class RawIslandRuntime {
    #pool;
    #hash;
    #records;
    #modes;
    #residences;
    #worldRedraw;
    #mapRevision;
    #hooks;
    #worldExtent;
    #players = null;

    // 실제 3204바이트 저장소와 필수 하위 효과·기존 모드/Player/목록을 확인한다. (2026-11-09 WB04-E161)
    constructor(pool, hash, records, modes, residences, worldRedraw, mapRevision, hooks, worldExtent = 256) {
        if (pool?.edition !== 'patch1078') throw new TypeError('섬 런타임은 10.78 SidPool 만 받습니다.');
        if (!(records instanceof Uint8Array) || records.length !== ISLAND_STORAGE_BYTES || !hash || !modes || !residences || !worldRedraw || !mapRevision
            || typeof hooks?.setResidenceOwner !== 'function' || typeof hooks.assertion !== 'function') {
            throw new TypeError('IslandRuntime 실제 기록/필수 효과 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#records = records;
        this.#modes = modes;
        this.#residences = residences;
        this.#worldRedraw = worldRedraw;
        this.#mapRevision = mapRevision;
        this.#hooks = { setResidenceOwner: hooks.setResidenceOwner, assertion: hooks.assertion };
        this.#worldExtent = worldExtent | 0;
    }

    // 실제 pool 의 동일성을 월드/후처리 바인딩에서 확인할 수 있도록 제공한다. (2026-11-09 WB04-E161)
    get pool() { return this.#pool; }

    // count 와 100개 원본 기록을 같은 월드의 다른 소비자에게 제공한다(같은 배열이다). (2026-11-09 WB04-E161)
    get bytes() { return this.#records; }

    // 원본 지정 폭의 낮은 바이트부터 결합한다(padding 이나 이웃을 읽지 않는다). (2026-11-09 WB04-E161)
    #read(offset, width = 4) {
        let value = 0;
        for (let index = 0; index < width; index += 1) value += this.#records[offset + index] * 2 ** (index * 8);
        return value >>> 0;
    }

    // 원본 지정 폭의 낮은 바이트부터 대입한다(필드 이외의 바이트는 그대로 남긴다). (2026-11-09 WB04-E161)
    #write(offset, value, width = 4) { writeField(this.#records, offset, value, width); }

    // 범위 밖 원본 포인터 산술을 호스트 배열 언더플로로 수행하지 않는다. (2026-11-09 WB04-E161)
    static #record(island) {
        if (island < 0 || island >= RECORDS) throw new RangeError('IslandRuntime 물리 기록 범위 오류');
        return 4 + island * STRIDE;
    }

    // count 와 100개 기록의 실제 Reset 이다(배치 축약 IslandList 를 새로 만들지 않는다). (2026-11-09 WB04-E161)
    reset() { resetRawIslandRecords(this.#records); }

    // 실제 unsigned DWORD count 이다. 삭제 뒤 줄이거나 빈 구간을 재사용하지 않는다. (2026-11-09 WB04-E161)
    get count() { return this.#read(0); }

    // 원본 signed 범위 검사와 존재 DWORD 반환을 보존한다(범위 밖은 0). (2026-11-09 WB04-E161)
    exists(island) {
        if (island < 0 || island >= (this.count | 0)) return 0;
        return this.#read(RawIslandRuntime.#record(island) + 4);
    }

    // missing 진단 뒤 현재 exists 를 다시 읽어 callback 의 원본 상태 변경도 보존한다. (2026-11-09 WB04-E161)
    #require(island) {
        if (!this.exists(island)) this.#hooks.assertion('MissingIsland');
        return this.exists(island) !== 0;
    }

    // 원본 Add 의 EAX 는 이전 count 이며 현재 끝에 원래 owner BYTE/현재 owner DWORD/지역 BYTE/존재 1 만 쓴다(나머지 신전/경계/padding 은 유지). (2026-11-09 WB04-E161)
    add(owner, territory) {
        const index = this.count;
        if (index >= RECORDS) throw new RangeError('IslandRuntime Add 원본 100기록 경계');
        const record = RawIslandRuntime.#record(index);
        this.#write(0, index + 1);
        this.#write(record + 8, owner);
        this.#write(record + 13, owner, 1);
        this.#write(record + 4, 1);
        this.#write(record + 12, territory, 1);
        return index;
    }

    // 삭제는 기록만 초기화하고 count/원래 번호 재사용 정책을 변경하지 않는다. (2026-11-09 WB04-E161)
    delete(island) { if (this.#require(island)) resetRecord(this.#records, island); }

    // 일반 모드는 signed 원래 owner/지역 BYTE, special/chaff 는 nonzero 현재 owner/지역 BYTE 로 판단한다. 현재 owner 가 바뀌어도 일반 모드의 원래 owner BYTE 는 유지된다. (2026-11-09 WB04-E161)
    matches(island, owner, territory) {
        if (!this.exists(island)) return false;
        const record = RawIslandRuntime.#record(island);
        if (!this.#modes.special && !this.#modes.chaffMode) { if (signedByte(this.#read(record + 13, 1)) !== (owner | 0)) return false; }
        else if (!this.#read(record + 8)) return false;
        return signedByte(this.#read(record + 12, 1)) === (territory | 0);
    }

    // 현재 신전 DWORD 를 조회한다(패치의 missing 진단 뒤에도 없으면 0). 타입이나 번호 세대 해석을 추가하지 않는다. (2026-11-09 WB04-E161)
    temple(island) { return this.#require(island) ? this.#read(RawIslandRuntime.#record(island)) : 0; }
    setTemple(island, temple) { if (this.#require(island)) this.#write(RawIslandRuntime.#record(island), temple); }

    // 현재 owner DWORD 를 조회한다(원래 owner BYTE 와 합치지 않는다). (2026-11-09 WB04-E161)
    owner(island) { return this.#require(island) ? this.#read(RawIslandRuntime.#record(island) + 8) : 0; }

    // 원본은 missing 진단 뒤에도 물리 기록의 네 float 를 읽는다. 물리 100개 밖은 메모리 경계로 진단한다. boundsBits 는 NaN 페이로드까지 같은 원본 비트이다. (2026-11-09 WB04-E161)
    boundsBits(island) {
        this.exists(island) || this.#hooks.assertion('MissingIsland');
        const record = RawIslandRuntime.#record(island);
        return [this.#read(record + 16), this.#read(record + 20), this.#read(record + 24), this.#read(record + 28)];
    }
    bounds(island) {
        const [left, top, right, bottom] = this.boundsBits(island).map(floatOf);
        return { left, top, right, bottom };
    }

    // 존재할 때만 원본 float 비트를 그대로 대입한다(패치의 missing 진단 보존). setBoundsBits 는 비트 배열[left, top, right, bottom]을 그대로 쓴다. (2026-11-09 WB04-E161)
    setBoundsBits(island, bits) {
        if (!this.#require(island)) return;
        const record = RawIslandRuntime.#record(island);
        for (let index = 0; index < 4; index += 1) this.#write(record + 16 + index * 4, bits[index]);
    }
    setBounds(island, bounds) {
        const view = new DataView(new ArrayBuffer(16));
        [bounds.left, bounds.top, bounds.right, bounds.bottom].forEach((value, index) => view.setFloat32(index * 4, value, true));
        this.setBoundsBits(island, [0, 4, 8, 12].map((offset) => view.getUint32(offset, true)));
    }

    // 패치는 네 좌표가 월드 안(0 초과·상한 미만)이고 left ≤ right, top ≤ bottom 인 경계만 유효하다(비정렬 NaN 은 거짓). (2026-11-09 WB04-E161)
    #validBounds(bounds) {
        const extent = Math.fround(this.#worldExtent);
        return bounds.left > 0 && bounds.left < extent && bounds.top > 0 && bounds.top < extent && bounds.right > 0 && bounds.right < extent && bounds.bottom > 0 && bounds.bottom < extent
            && bounds.left <= bounds.right && bounds.top <= bounds.bottom;
    }

    // 실제 후보 검사는 bounds 의 양끝을 포함한다. (2026-11-09 WB04-E161)
    static #contains(bounds, x, y) { return bounds.left <= x && x <= bounds.right && bounds.top <= y && y <= bounds.bottom; }

    // 현재 owner 의 Player 첫 지역 DWORD 를 읽는다(공유 기록이 있으면 그 현재 값을, 없으면 모드의 사본을). (2026-11-09 WB04-E161)
    #firstRegion(owner) {
        if (this.#players === null) {
            if (owner >= this.#modes.playerRegions.length) throw new RangeError('IslandRuntime Player 첫 지역 범위 오류');
            return this.#modes.playerRegions[owner];
        }
        const field = owner * PLAYER_STRIDE + PLAYER_FIRST_REGION;
        return (this.#players[field] | (this.#players[field + 1] << 8) | (this.#players[field + 2] << 16) | (this.#players[field + 3] << 24)) | 0;
    }

    // 공유 기록 모드에서는 첫 지역 필드 네 바이트만 저장한다. 다른 Player 메모리와 독립 상태는 보존한다. (2026-11-09 WB04-E161)
    #setFirstRegion(owner, region) {
        if (this.#players === null) { this.#modes.playerRegions[owner] = region; return; }
        const field = owner * PLAYER_STRIDE + PLAYER_FIRST_REGION;
        for (let index = 0; index < 4; index += 1) this.#players[field + index] = (region >>> (index * 8)) & 0xff;
    }

    // 실제 9 Player BYTE 의 첫 지역 DWORD 를 직접 공유한다. 잘못된 물리 크기는 쓰기 전에 거부하고 연결 뒤 modes.playerRegions 사본은 쓰지 않는다. (2026-11-09 WB04-E161)
    bindPlayerRecords(players) {
        if (!(players instanceof Uint8Array) || players.length !== 9 * PLAYER_STRIDE) throw new RangeError('IslandRuntime 실제 9 Player 크기 오류');
        this.#players = players;
    }

    // owner 변경의 각 전역 쓰기/현재 목록/가상 효과/Player 첫 지역을 원본 순서로 처리한다. (2026-11-09 WB04-E161)
    setOwner(island, owner) {
        if (!this.#require(island) || (this.#modes.fort && !this.#modes.special)) return;
        const record = RawIslandRuntime.#record(island);
        this.#write(record + 8, owner);
        this.#worldRedraw.value = (this.#worldRedraw.value + 1) >>> 0;
        this.#mapRevision.value = 1;
        const bounds = { left: floatOf(this.#read(record + 16)), top: floatOf(this.#read(record + 20)), right: floatOf(this.#read(record + 24)), bottom: floatOf(this.#read(record + 28)) };
        if (!this.#validBounds(bounds)) this.#hooks.assertion('InvalidBounds');
        const residences = this.#residences;
        // 원본은 목록 count/SID/type/좌표와 표면 WORD 를 매 반복 다시 읽고 free/dead 필터를 추가하지 않는다.
        for (let index = 0; (index | 0) < (residences.count | 0); index += 1) {
            if (index >= residences.entries.length) throw new RangeError('IslandRuntime 현재 residences 저장소 부족');
            const number = residences.entries[index];
            if (number >= this.#pool.capacity || number > 65535) throw new RangeError('IslandRuntime residence SID 범위 오류');
            const raw = this.#pool.slot(number);
            if (raw[10] !== this.#modes.residenceType) continue;
            const x = floatOf(raw[14] | (raw[15] << 8) | (raw[16] << 16) | (raw[17] << 24));
            const y = floatOf(raw[18] | (raw[19] << 8) | (raw[20] << 16) | (raw[21] << 24));
            if (!RawIslandRuntime.#contains(bounds, x, y)) continue;
            const support = rawPathSurfaceAt(this.#hash, x, y);
            this.#hooks.setResidenceOwner(number, this.#pool.slot(support)[OWNER]);
        }
        if (owner) {
            if (owner >= this.#modes.playerRegions.length) throw new RangeError('IslandRuntime Player 첫 지역 범위 오류');
            if (this.#firstRegion(owner) === UNASSIGNED_FIRST_REGION) this.#setFirstRegion(owner, island);
        }
    }
}
