// 원본 마우스 커서 리소스(RT_CURSOR)를 해독해 브라우저 CSS 커서로 적용하는 모듈이다.
// 원본 메인 메뉴의 커서는 기본 커서 번호 1(리소스 그룹 113 → RT_CURSOR_8, 흰 화살표, 핫스폿 10,6)이다
// (assets/game-data/cursors/README.md). 리소스는 핫스폿 4바이트 + 단색 DIB(XOR 마스크 위, AND 마스크 아래, 각 32×32)다.
// 게임 화면이 확대되면 원본과 같은 비율로 보이도록 커서도 정수 배율로 키운다.
// (2026-10-23 WB03-M1: 메인 메뉴에서 원본 커서 모양을 쓰기 위해 추가)

// 메인 메뉴 기본 커서(번호 1)의 데이터 폴더 기준 경로이다. (2026-10-23 WB03-M1)
export const DEFAULT_CURSOR_PATH = 'cursors/RT_CURSOR_8.bin';

// 커서 리소스의 핫스폿 머리 크기(바이트)이다. 그 뒤에 BITMAPINFOHEADER 가 온다. (2026-10-23 WB03-M1)
const HOTSPOT_HEADER_SIZE = 4;

// BITMAPINFOHEADER 크기(바이트)이다. (2026-10-23 WB03-M1)
const BITMAP_INFO_SIZE = 40;

// 단색 DIB 의 색 표 크기(RGBQUAD 2개, 바이트)이다. (2026-10-23 WB03-M1)
const MONO_PALETTE_SIZE = 8;

// CSS 커서 그림의 최대 배율이다. 브라우저 커서 최대 크기(128×128)를 넘지 않게 한다. (2026-10-23 WB03-M1)
const MAX_CURSOR_SCALE = 4;

// 커서 리소스 바이트를 해독한다. 반환: { width, height, hotspotX, hotspotY, rgba(Uint8ClampedArray), inverted }.
// AND=1·XOR=0 은 투명, AND=0 이면 XOR 비트로 색 표의 0/1 번 색, AND=1·XOR=1(화면 반전)은 브라우저에서 표현할 수 없어
// 검정으로 그리고 inverted 에 개수를 센다(원본 화살표에는 반전 픽셀이 없다). (2026-10-23 WB03-M1)
export function decodeCursorResource(bytes) {
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    const hotspotX = view.getUint16(0, true);
    const hotspotY = view.getUint16(2, true);
    const headerSize = view.getUint32(HOTSPOT_HEADER_SIZE, true);
    const width = view.getInt32(HOTSPOT_HEADER_SIZE + 4, true);
    const doubledHeight = view.getInt32(HOTSPOT_HEADER_SIZE + 8, true);
    const bitCount = view.getUint16(HOTSPOT_HEADER_SIZE + 14, true);
    if (headerSize !== BITMAP_INFO_SIZE || bitCount !== 1 || width <= 0 || doubledHeight <= 0 || doubledHeight % 2 !== 0) {
        throw new Error('지원하지 않는 커서 리소스 형식입니다.');
    }
    const height = doubledHeight / 2;
    const stride = Math.ceil(width / 32) * 4;
    const paletteOffset = HOTSPOT_HEADER_SIZE + BITMAP_INFO_SIZE;
    const xorOffset = paletteOffset + MONO_PALETTE_SIZE;
    const andOffset = xorOffset + stride * height;
    if (andOffset + stride * height > bytes.length) throw new Error('커서 리소스가 잘렸습니다.');
    // 색 표 0/1 번의 RGB(파일은 BGRX 순서)다.
    const colors = [0, 1].map((i) => [bytes[paletteOffset + i * 4 + 2], bytes[paletteOffset + i * 4 + 1], bytes[paletteOffset + i * 4]]);
    const rgba = new Uint8ClampedArray(width * height * 4);
    let inverted = 0;
    // DIB 는 아래 행부터 저장되므로 파일 행 번호를 화면 행으로 뒤집으며 읽는다.
    for (let row = 0; row < height; row += 1) {
        const y = height - 1 - row;
        // 한 행의 각 픽셀 비트를 읽는다(최상위 비트가 왼쪽).
        for (let x = 0; x < width; x += 1) {
            const mask = 0x80 >> (x & 7);
            const xorBit = (bytes[xorOffset + row * stride + (x >> 3)] & mask) !== 0;
            const andBit = (bytes[andOffset + row * stride + (x >> 3)] & mask) !== 0;
            if (andBit && !xorBit) continue;
            const color = andBit ? (inverted += 1, [0, 0, 0]) : colors[xorBit ? 1 : 0];
            const output = (y * width + x) * 4;
            rgba[output] = color[0];
            rgba[output + 1] = color[1];
            rgba[output + 2] = color[2];
            rgba[output + 3] = 255;
        }
    }
    return Object.freeze({ width, height, hotspotX, hotspotY, rgba, inverted });
}

// 해독한 커서를 CSS cursor 속성 값으로 만든다. cssScale 은 게임 논리 픽셀 1개의 CSS 픽셀 수(layout.scale),
// devicePixelRatio 는 기기 픽셀 비율이다. 그림은 정수 배율로 최근접 확대하고, 고해상도 화면에서는 image-set 으로 선명하게 한다.
// 반환: { primary(image-set 사용 값), fallback(url 만 쓰는 값) }. 브라우저가 primary 를 거부하면 fallback 을 쓴다. (2026-10-23 WB03-M1)
export function buildCursorCss(doc, cursor, cssScale, devicePixelRatio = 1) {
    const scale = Math.max(1, Math.min(MAX_CURSOR_SCALE, Math.round(cssScale) || 1));
    const deviceScale = Math.max(1, Math.round(scale * (devicePixelRatio > 0 ? devicePixelRatio : 1)));
    const hotspotX = Math.floor((cursor.hotspotX + 0.5) * scale);
    const hotspotY = Math.floor((cursor.hotspotY + 0.5) * scale);
    const lowUrl = renderCursorUrl(doc, cursor, scale);
    const highUrl = deviceScale === scale ? lowUrl : renderCursorUrl(doc, cursor, deviceScale);
    const resolution = deviceScale / scale;
    const fallback = `url("${lowUrl}") ${hotspotX} ${hotspotY}, default`;
    const primary = resolution === 1 ? fallback : `image-set(url("${highUrl}") ${resolution}x) ${hotspotX} ${hotspotY}, ${fallback}`;
    return { primary, fallback };
}

// 커서 그림을 정수 배율로 키운 PNG data URL 을 만든다. (2026-10-23 WB03-M1)
function renderCursorUrl(doc, cursor, scale) {
    const canvas = doc.createElement('canvas');
    canvas.width = cursor.width * scale;
    canvas.height = cursor.height * scale;
    const context = canvas.getContext('2d');
    const source = doc.createElement('canvas');
    source.width = cursor.width;
    source.height = cursor.height;
    const sourceContext = source.getContext('2d');
    const image = sourceContext.createImageData(cursor.width, cursor.height);
    image.data.set(cursor.rgba);
    sourceContext.putImageData(image, 0, 0);
    context.imageSmoothingEnabled = false;
    context.drawImage(source, 0, 0, canvas.width, canvas.height);
    return canvas.toDataURL('image/png');
}

// 요소에 원본 커서를 적용한다. primary 가 거부되면(브라우저가 image-set 커서를 모르면) fallback 을 쓴다.
// 사용: applyCursorStyle(canvas, buildCursorCss(document, cursor, layout.scale, devicePixelRatio)) (2026-10-23 WB03-M1)
export function applyCursorStyle(element, css) {
    // 거부된 값은 기존 값을 남기므로 먼저 비운 뒤 적용 여부를 확인한다.
    element.style.cursor = '';
    element.style.cursor = css.primary;
    if (!element.style.cursor) element.style.cursor = css.fallback;
}
