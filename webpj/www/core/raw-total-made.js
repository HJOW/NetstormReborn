// 원본 Totalmade 의 허용 표와 누적 통계를 같은 postPop 장부에서 조회/조작하는 모듈이다(cpppj `o/RawTotalMade.cpp/.h`, 10.78 전용). 004c23c0~004c2560 의 표 조작이며 건설/삭제가 이미 갱신하는 카운터를 복제하지 않는다.
//  - 허용 표(`restrictions`, 금지 DWORD 표): allowAll(allowed)(004c23c0)은 비영이면 첫 255칸을 0(허용), 0 이면 1(금지)로 채우고(256번째 칸은 보존), allow(type, allowed)(004c23e0)은 원본이 타입 유효성 없이 물리 표의 해당 칸에
//    allowed == 0 을 쓰는 몸체로 물리 범위 밖만 쓰기 전에 거부한다. allowed(type)(004c2400)은 유효 타입(1 이상 numTypes 미만)의 현재 금지 DWORD 가 0 일 때만 참이다.
//  - 집계 표(`counts`, SquidPostPopState): resetCounts()(004c2450)는 로컬/누적 로컬/전역 세 표의 첫 255칸만 이 순서로 지운다(TYPE/허용 표/장부 목록/256번째 칸은 쓰지 않음). local/built/global(type)(004c2480/004c24c0/004c2560)은 해당
//    표의 DWORD 비트를 그대로 돌려주고 유효하지 않은 타입은 0 이다. builtForGenus(genus)(004c2500)는 타입 70 부터 끝까지 genus(flags2)가 한 비트라도 겹치는 타입의 누적 로컬 집계를 DWORD 로 합산한다(부호 넘침 대신 DWORD 감김).
//  - 패치만 잘못된 타입/빈 genus 의 원본 debug assert 를 `assertion(TotalMadeAssertion)` 으로 보고하고 값은 false/0 으로 복귀한다. 진단 장치가 없으면 거부한다.
// 세 표와 허용 표는 매 조회마다 현재 물리를 읽는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E237: Totalmade 이식으로 추가)

// 원본 rep stosd 가 초기화하는 DWORD 수와 genus 집계가 시작하는 첫 실제 자산 타입 번호, 10.78 의 타입 수이다. (2026-11-10 WB04-E237)
const INITIALIZED_ENTRIES = 255;
const FIRST_TYPE = 70;
const TYPE_COUNT = 188;

// 패치의 debug assert 를 관찰하거나 실제 진단으로 전달하는 종류이다. (2026-11-10 WB04-E237)
export const TotalMadeAssertion = Object.freeze({
    InvalidType: 0, // 현재 getter 의 타입이 1 이상 numTypes 미만이 아닌 경우의 진단이다.
    ZeroGenus: 1, // 부류별 합계에 0 마스크를 넘긴 경우의 진단이다.
});

// 004c2450: 실제 장기 저장소와 독립 장부의 같은 255칸 reset 몸체이며 256번째 칸과 다른 상태에는 접근하지 않는다. storage 는 { localCounts, localSecondaryCounts, globalCounts }(Uint32Array 256) 이다. (2026-11-10 WB04-E237)
export function resetTotalMadeCounts(storage) {
    resetTables(storage.localCounts, storage.localSecondaryCounts, storage.globalCounts);
}

// 세 표의 앞 255칸만 로컬 → 누적 → 전역 순서로 0 으로 쓴다. (2026-11-10 WB04-E237)
function resetTables(local, secondary, global) {
    local.fill(0, 0, INITIALIZED_ENTRIES);
    secondary.fill(0, 0, INITIALIZED_ENTRIES);
    global.fill(0, 0, INITIALIZED_ENTRIES);
}

// 월드 설치 전의 같은 전역 허용 표도 기존 쓰기 몸체를 직접 쓴다. 원본은 허용값을 bool 로 저장하지 않고 금지 DWORD 의 0/1 로 뒤집어 쓴다. 256번째 칸은 전체 초기화에서 보존한다. (2026-11-10 WB04-E237)
export function setTotalMadeAllowAll(restrictions, allowed) {
    if (!restrictions || restrictions.length < INITIALIZED_ENTRIES) throw new RangeError('Totalmade 허용 표 초기화 크기');
    restrictions.fill((allowed | 0) === 0 ? 1 : 0, 0, INITIALIZED_ENTRIES);
}

// 사용 가능한 호스트 저장소 밖 쓰기는 원본의 무검사 메모리 접근 대신 명시적으로 거부한다. (2026-11-10 WB04-E237)
export function setTotalMadeAllow(restrictions, type, allowed) {
    if (!Number.isInteger(type) || type < 0 || type >= restrictions.length) throw new RangeError('Totalmade 허용 표 물리 범위');
    restrictions[type] = (allowed | 0) === 0 ? 1 : 0;
}

// 004c23c0~004c2560 의 표 조작이다. 사용: new RawTotalMade(types, counts, restrictions, assertion). types 는 188칸 {flags2}, counts 는 SquidPostPopState(localCounts()·localSecondaryCounts()·globalCounts()),
// restrictions 는 255칸 이상의 금지 DWORD 표(Uint32Array), assertion 은 선택(없으면 진단 경로에서 거부)이다. (2026-11-10 WB04-E237)
export class RawTotalMade {
    // 타입 표(현재 genus)·장부(Pop/Unpop 이 직접 갱신하는 동일 세 표)·메뉴/튜토리얼 명령이 공유하는 허용 표와 패치 진단 경계이다.
    #types;
    #counts;
    #restrictions;
    #assertion;

    // 판본의 타입 수와 초기화할 물리 저장소 크기만 확인하고 내용을 바꾸지 않는다. (2026-11-10 WB04-E237)
    constructor(types, counts, restrictions, assertion = null) {
        if (types?.length !== TYPE_COUNT || !(restrictions?.length >= INITIALIZED_ENTRIES)) throw new TypeError('Totalmade 판본/물리 표 크기 오류');
        this.#types = types;
        this.#counts = counts;
        this.#restrictions = restrictions;
        this.#assertion = assertion;
    }

    // 004c23c0: 비영이면 첫 255칸을 0(허용), 0 이면 1(금지)로 채운다. (2026-11-10 WB04-E237)
    allowAll(allowed) { setTotalMadeAllowAll(this.#restrictions, allowed); }

    // 004c23e0: 원본은 타입 유효성 없이 물리 표의 해당 칸에 allowed == 0 을 쓴다. (2026-11-10 WB04-E237)
    allow(type, allowed) { setTotalMadeAllow(this.#restrictions, type, allowed); }

    // 패치 debug 진단을 실행한 뒤에도 원본처럼 getter 의 범위 판정을 유지한다. (2026-11-10 WB04-E237)
    #valid(type) {
        if (type > 0 && type < this.#types.length) return true;
        if (typeof this.#assertion !== 'function') throw new TypeError('Totalmade 잘못된 타입의 원본 진단 미연결');
        this.#assertion(TotalMadeAssertion.InvalidType);
        return false;
    }

    // 004c2400: 유효 타입의 현재 금지 DWORD 가 0 일 때만 참이다. 허용 상태는 매 조회마다 현재 금지 표에서 읽는다. (2026-11-10 WB04-E237)
    allowed(type) { return this.#valid(type) && this.#restrictions[type] === 0; }

    // 004c2450: 실제 세 집계 표의 첫 255칸만 지운다. 256번째 물리 칸은 유지한다. (2026-11-10 WB04-E237)
    resetCounts() { resetTables(this.#counts.localCounts(), this.#counts.localSecondaryCounts(), this.#counts.globalCounts()); }

    // 004c2480: 원본 첫 로컬 집계 표의 DWORD 비트(유효하지 않은 타입은 0)이다. (2026-11-10 WB04-E237)
    local(type) { return this.#valid(type) ? this.#counts.localCounts()[type] : 0; }

    // 004c24c0: 원본 두 번째 로컬 집계 표의 DWORD 비트이다. (2026-11-10 WB04-E237)
    built(type) { return this.#valid(type) ? this.#counts.localSecondaryCounts()[type] : 0; }

    // 004c2560: 원본 전역 집계 표의 DWORD 비트이다. (2026-11-10 WB04-E237)
    global(type) { return this.#valid(type) ? this.#counts.globalCounts()[type] : 0; }

    // 004c2500: 타입 70 부터 현재 끝까지 genus 가 겹치는 두 번째 로컬 집계를 DWORD 로 합산해 부호 있는 결과로 돌려준다. (2026-11-10 WB04-E237)
    builtForGenus(genus) {
        const mask = genus >>> 0;
        if (!mask) {
            if (typeof this.#assertion !== 'function') throw new TypeError('Totalmade 빈 genus 의 원본 진단 미연결');
            this.#assertion(TotalMadeAssertion.ZeroGenus);
            return 0;
        }
        let total = 0; // 원본 signed 결과의 DWORD 감김을 계산할 누산기이다.
        const secondary = this.#counts.localSecondaryCounts();
        // 원본은 genus 의 모든 비트 일치가 아니라 한 비트라도 겹치는 타입을 더한다.
        for (let type = FIRST_TYPE; type < this.#types.length; type += 1) if ((this.#types[type].flags2 & mask) !== 0) total = (total + secondary[type]) >>> 0;
        return total | 0;
    }
}
