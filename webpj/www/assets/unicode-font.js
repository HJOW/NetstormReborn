// 유니코드 글꼴(한국어 갈무리11)의 글자를 원본 비트맵 글꼴(BitmapFont)과 같은 모양의 글리프로 바꿔 주는 모듈이다.
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

// 갈무리11 의 원래 픽셀 크기(ppem)이다. 이 글꼴은 11px 비트맵 글꼴이라 11 의 배수로 그려야 1px 가로획이 합쳐지거나 빠지지 않는다. 작은 슬롯은 이 크기(1배) 그대로 쓴다.
export const KOREAN_SMALL_EM_SIZE = 11;

// 정수 배율 한국어 글자의 기준선 위쪽 줄 수(글자 윗부분이 줄 맨 위 밖으로 나가 잘리지 않으려면 기준선이 이 값 이상이어야 한다)이다. 갈무리11 한글은 11px 에서 기준선 위로 10줄을 차지한다
// (실측: 22px 에서 `잔` 의 잉크가 기준선 위 20줄 = 10줄 × 2배, 44px 에서는 래스터라이저 반올림으로 41줄이 필요해 3배 이상은 1줄을 더한다). (2026-11-23 WB04-E1259)
const KOREAN_ASCENT_ROWS = 10;
export function koreanAscentRows(emSize) {
    const scale = Math.round(emSize / KOREAN_SMALL_EM_SIZE);
    return scale * KOREAN_ASCENT_ROWS + (scale >= 3 ? 1 : 0);
}

// 슬롯 높이에서 한국어 글자 크기(em, 픽셀)를 정한다: 원본 BitmapFont.cpp 의 정수 확대 규칙 `scale = max(1, (높이 + ppem/2) / ppem)` 을 따라 11 의 정수배(22·33·44px)로 고른다(정수 나눗셈).
// 슬롯 높이 [14, 13, -, 20, 48, 14, 12] 에서는 작은 슬롯 11px(1배), 슬롯 3(제목, 높이 20) 22px(2배), 슬롯 4(높이 48) 44px(4배)이다.
// 이전에는 큰 슬롯을 `round(높이 × 13/14)`(슬롯 3 은 19px, 슬롯 4 는 45px)로 정해 비정수 배율 때문에 가로획이 합쳐져 `라`→`다`, `잔`→`산` 처럼 읽혔다. (2026-11-23 WB04-E1259)
export function koreanEmSize(slotHeight) {
    return KOREAN_SMALL_EM_SIZE * Math.max(1, Math.floor((slotHeight + Math.floor(KOREAN_SMALL_EM_SIZE / 2)) / KOREAN_SMALL_EM_SIZE));
}

// 밑줄을 기준선 아래 몇 줄에 긋는지, 취소선을 기준선 위 em 의 얼마에 긋는지이다(작은 글자에서 중앙 부근). (2026-10-31 WB03-N)
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
        // 기준선 아래쪽 높이이다. 정수 배율 큰 글자는 줄 높이보다 기준선이 커질 수 있어 0 아래로는 내려가지 않게 한다(줄 배치는 높이와 기준선만 쓴다). (2026-11-23 WB04-E1259)
        this.descent = Math.max(0, height - ascent);
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
        const emSize = koreanEmSize(slotHeight);
        const spec = {
            emSize,
            // 굵은체는 굵게 스타일(2)일 때만 쓴다. 슬롯 0·3 의 굵기 700 은 영어 Arial 의 획을 위한 것이고, 한글을 12~14픽셀 단색으로 그릴 때는
            // 작은 글자의 가독성을 위해 보통체를 쓴다(그림자 패스가 굵게 보이게 한다). (2026-10-31 WB03-N)
            bold: style === STYLE_BOLD,
            italic: style === STYLE_ITALIC,
            underline: style === STYLE_UNDERLINE,
            strikeout: style === STYLE_STRIKEOUT,
        };
        const font = new UnicodeGlyphFont({
            rasterizer: this.rasterizer,
            spec,
            height: reference?.height ?? slotHeight,
            // 정수 배율 큰 슬롯(emSize > 11)의 기준선은 원본 글꼴 기준선과 갈무리11 글자의 기준선 위 높이(배율 × 10줄) 중 큰 값이다. 22px(슬롯 3)는 20줄이 필요한데 원본 Arial 20 의 기준선(15줄)에 맞추면
            // 한글 윗부분(ㅈ·ㅎ·ㄹ 의 맨 위 가로획)이 줄 맨 위 밖으로 나가 잘려 `잔`→`산`, `라`→`다` 처럼 읽힌다. 원본 BitmapFont.cpp 도 글자가 기준선 위보다 크면 위쪽을 0 줄에 맞춘다(`shift = max(0, ascent - bearingY*scale)`).
            // 작은 슬롯(11px)은 원본 기준선을 그대로 써서 기존 글자 위치가 변하지 않는다. (2026-11-23 WB04-E1259)
            ascent: ((original) => (emSize > KOREAN_SMALL_EM_SIZE ? Math.max(original, koreanAscentRows(emSize)) : original))(reference?.ascent ?? Math.round(emSize * 0.85)),
        });
        this.fonts.set(key, font);
        return font;
    }
}
