// 원본 HTML 부모의 선택 명령을 같은 입력 큐에 먼저 예약한 뒤 그 부모만 Kill 한다.
// (cpppj client/DialogCommandLifetime.cpp/.h, 기준선 1f00a25a, 10.78 전용, 원문 c79b0)
// command/parameter 는 c83b0 메타 처리 때 이미 해석한 값이다. Instant 여부와 입력 종류 판정은 호출자가 맡으며 모든 발화는 같은 예약/부모 반납 순서를 사용한다.
// 원본 관측 fixture(dialog-command-lifetime 258행)는 E467 의 dialog-native.test.mjs 가 직접 대조한다. 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E435: RawMenuControls 선행 HTML 대화상자 명령 수명 이식)

// 예약 콜백이 다른 페이지를 만들거나 메타를 바꾸어도 선택 당시 값과 원래 부모를 보존한다. 필수 경계(큐/Kill)가 없으면 아무것도 하지 않고 거부한다.
// code 0 도 원본 c79b0 처럼 예약하며 OnExit 의 조건 분기를 복제하지 않는다. 실제 소멸/OnExit 예약은 프레임 후반 Cleanup 에 남긴다. 사용: fireDialogCommand(parent, { command: 5, parameter: 7 }, queue, kill)
export function fireDialogCommand(parent, selected, queue, kill) {
    if (typeof queue !== 'function' || typeof kill !== 'function') throw new TypeError('HTML 선택의 실제 명령 큐/부모 Kill 경계 누락');
    const command = selected.command >>> 0; const parameter = selected.parameter >>> 0; // 선택 당시 값을 보존한다.
    queue(command, parameter, parameter);
    kill(parent);
}

// 실제 40칸 입력 큐에 DWORD 두 인자를 signed 표현으로 보존해 예약한다. OnExit 는 kill 안에서 즉시 실행하지 않는다.
export function fireDialogCommandWithQueue(parent, selected, queue, kill) {
    fireDialogCommand(parent, selected, (command, first, second) => queue.pushCommand(command, first | 0, second | 0), kill);
}
