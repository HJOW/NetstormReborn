// 표시·안내 키(F2 건물 숨김, F3 아래쪽 표시줄 전환, F7 배너, F8 첫 명령/챌린지 안내)를 현재 표시 전역과 일반 입력 큐에 연결하는 입력 분배 모듈이다(cpppj `client/ViewKeyInput.cpp/.h`, 10.78 전용, 원문 4d9670~4d998c).
// Guide/Gump/Console 관문을 지난 일반 입력 하나(event = { code, x, y })를 받아 소비 여부를 돌려준다. 키 뗌이거나 네 키가 아니면 장치를 요구하지 않고 false 이다.
// 순서: F8(수정키 없음)이면 Fort 가 있을 때 첫 명령(0x02000091,0,0)을 예약하고, 없으면 메타맵일 때 ChalIntro 를 Tell 한 뒤 편집기이면 mode 1 강조 → F3/F8 의 Shift(Control 없음)는 'theammode' 설정(초기 0, 실패해도 유지)을 읽어 SetIslands(0/1)을 Tell →
//   현재 정지가 참이면 여기서 끝(Tell 이 정지를 바꿀 수 있어 Tell 뒤에 조회) → F2 는 redraw=1 과 hideBuildings 반전, F7 은 현재 배너 값의 반대로 쓰기, F3(수정키 없음)은 (0x02000088,0,2) 예약. Ctrl+Shift+F7 은 별도 진단 범위여서 false 이다.
// 선택된 장치의 누락은 그 위치에서 거부하고 앞선 효과는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E25: 표시 키 입력 이식으로 추가)
import { InputCode } from './input-queue.js';

// Win32 가상 키 F2/F3/F7/F8 을 16비트 왼쪽으로 민 값과 원문 초기 명령(542914/542938)이다. (2026-11-08 WB04-E25)
const F2 = 0x710000;
const F3 = 0x720000;
const F7 = 0x760000;
const F8 = 0x770000;
const TOGGLE_BOTTOM = 0x02000088;
const FIRST = 0x02000091;
// 상위 입력 표시 비트를 지운 키 비교 마스크이다. (2026-11-08 WB04-E25)
const KEY_MASK = 0x03ffffff;

// 선택된 실제 장치가 없으면 빈 성공으로 바꾸지 않고 거부한다. (2026-11-08 WB04-E25)
function need(device) {
    if (typeof device !== 'function') throw new Error('표시 키의 실제 장치 미연결');
    return device;
}

// globals = { hideBuildings, redraw }(원문 0059a8ac/0059a8b0 의 읽고 쓰는 DWORD 보관 객체),
// hooks = { paused(), readInt(key, holder{value}) → 찾았는지, tell(section), queue(command, first, second), fort(), metamap(), editor(), highlight(mode), useBanners(), writeBanners(value) } 이다.
// 소비하면 true 이다. (2026-11-08 WB04-E25)
export function dispatchViewKeyInput(event, globals, hooks) {
    const code = event.code & KEY_MASK;
    if ((event.code & InputCode.release) !== 0 || (code !== F2 && code !== F3 && code !== F7 && code !== F8)) return false;
    const shift = (event.code & InputCode.shift) !== 0;
    const control = (event.code & InputCode.control) !== 0;
    if (code === F7 && shift && control) return false;
    if (code === F8 && !control && !shift) {
        // Fort 가 있으면 첫 명령, 없으면 메타맵 안내이며 편집기 조회는 그 뒤 현재 값이다.
        if (need(hooks.fort)()) need(hooks.queue)(FIRST, 0, 0);
        else if (need(hooks.metamap)()) need(hooks.tell)('ChalIntro');
        if (need(hooks.editor)()) need(hooks.highlight)(1);
    }
    if ((code === F3 || code === F8) && shift && !control) {
        // 설정 조회 실패는 초기 0 을 유지해 SetIslands(1)이 된다.
        const mode = { value: 0 };
        need(hooks.readInt)('theammode', mode);
        need(hooks.tell)(mode.value !== 0 ? 'SetIslands(0)' : 'SetIslands(1)');
    }
    // 정지는 Tell 뒤 현재 값을 읽는다.
    if (need(hooks.paused)()) return true;
    if (code === F2) {
        globals.redraw = 1;
        globals.hideBuildings = globals.hideBuildings === 0 ? 1 : 0;
    } else if (code === F7) need(hooks.writeBanners)(need(hooks.useBanners)() === 0 ? 1 : 0);
    else if (code === F3 && !control && !shift) need(hooks.queue)(TOGGLE_BOTTOM, 0, 2);
    return true;
}
