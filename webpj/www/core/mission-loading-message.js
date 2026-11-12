// 미션 시작 진행 안내("Starting Mission...")의 본문 조립과 현재 transient HTML 부모 재사용을 옮긴 모듈이다(cpppj `client/MissionLoadingMessage.cpp/.h`, 10.78, 원본 cf570).
//  - buildMissionLoadingMessageBody(formatted, cancelText): 이미 확장 printf 를 거친 C 문자열(첫 NUL 앞)에 줄바꿈과 기본 Cancel 단추 메타만 붙인다. 본문이 비었거나 마지막이 `\n` 이 아니면 `\n` 을 붙이고,
//    본문이 `~.` 로 시작하지 않고 대소문자 구별 `$Button` 부분 문자열도 없을 때만 `$ButtonInstant=` + 번역된 Cancel(첫 NUL 앞) + `,LeaveBattle,3` 을 붙인다. Cancel 번역은 이 분기에서만 조회한다.
//    원본 임시 버퍼는 NUL 포함 1024BYTE 라 본문(및 붙인 뒤)이 1024BYTE 이상이면 호스트 예외(`RangeError`)로 거부한다. 임의 스타일/따옴표 escape 는 만들지 않는다.
//  - showMissionLoadingMessage(state, formatted, hooks): 본문 조립 → 현재 transient 검색(`findSerial(state.transientSerial)`) → 있으면 기존 부모 갱신(깊이 +1 → 메타 제거 → 본문 갱신 → 깊이 −1, 깊이는 효과 뒤 현재 값을 다시 조회)
//    하고 `state.transientSerial = serial(부모)`; 없으면 `create(본문, 0, 400, 30)` 으로 새로 만들고(create 가 `state.createdSerial` 을 바꿀 수 있음) 그 뒤 **현재** `state.createdSerial` 의 부모가 있으면 같은 갱신을 한 번 더 한 다음
//    `state.transientSerial` 에 create 의 반환 serial 을 쓴다(둘을 서로 바꾸지 않는다). 반환은 최종 `state.transientSerial`(신규 경로는 create 반환) 이다.
//  - state = { transientSerial(005cab34), createdSerial(005cab38) } 를 속성으로 읽고 쓰는 객체이다. 실제 Client 의 비연속 전역을 빌릴 때는 같은 이름의 접근자 속성(getter/setter)을 가진 객체를 넘기면 값 사본 없이 현재 값을 읽고 쓴다.
//  - hooks = { cancelText() → 문자열, findSerial(serial) → 토큰(없으면 0), updateDepth(token) → `{ value }` 상자(부모 +21bc 의 살아 있는 DWORD), serial(token) → 현재 +2c, metadata(token, box `{ text }`) → 004c83b0 이 box.text 에서
//    메타를 지움, body(token, 문자열) → 004c7aa0 본문 갱신, create(본문, 0, 400, 30) → 새 serial }. 선택된 경로에 필요한 효과만 요구하며 누락은 성공으로 바꾸지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
//  글자 하나가 한 바이트인 원문 바이트 문자열을 다룬다(UTF-16 번역은 호출자가 표시 부호화 경계에서 바이트로 바꿔 넘긴다). (2026-11-12 WB04-E339: 4-D B 구획 MissionLoadingMessage 이식)

// cf570 이 요청하는 논리 임시 버퍼 크기이며 끝 NUL 까지 포함한다. (2026-11-12 WB04-E339)
const TEMPORARY_BYTES = 1024;
// 기본 Cancel 단추 메타의 앞/뒤 원문이다(PE 00515ee0/00515ed0). (2026-11-12 WB04-E339)
const BUTTON_PREFIX = '$ButtonInstant=';
const BUTTON_SUFFIX = ',LeaveBattle,3';
// 새 부모(cf270)에 전달하는 요청 크기이다. 최종 Fit 결과 크기와 구별한다. (2026-11-12 WB04-E339)
const REQUESTED_WIDTH = 400;
const REQUESTED_HEIGHT = 30;

// 실행 경로가 실제로 선택한 외부 효과만 요구하며 미연결을 정상 성공으로 대체하지 않는다. (2026-11-12 WB04-E339)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('진행 안내 HTML 실제 처리기 미연결');
    return effect;
}

// C 문자열 의미를 보존하되 임시 버퍼 밖의 원본 미정 쓰기는 재현하지 않는다. (2026-11-12 WB04-E339)
function checkSize(text) {
    if (text.length >= TEMPORARY_BYTES) throw new RangeError('진행 안내1024BYTE 임시 버퍼 범위 초과');
}

// 첫 NUL 앞까지의 C 문자열 부분이다. (2026-11-12 WB04-E339)
function visibleText(text) {
    const nul = text.indexOf('\0');
    return nul < 0 ? text : text.slice(0, nul);
}

// 기존 부모의 갱신 깊이는 가상 효과 뒤 현재 값을 다시 읽어 감소시킨다. 메타가 바꾼 같은 C 문자열의 첫 NUL 앞만 본문에 전달한다. (2026-11-12 WB04-E339)
function update(token, source, hooks) {
    const before = need(hooks.updateDepth)(token);
    before.value = (before.value + 1) >>> 0;
    need(hooks.metadata)(token, source);
    need(hooks.body)(token, visibleText(source.text));
    const after = need(hooks.updateDepth)(token);
    after.value = (after.value - 1) >>> 0;
}

// 원본은 첫 `~.` 시작 또는 대소문자 구별 `$Button` 부분 문자열이 있으면 기본 버튼을 추가하지 않는다. (2026-11-12 WB04-E339)
export function buildMissionLoadingMessageBody(formatted, cancelText) {
    const first = visibleText(String(formatted));
    checkSize(first);
    let body = first;
    if (body === '' || !body.endsWith('\n')) body += '\n';
    checkSize(body);
    if (!body.startsWith('~.') && !body.includes('$Button')) {
        body += BUTTON_PREFIX;
        // 번역의 첫 NUL 뒤 바이트는 복사하지 않는다.
        body += visibleText(String(need(cancelText)()));
        body += BUTTON_SUFFIX;
        checkSize(body);
    }
    return body;
}

// 본문 조립 → 현재 transient 검색 → 기존 갱신 또는 새 생성/현재 created 검색 → 현재 serial 저장 순서이다. (2026-11-12 WB04-E339)
export function showMissionLoadingMessage(state, formatted, hooks) {
    const source = { text: buildMissionLoadingMessageBody(formatted, hooks?.cancelText) };
    const existing = need(hooks.findSerial)(state.transientSerial);
    if (existing) {
        update(existing, source, hooks);
        state.transientSerial = need(hooks.serial)(existing);
        return state.transientSerial;
    }
    const created = need(hooks.create)(source.text, 0, REQUESTED_WIDTH, REQUESTED_HEIGHT);
    // create 가 바꾼 현재 createdSerial 을 다시 읽어 부모가 있으면 같은 갱신을 한 번 더 한다.
    const current = need(hooks.findSerial)(state.createdSerial);
    if (current) update(current, source, hooks);
    state.transientSerial = created;
    return created;
}
