// 정적 SID 목록 여덟 개와 그 20칸 등록 표(00592938)를 옮긴 모듈이다(cpppj `o/RawStateSidLists.cpp/.h`, 10.78 전용, 원문 00472830/00472880/004728a0, 정적 생성 0044fb20).
//  - registerStateSidList(headers, header) [00472880]: 20칸 표의 첫 빈 칸(0)에만 기록하고 가득 찬 표에는 쓰지 않는다. header 0 도 원문대로 허용한다.
//  - unregisterStateSidList(headers, header) [004728a0]: 중복을 포함한 모든 일치 칸을 0 으로 만든다. 목록 개수/데이터는 건드리지 않는다.
//  - resetRegisteredStateSidLists(headers, resetCount) [00472830]: 매 칸의 현재 포인터를 읽어 비영 헤더마다 resetCount(헤더)로 +0c 개수만 0 으로 쓰게 한다. 앞선 쓰기가 뒤 칸을 바꾸면 다음 조회에 반영하고, 쓰기 장치 누락은 쓸 헤더에 도달했을 때 거부한다.
//  - RawStateSidLists: 정적 목록 여덟 개(공장 500·인간형 30·아웃포스트 100·공급원 256·간헐천 100·사제 16·거주 구역 50·신전 20)를 원문 용량으로 확보하고 아홉 헤더(다중선택 0x5caf28 포함)를 원문 순서로 등록 표에 올린다.
// 헤더 숫자는 식별 토큰이며 호스트 포인터로 역참조하지 않는다. 표가 20칸 미만이면 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E26: State SID 목록 등록 표 이식으로 추가)
import { SquidPostPopList } from './squid-postpop-state.js';

// 등록 표 칸 수와 정적 생성 순서의 아홉 헤더이다(다섯째 0x5caf28 은 다른 Client 소유 다중선택). (2026-11-08 WB04-E26)
export const STATE_SID_REGISTRY_CAPACITY = 20;
const HEADERS = Object.freeze([0x55a4d0, 0x55a4ec, 0x55a708, 0x592f80, 0x5950d8, 0x5954c0, 0x59a9d4, 0x5caf28, 0x5e1898]);
// 여덟 목록의 원문 용량(공장·인간형·아웃포스트·공급원·간헐천·사제·거주 구역·신전)이다. (2026-11-08 WB04-E26)
const CAPACITIES = Object.freeze({ factories: 500, dudes: 30, outposts: 100, providers: 256, geysers: 100, priests: 16, residences: 50, temples: 20 });

// 짧은 호스트 뷰를 원문 고정 20칸으로 잘못 다루지 않는다. (2026-11-08 WB04-E26)
function requireRegistry(headers) {
    if (!headers || headers.length < STATE_SID_REGISTRY_CAPACITY) throw new RangeError('State SID 등록 표의 20DWORD 누락');
}

// 첫 빈 칸에서 즉시 반환한다. (2026-11-08 WB04-E26)
export function registerStateSidList(headers, header) {
    requireRegistry(headers);
    // 앞에서부터 첫 빈 칸을 찾는다.
    for (let index = 0; index < STATE_SID_REGISTRY_CAPACITY; index += 1) {
        if (!headers[index]) { headers[index] = header >>> 0; return; }
    }
}

// 첫 일치에서 끝내지 않고 모든 일치 칸을 지운다. (2026-11-08 WB04-E26)
export function unregisterStateSidList(headers, header) {
    requireRegistry(headers);
    // 20칸 전체에서 일치하는 칸을 0 으로 만든다.
    for (let index = 0; index < STATE_SID_REGISTRY_CAPACITY; index += 1) {
        if (headers[index] === (header >>> 0)) headers[index] = 0;
    }
}

// 매 칸의 현재 값을 읽는다. 비영 헤더에서만 쓰기 장치를 요구하며 쓰기 실패 앞의 효과는 되돌리지 않는다. (2026-11-08 WB04-E26)
export function resetRegisteredStateSidLists(headers, resetCount) {
    requireRegistry(headers);
    // 앞선 실제 count 쓰기가 뒤 포인터를 바꾸면 다음 조회에 반영한다.
    for (let index = 0; index < STATE_SID_REGISTRY_CAPACITY; index += 1) {
        const header = headers[index];
        if (header) {
            if (typeof resetCount !== 'function') throw new Error('State SID 개수 정리의 실제 쓰기 미연결');
            resetCount(header);
        }
    }
}

// PATCH 0040ea00 의 signed count 반복이다: 현재 count 아래의 각 DWORD 를 순서대로 읽어 일치하면 세고 아니면 앞으로 압축한다(일치가 여럿이면 모두 제거).
//  선검사·꼬리 지우기를 하지 않으므로 비활성 꼬리·용량·등록 표는 보존하고, 음수 count 는 읽기/쓰기 없이 지나가며 마지막에만 count 에서 제거 수를 뺀다(DWORD 감김).
//  count 가 가리키는 위치가 배열 밖이면 그 위치의 현재 물리 범위 오류로 멈춘다. (2026-11-10 WB04-E259: cpppj RemoveStateSidList 재반영 — 거주 구역·인간형·사제 삭제가 쓴다)
export function removeStateSidList(list, sid) {
    const target = sid >>> 0;
    const count = list.count | 0;
    let removed = 0;
    // 현재 signed count 아래의 항목을 읽기 위치 순서대로 압축한다.
    for (let index = 0; index < count; index += 1) {
        if (index >= list.entries.length) throw new RangeError('SID 목록 제거의 현재 물리 범위');
        const value = list.entries[index] >>> 0;
        if (value === target) removed += 1;
        else list.entries[index - removed] = value;
    }
    list.count = (list.count - removed) >>> 0;
}

// 정적 목록 여덟 개와 등록 표를 Session 보다 오래 유지한다. (2026-11-08 WB04-E26)
export class RawStateSidLists {
    // 목록 여덟 개(SquidPostPopList)이다. entries.length 가 원문 용량이다.
    factories = new SquidPostPopList();
    dudes = new SquidPostPopList();
    outposts = new SquidPostPopList();
    providers = new SquidPostPopList();
    geysers = new SquidPostPopList();
    priests = new SquidPostPopList();
    residences = new SquidPostPopList();
    temples = new SquidPostPopList();
    // 정적 생성 순서의 아홉 헤더와 빈 열한 칸이며 개수 정리에서 보존한다.
    registeredHeaders = new Array(STATE_SID_REGISTRY_CAPACITY).fill(0);

    // 최초 한 번 원문 용량을 확보하고 아홉 헤더를 원문 순서로 등록한다.
    constructor() {
        // 목록별로 원문 용량의 0 DWORD 배열을 만든다.
        for (const [name, capacity] of Object.entries(CAPACITIES)) this[name].entries = new Array(capacity).fill(0);
        // 정적 생성자가 같은 20칸 표에 원문 순서로 등록한다.
        for (const header of HEADERS) registerStateSidList(this.registeredHeaders, header);
    }

    // 그때 선택된 실제 count 만 0 으로 쓴다. 다중선택(0x5caf28)은 다른 Client 필드 selection.multipleCount 를 빌린다.
    resetCount(header, selection) {
        switch (header) {
        case 0x55a4d0: this.factories.count = 0; return;
        case 0x55a4ec: this.dudes.count = 0; return;
        case 0x55a708: this.outposts.count = 0; return;
        case 0x592f80: this.providers.count = 0; return;
        case 0x5950d8: this.geysers.count = 0; return;
        case 0x5954c0: this.priests.count = 0; return;
        case 0x59a9d4: this.residences.count = 0; return;
        case 0x5caf28: selection.multipleCount = 0; return;
        case 0x5e1898: this.temples.count = 0; return;
        default: throw new RangeError('State SID 등록 표의 미연결 실제 헤더');
        }
    }

    // 기존 일반 설치의 fresh 의미만 호환한다: 같은 배열 주소에서 내용과 개수를 0 으로 만든다. Native 개수 정리와 prepared 대여에는 호출하지 않는다.
    prepareGeneralWorld() {
        // 여덟 목록을 같은 vector 에서 초기화한다.
        for (const list of [this.factories, this.dudes, this.outposts, this.providers, this.geysers, this.priests, this.residences, this.temples]) {
            list.entries.fill(0);
            list.count = 0;
        }
    }
}
