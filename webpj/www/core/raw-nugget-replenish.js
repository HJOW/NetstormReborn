// 보상 내용물 제거 뒤 현재 권한·모드와 지역 0 의 원본 보충 범위를 계산하는 너겟 보충(00485e40)을 옮긴 모듈이다(cpppj `o/RawNuggetReplenish.cpp/.h`, 10.78 전용).
// 순서: 현재 권한이 꺼져 있으면 원본 진단(Nugget.cpp 246)을 요구한 뒤 권한을 다시 읽어 여전히 꺼져 있으면 끝 → 단독 미션·요새·편집 모드이면 자연 반환 → 현재 지역 0 의 첫 record 와 원점 배치(RawPlayerGeyserBounds.originPlacement) →
//   지역 확장 단위(signed)를 `<< 4` 한 뒤 사각형 네 변 = (칸 `<< 4` 를 float 로 저장한 값) ∓ (`확장*16 - 16` / `확장*16`)을 두 번째 반올림 없이 계산해 실제 무작위 배치 하위에 수량 1 로 요청한다.
// 진단 효과가 바꾸는 전역(권한·모드·지역 확장 단위)은 다음 조회에서 직접 읽으며 사용하지 않는 큰 하위는 생성 때 요구하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E71: 너겟 보충 이식으로 추가)

// 권한 진단의 원문 줄 번호(Nugget.cpp 246), 칸 → 좌표 이동량(<< 4), 보충 수량이다. (2026-11-08 WB04-E71)
const AUTHORITY_ASSERTION_LINE = 0x246;
const CELL_SHIFT = 4;
const SPAN_TRIM = 16;
const QUANTITY = 1;

// 원본 DWORD 연산 뒤 signed 해석만 수행한다. (2026-11-08 WB04-E71)
const signed = (value) => value | 0;

// 0040d900 의 정수 `<< 4` 후 float 저장이다. (2026-11-08 WB04-E71)
const cell = (value) => Math.fround(signed(value << CELL_SHIFT));

// bounds 는 같은 월드의 RawPlayerGeyserBounds(regionRecord/originPlacement), state 는 현재 전역을 속성으로 읽는 객체 { boss, single, fort, editor, archipelagoSpan }(getter 가능),
//   hooks = { assertion(줄 번호), place(사각형 { left, top, right, bottom }, 수량) } 이며 도달한 것만 필요하다. (2026-11-08 WB04-E71)
export class RawNuggetReplenish {
    // 기존 간헐천과 같은 실제 지도·패턴을 쓰는 범위 계산기, 호출 중에도 바뀔 수 있는 현재 전역, 선택된 큰 하위이다.
    #bounds;
    #state;
    #hooks;

    // 모드 관문에서 쓰지 않는 큰 하위는 생성 때 요구하지 않는다. (2026-11-08 WB04-E71)
    constructor(bounds, state, hooks = {}) {
        this.#bounds = bounds;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 현재 boss 는 진단 뒤 재독하고 확장 단위는 실제 패턴 계산 뒤에 읽는다. (2026-11-08 WB04-E71)
    run() {
        const state = this.#state;
        if (!state.boss) {
            if (typeof this.#hooks.assertion !== 'function') throw new Error('너겟 보충의 권한 진단 미연결');
            this.#hooks.assertion(AUTHORITY_ASSERTION_LINE);
            if (!state.boss) return;
        }
        if (state.single || state.fort || state.editor) return;
        // 현재 지역 0 의 첫 record 를 값으로 보존한다.
        const record = this.#bounds.regionRecord(0);
        const area = this.#bounds.originPlacement(record);
        // 두 signed 정수 모두 float 로 먼저 축약하지 않는다.
        const scaled = (state.archipelagoSpan << CELL_SHIFT) >>> 0;
        const trim = signed((scaled - SPAN_TRIM) >>> 0);
        const extent = signed(scaled);
        // 원본의 각 fstp DWORD 에서만 반올림한다.
        const rectangle = {
            left: Math.fround(cell(area.left) - trim),
            top: Math.fround(cell(area.top) - trim),
            right: Math.fround(cell(area.right) + extent),
            bottom: Math.fround(cell(area.bottom) + extent)
        };
        if (typeof this.#hooks.place !== 'function') throw new Error('비미션 너겟 보충의 실제 무작위 배치 미연결');
        this.#hooks.place(rectangle, QUANTITY);
    }
}
