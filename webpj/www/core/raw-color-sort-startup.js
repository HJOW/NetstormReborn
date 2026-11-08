// ColorSort 시작 부모 두 개(43b430 InitializeColorSort, 43b300 InitializeColorSortGradient)의 현재 슬롯·호출 순서·중단 전 부분 효과를 옮긴 모듈이다(cpppj `o/RawColorSortStartup.cpp/.h`).
// 생성·정렬·색띠의 실제 계산은 원문 하위와 같은 필수 경계(훅)로 분리하며, 부모는 같은 Client 수명의 실제 포인터/개수/로드 DWORD 를 직접 빌려 읽고 쓴다(토큰은 실제 host 포인터가 아님).
//  - initializeColorSortGradient(globals, hooks): 현재 gradient 가 비영이면 해제한 뒤 0 을 쓰고, 0x5020 을 할당한다. 할당 callback 뒤에만 현재 count 를 읽어(이후 생성자 callback 이 바꿔도 인자는 캡처값) 색띠 생성
//    (this, 회색 RGB 세 double → [1,0,0], count, 폭 2.4)의 반환 주소를 gradient 에 대입한다. 할당이 0 이면 gradient 는 0 이다.
//  - initializeColorSort(logical, globals, hooks): 로드 플래그가 0 이면 진단(Colorsort.cpp 451)을 올린 뒤 계속하고, 현재 primary·search 를 각각 비영일 때 해제 후 0 으로 쓴다. 0x3800 할당이 비영이면 생성자(this, 같은 logical)의 반환 주소를 primary 에 대입하고,
//    0x5008 을 할당해 비영이면 검색 객체를 생성하고 같은 객체의 첫 DWORD 에 파생 vtable 0x506658 을 직접 쓴 뒤 qsort(search+8, 256, 80, 0x43a1e0)를 부른다. 정렬 뒤 현재 전역이 바뀌었어도 캡처한 할당 주소를 search 에 대입하고, count 를 10 으로 쓴 뒤 색띠 부모를 실행한다.
// globals = { primary(00555d88), search(00555dd8), gradient(00555d78), count(0053199c), paletteLoaded(00555e08) } 의 현재 값을 읽고 쓰는 속성이고, 훅 = { assertion(줄), release(토큰), allocate(폭), construct(대상, logical), constructSearch(대상),
// memory(토큰)→Uint8Array, sort(대상, 개수, 폭, 비교기), constructGradient(대상, from[3], to[3], count, 폭) } 이다. 선택한 경계가 미연결이면 도달 시 거부하며 선행 효과는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E77: ColorSort 시작 부모 이식으로 추가)

// 원문 00506698/00503678 의 정확한 double 입력(회색 채널 0x3fcb9b9b9b9b9b9c)과 색띠 폭이다. (2026-11-08 WB04-E77)
const GRAY = (() => {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, 0x3fcb9b9b9b9b9b9cn, true);
    return view.getFloat64(0, true);
})();
const GRADIENT_WIDTH = 2.4;
// 색띠 시작/끝 RGB 와 종료 때 쓰는 개수, 세 할당 폭, 검색 파생 vtable, 정렬 인자(개수·폭·비교기), 진단 줄 번호이다. (2026-11-08 WB04-E77)
const GRADIENT_FROM = Object.freeze([GRAY, GRAY, GRAY]);
const GRADIENT_TO = Object.freeze([1.0, 0.0, 0.0]);
const PRIMARY_BYTES = 0x3800;
const SEARCH_BYTES = 0x5008;
const GRADIENT_BYTES = 0x5020;
const SEARCH_VTABLE = 0x506658;
const SORT_COUNT = 256;
const SORT_WIDTH = 80;
const SORT_COMPARATOR = 0x43a1e0;
const STARTUP_COUNT = 10;
const PALETTE_ASSERT_LINE = 451;

// 도달한 하위 경계만 요구하고 빈 성공으로 바꾸지 않는다. (2026-11-08 WB04-E77)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('ColorSort 시작의 실제 하위 미연결');
    return hook;
}

// 같은 검색 객체의 첫 DWORD 만 little endian vtable 로 쓴다(인접한 미작성 BYTE 는 보존). (2026-11-08 WB04-E77)
function writeSearchVtable(memory) {
    if (!memory || memory.length < 4) throw new RangeError('ColorSort 검색 vtable 물리 범위');
    // 낮은 바이트부터 4바이트만 쓴다.
    for (let index = 0; index < 4; index += 1) memory[index] = (SEARCH_VTABLE >>> (index * 8)) & 0xff;
}

// 현재 gradient 조건부 해제/0 → 할당 → 현재 count/고정 RGB → 실제 색띠 생성 → 반환 주소 대입이다. (2026-11-08 WB04-E77)
export function initializeColorSortGradient(globals, hooks) {
    if (globals.gradient !== 0) {
        need(hooks.release)(globals.gradient);
        globals.gradient = 0;
    }
    // 할당 callback 뒤에만 현재 count 를 읽는다.
    const allocated = need(hooks.allocate)(GRADIENT_BYTES);
    if (allocated !== 0) {
        // 이후 생성자 callback 중 바뀌어도 현재 인자는 캡처값이다.
        const count = globals.count;
        globals.gradient = need(hooks.constructGradient)(allocated, [...GRADIENT_FROM], [...GRADIENT_TO], count, GRADIENT_WIDTH);
    } else {
        globals.gradient = 0;
    }
}

// 로드 진단 → 두 현재 슬롯 해제/0 → primary 생성 → 검색 생성/vtable/qsort → search 대입 → count 10 → 색띠 부모이다. (2026-11-08 WB04-E77)
export function initializeColorSort(logical, globals, hooks) {
    if (globals.paletteLoaded === 0) need(hooks.assertion)(PALETTE_ASSERT_LINE);
    if (globals.primary !== 0) {
        need(hooks.release)(globals.primary);
        globals.primary = 0;
    }
    if (globals.search !== 0) {
        need(hooks.release)(globals.search);
        globals.search = 0;
    }
    // 같은 logical 포인터의 내용은 뒤 생성자 진입에서 처음 읽는다.
    const primary = need(hooks.allocate)(PRIMARY_BYTES);
    globals.primary = primary !== 0 ? need(hooks.construct)(primary, logical) : 0;
    // 반환 주소는 이후 callback 동안 캡처한다.
    const search = need(hooks.allocate)(SEARCH_BYTES);
    if (search !== 0) {
        need(hooks.constructSearch)(search);
        writeSearchVtable(need(hooks.memory)(search));
        need(hooks.sort)((search + 8) >>> 0, SORT_COUNT, SORT_WIDTH, SORT_COMPARATOR);
    }
    // qsort 뒤 현재 전역이 바뀌었어도 캡처한 할당 주소를 쓴다.
    globals.search = search;
    globals.count = STARTUP_COUNT;
    initializeColorSortGradient(globals, hooks);
}
