// UberGump 의 실제 입력·Present 위치에 가장자리 스크롤과 카메라 적용을 현재 Client 장치로 연결하는 부분을 옮긴다(cpppj `client/UberGumpCamera.cpp`, 기준선 1f00a25a, 10.78 전용).
// 원문 계산 본체는 이미 따로 이식돼 있다: 가장자리 입력(`processEdgeScroll`·`requestCameraDelta`, core/raw-edge-scroll.js), 497960 카메라 적용(`commitCameraTarget`, render/raw-camera-commit.js), Present 말미(`presentCameraTarget`,
// render/raw-camera-present.js), 대본 ViewSpot 해석(`parseViewSpot`, core/camera-home.js)과 433aa0 즉시 이동(`ClientCameraGlobals.centerImmediately`). 이 모듈은 그 본체가 요구하는 훅/전역을 같은 현재 화면·렌더러·월드·버튼 장치에서
// 공급하고, 카메라 위치/배율이 바뀌었을 때만 월드에 공개하고 다음 장면 합성을 예약하는 호출 순서를 보존한다.
// 사용: const camera = new UberGumpCamera(host); camera.pollEdgeScroll(mouse); camera.applyCameraFrame(); camera.centerViewSpot('(139x88)').
// host 는 이식되지 않은 UberGump/Client 의 현재 장치를 빌려 주는 계약이다(C++ 멤버 이름에서 소문자로 옮김). 모든 장치 조회는 호출 때마다 현재 값을 다시 읽는다:
//   client = { configuration()→readInt(key, current)→{changed,value}, screen()→{width,height,flags,borderOffset(),scroll(x,y,w,h,dx,dy),flip()}, renderer()→{cameraRedraw,cameraDirtyEntries(),invalidate(rect,flags),setSoftwareCursor(image,at,force)},
//     gameplay()→{camera()→ClientCameraGlobals, cursor()→{lockDepth}, selection()→{mouse:{x,y}}, patchDiagnostic()→{value}, worldCells()→{value}}, localState()→{metaRebuild()→{value}}, time()→{wall},
//     active()→불리언, poll(code)→{code}, cursor()→{hotspot()→{x,y}, softwareImage()} },
//   buttons() = { serialGump(serial), blockingGumpOpen(), requestClosePopups(), currentDialog(), redrawRoots() }, world() = null | { mapView()→{left,top,right,bottom}, session()→{hasCursorRuntimeForState(), cursor()→{lock(skip),unlock()}}, cameraApplied() },
//   gameMenuState() = null | { menuSerial }, pointerBoundary = { initializationFlags }(GumpPointerBoundaryState 와 같은 객체), worldRedraw/rebuild = `{ value }` 상자(0059a8b0 / UberGump 의 재구성 예약).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E469: 5-B UberGumpCamera 이식, 기준선 뒤 cpppj 변경 없음)
import { parseViewSpot } from '../core/camera-home.js';
import { processEdgeScroll, requestCameraDelta } from '../core/raw-edge-scroll.js';
import { processCameraPan } from '../core/raw-camera-pan.js';
import { commitCameraTarget } from '../render/raw-camera-commit.js';
import { presentCameraTarget } from '../render/raw-camera-present.js';
import { ScreenMode } from '../render/screen.js';

// 카메라 64BYTE 상태에서 현재 Q16 배율(+18)의 위치이다. CameraView 에 배율 접근자가 없어 Present 전후 비교에 직접 읽는다. (2026-11-15 WB04-E469)
const ZOOM_AT = 24;

// 실제 호스트 장치 한 묶음이 계약을 갖추었는지 확인한다. 부족하면 첫 호출 전에 거부한다(C++ invalid_argument → TypeError). (2026-11-15 WB04-E469)
function checkHost(host) {
    const client = host?.client;
    if (!client || ['configuration', 'screen', 'renderer', 'gameplay', 'localState', 'time', 'active', 'poll', 'cursor'].some((name) => typeof client[name] !== 'function')
        || ['buttons', 'world', 'gameMenuState'].some((name) => typeof host[name] !== 'function')
        || typeof host.pointerBoundary !== 'object' || host.pointerBoundary === null
        || typeof host.worldRedraw !== 'object' || host.worldRedraw === null || typeof host.rebuild !== 'object' || host.rebuild === null) throw new TypeError('UberGumpCamera 현재 장치 계약 부족');
}

// UberGump 의 카메라 입력/적용 멤버(PollEdgeScroll·CommitCameraNow·CenterViewSpot·CenterViewPoint·ApplyCameraFrame)와 그 장기 상태(가장자리 설정 두 값과 최초 시각)를 가진다. (2026-11-15 WB04-E469)
export class UberGumpCamera {
    // 현재 장치를 빌려 주는 host 와 보호 PE 초기값을 가진 가장자리 설정(005318f8=1, 005318f0=20), 005cafc8 의 장기 최초 시각이다. 누락 설정은 현재값을 유지한다.
    #host;
    #enabled = 1;
    #speed = 20;
    #since = 0;
    // 005318xx 의 `scrollWhileMouseOut`(창 밖에서도 Alt/가운데 버튼 이동을 허용) 설정값이다. 누락 시 현재 값을 유지한다. (2026-11-21 WB04-E1145)
    #scrollWhileMouseOut = 0;
    // processEdgeScroll 이 제자리에서 바꾸는 상태이다. 초기화 비트는 GumpPointerBoundaryState 와, 최초 시각은 이 객체의 같은 멤버와 공유한다(복사본이 아님).
    #edgeState;

    // 장치 계약을 확인하고 공유 상태를 연결한다. 사용: new UberGumpCamera({ client, buttons, world, gameMenuState, pointerBoundary, worldRedraw, rebuild }). (2026-11-15 WB04-E469)
    constructor(host) {
        checkHost(host);
        this.#host = host;
        const owner = this;
        this.#edgeState = {
            get initializationFlags() { return host.pointerBoundary.initializationFlags; },
            set initializationFlags(value) { host.pointerBoundary.initializationFlags = value; },
            get since() { return owner.#since; },
            set since(value) { owner.#since = value; },
        };
    }

    // 현재 가장자리 스크롤 설정(켜짐 DWORD)이다. 읽기 전용 관찰 접근자이다. (2026-11-15 WB04-E469)
    get edgeScrollEnabled() { return this.#enabled; }

    // 현재 가장자리 스크롤 속도 한계이다. (2026-11-15 WB04-E469)
    get edgeScrollSpeed() { return this.#speed; }

    // 005cafc8 의 장기 가장자리 최초 시각이다(World/페이지 교체로 지우지 않음). (2026-11-15 WB04-E469)
    get edgeScrollSince() { return this.#since; }

    // Guide 통지 뒤 현재 폴링을 공급하고 목표만 쓴다. 실제 현재 view 는 Present 뒤(applyCameraFrame)에 적용한다. mouse = { x, y }(현재 입력 사건 좌표). (2026-11-15 WB04-E469)
    pollEdgeScroll(mouse) {
        const client = this.#host.client;
        // 설정에서 두 값을 읽는다(키가 없거나 같은 값이면 현재 값을 유지).
        this.#enabled = client.configuration().readInt('edgeScroll', this.#enabled).value | 0;
        this.#speed = client.configuration().readInt('edgeScrollSpeed', this.#speed).value | 0;
        const screen = client.screen(); const border = screen.borderOffset();
        // 원문 장치 호출 위치마다 현재 버튼 소유자/카메라/화면 크기를 다시 조회한다.
        const hooks = {
            poll: (code) => client.poll(code),
            menu: (serial) => this.#host.buttons().serialGump(serial),
            blockingPopup: () => this.#host.buttons().blockingGumpOpen(),
            closePopups: () => { this.#host.buttons().requestClosePopups(); },
            request: (dx, dy) => { const camera = client.gameplay().camera(); requestCameraDelta(camera.rendererView(), camera.target(), client.screen().height, dx, dy, client.localState().metaRebuild()); },
        };
        // Windows10/11 borderless 호환 모드는 다른 앱이 활성일 때 바깥 포인터로 배경 카메라를 이동하지 않는다. 원문 입력 계산/타이머는 같은 본체를 사용한다.
        const enabled = client.active() ? this.#enabled >>> 0 : 0;
        const menu = this.#host.gameMenuState();
        processEdgeScroll(this.#edgeState, { enabled, dialog: this.#host.buttons().currentDialog(), menuSerial: menu ? menu.menuSerial : 0, borderX: border.x, borderY: border.y,
            width: screen.width, height: screen.height, x: mouse.x, y: mouse.y, speed: this.#speed, wall: client.time().wall }, hooks);
    }

    // 원본 4d6b54 의 Alt/가운데 버튼 카메라 이동이다. Alt 또는 가운데 버튼을 누른 동안 포인터가 지도 영역 중심에서 떨어진 만큼 그 쪽으로 카메라 목표를 옮긴다(Shift 를 함께 누르면 한 축만).
    // 이동 경로가 처리한 프레임(true)에는 호출자가 큐의 사건을 일반 입력으로 나누지 않고 버려야 한다(원본·cpppj UberGump::PollCameraPan 과 같다). 목표만 쓰고 실제 적용은 Present 뒤(applyCameraFrame)이다.
    // mouse = { code, x, y }(현재 포인터 폴링 사건: Alt 폴링 결과). 지도 영역은 월드의 현재 mapView 이며 없으면 전체 화면이다. (2026-11-21 WB04-E1145: 웹 앱 미연결이던 ALT 스크롤 연결)
    pollCameraPan(mouse) {
        const client = this.#host.client;
        this.#scrollWhileMouseOut = client.configuration().readInt('scrollWhileMouseOut', this.#scrollWhileMouseOut).value | 0;
        const screen = client.screen(); const world = this.#host.world();
        const view = world ? world.mapView() : { left: 0, top: 0, right: screen.width, bottom: screen.height };
        const buttons = this.#host.buttons();
        const hooks = {
            poll: (code) => client.poll(code),
            // 창 밖 이동 허용 설정·대화상자(현재 대화창 또는 팝업)·활성 여부·화면 크기·지도 영역을 호출마다 다시 읽는다.
            globals: () => ({ scrollWhileMouseOut: this.#scrollWhileMouseOut, dialog: buttons.currentDialog() || (buttons.blockingGumpOpen() ? 1 : 0), active: client.active() ? 1 : 0,
                clientWidth: screen.width, clientHeight: screen.height, viewX: view.left, viewY: view.top, viewWidth: (view.right - view.left) | 0, viewHeight: (view.bottom - view.top) | 0 }),
            closePopups: () => { buttons.requestClosePopups(); },
            request: (dx, dy) => { const camera = client.gameplay().camera(); requestCameraDelta(camera.rendererView(), camera.target(), client.screen().height, dx, dy, client.localState().metaRebuild()); },
        };
        return processCameraPan(mouse, hooks);
    }

    // 11.01 방향키 카메라 이동: 키 사건 한 번(누름·브라우저 자동 반복)이 낸 논리 화면 dx·dy 픽셀을 카메라 목표에 누적한다(cpppj `UserInput::Event` 의 방향키 분기).
    // 기준점을 현재 적용 위치가 아니라 현재 목표(`target().pixels`)로 두어, 같은 프레임의 여러 사건이 서로를 덮어쓰지 않고 쌓이게 한다. 지도 경계 제한은 기존 Present(`presentCameraTarget`)가 적용한다.
    // 이동량은 호출자가 정한다(웹은 32). 목표만 쓰고 실제 적용은 Present 뒤(applyCameraFrame)이다. 사용: camera.requestKeyDelta(-32, 0). (2026-11-23 WB04-E1291, 사용자 요구)
    requestKeyDelta(dx, dy) {
        const client = this.#host.client; const camera = client.gameplay().camera(); const target = camera.target();
        requestCameraDelta({ pixels: { x: target.pixels[0], y: target.pixels[1] } }, target, client.screen().height, dx | 0, dy | 0, client.localState().metaRebuild());
    }

    // 497960 의 실제 장치/dirty/두 setter 를 같은 현재 Client 에서 실행한다. 표시/음향 공개 시점은 호출자가 소유한다. (2026-11-15 WB04-E469)
    commitCameraNow() {
        const client = this.#host.client; const gameplay = client.gameplay(); const camera = gameplay.camera(); const renderer = client.renderer(); const screen = client.screen(); const world = this.#host.world();
        // 월드가 설치돼 있으면 현재 지도 영역(left/top/right/bottom), 없으면 전체 화면을 viewport 로 쓴다. 값 사본이다.
        const mapView = world ? world.mapView() : { left: 0, top: 0, right: screen.width, bottom: screen.height };
        const clip = { left: mapView.left, top: mapView.top, right: mapView.right, bottom: mapView.bottom };
        const viewport = { x: clip.left, y: clip.top, width: (clip.right - clip.left) | 0, height: (clip.bottom - clip.top) | 0 };
        // 원문 지역 사본이다: 화면 크기와 모드 플래그는 호출 뒤 장치 효과가 바꿔도 이 값을 쓴다.
        const width = screen.width; const height = screen.height; const flags = screen.flags;
        const hooks = {
            // 커서 잠금/해제는 설치된 World 의 활성 커서가 있으면 그것을, 없으면 같은 DWORD 만 증가/감소한다(원문은 비영 깊이일 때 Activate/장치 없이 증감).
            lockCursor: (skip) => {
                const current = this.#host.world();
                if (current && current.session().hasCursorRuntimeForState()) { current.session().cursor().lock(skip); return; }
                const cursor = gameplay.cursor();
                if (!cursor.lockDepth) throw new Error('미설치 World의 활성 커서 Lock 경계 필요');
                cursor.lockDepth = (cursor.lockDepth + 1) >>> 0;
            },
            unlockCursor: () => {
                const current = this.#host.world();
                if (current && current.session().hasCursorRuntimeForState()) { current.session().cursor().unlock(); return; }
                const cursor = gameplay.cursor();
                // 감소는 먼저 일어나며 결과가 0 이면 원문 preview 재생성 경계가 필요하다(거부).
                cursor.lockDepth = (cursor.lockDepth - 1) >>> 0;
                if (!cursor.lockDepth) throw new Error('미설치 World의 활성 커서 preview 재생성 경계 필요');
            },
            redrawRoots: () => { this.#host.buttons().redrawRoots(); this.#host.rebuild.value = true; },
            scroll: (x, y, w, h, dx, dy) => { screen.scroll(x, y, w, h, dx, dy); },
            // Windows10/11 호환 Screen 은 항상 DIB2 이므로 원문 부모가 이 장치를 선택하지 않는다.
            scrollBackground: () => { throw new Error('비DIB DirectDraw 시차 장치 미복원'); },
            addDirty: (rect, value) => { renderer.invalidate(rect, value); },
            addBackgroundDirty: (rect) => { renderer.invalidate(rect); },
            // 소프트웨어 마우스 비트가 없으면 4a41c0 도 자연 반환한다. 핫스폿을 먼저 읽고 활성일 때만 그림을 건넨다.
            updateMouse: (point) => {
                if (!(screen.flags & ScreenMode.softwareMouse)) return;
                const cursor = client.cursor(); const hotspot = cursor.hotspot();
                renderer.setSoftwareCursor(client.active() ? cursor.softwareImage() : null, { x: point.x - hotspot.x, y: point.y - hotspot.y }, true);
            },
        };
        // 같은 현재 Renderer·snapshot·목표·변경 표를 빌려 주고, 분모와 마우스는 호출 뒤에도 현재 값을 다시 읽게 한다.
        commitCameraTarget({ renderer: camera.rendererView(), snapshot: camera.snapshot(), target: camera.target(), redraw: renderer.cameraRedraw, worldRedraw: this.#host.worldRedraw, dirty: renderer.cameraDirtyEntries() },
            { screenFlags: flags, get denominator() { return camera.denominator().current; }, width, height, viewport, fullViewport: clip, get mouse() { return gameplay.selection().mouse; } }, hooks);
    }

    // 대본 ViewSpot 은 Home 과 같은 즉시 이동이다. 변경된 현재 값만 표면/입자/음향에 공개하고 다음 Compose 를 예약한다(단위676). text = `(XxY)`. (2026-11-15 WB04-E469)
    centerViewSpot(text) {
        const spot = parseViewSpot(text);
        this.centerViewPoint(spot.x, spot.y);
    }

    // 문자열을 다시 만들지 않고 같은 즉시 카메라 부모/현재 Renderer·음향·표면 통지를 공유한다. x/y = 칸 단위 world float. (2026-11-15 WB04-E469)
    centerViewPoint(x, y) {
        const client = this.#host.client; const camera = client.gameplay().camera(); const screen = client.screen();
        const before = camera.rendererView().pixels;
        camera.centerImmediately(x, y, screen.width, screen.height, this.#host.worldRedraw, client.localState().metaRebuild(), () => this.commitCameraNow());
        const after = camera.rendererView().pixels;
        if (after.x !== before.x || after.y !== before.y) {
            const world = this.#host.world();
            if (world) world.cameraApplied();
            this.#host.rebuild.value = true;
        }
    }

    // 실제 화면 출력 뒤에만 flip/목표 제한을 수행하고 현재 카메라를 적용한다. Home 의 직접 적용과 구별한다. (2026-11-15 WB04-E469)
    applyCameraFrame() {
        const client = this.#host.client; const gameplay = client.gameplay(); const camera = gameplay.camera(); const screen = client.screen();
        const view = camera.rendererView();
        const beforePixels = view.pixels; const beforeZoom = view.data.getUint32(ZOOM_AT, true);
        // 원문 지역 사본이다: flip 효과가 화면 크기를 바꿔도 제한 계산은 이 값을 쓴다.
        const width = { value: screen.width }; const height = { value: screen.height };
        presentCameraTarget(camera.target(), { diagnostic: gameplay.patchDiagnostic(), width, height, worldCells: gameplay.worldCells() },
            { flip: () => { screen.flip(); }, commit: () => { this.commitCameraNow(); } });
        const afterPixels = view.pixels;
        // 위치나 배율이 바뀐 경우에만 프레임 말미의 같은 원점으로 표면/입자/위치 음향을 갱신하고 다음 Compose 를 예약한다.
        if (afterPixels.x !== beforePixels.x || afterPixels.y !== beforePixels.y || view.data.getUint32(ZOOM_AT, true) !== beforeZoom) {
            const world = this.#host.world();
            if (world) world.cameraApplied();
            this.#host.rebuild.value = true;
        }
    }
}
