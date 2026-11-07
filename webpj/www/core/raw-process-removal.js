// 부모에 붙은 프로세스들을 실제 ProcessFinder 순서로 골라 제거하는 모듈이다(cpppj `o/RawProcessRemoval.cpp/.h`, 원본 PATCH 004b27a0).
// 부모의 종속 체인을 머리부터 따라가며 **후보의 다음 링크를 삭제 전에 저장**하고, 정확한 타입(0 이면 전체)의 프로세스 Form 만 flags 0x40 으로 Kill 한다.
// 머리부터 반복 검색하면 삭제 콜백이 새로 붙인 객체까지 지우거나 권한 없는 Form 에서 끝나지 않을 수 있어 그렇게 하지 않는다.
// 후보 판정은 원문 Finder 의 Form 상한(TYPE 70 미만) 뒤 가상 425e80→4b1bf0 조건(TYPE 10 이상·현재 프로세스 TYPE 상한 미만)이며 dead/free 를 추가로 제외하지 않는다.
// 사용: removeAttachedProcesses(host, parentSid, 0) — 모든 프로세스, removeAttachedProcesses(host, parentSid, 46) — 일반 Regular 만.
// 근거: cpppj `o/RawProcessRemoval.cpp` 와 원본 관측 fixture process-removal-filter-x86.tsv 64행(tests/raw-process-removal.test.mjs). CD 10.72 는 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-D5c: Process 부착 부속으로 이식)

// 종속 체인의 다음(+4)·머리(+6)·TYPE(+10) 위치와 후보 TYPE 범위(Form 상한 70, 프로세스 첫 번호 10)이다. (2026-11-08 WB04-D5c)
const NEXT = 4, HEAD = 6, TYPE = 10;
const FORM_LIMIT = 70, FIRST_PROCESS_TYPE = 10;
// 제거에 쓰는 Kill flags(종속 삭제 비트 0x40)이다. (2026-11-08 WB04-D5c)
const REMOVAL_FLAGS = 0x40;

// 슬롯의 비정렬 WORD 링크를 읽는다. (2026-11-08 WB04-D5c)
function link(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 부모에 붙은 프로세스를 정확한 타입(0 이면 전체)으로 골라 제거한다. host 는 SquidProcessHost 이며 같은 풀을 쓴다.
// 체인 손상(범위 밖·순환)은 오류이고, 후보가 프로세스 TYPE 인데 Kernel 에 없으면 '실제 제거할 프로세스 부재' 오류이다. (2026-11-08 WB04-D5c)
export function removeAttachedProcesses(host, parent, type) {
    const pool = host.pool;
    let current = link(pool.slot(parent), HEAD);
    const visited = new Set();
    // 현재 후보의 next 를 먼저 저장하고 효과 뒤 그 번호의 현재 raw 를 재조회한다.
    while (current) {
        if (current >= pool.capacity || visited.has(current)) throw new Error('프로세스 제거 종속 체인 손상');
        visited.add(current);
        const raw = pool.slot(current);
        const next = link(raw, NEXT);
        const candidate = raw[TYPE];
        // Finder 의 Form 상한 뒤 가상 425e80→4b1bf0 조건(10 이상·현재 ProcessTypeEnd 미만)을 적용한다.
        if (candidate < FORM_LIMIT && candidate >= FIRST_PROCESS_TYPE && candidate < host.processTypeEnd) {
            const process = host.fromForm(current);
            if (!process) throw new Error('실제 제거할 프로세스 부재');
            if (!type || pool.slot(process.form)[TYPE] === type) host.kill(process, REMOVAL_FLAGS);
        }
        current = next;
    }
}
