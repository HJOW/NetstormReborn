// 브라우저에서 한국어 갈무리11 글자를 Canvas 2D 로 그려 알파 그림으로 돌려주는 래스터라이저 모듈이다.
// 글꼴은 CSS 파일 없이 FontFace API 로 등록한다(웹 판본은 CSS 파일을 만들지 않는 방침). 알파 그림을 색 번호 글리프로 바꾸는 일은
// assets/unicode-font.js 가 하며, 이 모듈은 "글자 하나 → 정수 전진 폭 + 잘라낸 알파 그림" 만 맡는다.
// 인터페이스는 assets/unicode-font.js 머리 주석을 따른다: glyph(codePoint, spec) → { advance, left, top, width, height, coverage }.
// (2026-10-31 WB03-N: 한국어 UI 글자 출력을 위해 추가)

// 글꼴 등록 이름이다. 같은 이름의 시스템 글꼴이 있어도 FontFace 로 등록한 파일이 우선한다.
// netstorm.js 의 FONT_FAMILY 도 이 이름을 가장 먼저 부르므로, 등록 뒤에는 로딩·클릭 안내 화면도 같은 글꼴로 나온다. (2026-10-31 WB03-N)
import { KOREAN_FONT_FAMILY } from '../assets/korean-font-config.js';
export const GLYPH_FONT_FAMILY = KOREAN_FONT_FAMILY;

// 굵은 글꼴(Galmuri11-Bold)이 갖지 않는 글자의 대체 글꼴 등록 이름이다. 제공된 굵은 파일에는 일반 파일에 있는 글자 가운데 라틴 확장·그리스·키릴·가나·한자·호환 자모·일부 기호 등 8천여 자가 없다(두 파일의 cmap 대조).
// 같은 일반 파일을 다른 이름으로 weight 범위 '100 900' 로 등록하면 굵게 요청해도 합성 굵기 없이 일반 글리프가 쓰인다. 원본 복원 cpppj 1322(BitmapFont::ExtendedGlyph)도 굵은 글꼴에 없는 문자를 같은 갈무리11 일반 파일에서 찾고 시스템 대체나 합성 굵기를 쓰지 않는다. (2026-11-23 WB04-E1244)
export const GLYPH_FALLBACK_FONT_FAMILY = `${KOREAN_FONT_FAMILY}Regular`;

// 글자 그림 주변에 더하는 여백(픽셀)이다. 기울임·굵은 글씨로 칸 밖으로 삐져나오는 부분을 담는다. (2026-10-31 WB03-N)
const CANVAS_MARGIN_EM = 1;

// 글자를 몇 배 크기로 그린 뒤 배율×배율 칸의 알파 평균으로 줄여 목표 크기 픽셀 하나의 덮임 정도를 구하는지이다.
// Chromium Canvas 는 글자를 가로로 약간 번지게 그려(12px 갈무리11 의 1px 세로획 양옆에 알파 78~103) 1px 간격으로 붙은 두 획(ㅔ·ㅐ·ㅒ 등) 사이 픽셀에
// 양쪽 번짐이 합쳐져 문턱(110)을 넘고 간격이 메워졌다. 4배로 그리면 번짐은 목표 픽셀 칸 가장자리 한두 열에만 남아 평균이 문턱보다 훨씬 작아지고,
// 갈무리11 의 획은 정수배 크기에서 칸을 꽉 채워 평균 255 가 되므로 설계 픽셀이 그대로 남는다. (2026-11-23 WB04-E1288)
export const GLYPH_SUPERSAMPLE = 4;

// 보통체·굵은체 sfnt 바이트로 FontFace 두 개(와 굵은체 대체용 보조 FontFace 하나)를 등록하고 불러온 뒤 래스터라이저를 돌려준다. document 는 글꼴을 등록할 문서이다.
// 글꼴을 불러오지 못하면 FontFace.load() 가 던지는 오류를 그대로 전달한다(호출자가 영어로 대체한다).
// 사용: const rasterizer = await createCanvasGlyphRasterizer({ document, regular, bold }); (2026-10-31 WB03-N)
export async function createCanvasGlyphRasterizer({ document, regular, bold }) {
    const view = document.defaultView ?? globalThis;
    const FontFaceClass = view.FontFace ?? globalThis.FontFace;
    if (typeof FontFaceClass !== 'function') throw new Error('이 브라우저는 FontFace 를 지원하지 않습니다.');
    // 보통체(400)와 굵은체(700)를 같은 이름으로 등록한다. 글자를 그릴 때 font 문자열의 굵기로 고른다.
    // 세 번째는 굵은체에 없는 글자를 일반 글리프로 대체하는 보조 이름이다(모든 굵기 범위라 합성 굵기가 없다). (2026-11-23 WB04-E1244)
    const faces = [
        new FontFaceClass(GLYPH_FONT_FAMILY, regular, { weight: '400', style: 'normal' }),
        new FontFaceClass(GLYPH_FONT_FAMILY, bold, { weight: '700', style: 'normal' }),
        new FontFaceClass(GLYPH_FALLBACK_FONT_FAMILY, regular, { weight: '100 900', style: 'normal' }),
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
        // 글자는 GLYPH_SUPERSAMPLE 배 크기로 그리고 목표 픽셀마다 배율×배율 칸의 알파 평균을 덮임 정도로 쓴다(가로 번짐으로 1px 간격이 메워지는 것을 막음). (2026-11-23 WB04-E1288)
        glyph(codePoint, spec) {
            const character = String.fromCodePoint(codePoint);
            const scale = GLYPH_SUPERSAMPLE;
            // 굵게는 굵은체를 먼저 쓰고 그 파일에 없는 글자만 일반 글리프(보조 이름)로 대체한다. 보통체는 한 이름만 쓴다. (2026-11-23 WB04-E1244)
            const families = spec.bold ? `"${GLYPH_FONT_FAMILY}", "${GLYPH_FALLBACK_FONT_FAMILY}"` : `"${GLYPH_FONT_FAMILY}"`;
            const font = `${spec.italic ? 'italic ' : ''}${spec.bold ? 700 : 400} ${spec.emSize * scale}px ${families}`;
            context.font = font;
            const advance = Math.max(0, Math.round(context.measureText(character).width / scale));
            const margin = Math.ceil(spec.emSize * CANVAS_MARGIN_EM);
            const width = advance + margin * 2;
            const height = spec.height + margin * 2;
            const bigWidth = width * scale;
            const bigHeight = height * scale;
            // 크기를 바꾸면 캔버스 상태가 지워지므로 큰 쪽에 맞춰 한 번만 키우고, 그 뒤에는 필요한 영역만 지운다.
            if (canvas.width < bigWidth || canvas.height < bigHeight) {
                canvas.width = Math.max(canvas.width, bigWidth);
                canvas.height = Math.max(canvas.height, bigHeight);
                context.font = font;
            }
            context.clearRect(0, 0, bigWidth, bigHeight);
            context.fillStyle = '#fff';
            context.textBaseline = 'alphabetic';
            context.textAlign = 'left';
            // 펜 위치와 기준선을 배율의 정수배에 두어 확대한 글자의 설계 픽셀이 목표 픽셀 칸에 정확히 맞게 한다.
            context.fillText(character, margin * scale, (margin + spec.ascent) * scale);
            const pixels = context.getImageData(0, 0, bigWidth, bigHeight).data;
            const alpha = new Uint8Array(width * height);
            const cell = scale * scale;
            let minX = width;
            let minY = height;
            let maxX = -1;
            let maxY = -1;
            // 목표 픽셀마다 배율×배율 칸의 알파 평균을 구하고, 알파가 있는 픽셀의 경계 사각형을 함께 구한다.
            for (let y = 0; y < height; y += 1) {
                for (let x = 0; x < width; x += 1) {
                    let sum = 0;
                    // 칸 안의 확대 픽셀 알파를 더한다.
                    for (let dy = 0; dy < scale; dy += 1) {
                        const row = (y * scale + dy) * bigWidth + x * scale;
                        for (let dx = 0; dx < scale; dx += 1) sum += pixels[(row + dx) * 4 + 3];
                    }
                    const value = Math.round(sum / cell);
                    if (value === 0) continue;
                    alpha[y * width + x] = value;
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
            // 경계 사각형 안의 평균 알파 값만 잘라 담는다.
            for (let y = 0; y < cropHeight; y += 1) {
                for (let x = 0; x < cropWidth; x += 1) coverage[y * cropWidth + x] = alpha[(minY + y) * width + (minX + x)];
            }
            return { advance, left: minX - margin, top: minY - margin, width: cropWidth, height: cropHeight, coverage };
        },
        // 등록한 글꼴을 문서에서 뺀다(앱 정리 시). 한 번 더 불러도 안전하다.
        dispose() {
            for (const face of faces) document.fonts.delete(face);
        },
    };
}
