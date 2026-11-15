// 원본 10.78 Cursor 446b50 의 정수 마우스 입력과 4971e0 역투영·41d770 절삭을 연결하는 위치 갱신 모듈이다(cpppj `client/RawCursorPosition.cpp/.h`, 10.78 전용).
// 사용:
//   const position = new RawCursorPosition(drop, placement, snapToGrid, camera, hooks);
//     drop = CursorDropState(screenX·screenY·x·y·lockDepth·legalGump), placement = CursorPlacementState(첫 그림의 hotspotX/Y·imageWidth/Height), snapToGrid = 55a2d8 을 읽는 `{ value }` 상자 또는 함수,
//     camera = 현재 59a900/904 의 float 비트 두 칸 배열(같은 배열을 제자리 갱신하면 현재 값을 읽는다) 또는 그 배열을 돌려주는 함수, hooks = { activate(0) — 445aa0, selectCursor(2|3) — 4a4410, createPreviews() — 445b30 }.
//   position.update(mouseX, mouseY) → 정수 두 인자(int32)로 화면점을 갱신하고, 월드 위치가 바뀌면 현재 잠금을 다시 읽어 정리(Activate)·목록 생성·커서 선택(legalGump 이면 3, 아니면 2)을 수행한다.
//   RawCursorPosition.screenToWorld(x, y, camera) → 4971e0: 정수 감김 뒤 단정도 역배율과 현재 카메라를 x87 중간 정밀도(double)로 합해 마지막에만 float 로 저장한다.
//   RawCursorPosition.cameraFromPixels(x, y) → 497490: scroll pixel 을 먼저 float 로 좁히지 않고 현재 float 카메라 비트를 만든다.   RawCursorPosition.truncatePoint([x, y]) → 41d770: _ftol signed64 하위 DWORD 를 float 로 다시 저장(반올림/floor 아님).
//   makeCursorPositionHooks(lifecycle, previews, selectCursor) → 같은 풀의 실제 정리/생성 두 몸체와 커서 선택기를 연결한다.   bindCursorPosition(lifecycleHooks, position) → Take 꼬리가 읽은 현재 마우스를 같은 위치 갱신에 전달한다.
// 별도 카메라/위치 사본을 만들지 않고 도달한 효과가 없으면 미복원을 명시한다(Error). CD 판본은 웹 범위 밖이다.
// 원본 관측: `cursor-position-x86.tsv` 687행(Point 623: 갱신·역투영·절삭의 18 DWORD 독립 입력, Chain 64: 실제 정리→생성→판정→화면 준비를 두 번 이은 사슬)을 tests/raw-cursor-position.test.mjs 가 대조한다.
// 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E548: 커서 위치 갱신 RawCursorPosition 이식)

// 원본 503b54/504194 의 단정도 역배율이다. 특히 Y 는 double 1/11 로 바꾸지 않는다. (2026-11-15 WB04-E548)
const INVERSE_X = bitsToFloat(0x3d800000);
const INVERSE_Y = bitsToFloat(0x3dba2e8c);

// 도달한 효과가 없으면 미복원을 명시한다. (2026-11-15 WB04-E548)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('커서 위치 갱신 의존 동작 누락');
    return effect;
}

// 단정도 비트를 수로 바꾼다. (2026-11-15 WB04-E548)
function bitsToFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 수를 단정도 비트로 바꾼다(NaN payload 는 호스트 기본값이다). (2026-11-15 WB04-E548)
function floatToBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 정수 화면 산술의 DWORD 감김을 유지한다. (2026-11-15 WB04-E548)
const add = (left, right) => (left + right) | 0;
const sub = (left, right) => (left - right) | 0;

// 실제 _ftol 하위 DWORD 의 signed 해석이다. indefinite 의 하위 DWORD 는 0 이다. (2026-11-15 WB04-E548)
function truncate(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 읽기 원천(`{ value }` 상자 또는 함수)의 현재 값을 읽는다. (2026-11-15 WB04-E548)
const readSource = (source) => (typeof source === 'function' ? source() : source.value) >>> 0;

// 좌표 갱신 뒤 실제 수명/미리보기/하드웨어 커서로 전달하는 위치 갱신 처리기이다. 참조는 처리기보다 오래 살아야 한다. (2026-11-15 WB04-E548)
export class RawCursorPosition {
    // 카메라/스냅과 기존 Drop/Placement 전역을 빌리며 새로운 위치 사본을 소유하지 않는다(10.78 판본만).
    constructor(drop, placement, snapToGrid, camera, hooks, edition = 'patch1078') {
        if (edition !== 'patch1078') throw new TypeError('커서 위치 갱신 10.78 판본 계약 오류');
        this.drop = drop;
        this.placement = placement;
        this.snapToGrid = snapToGrid;
        this.camera = camera;
        this.hooks = hooks;
    }

    // 497490: fild32×단정도 역배율 뒤 최종 float 저장이다(별도 사본의 카메라를 추정하지 않는다).
    static cameraFromPixels(x, y) {
        return [floatToBits(Math.fround((x | 0) * INVERSE_X)), floatToBits(Math.fround((y | 0) * INVERSE_Y))];
    }

    // 4971e0: 실제 fild32 는 정수를 먼저 float 로 좁히지 않는다. Y 와 X 모두 마지막 저장에서만 좁힌다.
    static screenToWorld(x, y, camera) {
        const worldY = Math.fround(add(y | 0, 11) * INVERSE_Y + bitsToFloat(camera[1]));
        const worldX = Math.fround(add(x | 0, 16) * INVERSE_X + bitsToFloat(camera[0]));
        return [worldX, worldY];
    }

    // fild 가 읽는 것은 EDX:EAX 전체가 아니라 EAX 에 저장한 signed32 이다. (41d770)
    static truncatePoint(point) {
        return [Math.fround(truncate(point[0])), Math.fround(truncate(point[1]))];
    }

    // 화면 좌표는 위치 불변/잠금 경로에서도 먼저 갱신한다. 두 인자는 int32 이다. (446b50)
    update(mouseX, mouseY) {
        const drop = this.drop;
        const placement = this.placement;
        drop.screenX = sub(add(mouseX | 0, sub(placement.hotspotX | 0, Math.trunc((placement.imageWidth | 0) / 2))), 16);
        drop.screenY = sub(add(mouseY | 0, sub(placement.hotspotY | 0, Math.trunc((placement.imageHeight | 0) / 2))), 11);
        const camera = typeof this.camera === 'function' ? this.camera() : this.camera;
        let point = RawCursorPosition.screenToWorld(drop.screenX, drop.screenY, camera);
        if (readSource(this.snapToGrid)) point = RawCursorPosition.truncatePoint(point);
        if (point[0] !== drop.x || point[1] !== drop.y) {
            if (!(drop.lockDepth >>> 0)) need(this.hooks.activate)(0);
            drop.x = point[0];
            drop.y = point[1];
            if (!(drop.lockDepth >>> 0)) {
                need(this.hooks.createPreviews)();
                need(this.hooks.selectCursor)(drop.legalGump ? 3 : 2);
            }
        }
    }
}

// 같은 풀의 실제 정리/생성 두 몸체를 연결한다. selectCursor 는 기존 Cursor 의 실제 선택기이며 서로 다른 풀과 선택기 누락은 거부한다. (2026-11-15 WB04-E548)
export function makeCursorPositionHooks(lifecycle, previews, selectCursor) {
    if (lifecycle.pool !== previews.pool || typeof selectCursor !== 'function') throw new TypeError('커서 위치 갱신 단일 풀/선택기 연결 오류');
    return { activate: (force) => lifecycle.activate(force), selectCursor, createPreviews: () => previews.create() };
}

// 실제 Take 의 마지막 두 정수 인자를 보존한다. (2026-11-15 WB04-E548)
export function bindCursorPosition(lifecycleHooks, position) {
    lifecycleHooks.updatePosition = (x, y) => position.update(x, y);
}
