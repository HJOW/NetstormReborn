// 실제 Google Fonts CSS와 등록 FontFace를 기다린 뒤 일본어 Canvas 글꼴을 제공한다.
import { createRegisteredGlyphRasterizer } from './glyph-rasterizer.js';
// AGENTS의 WOFF2 없는 글꼴 규칙에 따른Noto Sans JP CDN이다. 글자 내용은 URL로 보내지 않는다.
import { JAPANESE_FONT_STYLESHEET, JAPANESE_FONT_FAMILY } from '../assets/japanese-asset-config.js';
// 외부 글꼴 대기를 제한하는 시간이며 실패하면 호출자가 영어 자료로 대체한다.
const FONT_TIMEOUT_MS = 30000;
// 문서 소유의 링크와 실제 글꼴을 준비한다. text는 내려받은 일본어 자료의 글리프 선적재에만 쓴다.
export async function createJapaneseGlyphRasterizer({ document, text }) {
    // 브라우저와 이 적재만 소유하는 스타일시트 링크다.
    const view = document.defaultView ?? globalThis;
    const link = document.createElement('link');
    link.rel = 'stylesheet'; link.href = JAPANESE_FONT_STYLESHEET;
    // 모든 실패/정상 반납에서 같은 링크를 제거하며 전역 글꼴/설정은 건드리지 않는다.
    const release = () => link.remove();
    try {
        // 로드 이벤트와 오류/시간 초과를 모두 처리하고 타이머/수신기를 해제한다.
        await new Promise((resolve, reject) => {
            const finish = (error) => { view.clearTimeout(timer); link.onload = null; link.onerror = null; error ? reject(error) : resolve(); };
            const timer = view.setTimeout(() => finish(new Error('일본어 CDN 스타일시트 대기 시간 초과')), FONT_TIMEOUT_MS);
            link.onload = () => finish(); link.onerror = () => finish(new Error('일본어 CDN 스타일시트를 불러올 수 없습니다.'));
            document.head.append(link);
        });
        // 같은 페이지의 자료에 필요한 문자만 고유하게 모은다. 요청URL에는 포함하지 않는다.
        const characters = [...new Set(text)].join('') || '日本語';
        // 임의의사용자일본어도첫글리프부터같은글꼴을쓰도록CSS가광고한전체부분글꼴을먼저적재한다.
        const registeredFonts = Array.from(document.fonts).filter((face) => face.family.replaceAll('"', '').replaceAll("'", '') === JAPANESE_FONT_FAMILY);
        if (registeredFonts.length === 0) throw new Error('일본어 CDN 글꼴 등록이 없습니다.');
        let timer;
        try {
            // 실제 보통/굵은 FontFace가 준비되기 전에는 Canvas 캐시를 만들지 않는다.
            const [, faces] = await Promise.race([
                Promise.all([Promise.all(registeredFonts.map((face) => face.load())), Promise.all([document.fonts.load(`400 12px "${JAPANESE_FONT_FAMILY}"`, characters), document.fonts.load(`700 12px "${JAPANESE_FONT_FAMILY}"`, characters)])]),
                new Promise((resolve, reject) => { timer = view.setTimeout(() => reject(new Error('일본어 CDN 글꼴 대기 시간 초과')), FONT_TIMEOUT_MS); }),
            ]);
            if (faces.some((group) => group.length === 0)) throw new Error('일본어 CDN 글꼴 등록이 없습니다.');
        } finally { view.clearTimeout(timer); }
        return createRegisteredGlyphRasterizer({ document, family: JAPANESE_FONT_FAMILY, releaseFonts: release });
    } catch (error) { release(); throw error; }
}
