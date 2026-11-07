// 현재 모달 대화상자를 화면의 가장자리로 옮기는 명령(004c8df0 MoveDialog)을 공통 Gump 노드의 실제 필드와 연결하는 모듈이다(cpppj `client/MoveDialog.cpp/.h`, 10.78 전용).
// moveDialog(current, where, screenWidth, screenHeight, hooks): 원문은 565dd4 의 현재 노드를 먼저 캡처하고 가상 Push 이후에도 그 포인터의 현재 필드를 쓴다. 노드가 없으면 끝낸다.
//   push() [캡처한 노드의 가상 +18] 뒤 'where' 문자열(첫 NUL 앞)에서 소문자 b/t/r/l 의 존재를 원문 순서로 검사한다: b → y = float(화면 높이 − 노드 높이), t → y = 0, r → x = float(화면 폭 − 노드 폭), l → x = 0 (b 보다 t, r 보다 l 이 우선, m/c 는 좌표를 바꾸지 않음).
//   화면 정수는 산술(double) 뒤에만 float 로 저장하며 Push 효과 뒤의 실제 화면/창 크기를 읽는다(screenWidth/screenHeight 는 getter). 마지막으로 attach(x, y, 0, 0)(00466750)으로 같은 노드를 다시 붙인다. 노드 flags/depth 는 건드리지 않는다.
// 명령 번호는 MOVE_DIALOG_COMMAND(0x020000ed, 542aa8)이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E33: MoveDialog 이식으로 추가)

// 보호 PE 542aa8 의 일반 입력 명령 번호이다. (2026-11-08 WB04-E33)
export const MOVE_DIALOG_COMMAND = 0x020000ed;

// current = { x, y, width, height }(float), where 는 위치 문자열, hooks = { push(), attach(x, y, parent, mode) } 이다. (2026-11-08 WB04-E33)
export function moveDialog(current, where, screenWidth, screenHeight, hooks) {
    if (!current) return;
    if (typeof hooks?.push !== 'function' || typeof hooks.attach !== 'function') throw new TypeError('MoveDialog 의 실제 Push/Attach 미연결');
    hooks.push();
    const nul = where.indexOf('\0');
    const text = nul < 0 ? where : where.slice(0, nul);
    if (text.includes('b')) current.y = Math.fround(screenHeight() - current.height);
    if (text.includes('t')) current.y = 0;
    if (text.includes('r')) current.x = Math.fround(screenWidth() - current.width);
    if (text.includes('l')) current.x = 0;
    hooks.attach(current.x, current.y, 0, 0);
}
