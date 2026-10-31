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
    // 원본 글꼴 전용 256색 표다. 글자 픽셀은 100 번만 쓰므로 그 칸만 패스마다 바꾼다.
    const colorMap = new Uint8Array(256);
    // 한 패스(오프셋·색)를 그린다.
    const pass = (offsetX, offsetY, color) => {
        colorMap[FONT_INK_INDEX] = color & 255;
        let atX = (x + offsetX) | 0;
        const atY = (y + offsetY) | 0;
        // 문자열의 각 바이트를 차례로 그린다.
        for (const code of bytes) {
            const glyph = font.glyph(code);
            if (!glyph.present) continue;
            if (glyph.image) surface.drawImage(glyph.image, (atX + glyph.image.left) | 0, (atY + glyph.image.top) | 0, clip, colorMap);
            atX = (atX + glyph.drawAdvance) | 0;
        }
    };
    if (context.flags & 2) pass(context.shadowX, context.shadowY, context.shadowColor);
    // 외곽선 플래그가 있으면 고정 네 방향을 순서대로 그린다. 그림자와는 독립이다.
    if (context.flags & 4) for (const [dx, dy] of OUTLINE_OFFSETS) pass(dx, dy, context.shadowColor);
    pass(0, 0, context.color);
}
