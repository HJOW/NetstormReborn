// 원본 비트맵 글꼴 글자를 8비트 표면에 그리는 모듈이다(원본 004a3f70·004a3430의 출력 순서).
// 글자 문맥(TextContext)의 플래그 2 는 그림자 패스, 플래그 4 는 네 방향 외곽선 패스이며, 마지막에 본문 패스를 그린다.
// 각 패스는 글리프 색 번호 100 을 그 패스의 색으로 바꾸는 표를 설치하고, 글리프마다 그리기 전진 폭만큼 오른쪽으로 옮긴다.
// 복원 기준: cpppj GumpTextDraw.cpp(DrawBitmapGlyphText·MakeGumpGlyphPixelHooks). (2026-10-23 WB03-M1)

// 원본 글꼴 캐시가 글자 픽셀에 쓰는 색 번호이다. 출력할 때 이 번호만 문맥의 색으로 바꾼다. (2026-10-23 WB03-M1)
export const FONT_INK_INDEX = 100;

// 외곽선 패스의 고정 오프셋 표(위 → 오른쪽 → 아래 → 왼쪽)이다. 원본 두 판본의 표 순서와 같다. (2026-10-23 WB03-M1)
const OUTLINE_OFFSETS = Object.freeze([
    Object.freeze([0, -1]),
    Object.freeze([1, 0]),
    Object.freeze([0, 1]),
    Object.freeze([-1, 0]),
]);

// 원본 32바이트 TextContext 와 같은 필드의 글자 문맥을 만든다. 빠진 필드는 0 이다.
// flags(2=그림자, 4=외곽선), font(글꼴 슬롯), color(본문 색 번호), style, extra, shadowColor, shadowX, shadowY 순서다.
// 사용: createTextContext({ flags: 2, color: white, shadowColor: black, shadowX: 1, shadowY: 1 }) (2026-10-23 WB03-M1)
export function createTextContext(fields = {}) {
    return Object.freeze({
        flags: fields.flags ?? 0,
        font: fields.font ?? 0,
        color: fields.color ?? 0,
        style: fields.style ?? 0,
        extra: fields.extra ?? 0,
        shadowColor: fields.shadowColor ?? 0,
        shadowX: fields.shadowX ?? 0,
        shadowY: fields.shadowY ?? 0,
    });
}

// 바이트 문자열을 (x, y) 에서 시작해 그린다. y 는 글자 칸의 위쪽, x 는 첫 글자 칸의 왼쪽이다.
// font 는 BitmapFont 또는 같은 인터페이스(glyph(code))의 UnicodeGlyphFont, bytes 는 글꼴에 맞는 글자 코드 배열(BitmapFont 는 Windows-1252 바이트 Uint8Array,
// UnicodeGlyphFont 는 UTF-16 코드 단위 Uint16Array), context 는 createTextContext 결과, clip 은 출력 제한 사각형이다. (2026-10-31 WB03-N: 유니코드 글꼴 지원)
// 그림자·외곽선·본문 패스마다 x 를 처음 위치로 되돌리며, 글리프 블록이 없는 코드는 그리지도 전진하지도 않는다. (2026-10-23 WB03-M1)
export function drawGlyphText(surface, font, bytes, x, y, context, clip = surface.bounds) {
    const colorMap = new Uint8Array(256);
    // 기존 UI는 명시 길이를 사용하며 공유 원문 반복기가 색/글리프 순서를 수행한다. (2026-11-14 WB04-E406)
    drawBitmapGlyphText(bytes, font, { x, y }, context, {
        color: (color) => { colorMap[FONT_INK_INDEX] = color; },
        // 원래 글리프의 상대 이미지 위치와 현재 clip을 기존 표면에 전달한다.
        glyph: (selected, code, at) => { const glyph = selected.glyph(code); if (glyph.image) surface.drawImage(glyph.image, (at.x + glyph.image.left) | 0, (at.y + glyph.image.top) | 0, clip, colorMap); },
    }, bytes.length);
}

// 원본 Screen 글리프 반복이다. -1은 범위 안 NUL 앞까지, 명시 길이는 중간0도 출력한다. UTF-16 입력은 기존 웹 유니코드 적응이다. (2026-11-14 WB04-E406)
export function drawBitmapGlyphText(bytes, font, position, context, hooks, length = -1) {
    let count;
    if (length === -1) { count = bytes.indexOf(0); if (count < 0) throw new TypeError('Gump 글자 출력 범위에 NUL 종료 필요'); }
    else { if (!Number.isInteger(length) || length < 0 || length > bytes.length) throw new RangeError('Gump 글자 출력 길이 범위 오류'); count = length; }
    if (typeof hooks?.color !== 'function' || typeof hooks?.glyph !== 'function') throw new TypeError('Gump 색/글리프 출력 경계 필요');
    // 빈 문자열/빈 글꼴도 매 패스 색을 설치하고 그림자·외곽선·본문 순서를 유지한다.
    const pass = (offsetX, offsetY, color) => {
        hooks.color(color & 255); const at = { x: (position.x + offsetX) | 0, y: (position.y + offsetY) | 0 };
        // 포인터 없는 글리프는 폭을 전진하지 않으며 특별 빈 글리프는 출력 경계와 폭을 유지한다.
        for (let index = 0; index < count; index += 1) {
            const code = bytes instanceof Uint16Array ? bytes[index] : bytes[index] & 255;
            if (!font) continue; const glyph = font.glyph(code); if (!glyph.present) continue;
            hooks.glyph(font, code, { x: at.x, y: at.y }); at.x = (at.x + glyph.drawAdvance) | 0;
        }
    };
    if (context.flags & 2) pass(context.shadowX, context.shadowY, context.shadowColor);
    // 고정 외곽선 표는 위→오른쪽→아래→왼쪽이다.
    if (context.flags & 4) for (const [dx, dy] of OUTLINE_OFFSETS) pass(dx, dy, context.shadowColor);
    pass(0, 0, context.color);
}
