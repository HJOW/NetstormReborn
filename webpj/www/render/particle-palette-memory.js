// 입자 색 표의 "요청 크기 할당 + 바로 뒤 인접 DWORD 읽기" 경계를 웹용으로 대체한 모듈이다(cpppj `client/ParticlePaletteMemory.cpp/.h`, adapt).
// 원본은 색 배열을 `HeapAlloc(heap, 0, 색수 × 4)` 로 요청한 크기 그대로 확보하고, 색 번호가 색수와 같아지는 입자(수명이 최대 수명인 입자)가 그 배열 바로 뒤 DWORD 를 읽는다.
// cpppj 는 이를 실제 Win32 heap 할당과 `ReadProcessMemory` 로 복사해 재현했지만, 64비트 프로세스의 인접 값이 원본 32비트 실행과 같다고 보장할 수 없어 고정 값으로 숨기지 않았다.
// 웹에는 OS 힙이 없으므로 "인접 DWORD" 를 **명시 입력**으로 받는다. 기본값은 원본 데모 실행(단위101)에서 관찰된 값 1 이며, 시험은 다른 값을 넣어 읽기 경로를 구별한다.
//  - new ParticlePaletteMemory(adjacent = 1): adjacent 는 색수 바로 다음에 보이는 DWORD(0~0xffffffff 정수)이다. 색 표가 필요하기 전에는 아무 것도 확보하지 않는다.
//  - assign(colors): 색 1~256개(각 0~0xffffffff 정수)를 받아 이전 배열을 교체한다. 잘못된 길이/값이면 이전 배열을 보존한 채 거부한다.
//  - snapshot(): 색수 + 1 개 DWORD(마지막이 인접 값)를 담은 `Uint32Array` 이다. 같은 배열을 다시 돌려주며 assign 뒤에는 새 배열이다. assign 전에 부르면 거부(Error).
//  - count: 요청 색 수(snapshot 길이보다 1 작음). address: 웹에는 실제 주소가 없으므로 할당 세대 번호(assign 마다 +1, 확보 전 0)이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E390: 5-A RawCampaignParticles 와 함께 이식, 인접 값 규칙은 "명시 입력, 기본 관찰값 1" 로 결정)

// 원본 데모 실행(단위101)에서 관찰된 색수 바로 뒤 DWORD 이다. 이 값이 모든 실행에서 같다는 증거는 아니다. (2026-11-13 WB04-E390)
export const OBSERVED_ADJACENT_DWORD = 1;
// 한 색 표가 가질 수 있는 최대 색 수이다(원본 팔레트 크기). (2026-11-13 WB04-E390)
const MAX_COLORS = 256;
// 32비트 DWORD 의 상한이다. (2026-11-13 WB04-E390)
const DWORD_MAX = 0xffffffff;

// DWORD 범위의 정수인지 검사한다. (2026-11-13 WB04-E390)
function checkDword(value, message) {
    if (!Number.isInteger(value) || value < 0 || value > DWORD_MAX) throw new RangeError(message);
}

// 입자 색 표 저장소이다. 한 객체가 색 표 하나(불꽃/빔/건설/연소/연기/악마 광선)를 소유한다. (2026-11-13 WB04-E390)
export class ParticlePaletteMemory {
    // 색수 뒤에 보이는 인접 DWORD, 현재 snapshot, 할당 세대이다.
    #adjacent;
    #readable = null;
    #generation = 0;

    // 인접 DWORD 를 검사해 보관한다. 기본값은 관찰된 1 이다. (2026-11-13 WB04-E390)
    constructor(adjacent = OBSERVED_ADJACENT_DWORD) {
        checkDword(adjacent, '입자 인접 DWORD 범위 오류');
        this.#adjacent = adjacent;
    }

    // 색 1~256개를 새로 확보한다(이전 배열은 반납). 색수 뒤 DWORD 는 인접 입력이며 색으로 취급하지 않는다. (2026-11-13 WB04-E390)
    assign(colors) {
        if (!colors || !(colors.length >= 1) || colors.length > MAX_COLORS) throw new RangeError('입자 색 배열 요청 수 오류');
        for (let index = 0; index < colors.length; index += 1) checkDword(colors[index], '입자 색 DWORD 범위 오류');
        const next = new Uint32Array(colors.length + 1);
        next.set(colors, 0);
        next[colors.length] = this.#adjacent;
        this.#readable = next;
        this.#generation += 1;
    }

    // 색수 + 인접 DWORD 를 돌려준다. 확보 전 조회는 거부하고 가짜 색으로 대체하지 않는다. (2026-11-13 WB04-E390)
    snapshot() {
        if (!this.#readable) throw new Error('입자 색 배열 생성 전 조회');
        return this.#readable;
    }

    // 요청 색 수이다(확보 전 0). (2026-11-13 WB04-E390)
    get count() {
        return this.#readable ? this.#readable.length - 1 : 0;
    }

    // 웹에는 실제 주소가 없다: assign 마다 늘어나는 할당 세대 번호(확보 전 0)이다. 소유권 교체를 시험이 구별한다. (2026-11-13 WB04-E390)
    get address() {
        return this.#generation;
    }
}
