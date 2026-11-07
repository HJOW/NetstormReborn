// 원본 10.78 Kernel(프로세스 슬롯 등록·삭제·슬롯 순서 실행)을 옮긴 모듈이다. 패치판은 1..39999 슬롯을 쓰며(CD 10.72 는 3999), 슬롯 0 은 평소 미등록이다.
// 실행은 그 순간의 슬롯을 앞에서부터 직접 읽는다: 실행 중 뒤 슬롯에 새로 등록한 프로세스는 같은 프레임에, 이미 지난 앞 슬롯이면 다음 프레임에 실행된다.
// 목록을 복사해 실행 순서를 바꾸지 않는다. 삭제는 프로세스의 destroy()(원본 가상 소멸자)를 먼저 부른 뒤 슬롯을 비운다(소멸 중에는 원래 슬롯이 보인다).
// 기준선 cpppj 의 진단 장치(WB04-D5c 에서 맞춤): 슬롯이 가득 찬 등록은 요약→상세→단언을 현재 장치로 차례로 부른 뒤 **캡처한 슬롯 0** 에 등록하고,
// 빈 슬롯 삭제는 단언을 부른 뒤 그 슬롯을 다시 읽는다. 슬롯 0 은 조회·삭제할 수 있지만 프레임 실행·Clear 순회에서는 제외한다. 실제 단언이 예외를 던지면 그 앞 효과만 남는다.
// 원본 procSumm.txt 요약 출력은 웹에 없으므로(cpppj 도 RawProcessSummary 는 개발 파일 출력) 요약 장치는 호스트가 연결한다.
// 근거: cpppj `o/Kernel.cpp/.h`(00471950 등록, 004719e0 삭제, 00471a30 실행, 00471af0 Clear), `o/BaseProcess.h`, 검사 KernelDiagnosticCurrentTests(Client 접합 제외)·
//       tests/RecoveryTests.cpp Kernel_MutationUsesLiveSlots. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-05 WB04-C1: 세션 장기 상태의 Process 실행기 이식으로 추가, 2026-11-08 WB04-D5c: 진단 장치·슬롯 0·최종 반납 보완)

// 패치판 Kernel 의 마지막 슬롯 번호이다. (2026-11-05 WB04-C1)
export const PROCESS_CAPACITY = 39999;

// 진단 단언이 보고하는 원문 파일 이름과 행이다(10.78). 소진과 빈 슬롯 삭제의 두 지점이다. (2026-11-08 WB04-D5c)
const KERNEL_CPP = '\\Ns\\O\\Kernel.cpp';
const EXHAUSTED_LINE = 0x7a;
const EMPTY_REMOVE_LINE = 0x82;

// 소진 상세 문구이다(원문 그대로). (2026-11-08 WB04-D5c)
const EXHAUSTED_DETAIL = 'Out of processes. A process count trace has been dumped\nto procSumm.txt.  Please deliver this file with the bug report.\n';

// 프로세스 공통 기반이다(원본 BaseProcess). runFrame 은 원본 가상 슬롯 +0x18 의 빈 기본 실행, destroy 는 가상 소멸자이다. 파생 프로세스가 재정의한다.
// 사용: class Thunder extends BaseProcess { runFrame() { … } }. (2026-11-05 WB04-C1)
export class BaseProcess {
    // 한 프레임 실행이다. 기본은 아무것도 하지 않는다.
    runFrame() {}

    // Kernel 이 삭제할 때 부르는 소멸 처리이다. 기본은 아무것도 하지 않는다.
    destroy() {}
}

// 프로세스 슬롯 실행기이다. 프로세스는 runFrame() 함수를 가진 객체이며 destroy() 는 선택이다.
// 사용: const kernel = new Kernel(); const id = kernel.add(process); kernel.runFrame(); kernel.remove(id). (2026-11-05 WB04-C1)
export class Kernel {
    // 슬롯 배열(0..capacity)이다. 0 은 평소 비어 있다.
    #slots;
    // 마지막 슬롯 번호이다.
    #capacity;
    // 현재 등록 수이다(슬롯 0 포함).
    #count = 0;
    // 슬롯에서 덮어써져 밀려났지만 소멸 효과는 아직 부르지 않은 프로세스들이다. 원문의 포인터 덮어쓰기에는 소멸 호출이 없으므로 최종 반납에서만 정리한다.
    #retired = [];
    // 지금 소멸 처리(destroy) 중인 슬롯 번호들이다. 같은 슬롯의 재진입 삭제를 거부한다.
    #destroying = new Set();
    // 진단 장치({ summary, detail, assertion })이다. 사용 지점마다 현재 값을 읽는다.
    #diagnostics = {};

    // 슬롯 수를 정한다. 1~39999 만 받는다.
    constructor(capacity = PROCESS_CAPACITY) {
        if (!Number.isInteger(capacity) || capacity < 1 || capacity > PROCESS_CAPACITY) throw new RangeError('Kernel 슬롯 크기 오류');
        this.#capacity = capacity;
        this.#slots = new Array(capacity + 1).fill(null);
    }

    // 진단 장치를 연결(교체)한다. summary() 는 소진 때 현재 프로세스 요약, detail(문구) 는 오류 문구 보관, assertion(식, 파일, 행) 은 원문 단언이다.
    // 장치 교체 자체는 슬롯을 읽거나 바꾸지 않으며, 호출 중 교체해도 이미 선택한 장치의 호출은 끝까지 진행된다. 미연결 장치는 그 실패 분기에서만 명시 오류가 된다.
    // 사용: kernel.bindDiagnostics({ summary: () => {}, detail: (text) => {}, assertion: (expression, file, line) => {} }). (2026-11-08 WB04-D5c)
    bindDiagnostics(devices = {}) {
        this.#diagnostics = { summary: devices?.summary ?? null, detail: devices?.detail ?? null, assertion: devices?.assertion ?? null };
    }

    // 00471950: 앞에서부터 첫 빈 슬롯을 기억하면서 같은 객체가 이미 있으면 그 번호를 돌려준다(뒤 슬롯이라도 재사용). 없으면 첫 빈 슬롯에 넣는다.
    // 빈 슬롯이 없으면 현재 요약→상세→단언 장치를 차례로 부르고(각각 미연결이면 그 위치에서 오류), 정상 복귀하면 진단 중 비워진 슬롯을 다시 찾지 않고 **캡처한 슬롯 0** 에 등록한다.
    add(process) {
        if (process === null || typeof process !== 'object' || typeof process.runFrame !== 'function') throw new TypeError('Kernel 에는 runFrame 을 가진 프로세스만 등록할 수 있습니다.');
        let first = 0;
        // 빈 앞 슬롯이 있어도 같은 객체의 기존 뒤 슬롯까지 찾는다.
        for (let id = 1; id <= this.#capacity; id += 1) {
            const current = this.#slots[id];
            if (current === null && first === 0) first = id;
            if (current === process) return id;
        }
        if (first === 0) {
            const summary = this.#diagnostics.summary;
            if (typeof summary !== 'function') throw new Error('Kernel 소진의 현재 프로세스 요약 미연결');
            summary();
            const detail = this.#diagnostics.detail;
            if (typeof detail !== 'function') throw new Error('Kernel 소진의 현재 상세 저장소 미연결');
            detail(EXHAUSTED_DETAIL);
            const assertion = this.#diagnostics.assertion;
            if (typeof assertion !== 'function') throw new Error('Kernel 소진의 현재 단언 미연결');
            assertion('firstSlot != 0', KERNEL_CPP, EXHAUSTED_LINE);
        }
        // 슬롯 0 을 덮어쓰면 밀려난 객체는 소멸 호출 없이 보관만 한다. 소멸 처리 중인 객체는 이미 소멸을 시작했으므로 보관하지 않는다.
        if (this.#slots[first] !== null) {
            if (!this.#destroying.has(first)) this.#retired.push(this.#slots[first]);
        } else this.#count += 1;
        this.#slots[first] = process;
        return first;
    }

    // 같은 객체가 등록된 첫 슬롯 번호이다(슬롯 0 은 찾지 않는다). 없으면 0 이다.
    find(process) {
        // 앞에서부터 같은 객체를 찾는다.
        for (let id = 1; id <= this.#capacity; id += 1) if (this.#slots[id] === process) return id;
        return 0;
    }

    // 00471aa0: 슬롯의 프로세스이다. 슬롯 0 도 그대로 읽는다. 빈 슬롯·범위 밖은 null 이다(원본의 범위 밖 읽기는 옮기지 않는다).
    get(id) {
        return Number.isInteger(id) && id >= 0 && id <= this.#capacity ? this.#slots[id] : null;
    }

    // 004719e0: 빈 슬롯이면 현재 단언 장치를 부른 뒤(미연결이면 오류) 그 슬롯을 다시 읽는다. 등록된 프로세스의 destroy() 를 부른 뒤 슬롯을 비운다.
    // 범위 밖 번호는 단언 전에 거부한다. 소멸 중 같은 슬롯의 재삭제는 오류이고, 소멸 처리가 슬롯에 남긴 다른 프로세스는 마지막 비우기와 함께 밀려난다.
    remove(id) {
        if (!Number.isInteger(id) || id < 0 || id > this.#capacity) throw new RangeError('Process slot outside Kernel');
        if (this.#slots[id] === null) {
            const assertion = this.#diagnostics.assertion;
            if (typeof assertion !== 'function') throw new Error('Kernel 빈 슬롯의 현재 단언 미연결');
            assertion('processes[pid] != NULL', KERNEL_CPP, EMPTY_REMOVE_LINE);
        }
        // 단언이 슬롯을 채웠을 수도 있으므로 현재 값을 다시 읽는다.
        const victim = this.#slots[id];
        if (victim !== null) {
            if (this.#destroying.has(id)) throw new Error('Kernel 소멸 중인 같은 포인터의 중복 삭제');
            this.#destroying.add(id);
            try {
                // 소멸 처리 안의 get/find/재등록은 아직 원래 슬롯을 본다.
                victim.destroy?.();
            } finally {
                this.#destroying.delete(id);
            }
            // 소멸 처리가 같은 슬롯에 다른 프로세스를 남겼다면 마지막 비우기에는 새 소멸 효과가 없다.
            const left = this.#slots[id];
            if (left !== null && left !== victim) this.#retired.push(left);
        }
        if (this.#slots[id] !== null) this.#count -= 1;
        this.#slots[id] = null;
    }

    // 00471af0: 슬롯 1부터 끝까지 한 번만 읽어 차례로 삭제한다. 소멸 처리가 이미 지난 앞 슬롯에 새로 등록하면 이번 정리에서는 남는다. 슬롯 0 은 제외한다.
    clear() {
        // 원본의 증가 순서를 유지하고 빈 슬롯은 건드리지 않는다.
        for (let id = 1; id <= this.#capacity; id += 1) if (this.#slots[id] !== null) this.remove(id);
    }

    // 호스트 수명 종료의 최종 반납이다. select(process) 가 참인(없으면 모든) 프로세스를 더 이상 없을 때까지 반복해 정리한다:
    // 슬롯 1..끝(Clear 순서) → 슬롯 0 → 밀려난 객체들 순이며, 소멸 처리가 새로 등록한 프로세스(앞 슬롯 포함)도 다음 순회에서 처리한다. 밀려난 객체는 destroy() 만 부른다.
    // 원본 Clear 한 번의 결과와 구분되는 후속 정리이며(cpppj Kernel 소멸자·DrainOwnedForShutdown), 소멸 처리의 신규 등록은 유한하게 끝나야 한다.
    drainOwnedForShutdown(select = null) {
        if (select !== null && typeof select !== 'function') throw new TypeError('Kernel 최종 반납 선택자는 함수여야 합니다.');
        // 이번 호출의 선택 계약은 도중 장치가 바뀌어도 유지한다.
        const selected = select;
        for (;;) {
            let removed = false;
            // 원본 Clear 의 증가 순서를 먼저 유지한다.
            for (let id = 1; id <= this.#capacity; id += 1) {
                const process = this.#slots[id];
                if (process !== null && (!selected || selected(process))) {
                    this.remove(id);
                    removed = true;
                }
            }
            const zero = this.#slots[0];
            if (zero !== null && (!selected || selected(zero))) {
                this.remove(0);
                removed = true;
            }
            // 소멸 처리의 재진입이 목록을 바꿀 수 있어 원소 참조를 오래 들고 있지 않는다.
            for (let index = 0; index < this.#retired.length; index += 1) {
                const process = this.#retired[index];
                if (process !== null && (!selected || selected(process))) {
                    this.#retired[index] = null;
                    process.destroy?.();
                    removed = true;
                }
            }
            if (!removed) break;
        }
        // 정리된 자리를 줄인다.
        this.#retired = this.#retired.filter((process) => process !== null);
    }

    // 호스트 수명 종료: 남은 프로세스가 없을 때까지 정리한다(drainOwnedForShutdown 전체). 원본 Clear 한 번의 결과와 구분되는 후속 정리이다.
    dispose() {
        this.drainOwnedForShutdown();
    }

    // 00471a30: 슬롯 1..capacity 를 그 순간의 값으로 읽어 등록된 프로세스의 runFrame() 을 부른다. 슬롯 0 은 건너뛴다.
    runFrame() {
        // 목록을 복사하지 않고 매 슬롯을 직접 읽는다.
        for (let id = 1; id <= this.#capacity; id += 1) {
            const process = this.#slots[id];
            if (process !== null) process.runFrame();
        }
    }

    // 현재 등록 수이다(슬롯 0 포함).
    get size() { return this.#count; }

    // 마지막 슬롯 번호이다.
    get capacity() { return this.#capacity; }
}
