// 메인 메뉴 무활동 감시(004cee80)와 활동 사건(004cebf0)을 옮긴 모듈이다(cpppj `client/MainMenuActivity.cpp/.h`, 10.78 전용). 벽시각 기준이며 GameClock 정지와 무관하다.
// state = { deadline(마지막 활동의 벽시각 + 45초), pointer: { x, y }(마지막 재설정 때 복사한 마우스 위치, 생성 시 0,0) } 이다.
//  - idleMainMenuActivity(state, enabled, htmlInstances, pointer, currentDialog, self, hooks): enabled()(0054db90 autoDemo 옵션)가 0 이면 끝낸다(누락 옵션을 0 으로 덮지 않음). 아니면 HTML 인스턴스가 양수이거나(이때 팝업을 조회하지 않음) 팝업(004641d0)이 있거나 포인터가 바뀌었으면
//    deadline = now() + 45 와 pointer 사본을 갱신한다(now 가 포인터를 바꿔도 그 뒤 현재 값을 복사). 현재 모달이 self 이면 한 번 더 now() 를 읽어 deadline 이 지났으면 queue(0x020000c0, 'DemoStormRages', 'DemoStormRages') 뒤 kill() 한다(재설정 뒤에도 같은 프레임에 만료를 검사, 큐 효과는 kill 장치 누락이 되돌리지 않음).
//    enabled/htmlInstances/pointer/currentDialog 는 원문 전역의 현재 값을 읽는 getter 이다.
//  - handleMainMenuActivity(state, event, now): 수정키를 지운 코드가 활동 사건(0x020000ac)이 아니면 false, 맞으면 뗌/autoDemo 와 무관하게 deadline = now() + 45 로 갱신한다.
// 필요한 실제 조회/장치만 그 분기에서 요구한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E33: 메인 메뉴 활동 이식으로 추가)

// 005429a4/005429f4 의 초기 DWORD, 수정키 제거 마스크, 무활동 허용 시간(초)이다. (2026-11-08 WB04-E33)
export const MAIN_MENU_ACTIVITY_EVENT = 0x020000ac;
export const MAIN_MENU_DEMO_COMMAND = 0x020000c0;
const KEY_MASK = 0x03ffffff;
const IDLE_SECONDS = 45;

// 필요한 실제 장치가 없으면 그 조회에서 거부한다. (2026-11-08 WB04-E33)
function need(device, name) {
    if (typeof device !== 'function') throw new TypeError(`${name} 없음`);
    return device;
}

// hooks = { popup(), now(), queue(명령, 문자열, 문자열), kill() } 이다. (2026-11-08 WB04-E33)
export function idleMainMenuActivity(state, enabled, htmlInstances, pointer, currentDialog, self, hooks) {
    if (!enabled()) return;
    // 필요한 실제 조회에서만 현재 장치를 요구해 선행 효과를 보존한다.
    const wall = () => need(hooks.now, '메인 활동 벽시각')();
    if (htmlInstances() > 0 || need(hooks.popup, '메인 현재 팝업 장치')() || pointer().x !== state.pointer.x || pointer().y !== state.pointer.y) {
        state.deadline = wall() + IDLE_SECONDS;
        const here = pointer();
        state.pointer = { x: here.x, y: here.y };
    }
    if (currentDialog() === self) {
        const now = wall();
        if (state.deadline <= now) {
            need(hooks.queue, '메인 데모 큐 장치')(MAIN_MENU_DEMO_COMMAND, 'DemoStormRages', 'DemoStormRages');
            need(hooks.kill, '메인 부모 Kill 장치')();
        }
    }
}

// 활동 사건은 코드 마스크만 적용하며 release/autoDemo 켜짐 조건을 추가하지 않는다. (2026-11-08 WB04-E33)
export function handleMainMenuActivity(state, event, now) {
    if (((event & KEY_MASK) >>> 0) !== MAIN_MENU_ACTIVITY_EVENT) return false;
    state.deadline = need(now, '메인 활동 벽시각')() + IDLE_SECONDS;
    return true;
}
