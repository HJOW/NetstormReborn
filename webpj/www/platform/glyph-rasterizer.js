// 브라우저에서 한국어 D2Coding 글자를 Canvas 2D 로 그려 알파 그림으로 돌려주는 래스터라이저 모듈이다.
// 글꼴은 CSS 파일 없이 FontFace API 로 등록한다(웹 판본은 CSS 파일을 만들지 않는 방침). 알파 그림을 색 번호 글리프로 바꾸는 일은
// assets/unicode-font.js 가 하며, 이 모듈은 "글자 하나 → 정수 전진 폭 + 잘라낸 알파 그림" 만 맡는다.
// 인터페이스는 assets/unicode-font.js 머리 주석을 따른다: glyph(codePoint, spec) → { advance, left, top, width, height, coverage }.
// (2026-10-31 WB03-N: 한국어 UI 글자 출력을 위해 추가)

// 글꼴 등록 이름이다. 같은 이름의 시스템 글꼴이 있어도 FontFace 로 등록한 파일이 우선한다.
// netstorm.js 의 FONT_FAMILY 도 이 이름을 가장 먼저 부르므로, 등록 뒤에는 로딩·클릭 안내 화면도 같은 글꼴로 나온다. (2026-10-31 WB03-N)
export const GLYPH_FONT_FAMILY = 'D2Coding';

// 글자 그림 주변에 더하는 여백(픽셀)이다. 기울임·굵은 글씨로 칸 밖으로 삐져나오는 부분을 담는다. (2026-10-31 WB03-N)
const CANVAS_MARGIN_EM = 1;

// 보통체·굵은체 sfnt 바이트로 FontFace 두 개를 등록하고 불러온 뒤 래스터라이저를 돌려준다. document 는 글꼴을 등록할 문서이다.
// 글꼴을 불러오지 못하면 FontFace.load() 가 던지는 오류를 그대로 전달한다(호출자가 영어로 대체한다).
// 사용: const rasterizer = await createCanvasGlyphRasterizer({ document, regular, bold }); (2026-10-31 WB03-N)
export async function createCanvasGlyphRasterizer({ document, regular, bold }) {
    const view = document.defaultView ?? globalThis;
    const FontFaceClass = view.FontFace ?? globalThis.FontFace;
    if (typeof FontFaceClass !== 'function') throw new Error('이 브라우저는 FontFace 를 지원하지 않습니다.');
    // 보통체(400)와 굵은체(700)를 같은 이름으로 등록한다. 글자를 그릴 때 font 문자열의 굵기로 고른다.
    const faces = [
        new FontFaceClass(GLYPH_FONT_FAMILY, regular, { weight: '400', style: 'normal' }),
        new FontFaceClass(GLYPH_FONT_FAMILY, bold, { weight: '700', style: 'normal' }),
    ];
    await Promise.all(faces.map((face) => face.load()));
    // 불러온 글꼴을 문서에 등록한다(캔버스가 이름으로 찾는다).
    for (const face of faces) document.fonts.add(face);
    const canvas = document.createElement('canvas');
    const context = canvas.getContext('2d', { willReadFrequently: true });
    if (!context) throw new Error('2D 캔버스를 만들 수 없어 한국어 글자를 그릴 수 없습니다.');
    return {
        // 글자 하나를 그려 정수 전진 폭과 알파 그림(좌우·상하 빈 줄을 잘라낸 것)을 돌려준다.
        // 글자 칸의 왼쪽 위(펜 x = 0, 줄 맨 위 y = 0)에서 기준선은 spec.ascent 아래에 둔다.
        glyph(codePoint, spec) {
            const character = String.fromCodePoint(codePoint);
            const font = `${spec.italic ? 'italic ' : ''}${spec.bold ? 700 : 400} ${spec.emSize}px "${GLYPH_FONT_FAMILY}"`;
            context.font = font;
            const advance = Math.max(0, Math.round(context.measureText(character).width));
            const margin = Math.ceil(spec.emSize * CANVAS_MARGIN_EM);
            const width = advance + margin * 2;
            const height = spec.height + margin * 2;
            // 크기를 바꾸면 캔버스 상태가 지워지므로 큰 쪽에 맞춰 한 번만 키우고, 그 뒤에는 필요한 영역만 지운다.
            if (canvas.width < width || canvas.height < height) {
                canvas.width = Math.max(canvas.width, width);
                canvas.height = Math.max(canvas.height, height);
                context.font = font;
            }
            context.clearRect(0, 0, width, height);
            context.fillStyle = '#fff';
            context.textBaseline = 'alphabetic';
            context.textAlign = 'left';
            context.fillText(character, margin, margin + spec.ascent);
            const pixels = context.getImageData(0, 0, width, height).data;
            let minX = width;
            let minY = height;
            let maxX = -1;
            let maxY = -1;
            // 알파가 있는 픽셀의 경계 사각형을 구한다.
            for (let y = 0; y < height; y += 1) {
                for (let x = 0; x < width; x += 1) {
                    if (pixels[(y * width + x) * 4 + 3] === 0) continue;
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
            if (maxX < 0) return { advance, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array(0) };
            const cropWidth = maxX - minX + 1;
            const cropHeight = maxY - minY + 1;
            const coverage = new Uint8Array(cropWidth * cropHeight);
            // 경계 사각형 안의 알파 값만 잘라 담는다.
            for (let y = 0; y < cropHeight; y += 1) {
                for (let x = 0; x < cropWidth; x += 1) coverage[y * cropWidth + x] = pixels[((minY + y) * width + (minX + x)) * 4 + 3];
            }
            return { advance, left: minX - margin, top: minY - margin, width: cropWidth, height: cropHeight, coverage };
        },
        // 등록한 글꼴을 문서에서 뺀다(앱 정리 시). 한 번 더 불러도 안전하다.
        dispose() {
            for (const face of faces) document.fonts.delete(face);
        },
    };
}
