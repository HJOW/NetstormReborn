// 패치 004923e0 의 기본 기술/지역 사제 준비와 00492120/004921a0 의 현재 목록 순서를 옮긴 모듈이다(cpppj `o/RawFortPriestPreparation.cpp/.h`, 10.78 전용).
// 일반 Fort 설치에서 한 번 실행한다. 순서:
//  1) 미션 후처리 전역이 꺼져 있으면 기술 네 가지(앞 세 개는 기본 TYPE, 네 번째는 `subscriber % 3` 의 테마 TYPE)를 TYPE 먼저 읽고 현재 기술 부모를 그 다음에 읽어, 부모에 같은 TYPE 내용물이 없으면 만든다.
//  2) 편집기이거나 미션 후처리이면 끝(전역은 기술 생성 뒤 다시 읽는다). 고정 Template 가 있으면 State+17 BYTE 가 켜져 있을 때 끝.
//  3) Territory 의 20×6 BYTE 기록 중 각 기록 BYTE1 의 bit0 이 켜진 지역마다(현재 playerOwner 와 인덱스로 섬 번호를 조회, 127 이면 건너뜀) 사제 목록(최대 20개 임시 목록)을 모아 bit 0x80 표시가 있는 첫 항목을 고르고,
//     표시도 없고 목록도 비면 사제 초기 배치, 아니면 표시가 없을 때 첫 항목의 소유자를 지정한 뒤 같은 번호를 모두 안정 제거하고, 남은 사제마다 소유자 0 을 지정하며 남은 수가 양수이면 안내한다.
// 큰 하위 효과(기술 생성, 섬 조회, 소유자 지정, 초기 배치, 안내)는 명시 경계이며 선택하지 않은 경계는 미리 요구하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E70: Fort 사제 준비 이식으로 추가)

// 슬롯의 좌표/표시 비트 위치, 기술 종류 수와 테마 나머지, 지역 수와 기록 폭, 임시 목록 용량, 섬 없음 표식, State 의 참조 BYTE 위치이다. (2026-11-08 WB04-E70)
const X_OFFSET = 14;
const Y_OFFSET = 18;
const MARK_OFFSET = 12;
const MARK_BIT = 0x80;
const TECHNOLOGY_COUNT = 4;
const BASE_TECHNOLOGY_COUNT = 3;
const THEME_COUNT = 3;
const TERRITORY_COUNT = 20;
const TERRITORY_RECORD = 6;
const LIST_CAPACITY = 20;
const NO_ISLAND = 127;
const STATE_REFERENCE = 17;
const MAX_WORD = 65535;

// 도달한 선택 장치의 누락만 거부하고 미선택 하위를 미리 요구하지 않는다. (2026-11-08 WB04-E70)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Fort 사제 준비의 선택 장치 미연결');
    return hook;
}

// 비트 보존을 위해 슬롯 좌표를 단정도로 읽는다. (2026-11-08 WB04-E70)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    // 네 바이트를 리틀엔디언으로 옮긴다.
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, priests 는 PriestPostPopState(등록/삭제와 공유하는 사제 목록), globals 는 현재 전역을 읽는 객체 { missionPost, editor, subscriber, templateOwner, playerOwner }(매번 속성을 읽으므로 getter 로 둘 수 있음),
//   hooks = { baseType(번호), themeType(테마), technology()→부모, contains(부모, 타입)→SID, createTechnology(부모, 타입), templatePresent(소유자), state(소유자)→BYTE 배열, territory(소유자, 120바이트 출력), island(소유자, 인덱스)→섬,
//   region(x, y)→섬, setOwner(sid, 소유자), populate(섬), tellPriests(수) } 이다. (2026-11-08 WB04-E70)
export class RawFortPriestPreparation {
    // 같은 raw 풀, 공유 사제 목록, 현재 전역, 하위 효과이다.
    #pool;
    #priests;
    #globals;
    #hooks;

    // 판본을 구성 때 확인한다. 모드 사본이나 기본 성공 장치를 만들지 않는다. (2026-11-08 WB04-E70)
    constructor(pool, priests, globals, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Fort 사제 준비는 10.78 풀만 받습니다.');
        this.#pool = pool;
        this.#priests = priests;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 현재 DWORD SID 가 실제 풀의 물리 범위 안인지 접근할 때만 검사한다. (2026-11-08 WB04-E70)
    #raw(sid) {
        if (sid > MAX_WORD || sid >= this.#pool.capacity) throw new RangeError('Fort 사제 준비의 실제 DWORD SID 범위');
        return this.#pool.slot(sid);
    }

    // 00492120: 지역 번호가 같은 사제를 현재 목록 순서대로 최대 용량까지 entries 에 모으고 모은 수를 반환한다. 콜백이 바꾼 목록 개수와 다음 항목도 원문처럼 매번 다시 읽는다. (2026-11-08 WB04-E70)
    collect(region, entries) {
        let count = 0;
        const list = this.#priests.priests;
        // 개수는 signed 로 매번 다시 읽는다.
        for (let index = 0; (index | 0) < (list.readCount() | 0); index += 1) {
            if (index >= list.capacity()) throw new RangeError('Fort 사제 준비 현재 목록 물리 범위');
            const sid = list.storage()[index] >>> 0;
            const raw = this.#raw(sid);
            if ((need(this.#hooks.region)(floatAt(raw, X_OFFSET), floatAt(raw, Y_OFFSET)) | 0) === (region | 0) && count < entries.length) {
                entries[count] = sid;
                count += 1;
            }
        }
        return count;
    }

    // 004921a0: 임시 목록에서 첫 raw+12 의 bit 0x80 항목을 반환한다. owner/생존 필터를 추가하지 않는다. 없으면 0 이다. (2026-11-08 WB04-E70)
    marked(entries) {
        // 실제 순서의 첫 표시를 찾는다.
        for (const sid of entries) {
            if (this.#raw(sid)[MARK_OFFSET] & MARK_BIT) return sid;
        }
        return 0;
    }

    // 기술 네 가지를 보장한 뒤 현재 관문을 다시 읽고 지역 스무 개를 원문 순서로 처리한다. (2026-11-08 WB04-E70)
    run() {
        const hooks = this.#hooks;
        const globals = this.#globals;
        if (!globals.missionPost) {
            // TYPE 는 먼저, 현재 기술 부모는 그 다음에 읽으며 네 번째만 현재 subscriber % 3 을 사용한다.
            for (let index = 0; index < TECHNOLOGY_COUNT; index += 1) {
                const type = index < BASE_TECHNOLOGY_COUNT ? need(hooks.baseType)(index) : need(hooks.themeType)((globals.subscriber >>> 0) % THEME_COUNT);
                const parent = need(hooks.technology)();
                if (!need(hooks.contains)(parent, type)) need(hooks.createTechnology)(parent, type);
            }
        }
        if (globals.editor || globals.missionPost) return;
        if (need(hooks.templatePresent)(globals.templateOwner)) {
            const state = need(hooks.state)(globals.templateOwner);
            if (state.length <= STATE_REFERENCE) throw new RangeError('Fort 사제 준비 State+17 물리 범위');
            if (state[STATE_REFERENCE]) return;
        }
        const records = new Uint8Array(TERRITORY_COUNT * TERRITORY_RECORD);
        need(hooks.territory)(globals.templateOwner, records);
        // 사본 Territory 의 각 기록 BYTE1 bit0 만 판정하고 조회 owner 는 그 시점의 현재값이다.
        for (let index = 0; index < TERRITORY_COUNT; index += 1) {
            if (!(records[index * TERRITORY_RECORD + 1] & 1)) continue;
            const region = need(hooks.island)(globals.playerOwner, index) | 0;
            if (region === NO_ISLAND) continue;
            const entries = new Array(LIST_CAPACITY).fill(0);
            let count = this.collect(region, entries);
            let selected = this.marked(entries.slice(0, count));
            if (!selected && !count) {
                need(hooks.populate)(region);
            } else {
                if (!selected) {
                    selected = entries[0];
                    need(hooks.setOwner)(selected, globals.playerOwner);
                }
                // 0040ea00 처럼 모든 같은 번호를 안정적으로 제거한다.
                let remaining = 0;
                for (let at = 0; at < count; at += 1) {
                    if (entries[at] !== selected) {
                        entries[remaining] = entries[at];
                        remaining += 1;
                    }
                }
                count = remaining;
            }
            // 가상 owner 가 원래 목록을 바꿔도 지역 임시 목록의 반복 수는 원문 지역값이다.
            const remainingCount = count;
            // 남은 사제마다 현재 실제 가상 74 를 실행한다.
            for (let at = 0; at < remainingCount; at += 1) need(hooks.setOwner)(entries[at], 0);
            if (remainingCount) need(hooks.tellPriests)(remainingCount);
        }
    }
}
