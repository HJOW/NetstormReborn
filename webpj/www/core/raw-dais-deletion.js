// 다이스·제단의 고유 삭제 순서를 같은 현재 물리와 선택된 부모에 공유하는 상태 없는 실행기이다(cpppj `o/RawDaisDeletion.cpp/.h`, 10.78 전용).
//  - altarPre: 다이스 가상 삭제(제단 삭제 flags 에 0x400000 이 없을 때만, 죽은 다이스는 제외) → 발자국 안의 현재 추상 룬 정리(권한이 있거나 클라이언트 번호면) → 전체 Damageable.
//  - daisPre: extra 9 관문(추상/매장이면 건너뜀) → (flags 0x400000 이면) 제단 24 완료 → 강한 사제의 604 중지 → 발자국 안 제단 삭제(flags 0x48) → owner 1..8 갱신 → 부모.
//  - daisPost: 다이스 owner 의 Player 기록에서 이 SID 를 가리키는 두 칸(+0x94, +0x8c)을 비우고 보조 칸을 채운 뒤 전체 Damageable 에 위임한다.
//  - findTypeAt / findTypeAtPoint: 원문 4b1fa0 의 현재 좌표/typed 점 조회이며 추가 상태/owner 필터가 없다. finishForms: 제단의 현재 Form 마다 Regular/Shared 24 면 희생 완료를 부른다.
// 현재 역할(제단/룬 TYPE)과 후보를 후보마다 다시 읽고(숫자 또는 getter), 선행 삭제의 부분 효과를 실패 뒤에도 보존한다. 이 모듈은 DOM 을 쓰지 않는다.
// 근거: cpppj `o/RawDaisDeletion.cpp/.h`(기준선 1f00a25a 뒤 새 파일 `20261109-95 CP`, `20261110-52 CP` 수정). (2026-11-13 WB04-E368: 7-P3 RawAltarSacrifice 이식의 선행 부품으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { RegularProcess } from './squid-process.js';

// 우하단만 거의 올리는 float 상수(원문 41def0)의 정확한 값, 플레이어 수, 다이스 삭제가 읽는 슬롯 필드 위치이다. (2026-11-13 WB04-E368)
const ALMOST_CEIL = 0.9999899864196777;
const PLAYER_COUNT = 8;
const TYPE_AT = 10, STATE_AT = 11, OWNER_AT = 34, EXTRA_AT = 40;
// 제단 삭제 flags 의 다이스 최종 삭제 비트, 다이스 삭제가 제단에 주는 flags, 추상/매장 extra 마스크, Player 기록 크기와 두 칸의 위치이다. (2026-11-13 WB04-E368)
const SACRIFICE_DESTROY = 0x400000;
const ALTAR_DESTROY_FLAGS = 0x48;
const SKIP_EXTRA = 9;
const PLAYER_RECORD_SIZE = 0xb4;
const PLAYER_FIRST_AT = 0x94;
const PLAYER_SECOND_AT = 0x8c;

// 호출 중 장치가 바뀌어도 선택한 함수를 복귀까지 유지하도록 필수 훅을 꺼낸다(없으면 거부). (2026-11-13 WB04-E368)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('다이스 삭제의 선택된 실제 하위 미연결');
    return hook;
}

// 비정렬 little endian 값을 폭만큼 부호 없이 읽는다. (2026-11-13 WB04-E368)
function read(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// Player 기록 span 에서 범위를 확인하며 DWORD 를 읽는다. (2026-11-13 WB04-E368)
function readPlayer(bytes, offset) {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('다이스 삭제의 현재 Player 물리 범위');
    return read(bytes, offset);
}

// Player 기록 span 에 DWORD 네 바이트만 쓴다. (2026-11-13 WB04-E368)
function writePlayer(bytes, offset, value) {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('다이스 삭제의 현재 Player 물리 범위');
    // 실제 폭을 유지해 네 바이트만 저장한다.
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// DWORD 비트를 float32 로 읽는다. (2026-11-13 WB04-E368)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(raw, offset), true);
    return view.getFloat32(0, true);
}

// 정상 좌표를 0 방향으로 절삭한다(비유한/signed32 밖은 거부). (2026-11-13 WB04-E368)
function cell(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('다이스 삭제의 사각형 절삭 범위');
    return Math.trunc(value);
}

// 발자국 사각형을 finder 영역으로 바꾼다(왼쪽/위는 절삭, 오른쪽/아래는 거의 올림). (2026-11-13 WB04-E368)
function area(box) {
    return { left: cell(box.left), top: cell(box.top), right: cell(box.right + ALMOST_CEIL), bottom: cell(box.bottom + ALMOST_CEIL) };
}

// 숫자이거나 현재 값을 돌려주는 getter 인 역할 번호를 지금 값으로 읽는다. (2026-11-13 WB04-E368)
const current = (role) => (typeof role === 'function' ? role() : role) >>> 0;

// 다이스·제단 고유 삭제 실행기이다. pool 은 10.78 SidPool, hash 는 SquidHash, types 는 188칸 TYPE 레코드, footprint 는 같은 풀의 RawSquidNeighbors,
// altarType/runeType 은 후보마다 다시 읽는 현재 TYPE 번호(숫자 또는 getter)이다. hooks = { findDais, findAltar, findPriest(sid)→SID, finishForms(dais, altar), stopPriest(priest),
// refreshOwner(owner), boss()→bool, destroy(sid, flags), pre(sid, flags), post(sid, flags), players()→{ records } }. 도달한 훅만 요구한다.
// 사용: new RawDaisDeletion(pool, hash, types, footprint, () => rune.readAltarType(), () => ritualState.currentRuneType, hooks).daisPre(dais, flags). (2026-11-13 WB04-E368)
export class RawDaisDeletion {
    // 실제 물리 슬롯·공간 해시·TYPE 표·발자국, 현재 역할 번호, 효과 훅이다.
    #pool;
    #hash;
    #types;
    #footprint;
    #altarType;
    #runeType;
    #hooks;

    // 같은 풀의 발자국 객체인지 확인한다.
    constructor(pool, hash, types, footprint, altarType, runeType, hooks = {}) {
        if (pool?.edition !== 'patch1078' || footprint?.pool !== pool) throw new TypeError('다이스 삭제의 같은 발자국 Pool 계약');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#footprint = footprint;
        this.#altarType = altarType;
        this.#runeType = runeType;
        this.#hooks = { ...hooks };
    }

    // 같은 물리 정체성만 공개한다.
    get pool() { return this.#pool; }

    // 제단 고유 전처리이다. 제단의 추상/매장 비트는 이 전처리의 생략 조건이 아니다.
    altarPre(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (!(flags & SACRIFICE_DESTROY)) {
            const dais = need(hooks.findDais)(sid);
            // 다이스가 풀 범위 안이고 죽지 않았으면 같은 flags 로 지운다.
            if (dais && dais < pool.capacity && !(pool.slot(dais)[STATE_AT] & 2)) need(hooks.destroy)(dais, flags);
        }
        const finder = new RawSquidFinder(pool, this.#hash, this.#types);
        // 다음 링크는 finder 가 반환 전에 저장한다. 현재 추상 룬만 권한이 있거나 클라이언트 번호면 지운다.
        for (let found = finder.begin(area(this.#footprint.footprint(sid))); found; found = finder.next()) {
            if (pool.slot(found)[TYPE_AT] === current(this.#runeType) && (pool.slot(found)[EXTRA_AT] & 1) && (need(hooks.boss)() || (found > 4 && found < pool.layout.serverFirst))) need(hooks.destroy)(found, 0);
        }
        need(hooks.pre)(sid, flags);
    }

    // 추상/매장 관문은 한 번만 평가하며 큰 완료/삭제가 바꾼 현재 공간과 역할은 뒤에서 다시 읽는다.
    daisPre(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (!(pool.slot(sid)[EXTRA_AT] & SKIP_EXTRA)) {
            if (flags & SACRIFICE_DESTROY) {
                const altar = need(hooks.findAltar)(sid);
                need(hooks.finishForms)(sid, altar);
            }
            const priest = need(hooks.findPriest)(sid);
            if (priest && priest < pool.capacity) need(hooks.stopPriest)(priest);
            const finder = new RawSquidFinder(pool, this.#hash, this.#types);
            // 현재 제단마다 flags 0x48 을 전달한다.
            for (let found = finder.begin(area(this.#footprint.footprint(sid))); found; found = finder.next()) {
                if (pool.slot(found)[TYPE_AT] === current(this.#altarType)) need(hooks.destroy)(found, ALTAR_DESTROY_FLAGS);
            }
            // 현재 1..8 참여 상태 갱신 순서를 보존한다.
            for (let owner = 1; owner <= PLAYER_COUNT; owner += 1) need(hooks.refreshOwner)(owner);
        }
        need(hooks.pre)(sid, flags);
    }

    // 449580 의 첫 기록 보유/조건부 재조회와 각 현재 owner 계산을 실제 Player 기록 대여로 보존한다.
    daisPost(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (pool.slot(sid)[OWNER_AT]) {
            let access = need(hooks.players)();
            // 각 주소 계산에서 현재 owner BYTE 를 읽는다.
            const at = (field) => pool.slot(sid)[OWNER_AT] * PLAYER_RECORD_SIZE + field;
            if (readPlayer(access.records, at(PLAYER_FIRST_AT)) === sid) {
                writePlayer(access.records, at(PLAYER_FIRST_AT), 0);
                access = need(hooks.players)();
                const primary = at(PLAYER_FIRST_AT);
                writePlayer(access.records, primary + 4, readPlayer(access.records, primary));
                access = need(hooks.players)();
            }
            if (readPlayer(access.records, at(PLAYER_SECOND_AT)) === sid) {
                writePlayer(access.records, at(PLAYER_SECOND_AT), 0);
                access = need(hooks.players)();
                const primary = at(PLAYER_SECOND_AT);
                writePlayer(access.records, primary + 4, readPlayer(access.records, primary));
            }
        }
        need(hooks.post)(sid, flags);
    }

    // 원문 4b1fa0: 기준 객체의 현재 좌표로 같은 TYPE 의 첫 후보를 찾는다.
    static findTypeAt(pool, hash, types, source, type) {
        const raw = pool.slot(source);
        return RawDaisDeletion.findTypeAtPoint(pool, hash, types, floatAt(raw, 14), floatAt(raw, 18), type);
    }

    // 이미 저장한 좌표 인자로 같은 실제 typed 검색을 한다. 좌표를 절삭한 뒤 TYPE 의 genus 로 시작 단계(표면 포함 여부)를 정하고 첫 일치에서 즉시 반환한다.
    static findTypeAtPoint(pool, hash, types, px, py, type) {
        const x = cell(px);
        const y = cell(py);
        if (type >= types.length) throw new RangeError('다이스 삭제의 현재 typed 조회 TYPE 범위');
        const finder = new RawSquidFinder(pool, hash, types);
        for (let found = finder.begin({ left: x, top: y, right: x, bottom: y }, (types[type].flags2 & 6) === 0 ? 1 : 0); found; found = finder.next()) {
            if (pool.slot(found)[TYPE_AT] === type) return found;
        }
        return 0;
    }

    // 제단의 현재 Form 사슬을 다음 링크를 먼저 저장하며 돌면서 Regular/Shared 24 프로세스마다 희생 완료를 부른다. 사슬 손상(순환/풀 밖)은 거부한다.
    // regularType/sharedType 은 후보마다 다시 읽는 현재 프로세스 Form TYPE(숫자 또는 getter)이다.
    static finishForms(pool, host, dais, altar, regularType, sharedType, finish) {
        let found = read(pool.slot(altar), 6, 2);
        const visited = new Set();
        // 현재 후보의 next 는 완료 콜백보다 먼저 저장한다. 원문은 부모 0 도 물리 슬롯으로 조회하며 가짜 제단 유효성 검사를 넣지 않는다.
        while (found) {
            if (found >= pool.capacity || visited.has(found)) throw new Error('다이스 제단 프로세스 체인 손상');
            visited.add(found);
            const raw = pool.slot(found);
            const next = read(raw, 4, 2);
            if (raw[TYPE_AT] <= 69) {
                if (raw[TYPE_AT] < 10 || raw[TYPE_AT] >= host.processTypeEnd) throw new Error('다이스 제단 process finder isProcess assert');
                const process = host.fromForm(found);
                if (!process) throw new Error('다이스 제단 실제 Kernel 프로세스 누락');
                const formType = pool.slot(process.form)[TYPE_AT];
                if (formType === current(regularType) || formType === current(sharedType)) {
                    if (!(process instanceof RegularProcess)) throw new Error('다이스 제단의 실제 Regular 필드 미복원');
                    if (process.event === 24) need(finish)(dais);
                }
            }
            found = next;
        }
    }
}
