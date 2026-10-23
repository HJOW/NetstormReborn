// 원본 돌 버튼(Buttongump)의 배경 질감 반복·2픽셀 명암 테두리·외곽선·글자 배치를 계획하고 그리는 모듈이다.
// 원본 주소: 배경 그리기 00465880 → 질감 반복 00465690, 버튼 Draw 00424ff0, 버튼 생성자 004254b0.
// 원본 x86 정수 연산(32비트 감김, 0 방향 절삭 나눗셈)을 그대로 따른다.
// 복원 기준: cpppj GumpBackground.cpp·GumpVisual.cpp, 문서 docs/exe/cpp-gump-background-reconstruction.md·cpp-gump-visual-reconstruction.md.
// (2026-10-23 WB03-M1: 메인 메뉴 돌 버튼을 원본과 같은 픽셀로 그리기 위해 추가)
import { clipRect } from './indexed-surface.js';
import { createTextContext } from './glyph-text.js';

// 배경 자식의 플래그 비트이다. 원본 배경 노드의 backgroundFlags 와 같은 값이다. (2026-10-23 WB03-M1)
export const GumpBackgroundFlags = Object.freeze({
    // 질감 경로를 고른다. 이 모듈은 질감 경로만 계획한다.
    TEXTURE: 0x02000000,
    // 단색 채우기 경로다(질감보다 우선). 이 모듈은 아직 지원하지 않는다.
    SOLID: 0x04000000,
    // 체크 채우기 경로다(단색·질감보다 우선). 이 모듈은 아직 지원하지 않는다.
    CHECKER: 0x08000000,
    // 왼쪽 2픽셀 테두리를 그린다.
    LEFT: 0x01000000,
    // 위쪽 2픽셀 테두리를 그린다.
    TOP: 0x00800000,
    // 오른쪽 2픽셀 테두리를 그린다.
    RIGHT: 0x00400000,
    // 아래쪽 2픽셀 테두리를 그린다.
    BOTTOM: 0x00200000,
    // 어두운 명암 변환표를 쓴다.
    DARK: 0x00100000,
    // 밝은 명암 변환표를 쓴다(어두움과 함께 있으면 우선).
    BRIGHT: 0x00080000,
    // 눌림: 테두리 명암 방향을 바꾼다. 바탕 전체를 어둡게 하지는 않는다.
    PRESSED: 0x00040000,
    // 돌 버튼 생성자가 배경 자식에 주는 기본 플래그(질감 + 네 테두리)다.
    BUTTON: 0x03e00000,
});

// 32비트 정수 덧셈(감김 유지)이다. 원본 x86 의 DWORD 덧셈과 같다. (2026-10-23 WB03-M1)
function add(a, b) {
    return (a + b) | 0;
}

// 32비트 정수 뺄셈(감김 유지)이다. (2026-10-23 WB03-M1)
function sub(a, b) {
    return (a - b) | 0;
}

// C/C++ 정수 나눗셈(0 방향 절삭)이다. 음수에서도 원본과 같이 0 쪽으로 자른다. (2026-10-23 WB03-M1)
function div(a, b) {
    return Math.trunc(a / b) | 0;
}

// 질감 반복 함수 한 번의 호출 계획을 만든다(원본 00465690). 타일 기준점은 화면 좌표 격자에 고정된다.
// 반복 주기는 SHP 메타 폭/높이에서 1을 뺀 값이며, 시작/끝 배수와 여분 타일까지 원본 정수 계산 그대로 구한다.
// 반환: { rect, clip, flags, shade(0 원색·1 밝음·2 어두움), validTile, origins[{x,y}] } (2026-10-23 WB03-M1)
function planTexturePass(rect, clip, tileWidth, tileHeight, flags) {
    const result = { rect, clip: { left: 0, top: 0, right: 0, bottom: 0 }, flags, shade: 0, validTile: false, origins: [] };
    const dx = tileWidth - 1;
    const dy = tileHeight - 1;
    if (dx < 1 || dy < 1) return result;
    if (rect.left > rect.right || rect.top > rect.bottom) throw new Error('GUI 질감 경계가 뒤집혔습니다.');
    result.validTile = true;
    result.clip = clipRect(clip, rect);
    result.shade = flags & GumpBackgroundFlags.BRIGHT ? 1 : flags & GumpBackgroundFlags.DARK ? 2 : 0;
    const startX = Math.imul(div(rect.left, dx), dx);
    const startY = Math.imul(div(rect.top, dy), dy);
    const endX = Math.imul(add(add(div(sub(rect.right, rect.left), dx), 2), div(rect.left, dx)), dx);
    const endY = Math.imul(add(add(div(sub(rect.bottom, rect.top), dy), 2), div(rect.top, dy)), dy);
    // 위에서 아래로 끝 배수를 포함해 여분 타일까지 기준점을 만든다. 실제 출력 범위는 clip 이 제한한다.
    for (let y = startY; y <= endY; y += dy) {
        // 같은 화면 기준 x 배수를 각 행에서 다시 쓴다(버튼의 상대 원점에서 시작하지 않는다).
        for (let x = startX; x <= endX; x += dx) result.origins.push({ x, y });
    }
    return result;
}

// 질감 배경 전체의 출력 계획(바탕 → 왼쪽 → 오른쪽 → 위 → 아래 순서)을 만든다(원본 00465880 의 질감 경로).
// rect 는 배경 자식의 화면 사각형, clip 은 현재 출력 클립, tileWidth/Height 는 질감 프레임의 Squid 메타 폭/높이다.
// 정상은 왼쪽·위에 밝은 표, 오른쪽·아래에 어두운 표를 쓰고, PRESSED 면 방향을 바꾼다.
// 사용: drawGumpBackground(surface, texture, planGumpBackground(...), shadeMaps) (2026-10-23 WB03-M1)
export function planGumpBackground(rect, clip, tileWidth, tileHeight, flags) {
    if ((flags & 0x0c000000) !== 0 || (flags & GumpBackgroundFlags.TEXTURE) === 0) throw new Error('지원하지 않는 GUI 배경 경로입니다.');
    const pressed = (flags & GumpBackgroundFlags.PRESSED) !== 0;
    const nearShade = pressed ? GumpBackgroundFlags.DARK : GumpBackgroundFlags.BRIGHT;
    const farShade = pressed ? GumpBackgroundFlags.BRIGHT : GumpBackgroundFlags.DARK;
    const passes = [planTexturePass(rect, clip, tileWidth, tileHeight, flags)];
    if (flags & GumpBackgroundFlags.LEFT) {
        passes.push(planTexturePass(clipRect({ left: rect.left, top: rect.top, right: add(rect.left, 2), bottom: rect.bottom }, clip), clip, tileWidth, tileHeight, flags | nearShade));
    }
    if (flags & GumpBackgroundFlags.RIGHT) {
        passes.push(planTexturePass(clipRect({ left: sub(rect.right, 2), top: rect.top, right: rect.right, bottom: rect.bottom }, clip), clip, tileWidth, tileHeight, flags | farShade));
    }
    if (flags & GumpBackgroundFlags.TOP) {
        passes.push(planTexturePass(clipRect({ left: rect.left, top: rect.top, right: rect.right, bottom: add(rect.top, 2) }, clip), clip, tileWidth, tileHeight, flags | nearShade));
    }
    if (flags & GumpBackgroundFlags.BOTTOM) {
        passes.push(planTexturePass(clipRect({ left: rect.left, top: sub(rect.bottom, 2), right: rect.right, bottom: rect.bottom }, clip), clip, tileWidth, tileHeight, flags | farShade));
    }
    return passes;
}

// 배경 출력 계획을 표면에 그린다. texture 는 질감 프레임 그림(left/top 은 SHP 기준점 대비 그림 위치)이다.
// 명암 패스는 질감의 색 번호를 shadeMaps.bright / dark 표로 바꿔 그린다. 투명 픽셀은 기존 색을 유지한다. (2026-10-23 WB03-M1)
export function drawGumpBackground(surface, texture, passes, shadeMaps) {
    // 원본 호출 순서대로 그려 겹치는 테두리 모서리의 최종 색을 유지한다.
    for (const pass of passes) {
        if (!pass.validTile) continue;
        const map = pass.shade === 1 ? shadeMaps.bright : pass.shade === 2 ? shadeMaps.dark : null;
        // 화면 격자에 고정된 각 기준점에 질감을 그린다. 여러 버튼이 같은 질감 위상을 공유한다.
        for (const origin of pass.origins) surface.drawImage(texture, add(origin.x, texture.left), add(origin.y, texture.top), pass.clip, map);
    }
}

// 돌 버튼의 정상·눌림·비활성 글자 문맥 세 개를 만든다(원본 004254b0 의 +104/+124/+144).
// 정상·눌림은 흰 글자 + 검정 그림자 (1,1)(플래그 2), 비활성은 그림자 없는 검정 글자다. 글꼴은 슬롯 0 이다. (2026-10-23 WB03-M1)
export function buttonTextContexts(buttonPalette) {
    const active = createTextContext({ flags: 2, font: 0, color: buttonPalette.white, shadowColor: buttonPalette.black, shadowX: 1, shadowY: 1 });
    const disabled = createTextContext({ flags: 0, font: 0, color: buttonPalette.black });
    return Object.freeze([active, active, disabled]);
}

// 돌 버튼 Draw 의 외곽선 네 개와 글자 상대 위치·문맥을 계획한다(원본 00424ff0).
// state: { rect(부모 버튼 화면 사각형), textWidth, textHeight(측정값), value, pressed, flags }.
// flags 8 은 외곽선 생략, flags 1 은 비활성 문맥이다. value 나 pressed 가 0이 아니면 글자를 오른쪽·아래로 1픽셀 민다.
// 반환: { lines[{x1,y1,x2,y2,color}], textOffset{x,y}(부모 왼쪽 위 기준), text(문맥) } (2026-10-23 WB03-M1)
export function planButtonDraw(state, contexts, edge) {
    let rect = state.rect;
    const down = state.value !== 0 || state.pressed !== 0;
    let lines = [];
    if ((state.flags & 8) === 0) {
        lines = [
            { x1: add(rect.left, 1), y1: rect.top, x2: sub(rect.right, 2), y2: rect.top, color: edge },
            { x1: add(rect.left, 1), y1: sub(rect.bottom, 2), x2: sub(rect.right, 2), y2: sub(rect.bottom, 2), color: edge },
            { x1: rect.left, y1: add(rect.top, 1), x2: rect.left, y2: sub(rect.bottom, 3), color: edge },
            { x1: sub(rect.right, 1), y1: add(rect.top, 1), x2: sub(rect.right, 1), y2: sub(rect.bottom, 3), color: edge },
        ];
        rect = { left: add(rect.left, 1), top: add(rect.top, 1), right: sub(rect.right, 1), bottom: sub(rect.bottom, 2) };
    }
    const textOffset = {
        x: add(div(sub(sub(rect.right, state.textWidth), rect.left), 2), down ? 3 : 2),
        y: add(div(sub(sub(rect.bottom, state.textHeight), rect.top), 2), down ? 2 : 1),
    };
    const text = contexts[(state.flags & 1) !== 0 ? 2 : down ? 1 : 0];
    return { lines, textOffset, text };
}
