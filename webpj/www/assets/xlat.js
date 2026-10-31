// 원본 UI 문자열 번역표(`d/xlat.<언어>`)를 읽고 조회하는 모듈이다.
// 원본 Xlat.cpp(004de260 읽기, 004de1e0 조회, 004de9a0 번역 함수)의 규칙을 cpppj o/Xlat.cpp 와 같게 옮겼다. 형식은 docs/formats/xlat.md 를 따른다.
//  - 파일 전체에서 CR 을 지우고 줄의 첫 글자만 본다: `*` 줄 다음부터 원문, `-` 줄 앞까지가 원문이고 그 다음부터 번역문, `=` 줄 앞까지가 번역문이다.
//  - 원문·번역문 끝의 줄바꿈 하나는 뗀다. 같은 원문이 다시 나오면 번역문을 덮어쓰고, 끝나지 않은 마지막 블록은 버린다.
//  - 조회는 대소문자를 구분하는 완전 일치이며, 번역이 없으면 원문을 그대로 돌려준다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다. (2026-10-31 WB03-N: 한국어 메인 메뉴·펼침 메뉴 라벨 번역을 위해 추가)

// 번역표 하나이다. 원문 문자열이 키, 번역문이 값이다.
// 사용: const table = new XlatTable(decodeOriginalText(bytes)); table.translate('Campaign') → '캠페인', table.size → 고유 원문 수. (2026-10-31 WB03-N)
export class XlatTable {
    // 번역 파일 전체 문자열로 표를 만든다. 형식이 맞지 않는 줄은 상태가 맞을 때까지 무시한다.
    constructor(text) {
        // 원문 → 번역문 표이다.
        this.entries = new Map();
        const source = String(text ?? '').replaceAll('\r', '');
        // 현재 읽고 있는 블록의 상태이다: 0 = `*` 를 기다림, 1 = 원문 읽는 중, 2 = 번역문 읽는 중.
        let state = 0;
        let originalStart = 0;
        let translatedStart = 0;
        let original = '';
        // 줄 단위로 훑으며 줄의 첫 글자만 구분 문자로 본다. 마지막 줄바꿈이 없어도 완성된 구분 줄은 처리한다.
        for (let position = 0; position < source.length;) {
            const newline = source.indexOf('\n', position);
            const next = newline < 0 ? source.length : newline + 1;
            const marker = source[position];
            if (state === 0 && marker === '*') {
                originalStart = next;
                state = 1;
            } else if (state === 1 && marker === '-') {
                original = source.slice(originalStart, trimmedEnd(source, position, originalStart));
                translatedStart = next;
                state = 2;
            } else if (state === 2 && marker === '=') {
                this.entries.set(original, source.slice(translatedStart, trimmedEnd(source, position, translatedStart)));
                state = 0;
            }
            position = next;
        }
    }

    // 중복 원문을 제외한 번역 항목 수이다.
    get size() {
        return this.entries.size;
    }

    // 원문에 대응하는 번역문을 돌려준다. 번역이 없으면 원문 그대로다(원본 004de9a0).
    translate(original) {
        return this.entries.get(original) ?? original;
    }

    // 번역이 있는 원문인지 검사한다. 번역표 누락을 검사에서 찾을 때 쓴다.
    has(original) {
        return this.entries.has(original);
    }
}

// 구분 줄 위치(position) 앞에서 줄바꿈 하나를 뗀 끝 위치를 구한다. 구간(start)보다 앞으로는 가지 않는다. (2026-10-31 WB03-N)
function trimmedEnd(source, position, start) {
    return position > start && source[position - 1] === '\n' ? position - 1 : position;
}
