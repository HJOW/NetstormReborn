// 원본 004d65ac..004d67ad 의 가장자리 스크롤 입력과 497e00 의 현재 카메라 목표 쓰기를 옮긴다(cpppj client/RawEdgeScroll.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 실제 GUI/OS 조회는 현재 소유자가 같은 호출 위치에서 훅으로 공급한다. 가장자리 2픽셀 관문·시간 가속(원문 FSUB→FMUL→FADD 저장 정밀도를 합치지 않음)·팝업 닫기·Shift 축 선택·최종 방향 반환과
// 현재 view 를 즉시 바꾸지 않고 목표에만 쓰는 카메라 이동 요청(requestCameraDelta)을 포함한다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const direction = processEdgeScroll(state, { enabled, dialog, menuSerial, borderX, borderY, width, height, x, y, speed, wall }, { poll, menu, blockingPopup, closePopups, request }).
// (2026-11-14 WB04-E442: RawMenuControls 선행 가장자리 스크롤 이식)
import { InputCode } from './input-queue.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 원문 Shift 폴링 코드다. 왼쪽 버튼 조회는 InputCode.leftButton 이다.
const SHIFT_POLL = 0x00100000;
// 원본 ADD 의 하위 32비트 signed 결과다(signed overflow 를 실행하지 않는다).
const add = (a, b) => (a + b) | 0;
// 실제 호출되는 장치만 요구한다(logic_error→Error).
function need(callback) { if (typeof callback !== 'function') throw new Error('가장자리 스크롤 현재 장치 미연결'); return callback; }

// 초기화→현재 관문→가속/팝업/Shift→목표 요청 순서이며 최종 방향 [X, Y](float32 -1/0/1)를 반환한다.
// state = { initializationFlags(005cafe8, GumpPointerBoundaryState 와 공유하며 비트 4 만 초기화), since(005cafc8 최초 가장자리 시각, 화면/미션마다 임의 초기화하지 않음) } 를 제자리에서 바꾼다.
// input = { enabled(005318f8), dialog(00565dd4), menuSerial(005cad24), borderX/borderY(실제 창 테두리), width/height(논리 화면), x/y(현재 포인터), speed(005318f0), wall(0055b4d8 현재 프레임 시각) },
// hooks = { poll(코드)(좌버튼/Shift 의 452e00 결과를 새로 읽음), menu(serial)(464020, 위쪽 가장자리만), blockingPopup()(4641d0), closePopups()(466710 Kill 요청), request(dx, dy)(497e00) } 이다.
export function processEdgeScroll(state, input, hooks) {
    const direction = [0, 0];
    // 전역 flags 는 입력 앞단과 공유하며 이 구간 밖의 포인터 타이머 비트 8 등은 보존한다.
    if (!(state.initializationFlags & 4)) { state.initializationFlags = (state.initializationFlags | 4) >>> 0; state.since = 0.0; }
    if (!(input.enabled >>> 0) || (input.borderX | 0) || (input.borderY | 0) || (input.dialog >>> 0)) return direction;
    if (!((need(hooks.poll)(InputCode.leftButton).code >>> 0) & InputCode.release)) return direction;
    if (input.x < 2) direction[0] = -1.0;
    if (input.y < 2 && !need(hooks.menu)(input.menuSerial)) direction[1] = -1.0;
    if (input.x >= add(input.width, -1)) direction[0] = 1.0;
    if (input.y >= add(input.height, -1)) direction[1] = 1.0;
    if ((direction[0] === 0.0 && direction[1] === 0.0) || need(hooks.blockingPopup)()) { state.since = 0.0; return direction; }
    if (state.since === 0.0) state.since = input.wall;
    const elapsed = input.wall - state.since; const accelerated = elapsed * 30.0;
    const amount = Math.fround(Math.min(input.speed | 0, cameraFtolLow(accelerated + 2.0)));
    need(hooks.closePopups)();
    // Shift 가 눌려 있으면(폴링 코드의 뗌 비트 없음) 절댓값이 큰 한 축만 남긴다(같으면 X 를 지움).
    if (!((need(hooks.poll)(SHIFT_POLL).code >>> 0) & InputCode.release)) {
        if (Math.abs(cameraFtolLow(direction[1])) < Math.abs(cameraFtolLow(direction[0]))) direction[1] = 0.0;
        else direction[0] = 0.0;
    }
    const dy = cameraFtolLow(amount * direction[1]); const dx = cameraFtolLow(amount * direction[0]);
    need(hooks.request)(dx, dy);
    return direction;
}

// 497e00: 목표는 현재 Renderer 의 1c/20(view.pixels)을 쓰며 이전 목표 50/54 에 이동량을 누적하지 않는다: 목표 X = 현재 X + dx, 목표 Y = (height/2) + 현재 Y + dy − (height/2)(모두 하위 32비트 감김).
// 005c85dc(metaRebuild, { value } 상자)를 한 번 증가시킨다. view 는 CameraView(pixels 가 { x, y })이고 target 은 { pixels: [X, Y] }(ClientCameraGlobals.target()) 이다.
export function requestCameraDelta(view, target, height, dx, dy, metaRebuild) {
    const half = (height / 2) | 0; const pixels = view.pixels;
    target.pixels[0] = add(pixels.x, dx);
    target.pixels[1] = add(add(add(half, pixels.y), dy), (0 - half) | 0);
    metaRebuild.value = (metaRebuild.value + 1) >>> 0;
}
