// 원래 Client 공급원의 uint32 시각에서 모드 대기 시간만 뺀다. 게임과 wall 기한을 함께 정지한다.
export class ModTimeSource {
    // 누적 제외 시간과 중첩 장벽을 보관한다. 꺼져 있을 때 원래 정수/연산 순서를 그대로 반환한다.
    #source; #offset = 0; #entered = null; #depth = 0;
    // 실제 ClientTimeSource 또는 같은 계약의 검사 공급원을 받는다.
    constructor(source) { this.#source = source; }
    // 고정 오라클 모드의 판정/진행은 원래 공급원에 위임한다. 관리자는 이때 모드를 강제 끈다.
    get fixedEnabled() { return this.#source.fixedEnabled; }
    // 대기 중에는 진입 시각을, 이후에는 대기 경과를 뺀 unsigned 값을 반환한다.
    milliseconds() { return ((this.#entered ?? this.#source.milliseconds()) - this.#offset) >>> 0; }
    // 캘린더 시각은 설정/초기 seed용 원래 장치로 유지한다.
    unixSeconds() { return this.#source.unixSeconds(); }
    // 검사 프레임 전진을 가로채지 않는다.
    advanceFixed() { this.#source.advanceFixed(); }
    // 최초 대기에서만 시각을 잡아 여러 순차 작업도 한 정지 구간으로 묶는다.
    enter() { if (this.#depth++ === 0) this.#entered = this.#source.milliseconds(); }
    // 마지막 반납 때 uint32 감김을 포함한 실제 경과를 한 번 제외한다.
    leave() {
        if (!this.#depth) throw new Error('모드 시계 장벽 반납 불균형');
        if (--this.#depth === 0) { this.#offset = (this.#offset + ((this.#source.milliseconds() - this.#entered) >>> 0)) >>> 0; this.#entered = null; }
    }
}
