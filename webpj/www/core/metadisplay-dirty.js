// 원본 10.78 미니맵(Sky Overview) dirty 큐를 옮긴 모듈이다. Pop/Unpop 이 표면 객체의 좌표를 4×4 칸 묶음 셀로 고유 등록하고,
// 프레임마다 Refresh 가 비트맵 최초 확보 → 활성 표시 확인 → 카메라/사각형 비교 뒤 재생성 → 프레임 증가 → 전체(64×64)/부분 셀 그림 → 큐 비우기 순서로 소비한다.
// 실제 셀 그림(004808d0)과 표시 재생성(00480270)은 호출자가 연결하는 장치이며, 이 모듈은 큐·플래그·순서만 소유한다.
// 근거: cpppj `o/RawMetadisplayDirty.cpp/.h`(bae8b702), 원본 관측 `cpppj/tests/fixtures/metadisplaydirty-x86.tsv`(10.78 520행).
// CD 10.72 의 NaN 카메라 비교 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-D2: Unpop 처리기의 미니맵 통지로 추가)

// 원본 비트맵 행 폭(128)과 전체 갱신 셀 행/열 수(64)이다. 월드 256칸을 4칸씩 묶는다. (2026-11-05 WB04-D2)
export const METADISPLAY_BITMAP_WIDTH = 128;
export const METADISPLAY_CELL_COUNT = 64;

// 원본 고정 큐 길이이다. 가득 차면 전체 갱신을 요청한다. (2026-11-05 WB04-D2)
export const METADISPLAY_QUEUE_CAPACITY = 100;

// 슬롯의 x/y float 위치이다(10.78). (2026-11-05 WB04-D2)
const X = 14, Y = 18;

// 원본 BSS 상태와 같은 새 상태를 만든다. 셀은 { x, y, unused } 100칸이며 count 밖의 옛 자료도 보존한다.
// camera 는 float 비트(uint32) 2칸, rectangle 은 signed32 4칸이다. fullRefreshOwner 가 { value } 이면 전체 갱신 DWORD(005949d4)를 그 소유자와 공유한다.
// 사용: const state = createMetadisplayDirtyState(); state.displayPresent = true; state.displayFlags = 1; (2026-11-05 WB04-D2)
export function createMetadisplayDirtyState({ fullRefreshOwner = null } = {}) {
    if (fullRefreshOwner !== null && (typeof fullRefreshOwner !== 'object' || !Number.isInteger(fullRefreshOwner.value))) {
        throw new TypeError('미니맵 전체 갱신 소유자는 { value: 정수 } 여야 합니다.');
    }
    return {
        // 표시 객체(005949d0)가 있는지와 그 +4 플래그이다. 비트 1 만 활성 여부를 정한다.
        displayPresent: false,
        displayFlags: 0,
        // 최초 Refresh 에서 확보하는 128×128 BYTE 비트맵과 행 폭이다. null 은 아직 확보 전이다.
        bitmap: null,
        bitmapWidth: 0,
        // 등록 순서 큐와 유효 개수이다.
        cells: Array.from({ length: METADISPLAY_QUEUE_CAPACITY }, () => ({ x: 0, y: 0, unused: 0 })),
        count: 0,
        // 지역 전체 갱신 값이다. 소유자가 연결되면 쓰지 않는다.
        fullRefresh: 0,
        fullRefreshOwner,
        // 카메라/사각형 static 초기화 비트 1·2, 마지막 재생성 때의 view, 활성 Refresh 마다 증가하는 프레임 번호이다.
        initialized: 0,
        cached: { camera: [0, 0], rectangle: [0, 0, 0, 0] },
        frames: 0,
    };
}

// 현재 전체 갱신 값을 읽는다(소유자 우선). (2026-11-05 WB04-D2)
function fullRefresh(state) {
    return state.fullRefreshOwner ? state.fullRefreshOwner.value : state.fullRefresh;
}

// 현재 전체 갱신 값을 쓴다(소유자 우선, 사본 동기화 없음). (2026-11-05 WB04-D2)
function setFullRefresh(state, value) {
    if (state.fullRefreshOwner) state.fullRefreshOwner.value = value >>> 0;
    else state.fullRefresh = value >>> 0;
}

// float 비트를 수로 바꾼다. (2026-11-05 WB04-D2)
function bitsToFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 원본 float 동등 비교(10.78)이다. +0/-0 은 같고 NaN 은 다르다. (2026-11-05 WB04-D2)
function sameCamera(current, cached) {
    // 두 좌표가 모두 같아야 한다.
    for (let index = 0; index < 2; index += 1) {
        if (bitsToFloat(current[index]) !== bitsToFloat(cached[index])) return false;
    }
    return true;
}

// 미니맵 dirty 큐 처리기이다. pool 은 SidPool(같은 실제 풀), state 는 createMetadisplayDirtyState 결과,
// hooks 는 { rebuild(), redrawCell(x, y) } 이며 필요한 분기에서만 요구한다(비활성 자연 반환은 장치를 요구하지 않는다).
// 사용: const dirty = new RawMetadisplayDirty(pool, state, { rebuild, redrawCell }); dirty.enqueueSid(sid); dirty.refresh(view); (2026-11-05 WB04-D2)
export class RawMetadisplayDirty {
    // 같은 실제 풀이다(SID 좌표 조회와 연결 확인).
    #pool;
    // 원본 큐·프레임·view·비트맵 상태이다(복사하지 않는다).
    #state;
    // 재생성·셀 그림 장치이다.
    #hooks;

    // 풀·상태·장치를 연결한다. 상태의 count/비트맵 크기를 먼저 확인한다.
    constructor(pool, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('미니맵 dirty 는 10.78 SID 풀만 받습니다.');
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks ?? {};
        this.#validate();
    }

    // 연결된 실제 풀이다. Unpop/Pop 처리기가 같은 풀인지 확인한다.
    get pool() { return this.#pool; }

    // 연결된 상태 객체이다(같은 객체).
    get state() { return this.#state; }

    // 원본 메모리 손상에 해당하는 입력(count 범위 밖·비트맵 폭 불일치)을 거부한다.
    #validate() {
        const state = this.#state;
        if (!Number.isInteger(state.count) || state.count < 0 || state.count > METADISPLAY_QUEUE_CAPACITY) throw new RangeError('미니맵 dirty count 범위 오류');
        if (state.bitmap !== null && (state.bitmapWidth <= 0 || state.bitmapWidth * state.bitmapWidth !== state.bitmap.length)) {
            throw new RangeError('미니맵 bitmap 폭/크기 불일치');
        }
    }

    // 좌표를 0 방향 절삭한 signed32 를 다시 signed /4 한 셀 번호이다(-3.9 → 0, -4.0 → -1). 비유한/범위 밖은 오류이다.
    static cell(coordinate) {
        const value = Math.trunc(Math.fround(coordinate));
        if (!Number.isFinite(value) || value < -0x80000000 || value > 0x7fffffff) throw new RangeError('미니맵 dirty 유한 signed32 좌표가 필요합니다.');
        return (value / 4) | 0;
    }

    // 00481510: 활성 표시에서만 셀을 고유 추가한다. 이미 있으면 끝, 가득 찼으면 전체 갱신 1 을 쓴다. 비활성이면 좌표 변환도 하지 않는다.
    enqueue(x, y) {
        const state = this.#state;
        if (!state.displayPresent || (state.displayFlags & 1) === 0) return;
        this.#validate();
        const cx = RawMetadisplayDirty.cell(x);
        const cy = RawMetadisplayDirty.cell(y);
        // 같은 셀 재통지는 포화 검사보다 먼저 확인한다.
        for (let index = 0; index < state.count; index += 1) {
            if (state.cells[index].x === cx && state.cells[index].y === cy) return;
        }
        if (state.count >= METADISPLAY_QUEUE_CAPACITY) {
            setFullRefresh(state, 1);
            return;
        }
        state.cells[state.count] = { x: cx, y: cy, unused: 0 };
        state.count += 1;
    }

    // Pop/Unpop 호출 시점의 같은 슬롯 +14/+18 좌표로 등록한다. 표면 여부 판단은 호출자 몫이다.
    enqueueSid(sid) {
        const raw = this.#pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        this.enqueue(view.getFloat32(X, true), view.getFloat32(Y, true));
    }

    // 00481320: 비트맵 최초 확보(비활성이어도 전체 갱신 1) → 비활성이면 끝 → 초기화 비트 → 변경이 있으면 view 저장 뒤 재생성
    // → 프레임 증가 → 전체 갱신이면 y 바깥·x 안쪽 64×64 셀, 아니면 현재 count 까지 부분 셀(그림 중 새 등록도 같은 프레임에 처리) → count·전체 갱신 0.
    // view 는 { camera: [float 비트, float 비트], rectangle: [signed32 ×4] } 이다.
    refresh(view = { camera: [0, 0], rectangle: [0, 0, 0, 0] }) {
        const state = this.#state;
        this.#validate();
        if (state.bitmap === null) {
            state.bitmapWidth = METADISPLAY_BITMAP_WIDTH;
            state.bitmap = new Uint8Array(METADISPLAY_BITMAP_WIDTH * METADISPLAY_BITMAP_WIDTH);
            setFullRefresh(state, 1);
        }
        if (!state.displayPresent || (state.displayFlags & 1) === 0) return;
        state.initialized = (state.initialized | 1) >>> 0;
        if ((state.initialized & 2) === 0) {
            state.initialized = (state.initialized | 2) >>> 0;
            state.cached.rectangle = [0, 0, 0, 0];
        }
        const rectangleChanged = view.rectangle.some((value, index) => (value | 0) !== (state.cached.rectangle[index] | 0));
        if (state.count !== 0 || fullRefresh(state) !== 0 || !sameCamera(view.camera, state.cached.camera) || rectangleChanged) {
            if (typeof this.#hooks.rebuild !== 'function') throw new Error('미니맵 표시 재생성 몸체가 연결되지 않았습니다.');
            state.cached = { camera: [view.camera[0] >>> 0, view.camera[1] >>> 0], rectangle: view.rectangle.map((value) => value | 0) };
            this.#hooks.rebuild();
        }
        state.frames = (state.frames + 1) >>> 0;
        if (fullRefresh(state) !== 0) {
            if (typeof this.#hooks.redrawCell !== 'function') throw new Error('미니맵 전체 셀 그림 몸체가 연결되지 않았습니다.');
            // 전체 갱신은 y 를 바깥으로 모든 64×64 셀을 그린다.
            for (let y = 0; y < METADISPLAY_CELL_COUNT; y += 1) {
                // 새 큐 등록은 이 순서를 바꾸지 않는다.
                for (let x = 0; x < METADISPLAY_CELL_COUNT; x += 1) this.#hooks.redrawCell(x, y);
            }
        } else {
            if (state.count !== 0 && typeof this.#hooks.redrawCell !== 'function') throw new Error('미니맵 부분 셀 그림 몸체가 연결되지 않았습니다.');
            // 매 셀 뒤 현재 count 를 다시 읽는다.
            for (let index = 0; index < state.count; index += 1) {
                this.#validate();
                const cell = state.cells[index];
                this.#hooks.redrawCell(cell.x, cell.y);
            }
        }
        state.count = 0;
        setFullRefresh(state, 0);
    }
}
