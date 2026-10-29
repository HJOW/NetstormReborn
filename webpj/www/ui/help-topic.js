// 일반 도움말(F1) 창의 주제 본문을 만드는 모듈이다. 도움말 파일(`help.<언어>`) 전체에서 주제 이름(`<a name="이름">` 앵커)에 맞는 구간만 골라 StyleText 문자열로 바꾼다.
// 원본 흐름(QuickHelpHtml 파생 파서 + QuickHelpWindow::Show): 파서의 기본 표시를 꺼 두고 태그 안에서 `name` 속성을 찾아 이름 앞부분이 주제와 같으면(대소문자 무시) 표시를 켠 뒤
// 본문 시작에 `~5~w`(글꼴 슬롯 5 + 흰색)를 넣는다. 그 구간의 앵커를 닫는 `</a>` 에서 평문 처리가 멈춘다(textAllowed 가 0). 이어 설정 치환을 하고 비면 안내 문구를 쓴다.
// 링크(`<a href>`)는 LinkTable 에 등록해 `~[T번호]` 태그로, 조건(`<?{키}>`)은 설정 값으로, 그림(`<!"이름.프레임">`)은 `~[I이름.프레임]` 로 바꾼다.
// 지원하지 않는 부분: 타입 정보 머리글(`<info>`, 타입 그림·능력치)은 TYPE 자료가 아직 없어 건너뛴다(주제 본문만 나온다).
// 원본 주소: 파서 Inside/After 는 cpppj QuickHelpHtml.cpp, 본문 선택은 QuickHelpWindow.cpp(Show), 파일 읽기는 RawMenuControls.cpp(RenderHelp). 복원 기준: cpppj(10.78 경로).
// (2026-10-30 WB03-G: 메인 메뉴 Help → General Help 창을 위해 추가)
import { LinkTable, applyHtmlCondition, createParserGlobals, createParserState, htmlPictureText, parseHtml } from './html-text.js';

// 도움말 본문 변환 결과 최대 길이(바이트)이다(원본 HtmlBodyGump 본문 배열 8192). 평문이 이 한도 - 256 을 넘으면 변환을 멈춘다. (2026-10-30 WB03-G)
export const HELP_BODY_CAPACITY = 8192;

// 도움말 파일이 없거나 주제 구간이 비었을 때 보여 주는 문구이다(원본 번역 키). (2026-10-30 WB03-G)
export const NO_HELP_TEXT = 'No help information available';

// 태그 안의 글자 하나를 만날 때마다 불리는 훅이다(원본 QuickHelpHtml::Inside). 현재 글자가 `name` 이면 첫 따옴표 뒤 이름이 주제로 시작하는지 보고, 맞으면 표시를 켜고 `~5~w` 를 출력에 넣는다.
// 현재 읽는 위치는 그대로 돌려 두므로(원본: 진입 위치 복원) 이어서 태그의 나머지 글자도 같은 훅을 지난다. (2026-10-30 WB03-G)
function insideTag(state, source, topic) {
    const entry = state.inputOffset;
    let position = entry;
    if (position < source.length && source[position] === 'n' && source.slice(position, position + 4).toLowerCase() === 'name') {
        // 첫 따옴표(또는 끝)까지 가서 따옴표 하나를 건너뛴다. '=' 나 공백은 따로 해석하지 않는다.
        while (position < source.length && source[position] !== '"') position += 1;
        if (position < source.length) position += 1;
        if (source.slice(position, position + topic.length).toLowerCase() === topic.toLowerCase()) {
            state.shown = 1;
            state.textAllowed = 1;
            if (state.output.length + 4 >= HELP_BODY_CAPACITY) throw new RangeError('도움말 name 스타일 출력 범위를 넘었습니다.');
            state.output += '~5~w';
        }
    }
    state.inputOffset = entry;
}

// 닫는 태그를 처리하기 직전의 훅이다(원본 QuickHelpBeforeClose). 현재 위치 바로 앞 글자가 줄바꿈이면 평문 허용 수를 0 으로 만든다. (2026-10-30 WB03-G)
function beforeClose(state, source) {
    if (!state.inputOffset || state.inputOffset > source.length) throw new RangeError('도움말 닫힘 훅의 현재 입력 위치가 범위 밖입니다.');
    const previous = source[state.inputOffset - 1];
    if (previous === '\n' || previous === '\r') state.textAllowed = 0;
}

// 도움말 주제 하나를 StyleText 문자열로 만든다. helpText 는 도움말 파일 전체 문자열, topic 은 주제 이름(앵커 접두), substitute 는 설정 치환 함수(`{키}` 해석 + 조건 값)이다.
// 반환: { styleText(설정 치환을 마친 StyleText), links(LinkTable: `~[T번호]` 의 목적지), found(주제 앵커를 찾았는지) }.
// 앵커를 못 찾으면 안내 문구(NO_HELP_TEXT)를 본문으로 쓴다. 사용: const result = renderHelpTopic({ helpText, topic: 'F1Help', substitute: (text) => data.substitute(text) }).
// (2026-10-30 WB03-G)
export function renderHelpTopic({ helpText, topic, substitute, globals = createParserGlobals() }) {
    const links = new LinkTable();
    const state = createParserState();
    // 주제 앵커를 찾기 전까지는 아무것도 표시하지 않는다(원본 파생 생성자가 기본 표시 1 을 0 으로 바꿈).
    state.shown = 0;
    const hooks = {
        link: (_state, raw, closing) => links.text(raw, closing),
        condition: (parserState, raw, closing) => applyHtmlCondition(parserState, raw, closing, substitute),
        picture: (_state, raw) => htmlPictureText(raw),
        insideTag: (parserState, source) => insideTag(parserState, source, topic),
        beforeClose,
    };
    // 도움말 파일 끝에 줄바꿈을 하나 붙여 마지막 앵커가 끝까지 닿게 한다(원본이 불러온 뒤 push_back('\n')).
    const parsed = parseHtml(`${helpText}\n`, state, globals, hooks, HELP_BODY_CAPACITY);
    const found = state.shown !== 0 || parsed.length > 1;
    let styleText = substitute(parsed);
    const nul = styleText.indexOf('\0');
    if (nul >= 0) styleText = styleText.slice(0, nul);
    if (styleText.length === 0) styleText = NO_HELP_TEXT;
    return { styleText, links, found };
}
