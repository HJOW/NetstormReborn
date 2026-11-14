// 원본 4d6b54의 Alt/가운데 버튼 카메라 이동과 두 입력 큐 경로 선택을 분리해 옮긴다(cpppj client/RawCameraPan.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 현재 UI/창 입력을 소비하며 카메라 현재값은 직접 쓰지 않는다. 프레임 delta 나 임시 속도 대신 원문 signed 좌표/FTOL/8 과 Shift 축 선택을 쓴다.
// 목표 쓰기(request)는 497e00 의 현재 view 기준 요청이며 즉시 Scroll 과 다르다(화면 갱신 말미가 현재 view 에 적용한다). 원본 관측 fixture(camera-pan 259행)는 E465 의 raw-camera-native.test.mjs 가 직접 대조한다.
// 사용: const moved = processCameraPan({ code: InputCode.middleButton, x: 300, y: 100 }, { poll, globals, closePopups, request }); true 이면 이동 전용 큐 경로이며 일반 입력을 다시 나누지 않는다.
// (2026-11-14 WB04-E437: RawMenuControls 선행 카메라 pan 이식)
import { InputCode } from './input-queue.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 원문이 Shift 상태를 조회하는 폴링 코드다(`0x00100000`). 가운데 버튼 조회는 InputCode.middleButton 이다.
const SHIFT_POLL = 0x00100000;

// 도달한 장치만 요구한다(logic_error→Error).
function need(callback) {
    if (typeof callback !== 'function') throw new Error('카메라 pan 현재 장치 미연결');
    return callback;
}
// ADD/SUB 의 감긴 signed 32비트 결과다(C++ 부호 없는 덧셈/뺄셈 뒤 재해석과 같다).
const add = (a, b) => (a + b) | 0;
const sub = (a, b) => (a - b) | 0;
// 원문 ABS: INT_MIN 은 여전히 signed 음수다(0-INT_MIN 이 감긴다).
const abs = (value) => (value < 0 ? sub(0, value) : value);
// 비영 관문(signed 32비트로 읽은 값이 0 이 아닌지)이다.
const set = (value) => (value | 0) !== 0;

// 한 축의 이동량이다. 중심 = 원점 + 폭/2, 제한된 위치 = min(max(원점, 포인터), 원점+폭)(뒤집힌 폭도 원문 CMP 순서의 상한 선택을 유지),
// 결과 = FTOL(client/폭 × (제한된 위치 − 중심)) 의 하위 DWORD 다. 비율은 53비트로 먼저 저장하며 뒤 곱과 합치지 않는다(폭 0 이면 ±무한대/NaN 이라 FTOL 이 0 을 준다).
function axis(pointer, origin, extent, client) {
    const center = add(origin, (extent / 2) | 0);
    const bounded = Math.min(Math.max(origin, pointer), add(origin, extent));
    const ratio = client / extent;
    return cameraFtolLow(ratio * sub(bounded, center));
}

// 원본 카메라 이동이다. pointer 는 { code, x, y } 이고 hooks = { poll(코드) → 입력 사건, globals() → CameraPanGlobals, closePopups(), request(x, y) } 다.
// globals 의 필드는 모두 signed 32비트: scrollWhileMouseOut/dialog/active(비영 관문), clientWidth/clientHeight(5c78fc/7900), viewX/viewY/viewWidth/viewHeight(5ca99c/a0/b4/b8).
// 순서: 가운데 버튼 폴링 → 전역 읽기 → 이동 불가(창 밖이고 밖 이동 꺼짐 / 포인터와 가운데 버튼이 모두 뗌 / 대화상자)이면 false(팝업도 닫지 않음)
//   → 팝업 닫기(466710) → 전역 다시 읽기 → 좌표 -1/비활성이면 true(요청 없음) → 두 축 계산 → Shift 가 눌려 있으면(폴링 코드의 뗌 비트 없음) 절댓값이 큰 한 축만 남기고(같으면 X 를 지움)
//   → request(x/8, y/8)(0 방향 절삭) → true.
export function processCameraPan(pointer, hooks) {
    const middle = need(hooks.poll)(InputCode.middleButton);
    const gate = need(hooks.globals)();
    const code = pointer.code >>> 0;
    // 이동 전용 큐 경로로 들어가지 못하는 경우다. 뗌 비트(0x80000000)는 `& … !== 0` 으로 검사한다.
    if ((!set(gate.scrollWhileMouseOut) && (code & InputCode.outside) !== 0)
        || ((code & InputCode.release) !== 0 && ((middle.code >>> 0) & InputCode.release) !== 0)
        || set(gate.dialog)) return false;
    need(hooks.closePopups)();
    const current = hooks.globals();
    if ((pointer.x | 0) === -1 || (pointer.y | 0) === -1 || !set(current.active)) return true;
    let x = axis(pointer.x | 0, current.viewX | 0, current.viewWidth | 0, current.clientWidth | 0);
    let y = axis(pointer.y | 0, current.viewY | 0, current.viewHeight | 0, current.clientHeight | 0);
    // Shift 가 눌려 있으면(뗌 비트 없음) 각각 /8 이전 값의 절댓값이 큰 한 축만 남긴다. 같은 크기면 X 를 지운다.
    if (((hooks.poll(SHIFT_POLL).code >>> 0) & InputCode.release) === 0) {
        if (abs(x) > abs(y)) y = 0;
        else x = 0;
    }
    need(hooks.request)((x / 8) | 0, (y / 8) | 0);
    return true;
}
