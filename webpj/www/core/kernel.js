// 원본 10.78 Kernel(프로세스 슬롯 등록·삭제·슬롯 순서 실행)을 옮긴 모듈이다. 패치판은 1..39999 슬롯을 쓰며(CD 10.72 는 3999), 슬롯 0 은 미등록이다.
// 실행은 그 순간의 슬롯을 앞에서부터 직접 읽는다: 실행 중 뒤 슬롯에 새로 등록한 프로세스는 같은 프레임에, 이미 지난 앞 슬롯이면 다음 프레임에 실행된다.
// 목록을 복사해 실행 순서를 바꾸지 않는다. 삭제는 프로세스의 destroy()(원본 가상 소멸자)를 먼저 부른 뒤 슬롯을 비운다(소멸 중에는 원래 슬롯이 보인다).
// 근거: cpppj `o/Kernel.cpp/.h`(00471950 등록, 004719e0 삭제, 00471a30 실행, 00471af0 Clear), `o/BaseProcess.h`, `tests/RecoveryTests.cpp` Kernel_MutationUsesLiveSlots.
// 원본 procSumm.txt 디버그 출력은 cpppj 에서도 미복원이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-C1: 세션 장기 상태의 Process 실행기 이식으로 추가)

// 패치판 Kernel 의 마지막 슬롯 번호이다. (2026-11-05 WB04-C1)
export const PROCESS_CAPACITY = 39999;

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
    // 슬롯 배열(0..capacity)이다. 0 은 항상 비어 있다.
    #slots;
    // 마지막 슬롯 번호이다.
    #capacity;
    // 현재 등록 수이다.
    #count = 0;

    // 슬롯 수를 정한다. 1~39999 만 받는다.
    constructor(capacity = PROCESS_CAPACITY) {
        if (!Number.isInteger(capacity) || capacity < 1 || capacity > PROCESS_CAPACITY) throw new RangeError('Kernel 슬롯 크기 오류');
        this.#capacity = capacity;
        this.#slots = new Array(capacity + 1).fill(null);
    }

    // 00471950: 앞에서부터 첫 빈 슬롯을 기억하면서 같은 객체가 이미 있으면 그 번호를 돌려준다(뒤 슬롯이라도 재사용). 없으면 첫 빈 슬롯에 넣는다.
    // 빈 슬롯이 없으면 오류이다("Out of processes").
    add(process) {
        if (process === null || typeof process !== 'object' || typeof process.runFrame !== 'function') throw new TypeError('Kernel 에는 runFrame 을 가진 프로세스만 등록할 수 있습니다.');
        let first = 0;
        // 빈 앞 슬롯이 있어도 같은 객체의 기존 뒤 슬롯까지 찾는다.
        for (let id = 1; id <= this.#capacity; id += 1) {
            const current = this.#slots[id];
            if (current === null && first === 0) first = id;
            if (current === process) return id;
        }
        if (first === 0) throw new RangeError('Out of processes');
        this.#slots[first] = process;
        this.#count += 1;
        return first;
    }

    // 같은 객체가 등록된 첫 슬롯 번호이다. 없으면 0 이다.
    find(process) {
        // 앞에서부터 같은 객체를 찾는다.
        for (let id = 1; id <= this.#capacity; id += 1) if (this.#slots[id] === process) return id;
        return 0;
    }

    // 00471aa0: 슬롯의 프로세스이다. 0·범위 밖·빈 슬롯은 null 이다(원본의 범위 밖 읽기는 옮기지 않는다).
    get(id) {
        return Number.isInteger(id) && id >= 1 && id <= this.#capacity ? this.#slots[id] : null;
    }

    // 004719e0: 프로세스의 destroy() 를 부른 뒤 슬롯을 비운다. 0·범위 밖·빈 슬롯은 오류이다.
    remove(id) {
        if (!Number.isInteger(id) || id < 1 || id > this.#capacity || this.#slots[id] === null) throw new RangeError('Process not registered');
        // 소멸 처리 안의 get/find/재등록은 아직 원래 슬롯을 본다.
        this.#slots[id].destroy?.();
        this.#slots[id] = null;
        this.#count -= 1;
    }

    // 00471af0: 슬롯 1부터 끝까지 한 번만 읽어 차례로 삭제한다. 소멸 처리가 이미 지난 앞 슬롯에 새로 등록하면 이번 정리에서는 남는다.
    clear() {
        // 원본의 증가 순서를 유지하고 빈 슬롯은 건드리지 않는다.
        for (let id = 1; id <= this.#capacity; id += 1) if (this.#slots[id] !== null) this.remove(id);
    }

    // 호스트 수명 종료: 남은 프로세스가 없을 때까지 clear 를 반복한다(원본 Clear 한 번의 결과와 구분되는 후속 정리, cpppj 소멸자와 같음).
    dispose() {
        // 소멸 처리가 앞 슬롯에 새로 등록한 프로세스까지 정리한다.
        do this.clear(); while (this.#count !== 0);
    }

    // 00471a30: 슬롯 1..capacity 를 그 순간의 값으로 읽어 등록된 프로세스의 runFrame() 을 부른다.
    runFrame() {
        // 목록을 복사하지 않고 매 슬롯을 직접 읽는다.
        for (let id = 1; id <= this.#capacity; id += 1) {
            const process = this.#slots[id];
            if (process !== null) process.runFrame();
        }
    }

    // 현재 등록 수이다.
    get size() { return this.#count; }

    // 마지막 슬롯 번호이다.
    get capacity() { return this.#capacity; }
}
