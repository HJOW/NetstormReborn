// HTML $Input 의 확인은 입력을 다음 Tell 섹션의 인자로 전달한다. 설정 변경과 구별한다.
// (cpppj client/HtmlInputDialog.cpp/.h, 기준선 1f00a25a, 10.78 전용, 원문 004cfae0)
// 문자 편집/공통 모달은 기존 실제 처리기가 담당한다. 공백 제거처럼 보이는 원문의 NUL 검사도 그대로 두고 숫자 입력을 재서식하지 않는다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E435: RawMenuControls 선행 HTML 입력 확인 이식)

// 원문 고정 256바이트 입력 버퍼 크기다.
const TEXT_BYTES = 256;
// 도달한 실제 하위만 요구한다(부분 효과를 성공으로 감추지 않는다).
function required(hook) { if (typeof hook !== 'function') throw new Error('HTML 입력 확인의 실제 장치 미연결'); return hook; }

// 원문 CopyText→강제 NUL→첫 LF→NUL 부터 공백 검사→선택 atol→서식/Tell 순서다. mode 는 현재 값을 읽는 상자({ value })이고 continuation 은 고정 80바이트 후속 섹션(바이트 문자열 또는 Uint8Array)이다.
// hooks = { copyText(Uint8Array(256) 출력), space(int8) → 0/비0, parseLong(문자열), format(후속 섹션, 입력) → 문자열, tell(섹션) }. 첫 LF 에서 자르고 현재 mode/후속 섹션을 읽은 뒤 Tell 하고 1 을 반환한다.
// 원문은 마지막 문자가 아닌 종료 NUL 에서 공백 검사를 시작하므로 정상 CRT 에서는 공백을 지우지 않는다. 사용: confirmHtmlInput({ value: 2 }, '$Next', hooks)
export function confirmHtmlInput(mode, continuation, hooks) {
    const text = new Uint8Array(TEXT_BYTES); required(hooks.copyText)(text); text[TEXT_BYTES - 1] = 0;
    let end = text.indexOf(0); const newline = text.subarray(0, end).indexOf(0x0a);
    if (newline >= 0) { text[newline] = 0; end = newline; }
    // 원문은 마지막 문자가 아닌 종료 NUL 에서 시작해 앞쪽으로 공백을 지운다.
    while (end !== 0) {
        if (!required(hooks.space)((text[end] << 24) >> 24)) break;
        text[end] = 0; end -= 1;
    }
    const nul = text.indexOf(0); let input = ''; for (let index = 0; index < nul; index += 1) input += String.fromCharCode(text[index]);
    if (mode.value !== 1) required(hooks.parseLong)(input);
    let section = ''; let found = false;
    // 후속 섹션은 NUL 앞까지(없으면 거부)다.
    for (let index = 0; index < continuation.length; index += 1) {
        const code = typeof continuation === 'string' ? continuation.charCodeAt(index) : continuation[index];
        if (code === 0) { found = true; break; } section += String.fromCharCode(code);
    }
    if (!found) throw new RangeError('HTML 입력 후속 섹션의 80바이트 NUL 범위');
    required(hooks.tell)(required(hooks.format)(section, input)); return 1;
}
