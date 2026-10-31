// 유니코드 글꼴(한국어 D2Coding)의 글자를 원본 비트맵 글꼴(BitmapFont)과 같은 모양의 글리프로 바꿔 주는 모듈이다.
// 원본 글꼴 캐시(.chfnt)는 영어(Windows-1252) 글자만 있으므로, 한국어 UI 문구는 이 글꼴로 그린다.
//  - 글자 하나를 래스터라이저(브라우저는 platform/glyph-rasterizer.js 의 Canvas 2D)로 받아 알파 값을 문턱으로 잘라 색 번호 FONT_INK_INDEX(100) 한 가지로 만든다.
//    그 결과 글자 그리기(render/glyph-text.js)가 영어 글리프와 똑같이 그림자·외곽선·본문 패스의 색 표로 칠한다(8비트 화면에 맞는 단색 글리프).
//  - 글꼴 크기는 원본 글꼴 슬롯의 논리 높이(14·13·20·48·12)에서 정하고, 기준선 높이와 줄 높이는 같은 슬롯의 원본 비트맵 글꼴 값을 따라
//    영어 조각과 한 줄에 섞여도 기준선이 맞게 한다. 밑줄·취소선은 글자 칸 전체에 가는 선을 직접 긋는다.
// 래스터라이저 인터페이스: glyph(codePoint, spec) → { advance, left, top, width, height, coverage(Uint8Array 0~255) }.
//   advance 는 정수 전진 폭, left/top 은 글자 칸 왼쪽 위(펜 위치, 줄 맨 위)에서 그림 왼쪽 위까지의 거리, coverage 는 가로×세로 알파 값이다. 빈 글자는 width·height 가 0 이다.
//   spec 은 { emSize, bold, italic, ascent, height } 이다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 가짜 래스터라이저로 검사할 수 있다. (2026-10-31 WB03-N: 한국어 UI 글자 출력을 위해 추가)
import { FONT_SLOT_HEIGHTS } from './bitmap-font.js';
import { FONT_INK_INDEX } from '../render/glyph-text.js';

// 알파(0~255)가 이 값 이상이면 글자 픽셀로 본다. 작은 크기에서 가는 획이 끊기지 않으면서 번지지 않는 값이다. (2026-10-31 WB03-N)
export const COVERAGE_THRESHOLD = 110;

// 글꼴 슬롯의 논리 높이에 곱해 글자 크기(em, 픽셀)를 정하는 비율이다. 슬롯 높이 14 에서 em 13(한글 13×13, 영문 7폭), 높이 12 에서 11 이 된다.
// D2Coding 줄 높이(hhea 850+230)를 줄 높이 14 에 맞추는 GDI 방식(em 12)은 한글 획이 서로 붙어 읽기 어려웠고, 브라우저에서 1~3배 확대 화면을 비교해
// 13 이 줄 높이(기준선 11 + 아래 3) 안에서 획 간격이 가장 또렷했다. (2026-10-31 WB03-N)
export const EM_PER_SLOT_HEIGHT = 13 / 14;

// 글자 크기의 최솟값(픽셀)이다. 너무 작으면 글자가 뭉개져 읽을 수 없다. (2026-10-31 WB03-N)
const MIN_EM_SIZE = 6;

// 밑줄을 기준선 아래 몇 줄에 긋는지, 취소선을 기준선 위 em 의 얼마에 긋는지이다(D2Coding x 높이 0.526 의 절반 부근). (2026-10-31 WB03-N)
const UNDERLINE_BELOW_BASELINE = 1;
const STRIKEOUT_EM_RATIO = 0.26;

// 원본 스타일 번호이다(assets/font-set.js FONT_STYLE_NAMES 순서: 보통·기울임·굵게·취소선·밑줄). (2026-10-31 WB03-N)
const STYLE_ITALIC = 1;
const STYLE_BOLD = 2;
const STYLE_STRIKEOUT = 3;
const STYLE_UNDERLINE = 4;

// 유니코드 글꼴 한 벌(글꼴 슬롯 하나 × 스타일 하나)이다. BitmapFont 와 같은 인터페이스(height·ascent·descent·glyph·measure)를 가진다.
// 글자 코드는 UTF-16 코드 단위(Uint16Array 의 값)이다. 글리프는 처음 쓸 때 만들어 보관한다.
// 사용: const font = new UnicodeGlyphFont({ rasterizer, spec, height: 14, ascent: 11 }); font.glyph(0xcea0); font.measure(units). (2026-10-31 WB03-N)
export class UnicodeGlyphFont {
    // 래스터라이저와 글꼴 사양, 줄 높이·기준선 높이로 글꼴을 만든다. spec 은 { emSize, bold, italic, underline, strikeout } 이다.
    constructor({ rasterizer, spec, height, ascent }) {
        // 글자 하나를 그림으로 만드는 래스터라이저이다.
        this.rasterizer = rasterizer;
        // 글꼴 사양(글자 크기·굵기·기울임·밑줄·취소선)이다.
        this.spec = Object.freeze({ ...spec });
        // 줄 전체 높이이다. 같은 슬롯의 원본 비트맵 글꼴 값을 따른다.
        this.height = height;
        // 기준선 위쪽 높이이다.
        this.ascent = ascent;
        // 기준선 아래쪽 높이이다.
        this.descent = height - ascent;
        // 글자 코드 → 글리프 보관소이다.
        this.cache = new Map();
    }

    // 글자 코드의 글리프를 돌려준다({ advance, bearing, drawAdvance, present, image }, BitmapFont 글리프와 같은 모양). 처음 한 번만 만든다.
    glyph(code) {
        const cached = this.cache.get(code);
        if (cached) return cached;
        const glyph = this._build(code);
        this.cache.set(code, glyph);
        return glyph;
    }

    // 글자 코드 배열의 측정 폭(전진 폭의 합, 32비트 감김)을 돌려준다. 유니코드 글꼴은 측정 폭과 그리기 전진 폭이 같다.
    measure(units) {
        let width = 0;
        // 글자마다 전진 폭을 더한다.
        for (const code of units) width = (width + this.glyph(code).advance) | 0;
        return width;
    }

    // 글리프를 새로 만든다: 래스터라이저의 알파 그림을 문턱으로 잘라 색 번호 그림으로 만들고, 밑줄·취소선이 있으면 글자 칸 전체에 선을 더한다.
    _build(code) {
        const raw = this.rasterizer.glyph(code, { ...this.spec, ascent: this.ascent, height: this.height });
        const advance = raw.advance | 0;
        const lines = this._lineRows();
        // 그림이 차지하는 사각형이다: 글자 그림과 (선이 있으면) 글자 칸 [0, 전진 폭) × 선 줄들의 합집합.
        let left = raw.width > 0 ? raw.left : 0;
        let top = raw.height > 0 ? raw.top : 0;
        let right = raw.width > 0 ? raw.left + raw.width : 0;
        let bottom = raw.height > 0 ? raw.top + raw.height : 0;
        // 선 줄이 있으면 칸 전체 폭과 선 줄을 사각형에 포함시킨다.
        if (lines.length > 0 && advance > 0) {
            left = raw.width > 0 ? Math.min(left, 0) : 0;
            right = Math.max(right, advance);
            top = raw.height > 0 ? Math.min(top, ...lines) : Math.min(...lines);
            bottom = Math.max(bottom, ...lines.map((row) => row + 1));
        }
        const width = right - left;
        const height = bottom - top;
        if (width <= 0 || height <= 0) return Object.freeze({ advance, bearing: 0, drawAdvance: advance, present: true, image: null });
        const indices = new Uint8Array(width * height);
        const opacity = new Uint8Array(width * height);
        // 글자 그림의 알파를 문턱으로 잘라 불투명 픽셀로 옮긴다.
        for (let y = 0; y < raw.height; y += 1) {
            for (let x = 0; x < raw.width; x += 1) {
                if (raw.coverage[y * raw.width + x] < COVERAGE_THRESHOLD) continue;
                const at = (raw.top + y - top) * width + (raw.left + x - left);
                indices[at] = FONT_INK_INDEX;
                opacity[at] = 255;
            }
        }
        // 선 줄마다 글자 칸 전체(0 ~ 전진 폭)에 한 줄을 긋는다.
        if (advance > 0) {
            for (const row of lines) {
                for (let x = 0; x < advance; x += 1) {
                    const at = (row - top) * width + (x - left);
                    indices[at] = FONT_INK_INDEX;
                    opacity[at] = 255;
                }
            }
        }
        return Object.freeze({ advance, bearing: 0, drawAdvance: advance, present: true, image: Object.freeze({ width, height, left, top, indices, opacity }) });
    }

    // 이 글꼴의 밑줄·취소선 줄 번호(줄 맨 위 기준)를 돌려준다.
    _lineRows() {
        const rows = [];
        if (this.spec.underline) rows.push(this.ascent + UNDERLINE_BELOW_BASELINE);
        if (this.spec.strikeout) rows.push(this.ascent - Math.max(1, Math.round(this.spec.emSize * STRIKEOUT_EM_RATIO)));
        return rows;
    }
}

// 글꼴 슬롯·스타일마다 UnicodeGlyphFont 를 만들어 보관하는 공급자이다. FontSet 이 한국어 글꼴이 필요할 때 부른다.
// 사용: const provider = new UnicodeFontProvider(rasterizer); provider.get(0, 0, bitmapFont) → 슬롯 0 보통 글꼴. (2026-10-31 WB03-N)
export class UnicodeFontProvider {
    // 래스터라이저를 받는다.
    constructor(rasterizer) {
        // 글자 그림을 만드는 래스터라이저이다.
        this.rasterizer = rasterizer;
        // `슬롯:스타일` → UnicodeGlyphFont 보관소이다.
        this.fonts = new Map();
    }

    // 슬롯·스타일의 글꼴을 돌려준다. reference 는 같은 슬롯·스타일의 원본 비트맵 글꼴이며 줄 높이·기준선 높이를 맞추는 데 쓴다(없으면 크기에서 추정한다).
    get(slot, style, reference = null) {
        const key = `${slot}:${style}`;
        const cached = this.fonts.get(key);
        if (cached) return cached;
        const slotHeight = FONT_SLOT_HEIGHTS[slot];
        if (!slotHeight) throw new RangeError(`유니코드 글꼴을 만들 수 없는 슬롯입니다: ${slot}`);
        const emSize = Math.max(MIN_EM_SIZE, Math.round(slotHeight * EM_PER_SLOT_HEIGHT));
        const spec = {
            emSize,
            // 굵은체는 굵게 스타일(2)일 때만 쓴다. 슬롯 0·3 의 굵기 700 은 영어 Arial 의 획을 위한 것이고, 한글을 12~14픽셀 단색으로 그릴 때는
            // 굵은체의 획이 뭉쳐 읽기 어려워 보통체를 쓴다(그림자 패스가 굵게 보이게 한다). (2026-10-31 WB03-N)
            bold: style === STYLE_BOLD,
            italic: style === STYLE_ITALIC,
            underline: style === STYLE_UNDERLINE,
            strikeout: style === STYLE_STRIKEOUT,
        };
        const font = new UnicodeGlyphFont({
            rasterizer: this.rasterizer,
            spec,
            height: reference?.height ?? slotHeight,
            ascent: reference?.ascent ?? Math.round(emSize * 0.85),
        });
        this.fonts.set(key, font);
        return font;
    }
}
