// 시작 title 그림 세 장의 표시 수명을 옮긴 모듈이다(cpppj `client/TitleBitmapLifetime.cpp/.h`, 원문 4ab3e0/4ab370/4ab2d0). 시작 title 과 일반 BackgroundGif 의 소유 수명을 구별한다.
// 사용:
//   const globals = { bitmaps: Uint32Array(8), current: 정수(signed), paintDepth: DWORD, width, height, gameSeconds }  — 전역 사본 없이 현재 값을 읽고 쓰는 속성 묶음(005c7c24..40 의 8칸·005c7c20 현재 선택(실행 첫 단계에서 −1)·0054dc54 WM_PAINT 깊이·현재 SCREENW/H·0055b4d0 게임 시각).
//   hooks = { pump(), lock(), unlock(), present(), flip(), frontLock(), frontUnlock(), input() → 코드, game() → 시각, wall() → 시각, load(이름, out{width,height}) → 토큰, palette(이름), copy(c), frontCopy(c), fill(사각형, 색), release(토큰), clip(사각형) }
//     c = { x, y, bitmap, stride, source: { left, top, right, bottom } }. 원문 큰 부모가 도달한 장치만 호출하며 미도달 장치를 빈 성공으로 채우지 않는다.
//   runTitleBitmaps(globals, hooks) — 4ab3e0: 현재 −1·깊이 +1·전체 clip·pump 뒤 입력이 있으면 건너뛰고, 아니면 title0.gif 적재→title00.col 팔레트→표시(가운데 정렬 복사·Unlock·Present·Flip)→게임 시각 기록→3초 기한 안에 title6/7 적재(기한에 포함)→
//     입력 대기→title06.col→title6 표시→3초 대기→title7 표시→3초 대기. 마지막에는 8칸을 앞에서부터 release(0 으로 재작성하지 않음)하고 검정으로 채운 뒤 깊이를 되돌린다. 예외를 정상 cleanup 으로 바꾸지 않는다.
//   waitTitleBitmap(milliseconds, hooks) — 4ab370: 기한 = ms × 0.001 + wall() 이며 기한 도달은 pump/input 을 더 부르지 않고 false, 입력이 있으면 true.
//   redrawTitleBitmap(globals, hooks) — 4ab2d0 WM_PAINT: 현재 −1 은 검정 출력, 그 밖은 현재 bitmap 의 전체 SCREEN stride 복사(0 번 표/범위 밖은 거부).
//   requestInitialTitleBitmap(next) — 4b3500: next[0]=42, next[35]=0 만 쓰고 나머지 34DWORD 는 보존한다.
//   TitleBitmapQuit — 435b30 의 WM_QUIT/_exit 를 host Run 의 정상 반환으로 잇는 제어 예외이다(게임/파일 오류와 구별하고 State 41 정상 후반을 합성하지 않는다).
// 원본 관측: `title-bitmap-lifetime-x86.tsv` 33행을 tests/title-bitmap-lifetime.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E533: 5-C 메인 메뉴·타이틀 상태 TitleBitmapLifetime 이식)

// WM_QUIT 정상 종료 제어 예외이다. (2026-11-15 WB04-E533)
export class TitleBitmapQuit extends Error {
    constructor() {
        super('Title bitmap WM_QUIT');
        this.name = 'TitleBitmapQuit';
    }
}

// 선택된 실제 장치만 요구한다. 미도달 device 를 빈 성공으로 채우지 않는다. (2026-11-15 WB04-E533)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Title bitmap의 선택된 실제 장치 미연결');
    return hook;
}

// 현재 표를 Lock 뒤 읽어 raw 인자에 전달한다(가운데 정렬 위치, 그림 치수 stride, 전체 원본 사각형). (2026-11-15 WB04-E533)
function display(globals, hooks, width, height, index) {
    need(hooks.lock)();
    need(hooks.copy)({
        x: Math.trunc(globals.width / 2) - Math.trunc(width / 2),
        y: Math.trunc(globals.height / 2) - Math.trunc(height / 2),
        bitmap: globals.bitmaps[index] >>> 0,
        stride: width,
        source: { left: 0, top: 0, right: width, bottom: height },
    });
    need(hooks.unlock)();
    need(hooks.present)();
    need(hooks.flip)();
}

// ms 의 signed FILD 와 원문 0.001 곱을 유지한다. 기한 도달은 pump/input 을 더 호출하지 않는다. (2026-11-15 WB04-E533)
export function waitTitleBitmap(milliseconds, hooks) {
    const end = (milliseconds | 0) * 0.001 + need(hooks.wall)();
    for (;;) {
        if (end <= need(hooks.wall)()) return false;
        need(hooks.pump)();
        if (need(hooks.input)()) return true;
    }
}

// 큰 부모는 bitmap/paintDepth 를 예외에서 되돌리는 SEH 가 없다. (2026-11-15 WB04-E533)
export function runTitleBitmaps(globals, hooks) {
    globals.current = -1;
    globals.paintDepth = (globals.paintDepth + 1) >>> 0;
    need(hooks.clip)({ left: 0, top: 0, right: globals.width, bottom: globals.height });
    need(hooks.pump)();
    let interrupted = need(hooks.input)() !== 0;
    // 성공 Load 가 쓰기 전에는 원문 미정 local 을 게임 결정으로 사용하지 않는다.
    const size = { width: 0, height: 0 };
    if (!interrupted) {
        globals.bitmaps[0] = need(hooks.load)('title0.gif', size);
        need(hooks.palette)('title00.col');
        display(globals, hooks, size.width, size.height, 0);
        globals.current = 0;
        globals.gameSeconds = need(hooks.game)();
        const end = need(hooks.wall)() + 3.0;
        globals.bitmaps[6] = need(hooks.load)('title6.gif', size);
        globals.bitmaps[7] = need(hooks.load)('title7.gif', size);
        // 두 추가 파일을 읽은 시간도 첫 3초 deadline 에 포함된다.
        for (;;) {
            if (end <= need(hooks.wall)()) break;
            need(hooks.pump)();
            if (need(hooks.input)()) {
                interrupted = true;
                break;
            }
        }
        if (!interrupted) {
            need(hooks.palette)('title06.col');
            if (globals.bitmaps[6]) {
                globals.current = 6;
                display(globals, hooks, size.width, size.height, 6);
            }
            interrupted = waitTitleBitmap(3000, hooks);
            if (!interrupted) {
                if (globals.bitmaps[7]) {
                    globals.current = 7;
                    display(globals, hooks, size.width, size.height, 7);
                }
                waitTitleBitmap(3000, hooks);
            }
        }
    }
    // free callback 뒤 매 반복의 현재 SCREEN 값을 읽고 마지막 값을 fill 에 쓴다. 원문은 반납한 표 포인터를 0 으로 재작성하지 않는다.
    let screenWidth = 0;
    let screenHeight = 0;
    for (let index = 0; index < globals.bitmaps.length; index += 1) {
        if (globals.bitmaps[index]) need(hooks.release)(globals.bitmaps[index] >>> 0);
        screenWidth = globals.width;
        screenHeight = globals.height;
    }
    need(hooks.lock)();
    need(hooks.fill)({ left: 0, top: 0, right: screenWidth, bottom: screenHeight }, 0);
    need(hooks.unlock)();
    need(hooks.present)();
    need(hooks.flip)();
    globals.paintDepth = (globals.paintDepth - 1) >>> 0;
}

// 일반 Frame 이나 BackgroundGif 부모로 대신하지 않는다. 현재 −1 은 검정 출력, 그 밖은 전체 SCREEN stride 의 현재 bitmap 복사이다. (2026-11-15 WB04-E533)
export function redrawTitleBitmap(globals, hooks) {
    if (globals.current === -1) {
        const width = globals.width;
        const height = globals.height;
        need(hooks.lock)();
        need(hooks.fill)({ left: 0, top: 0, right: width, bottom: height }, 0);
        need(hooks.unlock)();
        need(hooks.present)();
        return;
    }
    need(hooks.frontLock)();
    if (globals.current < 0 || globals.current >= 8) throw new RangeError('Title 현재bitmap표의 실제 범위');
    const bitmap = globals.bitmaps[globals.current] >>> 0;
    if (bitmap) {
        need(hooks.frontCopy)({ x: 0, y: 0, bitmap, stride: globals.width, source: { left: 0, top: 0, right: globals.width, bottom: globals.height } });
    }
    need(hooks.frontUnlock)();
}

// 4b3500 의 next[0]=42, next[35]=0 만 쓰고 나머지 34DWORD 는 보존한다. (2026-11-15 WB04-E533)
export function requestInitialTitleBitmap(next) {
    next[0] = 42;
    next[35] = 0;
}
