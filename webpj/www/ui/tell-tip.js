// 시작 팁 창("Did You Know?")의 Prior Tip / Next Tip / OK 버튼(`TellTip,1|2|3`)이 하는 일이다. 원본 004d6fc9..004d7044 를 옮겼다(cpppj client/TellTip.cpp, 원본 기계어 388관측과 대조).
//  - 인자 y 가 1 이면 팁 번호를 1 줄이고, 2 이상이면 1 늘린다. 계산 결과가 부호 있는 32비트로 0 이하이면 0 이 된다(32비트 감김 뒤의 하한).
//  - 번호를 부호 있는 십진수(`%d`)로 `tipNumber` 설정에 쓴다. 인자가 3(OK)이 아니면 이어서 `Tips` 섹션을 다시 열고, OK 는 다음 팁을 미리 저장만 하고 열지 않는다.
//  - 인자가 1 보다 작으면(0 등) 번호를 바꾸지 않고 현재 번호를 그대로 설정에 쓴 뒤 Tips 를 다시 연다.
//  - 뗌(release) 입력이나 다른 명령 번호는 처리하지 않는다.
// 원본은 번호를 별도 32비트 전역으로 갖고 설정과 같이 쓴다. 웹 판본은 호출자가 설정의 현재 값을 넘기고 돌려받은 값이 같은 설정에 쓰인다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다. (2026-10-30 WB03-J: 시작 팁 창을 위해 추가)

// 팁 명령의 입력 코드이다(보호 PE 00542aa0 의 TellTip 명령값). (2026-10-30 WB03-J)
export const TELL_TIP_COMMAND = 0x020000eb;

// 입력 코드의 명령 부분을 고르는 마스크이다(위 두 비트는 입력 종류·뗌 표시). (2026-10-30 WB03-J)
const COMMAND_MASK = 0x03ffffff;

// 뗌 입력 표시 비트이다. (2026-10-30 WB03-J)
export const INPUT_RELEASE = 0x80000000;

// 팁 번호를 부호 있는 32비트 정수로 본 값이다. (2026-10-30 WB03-J)
function signed(value) {
    return value | 0;
}

// 팁 명령을 처리한다. event 는 { code, y }(code 는 부호 없는 32비트 입력 코드, y 는 버튼의 정수 인자), number 는 부호 없는 32비트 현재 팁 번호이다.
// hooks 는 { format(부호 있는 번호) → 십진 문자열, setConfig(키, 값), tell(섹션 이름) } 이며 실제로 닿는 단계에서만 쓴다(없는데 필요하면 Error).
// 반환: { handled, number }. handled 는 이 명령을 소비했는지, number 는 갱신된 팁 번호(부호 없는 32비트)이다. 훅이 예외를 던지면 갱신한 번호를 돌려주지 못한다
// (원본은 번호를 전역에 바로 써서 남기지만, 웹 호출자는 번호를 항상 `tipNumber` 설정에서 다시 읽는다).
// 사용: dispatchTellTip({ code: TELL_TIP_COMMAND, y: 2 }, 5, { format: String, setConfig: (k, v) => data.set(k, v), tell: openTell }) → { handled: true, number: 6 }. (2026-10-30 WB03-J)
export function dispatchTellTip(event, number, hooks = {}) {
    let current = number >>> 0;
    if (((event.code >>> 0) & COMMAND_MASK) !== TELL_TIP_COMMAND) return { handled: false, number: current };
    if (((event.code >>> 0) & INPUT_RELEASE) !== 0) return { handled: true, number: current };
    const parameter = event.y | 0;
    if (parameter === 1) {
        const value = (current - 1) >>> 0;
        current = signed(value) <= 0 ? 0 : value;
    } else if (parameter >= 2) {
        const value = (current + 1) >>> 0;
        current = signed(value) <= 0 ? 0 : value;
    }
    // 실제로 닿는 단계의 장치만 요구하는 보조 함수이다. 연결되지 않았으면 Error 를 던지고, 있으면 그 장치를 돌려준다.
    const need = (hook) => {
        if (!hook) throw new Error('TellTip 의 선택된 실제 장치가 연결되지 않았습니다.');
        return hook;
    };
    const text = need(hooks.format)(signed(current));
    need(hooks.setConfig)('tipNumber', text);
    if (parameter !== 3) need(hooks.tell)('Tips');
    return { handled: true, number: current };
}
