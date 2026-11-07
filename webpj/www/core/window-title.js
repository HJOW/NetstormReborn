// 창 제목 선택(PATCH 00460730)을 현재 설정·미션·창에 연결하는 모듈이다(cpppj `client/WindowTitle.cpp/.h`, 10.78 전용). 웹에서는 최종 제목을 document.title 같은 소비자에게 setTitle 로 넘긴다.
// refreshWindowTitle(state, hooks): 원본 순서대로 설정을 조회하고 마지막에 한 번 창 제목을 바꾼다.
//  - 요새 이름이 null 이거나 빈 문자열이면 pathSpec('blankWindowName', null). 편집 모드가 미션보다 우선: pathSpec('EditorWindowName', 요새 이름).
//  - 미션이 아니면 일반 요새 제목: 현재 Template 의 +0 과 +8 이 모두 비영(templatePresent && templateReadOnly)일 때만 pathSpec('readonly', null) + ' ' 를 앞에 붙이고, 이어서 pathSpec('windowName', 요새 이름).
//  - 미션이면 demo > testBattle > 일반 순서로 키('demoWindowName'/'testBattleWindowName'/'missionWindowName')를 고르고, 설정 치환보다 먼저 현재 미션 title(missionTitle())을 읽어(파일 이름으로 대신하지 않음) pathSpec(키, 미션 title).
//  pathSpec 의 첫 치환 인자는 null 과 빈 문자열을 구분한다. 완성된 제목이 256바이트 이상이거나 NUL 을 포함하면 원본 버퍼 범위라 RangeError 이다.
// state = { fortName(string|null), editor, mission, templatePresent, templateReadOnly, demo, testBattle }, hooks = { pathSpec(키, 인자|null) → 문자열, missionTitle(), setTitle(제목) } 이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E34: 창 제목 이식으로 추가)

// 원본 제목 버퍼 크기(0 으로 채운 256바이트 임시 버퍼)이다. (2026-11-08 WB04-E34)
const TITLE_BUFFER = 256;

// state 와 hooks 로 제목을 정해 한 번 적용한다. 소비자가 하나라도 없으면 거부한다. (2026-11-08 WB04-E34)
export function refreshWindowTitle(state, hooks) {
    if (typeof hooks?.pathSpec !== 'function' || typeof hooks.missionTitle !== 'function' || typeof hooks.setTitle !== 'function') throw new TypeError('창 제목의 실제 설정/미션/창 소비자 누락');
    let title;
    if (state.fortName === null || state.fortName === undefined || state.fortName === '') title = hooks.pathSpec('blankWindowName', null);
    else if (state.editor) title = hooks.pathSpec('EditorWindowName', state.fortName);
    else if (!state.mission) {
        title = '';
        if (state.templatePresent && state.templateReadOnly) title = `${hooks.pathSpec('readonly', null)} `;
        title += hooks.pathSpec('windowName', state.fortName);
    } else {
        // 실제 미션 두 플래그의 우선순위이다.
        const key = state.demo ? 'demoWindowName' : state.testBattle ? 'testBattleWindowName' : 'missionWindowName';
        const mission = hooks.missionTitle();
        title = hooks.pathSpec(key, mission);
    }
    if (title.length >= TITLE_BUFFER || title.includes('\0')) throw new RangeError('원본 창 제목의 NUL/256바이트 물리 범위 초과');
    hooks.setTitle(title);
}
