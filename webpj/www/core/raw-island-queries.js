// 실제 mutable 섬 목록에서 현재 owner/원래 owner/territory 를 읽는 세 검색 몸체이다(cpppj `o/RawIslandQueries.cpp/.h`, 10.78 전용: 패치 0046f380·0046f3f0·0046f430).
//  - findOwnerTerritory(owner, territory): 일반 모드는 signed 원래 owner BYTE(+13)와 현재 owner DWORD 가 모두 같아야 하고, fort 모드는 현재 owner 0 도 허용한다. 존재하는 기록 중 signed 지역 BYTE(+12)가 같은 첫 번호를 돌려준다(없으면 127).
//    패치는 시작할 때의 count 와 fort 모드를 한 번 읽어 반복 내내 쓴다(CD 와 다름).
//  - findTerritory(territory): fort/special/chaff/소유자 값을 읽지 않고 존재 DWORD 가 비영인 첫 기록의 signed 지역 BYTE 를 비교한다.
//  - countOwned(owner): 존재하는 기록 중 현재 owner DWORD 가 같은 개수이며 owner 0 도 계수한다. count 가 삭제로 줄지 않는 원본 정책을 유지한다.
// 같은 3204 BYTE 기록과 현재 fort 모드를 참조하며 별도 섬 벡터를 만들지 않는다. 물리 100기록 밖 접근은 섬 목록(`RawIslandRuntime`)이 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E220: 섬 검색 이식으로 추가)

// 원본 두 검색 함수가 아무 섬도 찾지 못했을 때 반환하는 고정 번호이다. (2026-11-10 WB04-E220)
export const ISLAND_NOT_FOUND = 127;

// 실제 32 BYTE 기록 안의 territory BYTE 와 원래 owner BYTE 위치, 기록 간격, count 뒤 시작 위치이다. (2026-11-10 WB04-E220)
const STRIDE = 32;
const TERRITORY = 12;
const ORIGINAL_OWNER = 13;
const RECORDS_START = 4;

// 섬 검색이다. islands 는 같은 월드의 RawIslandRuntime(count/exists/owner/bytes), modes 는 현재 fort 플래그를 가진 FortFinishOwnerState 이다. 목록과 모드의 장기 소유자 수명은 호출하는 월드가 유지한다.
// 사용: const queries = new RawIslandQueries(runtime, modes); queries.findOwnerTerritory(1, 2). (2026-11-10 WB04-E220)
export class RawIslandQueries {
    // 실제 currentCount/Exists/현재 owner 와 3204 BYTE 를 가진 같은 월드의 목록, owner/territory 검색의 현재 fort 플래그이다. special/chaff 는 이 세 원본 몸체에서 쓰지 않는다.
    #islands;
    #modes;

    // 판본(10.78)을 확인한다. (2026-11-10 WB04-E220)
    constructor(islands, modes) {
        if (islands?.pool?.edition !== 'patch1078') throw new TypeError('섬 검색은 10.78 섬 목록만 받습니다.');
        this.#islands = islands;
        this.#modes = modes;
    }

    // 실제 기록의 BYTE 를 MSVC signed char 처럼 32비트 부호 확장해 읽는다. Exists 로 확인한 물리 기록을 같은 배열에서 직접 읽는다(복사/owner BYTE·DWORD 병합 없음). (2026-11-10 WB04-E220)
    #signedField(island, field) {
        return (this.#islands.bytes[RECORDS_START + island * STRIDE + field] << 24) >> 24;
    }

    // 일반 모드는 원래 owner 가 같아도 현재 owner 가 달라졌으면 통과하지 않는다. 패치는 시작 count 와 fort 모드를 고정해 읽는다. (2026-11-10 WB04-E220)
    findOwnerTerritory(owner, territory) {
        const initialFort = Boolean(this.#modes.fort);
        const initialCount = this.#islands.count | 0;
        const wanted = owner >>> 0;
        // 실제 번호 0 부터 순서대로 존재/두 owner/지역을 확인해 최초 일치를 반환한다.
        for (let island = 0; island < initialCount; island += 1) {
            if (!this.#islands.exists(island)) continue;
            const current = this.#islands.owner(island);
            if (!initialFort) {
                if (this.#signedField(island, ORIGINAL_OWNER) !== (owner | 0) || current !== wanted) continue;
            } else if (current && current !== wanted) continue;
            if (this.#signedField(island, TERRITORY) === (territory | 0)) return island;
        }
        return ISLAND_NOT_FOUND;
    }

    // 이 검색은 fort/special/chaff 및 소유자 값을 읽지 않는다. 존재 DWORD 가 0 이 아닌 첫 기록의 signed 지역 BYTE 를 비교한다. (2026-11-10 WB04-E220)
    findTerritory(territory) {
        const initialCount = this.#islands.count | 0;
        for (let island = 0; island < initialCount; island += 1) {
            if (this.#islands.exists(island) && this.#signedField(island, TERRITORY) === (territory | 0)) return island;
        }
        return ISLAND_NOT_FOUND;
    }

    // 이미 복원된 exists/owner 를 같은 raw 목록에 호출하며 0 소유 섬도 계수한다. count 는 매 반복 다시 읽는다(삭제 뒤 줄지 않는 원본 정책, 빈 기록은 건너뜀). (2026-11-10 WB04-E220)
    countOwned(owner) {
        let count = 0;
        const wanted = owner >>> 0;
        for (let island = 0; island < (this.#islands.count | 0); island += 1) {
            if (this.#islands.exists(island) && this.#islands.owner(island) === wanted) count += 1;
        }
        return count;
    }
}
