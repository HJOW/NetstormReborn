// 원본 StyleText(스타일 문자열 조각 체인)를 복원하는 모듈이다.
// 스타일 문자열은 `~` 로 시작하는 짧은 명령(`~w` 색, `~3` 글꼴 슬롯, `~B` 굵게, `~E` 그림자, `~.` 기본 문맥 복귀 등)과
// 대괄호 명령(`~[T.5]` 태그, `~[C.3]` 색, `~[I...]` 그림)이 섞인 문자열이며, 명령마다 글자 문맥이 바뀐 조각들로 나뉜다.
// 원본 주소: 파서 004bb430(StyleText::Parse), 짧은 명령 표, 측정 00475760, 그리기 004755e0, 조각 생성 004b93c0.
// 복원 기준: cpppj StyleText.cpp·StyleTextMarkup.cpp(10.78 경로). 그림(`~[I...]`)은 options.resolvePicture 가 해석해 주면 그림 조각이 되고(2026-10-30 WB03-G), 해석하지 못하거나 소리(`~[S...]`)는 소비만 한다.
// 조각 체인은 원본의 연결 목록 대신 JS 배열로 표현한다(순회 순서와 문맥 값은 같다).
// (2026-10-28 WB03-D: Tell 대화상자 본문·메뉴 항목의 스타일 글자를 그리기 위해 추가)
import { createTextContext, drawGlyphText } from '../render/glyph-text.js';
import { encodeWindows1252 } from '../assets/text-encoding.js';
import { findPaletteColor } from '../assets/palette.js';

// 기본 색 이름(명령 글자)과 기본 색 표의 번호이다. 원본 기본 검정·빨강·초록·파랑·자홍·노랑·주황이 앞 7칸이다. (2026-10-28 WB03-D)
const BASIC_COLOR_COMMANDS = Object.freeze({ k: 0, r: 1, g: 2, b: 3, m: 4, y: 5, o: 6 });

// 기본 색 표(원본 팔레트 로더가 RGB 로 검색해 전역에 저장하는 값)의 목표 RGB 이다. 7번이 흰색, 8번이 회색이다. (2026-10-28 WB03-D)
const BASIC_COLOR_RGB = Object.freeze([
    [0, 0, 0], [255, 22, 22], [22, 255, 22], [22, 22, 255], [255, 22, 255], [255, 255, 22], [240, 61, 0], [255, 255, 255], [128, 128, 128],
]);

// 기본 색 밖의 명령 색이다. 명령 글자 → 목표 RGB 이며 원본이 팔레트에서 가장 가까운 번호를 찾아 쓴다. (2026-10-28 WB03-D)
const NAMED_COMMAND_RGB = Object.freeze({
    a: [255, 153, 0], d: [0, 255, 102], i: [249, 233, 145], j: [233, 214, 186], l: [151, 215, 210], p: [255, 102, 204],
    q: [200, 175, 144], s: [99, 66, 255], t: [219, 213, 203], u: [51, 255, 102], x: [47, 24, 17],
});

// 10.78 에서 색으로 쓸 수 있는 명령 글자 전체이다(공통 + 패치 추가). `n`(기본 색 복귀)도 포함하지만 색 조회 전에 따로 처리한다. (2026-10-28 WB03-D)
const COLOR_COMMAND_CHARS = 'bgijklmnoqrtwxyadpsu';

// 설명 본문(HTML 본문·도움말)의 기본 글자색 RGB 이다(원본 전역 005b5dc8). Tell 본문의 기본 색이 된다. (2026-10-28 WB03-D)
const INFORMATION_RGB = Object.freeze([219, 213, 203]);

// 비활성 메뉴 항목·메뉴 기본 그림자에 쓰는 어두운 갈색 RGB 이다(원본 전역 005c78d0). (2026-10-28 WB03-D)
const DIM_RGB = Object.freeze([47, 24, 17]);

// 원본 팔레트에서 UI 글자색 번호를 구해 두는 표이다. 짧은 명령의 색 조회와 메뉴·본문 기본 색에 쓴다.
// 사용: const colors = new StyleColors(palette.logical); colors.command('w') → 흰색 번호. (2026-10-28 WB03-D)
export class StyleColors {
    // 논리 팔레트(R | G<<8 | B<<16 DWORD 256개)에서 모든 색 번호를 한 번에 찾는다.
    constructor(logical) {
        // 기본 색 표(검정·빨강·초록·파랑·자홍·노랑·주황·흰색·회색)의 색 번호이다.
        this.basic = Object.freeze(BASIC_COLOR_RGB.map(([red, green, blue]) => findPaletteColor(logical, red, green, blue)));
        // 명령 글자 → 색 번호 표(기본 색 제외)이다.
        this.named = Object.freeze(Object.fromEntries(Object.entries(NAMED_COMMAND_RGB).map(([command, [red, green, blue]]) => [command, findPaletteColor(logical, red, green, blue)])));
        // 흰색 번호(기본 색 7번)다.
        this.white = this.basic[7];
        // 검정 번호(기본 색 0번)다. `~E` 그림자 색이다.
        this.black = this.basic[0];
        // 설명 본문 기본 글자색 번호다.
        this.information = findPaletteColor(logical, INFORMATION_RGB[0], INFORMATION_RGB[1], INFORMATION_RGB[2]);
        // 비활성 항목·메뉴 기본 그림자 색 번호다.
        this.dim = findPaletteColor(logical, DIM_RGB[0], DIM_RGB[1], DIM_RGB[2]);
    }

    // 색 명령 글자의 색 번호를 돌려준다. 색 명령이 아니면 undefined 이다(원본은 범위 오류).
    command(letter) {
        if (letter === 'w') return this.white;
        if (letter in BASIC_COLOR_COMMANDS) return this.basic[BASIC_COLOR_COMMANDS[letter]];
        return this.named[letter];
    }
}

// 짧은 스타일 명령 한 글자를 문맥에 적용한다(원본 StyleText::ApplyShortCommand, 10.78 경로). 처리했으면 true, 모르는 명령이면 false 이다.
//  `*` 효과 플래그 지움, `.` 기본 문맥 복귀, `0 1 3 4 5 6` 글꼴 슬롯, `B` 굵게, `I` 기울임, `S` 취소선, `U` 밑줄, `N` 보통(그림자 끔),
//  `E` 검정 그림자(1,1) 켬, `n` 기본 색 복귀, 그 밖의 색 글자는 색 번호를 바꾼다.
// context 는 바뀌는 가변 객체이고, defaultContext 는 `.`·`n` 이 돌아갈 기본 문맥이다. restricted 가 참이면 슬롯 0·1·3·4 변경을 소비만 한다.
// 사용: applyShortCommand('w', context, defaultContext, colors). (2026-10-28 WB03-D)
export function applyShortCommand(command, context, defaultContext, colors, restricted = false) {
    switch (command) {
        case '*':
            context.flags = 0;
            return true;
        case '.':
            Object.assign(context, defaultContext);
            return true;
        case '0': case '1': case '3': case '4':
            if (!restricted) context.font = Number(command);
            return true;
        case '5': case '6':
            context.font = Number(command);
            return true;
        case 'B':
            context.style = 2;
            return true;
        case 'I':
            context.style = 1;
            return true;
        case 'N':
            context.flags &= ~2;
            context.style = 0;
            return true;
        case 'E':
            context.flags |= 2;
            context.shadowColor = colors.black;
            context.shadowX = 1;
            context.shadowY = 1;
            return true;
        case 'S':
            context.style = 3;
            return true;
        case 'U':
            context.style = 4;
            return true;
        case 'n':
            context.color = defaultContext.color;
            return true;
        default:
            if (command.length !== 1 || !COLOR_COMMAND_CHARS.includes(command)) return false;
            context.color = colors.command(command);
            return true;
    }
}

// 글자 조각 하나다. bytes 는 Windows-1252 바이트, context 는 그 조각의 글자 문맥(고정), tag 는 링크 태그 번호(0 = 없음)이다.
// (2026-10-28 WB03-D)
export function createFragment(bytes, context, tag = 0) {
    return Object.freeze({ bytes, context: createTextContext(context), tag });
}

// 그림 조각 하나다(원본 StyleText 의 그림 외부 조각). picture 는 { image, width, height, hotspotX, hotspotY } 이며 글자 바이트는 비어 있다.
// 측정은 (폭, 높이), 기준선 위쪽 높이는 기준점 y 이고 그릴 때는 기준점 + 그림의 상대 위치(image.left/top)에 그린다. (2026-10-30 WB03-G)
export function createPictureFragment(picture, context, tag = 0) {
    return Object.freeze({ bytes: new Uint8Array(0), context: createTextContext(context), tag, picture });
}

// 조각의 기준선 위쪽 높이와 높이를 돌려준다(글자는 글꼴 값, 그림은 기준점 y 와 높이). (2026-10-30 WB03-G)
function fragmentMetrics(fonts, fragment) {
    return fragment.picture ? { ascent: fragment.picture.hotspotY, height: fragment.picture.height } : fonts.metrics(fragment.context);
}

// 첫 NUL 앞까지의 문자열만 쓴다(원본 문자열 끝). JS 문자열에 NUL 이 있을 때만 의미가 있다. (2026-10-28 WB03-D)
function cutAtNul(text) {
    const nul = text.indexOf('\0');
    return nul < 0 ? text : text.slice(0, nul);
}

// 대괄호 명령 안쪽 문자열에서 공백(스페이스)만 건너뛴다. 원본 정규식의 ' *' 이다. (2026-10-28 WB03-D)
function skipSpaces(text, position) {
    let at = position;
    while (at < text.length && text[at] === ' ') at += 1;
    return at;
}

// 원본 `[^.\r\n ]` 에 해당하는 글자인지 검사한다. 캡처(토큰)에 들어갈 수 있는 글자다. (2026-10-28 WB03-D)
function isTokenChar(char) {
    return char !== '.' && char !== '\r' && char !== '\n' && char !== ' ';
}

// 토큰의 가장 긴 끝 위치를 찾는다. (2026-10-28 WB03-D)
function tokenEnd(text, position) {
    let at = position;
    while (at < text.length && isTokenChar(text[at])) at += 1;
    return at;
}

// 원본 ' *. *([^.\r\n ]+)' 형식(구분 글자 하나 + 토큰)에서 토큰을 찾는다. 점은 이스케이프되지 않아 임의 글자 하나를 소비한다.
// 앞 공백은 가장 긴 끝부터 하나씩 줄여 가며 성립하는 첫 조합을 고른다. 없으면 null 이다. (2026-10-28 WB03-D)
function separatedToken(text, position) {
    let separator = skipSpaces(text, position);
    // 앞 공백의 끝을 되돌려 가며 구분 글자 뒤 토큰이 성립하는지 본다.
    for (;;) {
        if (separator < text.length) {
            const begin = skipSpaces(text, separator + 1);
            const end = tokenEnd(text, begin);
            if (end > begin) return text.slice(begin, end);
        }
        if (separator === position) return null;
        separator -= 1;
    }
}

// 대괄호 명령 한 종류의 형식을 부분 검색한다(원본 StyleTextMarkup Match). 명령 글자가 처음 나오는 위치부터 형식이 성립하는 첫 곳을 찾는다.
// T 는 숫자 하나, I·E·W 는 토큰 두 개, 그 밖(S·C·B)은 토큰 하나다. 성립하면 { first, second } 를, 아니면 null 을 돌려준다. (2026-10-28 WB03-D)
function matchBracket(text, command) {
    // 명령 글자가 나오는 각 위치를 앞에서부터 시도한다.
    for (let mark = 0; mark < text.length; mark += 1) {
        if (text[mark] !== command) continue;
        const begin = skipSpaces(text, mark + 1);
        if (command === 'T') {
            let end = begin;
            while (end < text.length && text[end] >= '0' && text[end] <= '9') end += 1;
            if (end > begin) return { first: text.slice(begin, end), second: '' };
        } else if (command === 'I' || command === 'E' || command === 'W') {
            let end = tokenEnd(text, begin);
            // 첫 토큰의 가장 긴 길이부터 두 번째 토큰이 성립하는 첫 조합을 고른다.
            while (end > begin) {
                const second = separatedToken(text, end);
                if (second !== null) return { first: text.slice(begin, end), second };
                end -= 1;
            }
        } else {
            const first = separatedToken(text, mark + 1);
            if (first !== null) return { first, second: '' };
        }
    }
    return null;
}

// 원본 CRT 정수 변환(앞 공백·부호·숫자)이다. 숫자가 없으면 0 이다. (2026-10-28 WB03-D)
function parseInteger(text) {
    const matched = /^\s*([+-]?\d+)/.exec(text);
    return matched ? Number.parseInt(matched[1], 10) | 0 : 0;
}

// 대괄호 명령 하나(`~[` 와 `]` 사이 문자열)를 해석한다(원본 StyleTextMarkup::Apply). 10.78 의 검사 순서는 I, S, T, C, B, E 이며
// 처음 형식이 성립한 명령만 처리하고 나머지는 시도하지 않는다. 처리 결과는 { tag }(링크 태그가 바뀐 경우) 로 돌려준다.
// 색(C)·배경색(B)·그림자 위치(E)는 설정(zonefgcolor 등)이 켜져 있을 때만 적용되며 기본은 꺼져 있어 소비만 한다(options 로 켤 수 있다).
// 그림(I)은 options.resolvePicture(이름, 프레임식)가 그림 정보를 돌려주면 result.picture 로 알리고(호출자가 그림 조각을 만듦), 아니면 onUnsupported 로 알린다. 소리(S)는 소비만 한다.
// (2026-10-28 WB03-D, 2026-10-30 WB03-G: 그림 해석 추가)
function applyBracketCommand(inner, context, colors, options) {
    const text = cutAtNul(inner);
    const result = { tag: null, picture: null };
    // 원본 검사 순서대로 형식이 성립하는 첫 명령을 처리한다.
    for (const code of ['I', 'S', 'T', 'C', 'B', 'E']) {
        const matched = matchBracket(text, code);
        if (!matched) continue;
        if (code === 'T') result.tag = parseInteger(matched.first);
        else if (code === 'C' && options.allowForegroundColor) context.color = parseInteger(matched.first) >>> 0;
        else if (code === 'B' && options.allowBackgroundColor) context.shadowColor = parseInteger(matched.first) >>> 0;
        else if (code === 'E' && options.allowOffset) {
            const x = parseInteger(matched.first);
            const y = parseInteger(matched.second);
            // 어느 한 축만 -4..4 안에 있어도 두 값을 함께 적용한다.
            if ((x >= -4 && x <= 4) || (y >= -4 && y <= 4)) {
                context.flags |= 2;
                context.shadowColor = colors.black;
                context.shadowX = x;
                context.shadowY = y;
            }
        } else if (code === 'I') {
            const picture = options.resolvePicture?.(matched.first, matched.second) ?? null;
            if (picture) result.picture = picture;
            else options.onUnsupported?.('그림 `~[I...]`', text);
        }
        return result;
    }
    return result;
}

// 스타일 문자열을 조각 배열로 나눈다(원본 StyleText::Parse). `~` 앞의 글자는 현재 문맥의 조각으로 만들고, 명령은 문맥을 바꾼다.
// `~~` 는 글자 `~` 하나(그때의 문맥), `~[...]` 는 대괄호 명령, 그 밖은 짧은 명령이며 모르는 명령은 `~` 만 버리고 다음 글자를 본문으로 남긴다.
// 마지막 구간은 비어 있어도 조각으로 추가한다. initialContext 는 처음 문맥이자 `~.` 가 돌아갈 기본 문맥이고, 링크 태그 `~[T.n]` 은 이후 조각에 남는다.
// options: { allowForegroundColor, allowBackgroundColor, allowOffset, onUnsupported }. fragments 에 결과를 이어 붙이고 같은 배열을 돌려준다.
// 사용: parseStyleText('~wHello~* world', context, colors). (2026-10-28 WB03-D)
export function parseStyleText(source, initialContext, colors, options = {}, fragments = []) {
    const text = cutAtNul(source);
    const context = { ...initialContext };
    const original = { ...initialContext };
    let tag = 0;
    let position = 0;
    // `~` 명령을 만날 때마다 앞 구간을 조각으로 추가하고 명령을 처리한다. 마지막 구간에서 끝난다.
    for (;;) {
        const mark = text.indexOf('~', position);
        if (mark < 0) {
            fragments.push(createFragment(encodeWindows1252(text.slice(position)), context, tag));
            return fragments;
        }
        if (mark > position) fragments.push(createFragment(encodeWindows1252(text.slice(position, mark)), context, tag));
        const command = text[mark + 1] ?? '';
        let next = mark + 2;
        if (command === '~') {
            // 글자 `~` 자체는 그 시점 문맥의 별도 조각이다.
            fragments.push(createFragment(encodeWindows1252('~'), context, tag));
        } else if (command === '[') {
            const close = text.indexOf(']', next);
            if (close >= 0) {
                const bracket = applyBracketCommand(text.slice(next, close), context, colors, options);
                if (bracket.tag !== null) tag = bracket.tag;
                if (bracket.picture) fragments.push(createPictureFragment(bracket.picture, context, tag));
                next = close + 1;
            }
        } else if (!applyShortCommand(command, context, original, colors)) {
            // 모르는 명령은 `~` 만 버리고 명령 글자를 본문으로 남긴다.
            next = mark + 1;
        }
        // 끝의 `~` 는 NUL 다음을 읽는 대신 마지막 빈 조각으로 끝낸다.
        position = Math.min(next, text.length);
    }
}

// 조각 하나의 폭·높이를 잰다(원본 StyleText::MeasureFragment). (2026-10-28 WB03-D)
export function measureFragment(fonts, fragment) {
    if (fragment.picture) return { width: fragment.picture.width, height: fragment.picture.height };
    return fonts.measure(fragment.bytes, fragment.context);
}

// 조각 체인의 전체 폭·높이를 잰다(원본 StyleText::Measure, 00475760). 첫 순회에서 최대 기준선 위쪽 높이를 구하고,
// 둘째 순회에서 폭은 합, 높이는 (조각 높이 - 조각 기준선 + 최대 기준선)의 최댓값으로 정한다. 빈 체인은 0×0 이다.
// 사용: 메뉴 항목 크기 계산. (2026-10-28 WB03-D)
export function measureChain(fonts, fragments) {
    if (fragments.length === 0) return { width: 0, height: 0 };
    let ascent = 0;
    // 첫 순회: 가장 큰 기준선 위쪽 높이를 찾는다.
    for (const fragment of fragments) ascent = Math.max(ascent, fragmentMetrics(fonts, fragment).ascent);
    let width = 0;
    let height = 0;
    // 둘째 순회: 폭을 더하고 높이를 기준선에 맞춰 최댓값을 구한다.
    for (const fragment of fragments) {
        const size = measureFragment(fonts, fragment);
        const baseline = fragmentMetrics(fonts, fragment).ascent;
        height = Math.max(height, (size.height - baseline + ascent) | 0);
        width = (width + size.width) | 0;
    }
    return { width, height };
}

// 조각 하나를 그린다(원본 DrawFragment). 글자 문맥의 글꼴을 고르고 8비트 표면에 그린다. 글꼴이 없는 조합은 그리지 않는다.
// (2026-10-28 WB03-D)
export function drawFragment(surface, fonts, fragment, x, y, clip) {
    if (fragment.picture) {
        // 그림은 기준점(x + 기준점 x, y + 기준점 y)에 그림의 상대 위치를 더한 곳에 그린다.
        const { image, hotspotX, hotspotY } = fragment.picture;
        surface.drawImage(image, (x + hotspotX + image.left) | 0, (y + hotspotY + image.top) | 0, clip);
        return;
    }
    const font = fonts.drawFont(fragment.context);
    if (font) drawGlyphText(surface, font, fragment.bytes, x, y, fragment.context, clip);
}

// 조각 체인을 한 줄로 그린다(원본 StyleText::Draw, 004755e0). 최대 기준선 위쪽 높이에 맞춰 각 조각의 y 를 내리고,
// 조각마다 측정 폭만큼 x 를 옮긴다. 반환값은 첫 순회에서 구한 최대 캐시 높이다.
// 사용: 메뉴 항목 글자 그리기. (2026-10-28 WB03-D)
export function drawChain(surface, fonts, fragments, x, y, clip) {
    if (fragments.length === 0) return 0;
    let height = 0;
    let ascent = 0;
    // 첫 순회: 가장 큰 캐시 높이와 기준선 위쪽 높이를 구한다.
    for (const fragment of fragments) {
        const metrics = fragmentMetrics(fonts, fragment);
        height = Math.max(height, metrics.height);
        ascent = Math.max(ascent, metrics.ascent);
    }
    let atX = x;
    // 둘째 순회: 기준선에 맞춰 조각을 그리고 측정 폭만큼 오른쪽으로 옮긴다.
    for (const fragment of fragments) {
        const baseline = fragmentMetrics(fonts, fragment).ascent;
        const size = measureFragment(fonts, fragment);
        drawFragment(surface, fonts, fragment, atX, (y + ascent - baseline) | 0, clip);
        atX = (atX + size.width) | 0;
    }
    return height;
}
