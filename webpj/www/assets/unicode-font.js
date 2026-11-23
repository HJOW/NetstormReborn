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
//  - Windows-1252 로 나타낼 수 있는 글자(숫자·영문·공백·문장 부호·라틴-1)는 같은 슬롯·스타일의 원본 비트맵 글리프를 그대로 쓰고 갈무리11 로는 그 밖의 글자만 그린다.
//    cpppj BitmapFont::ExtendedGlyph·GumpTextDraw 가 원본 바이트는 원본 글꼴로, 4바이트 확장 시퀀스만 갈무리11 로 그리는 규칙과 같다.
//    다만 웹이 2배 이상으로 키우는 큰 슬롯(제목 등)에서 한글이 섞인 문자열은 한글 기준선이 원본보다 아래라서 원본 글리프도 그만큼 내려 같은 줄에 앉힌다(UnicodeFontProvider.get 의 extended).
//    (2026-11-23 WB04-E1288: 한국어 모드의 SP 숫자가 갈무리11 2배로 커지고 대본의 공백 폭(갈무리 5px, 원본 Arial 3px)이 달라 그림 줄이 넘치던 문제를 고침)
import { FONT_SLOT_HEIGHTS } from './bitmap-font.js';
import { windows1252ByteOf } from './text-encoding.js';
import { FONT_INK_INDEX } from '../render/glyph-text.js';

// 알파(0~255)가 이 값 이상이면 글자 픽셀로 본다. 작은 크기에서 가는 획이 끊기지 않으면서 번지지 않는 값이다. (2026-10-31 WB03-N)
// 12px 의 갈무리11 은 세로가 픽셀 격자에 정확히 맞고 가로 획 양옆의 번짐 알파가 78~103(문턱 아래)이라 이 문턱으로 1px 획이 그대로 남는다. (2026-11-23 WB04-E1288 실측)
export const COVERAGE_THRESHOLD = 110;

// 갈무리11 웹 글꼴(Galmuri11.woff2)의 설계 격자 크기(1em 의 픽셀 수)이다. 이 크기와 그 정수배에서만 픽셀이 격자에 맞아 1px 획이 합쳐지거나 빠지지 않는다. 작은 슬롯은 이 크기(1배) 그대로 쓴다.
// 실측(Chromium Canvas): 12px 에서 한글 전진 폭 12·숫자 8·공백 5 이고 한글 잉크가 기준선 위 정확히 11줄이며, 24·48px 은 정확히 2·4배(22·44줄)이다.
// cpppj 의 비트맵 TTF strike 도 한글 칸이 전진 폭 12·기준선 위 12줄(빈 맨 윗줄 포함)이다(EmbeddedBitmapFontTests).
// (2026-11-23 WB04-E1288: 이전 값 11 은 글꼴을 11/12 배로 축소해 가로획이 빠져 `가`→`기`, `제`→`세` 처럼 읽혔다)
export const KOREAN_SMALL_EM_SIZE = 12;

// 1배(12px) 갈무리11 한글이 기준선 위로 차지하는 줄 수이다. 한글 잉크는 기준선 위 11줄이고 기준선 아래로는 내려가지 않는다(12·24·48px 실측). (2026-11-23 WB04-E1288)
const KOREAN_ASCENT_ROWS = 11;

// 정수 배율 한국어 글자의 기준선 위쪽 줄 수이다. 글자 윗부분이 줄 맨 위 밖으로 나가 잘리지 않으려면 한글을 그리는 기준선이 이 값 이상이어야 한다.
// 사용: koreanAscentRows(24) → 22. (2026-11-23 WB04-E1259 추가, WB04-E1288: 12px 격자 기준 배율 × 11줄로 바꾸고 3배 이상의 반올림 보정 1줄을 없앰(정수배에서 정확히 맞음))
export function koreanAscentRows(emSize) {
    return koreanScale(emSize) * KOREAN_ASCENT_ROWS;
}

// 글자 크기(em)가 갈무리11 설계 격자의 몇 배인지 돌려준다(1 이상의 정수). 사용: koreanScale(24) → 2. (2026-11-23 WB04-E1288)
export function koreanScale(emSize) {
    return Math.max(1, Math.round(emSize / KOREAN_SMALL_EM_SIZE));
}

// 슬롯 높이에서 한국어 글자 크기(em, 픽셀)를 정한다: 원본 BitmapFont.cpp 의 정수 확대 규칙 `scale = max(1, (높이 + ppem/2) / ppem)` 을 따라 12 의 정수배로 고른다(정수 나눗셈).
// 슬롯 높이 [14, 13, -, 20, 48, 14, 12] 에서는 작은 슬롯 12px(1배), 슬롯 3(제목, 높이 20) 24px(2배), 슬롯 4(높이 48) 48px(4배)이다.
// cpppj 는 같은 식에 비트맵 TTF strike 의 ppem 16 을 넣어 슬롯 3 은 1배, 슬롯 4 는 3배이다. 웹은 E1259 의 제목 2배 결정을 유지하며 이 차이는 단위 문서에 기록했다.
// (2026-11-23 WB04-E1259 정수 배율 도입, WB04-E1288: 기준 크기 11 → 12. 이전에는 큰 슬롯을 `round(높이 × 13/14)` 로 정해 비정수 배율이었다)
export function koreanEmSize(slotHeight) {
    return KOREAN_SMALL_EM_SIZE * Math.max(1, Math.floor((slotHeight + Math.floor(KOREAN_SMALL_EM_SIZE / 2)) / KOREAN_SMALL_EM_SIZE));
}

// UTF-16 코드 단위 배열에 Windows-1252 로 나타낼 수 없는 글자(한글 등 확장 글자)가 하나라도 있는지 검사한다.
// 사용: hasExtendedUnits(fonts.encode('2000')) → false, hasExtendedUnits(fonts.encode('제네레이터')) → true. (2026-11-23 WB04-E1288)
export function hasExtendedUnits(units) {
    // 코드 단위를 앞에서부터 훑어 Windows-1252 밖의 글자를 찾는다.
    for (const code of units) if (windows1252ByteOf(code) === null) return true;
    return false;
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
// native 에 원본 비트맵 글꼴을 주면 Windows-1252 로 나타낼 수 있는 글자는 그 글꼴의 글리프(측정 폭·그리기 전진 폭·그림)를 그대로 돌려주고, 나머지 글자만 래스터라이저로 만든다.
// 사용: const font = new UnicodeGlyphFont({ rasterizer, spec, height: 14, ascent: 11, native: arial14 }); font.glyph(0xcea0); font.measure(units).
// (2026-10-31 WB03-N, 2026-11-23 WB04-E1288: native·minimumLineHeight 추가)
export class UnicodeGlyphFont {
    // 래스터라이저와 글꼴 사양, 줄 높이·기준선 높이로 글꼴을 만든다. spec 은 { emSize, bold, italic, underline, strikeout } 이다.
    // native 는 Windows-1252 글자에 쓸 원본 비트맵 글꼴(없으면 모든 글자를 래스터라이저로 만든다), nativeOffset 은 원본 글리프 그림을 아래로 옮길 줄 수,
    // minimumLineHeight 는 이 글꼴로 잰 문구의 최소 논리 높이(0 이면 슬롯 높이 그대로)이다.
    constructor({ rasterizer, spec, height, ascent, native = null, nativeOffset = 0, minimumLineHeight = 0 }) {
        // 글자 하나를 그림으로 만드는 래스터라이저이다.
        this.rasterizer = rasterizer;
        // 글꼴 사양(글자 크기·굵기·기울임·밑줄·취소선)이다.
        this.spec = Object.freeze({ ...spec });
        // 줄 전체 높이이다. 같은 슬롯의 원본 비트맵 글꼴 값을 따른다.
        this.height = height;
        // 래스터라이저로 만드는 글자(한글 등)를 놓는 기준선의 줄 맨 위에서의 높이이다. 원본 비트맵 글리프는 원본 기준선 위치 그대로 쓴다.
        this.ascent = ascent;
        // 기준선 아래쪽 높이이다. 정수 배율 큰 글자는 줄 높이보다 기준선이 커질 수 있어 0 아래로는 내려가지 않게 한다(줄 배치는 높이와 기준선만 쓴다). (2026-11-23 WB04-E1259)
        this.descent = Math.max(0, height - ascent);
        // Windows-1252 글자에 쓰는 원본 비트맵 글꼴이다. null 이면 모든 글자를 래스터라이저로 만든다. (2026-11-23 WB04-E1288)
        this.native = native;
        // 원본 글리프 그림을 아래로 옮길 줄 수이다. 큰 슬롯의 한글 문구에서 원본 기준선을 한글 기준선에 맞춰 문장 부호·숫자가 한글과 같은 줄에 앉게 한다(그 밖에는 0). (2026-11-23 WB04-E1288)
        this.nativeOffset = nativeOffset;
        // 이 글꼴로 잰 문구의 최소 논리 높이이다. 큰 슬롯의 한글이 원본 슬롯 높이보다 커서 다음 줄과 겹치지 않게 FontSet·GumpText 측정이 이 값과 슬롯 높이 중 큰 값을 쓴다. (2026-11-23 WB04-E1288)
        this.minimumLineHeight = minimumLineHeight;
        // 글자 코드 → 글리프 보관소이다.
        this.cache = new Map();
    }

    // 글자 코드의 글리프를 돌려준다({ advance, bearing, drawAdvance, present, image }, BitmapFont 글리프와 같은 모양). 처음 한 번만 만든다.
    // 원본 비트맵 글꼴이 있고 Windows-1252 로 나타낼 수 있는 글자이면 원본 글리프를 돌려준다(없는 글리프는 present=false 라 그리지도 전진하지도 않는 원본 규칙 그대로).
    // nativeOffset 이 있으면 폭은 같고 그림만 그만큼 아래로 옮긴 사본이다. (2026-11-23 WB04-E1288)
    glyph(code) {
        const cached = this.cache.get(code);
        if (cached) return cached;
        const byte = this.native ? windows1252ByteOf(code) : null;
        let glyph = byte === null ? this._build(code) : this.native.glyph(byte);
        if (byte !== null && this.nativeOffset && glyph.image) glyph = Object.freeze({ ...glyph, image: Object.freeze({ ...glyph.image, top: glyph.image.top + this.nativeOffset }) });
        this.cache.set(code, glyph);
        return glyph;
    }

    // 글자 코드 배열의 측정 폭(전진 폭의 합, 32비트 감김)을 돌려준다. 래스터라이저 글자는 측정 폭과 그리기 전진 폭이 같고, 원본 글리프는 원본 측정 폭(첫 표)을 쓴다.
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
        // `슬롯:스타일:종류` → UnicodeGlyphFont 보관소이다. 종류는 원본 글리프를 원래 기준선에 두는 글꼴(n)과 한글 기준선까지 내리는 큰 슬롯 한글 문구 글꼴(g)이다. (2026-11-23 WB04-E1288: 종류 추가)
        this.fonts = new Map();
    }

    // 슬롯·스타일의 글꼴을 돌려준다. reference 는 같은 슬롯·스타일의 원본 비트맵 글꼴이며 줄 높이·기준선 높이를 맞추고 Windows-1252 글자를 그리는 데 쓴다(없으면 크기에서 추정하고 모든 글자를 래스터라이저로 만든다).
    // extended 는 잴·그릴 문구에 Windows-1252 밖의 글자(한글 등)가 있는지이다. 1배 슬롯은 이 값과 관계없이 원본 글리프와 갈무리11 한글을 섞고(cpppj 와 같음),
    // 2배 이상인 큰 슬롯(제목 등)은 extended 이면 원본 글리프를 한글 기준선까지 내려 한글과 문장 부호의 기준선을 맞추고, 아니면(예: SP 숫자 `2000`) 원본 글리프를 원래 자리에 그려 영어 화면과 같게 한다.
    // (2026-11-23 WB04-E1288: extended 와 원본 글리프 혼합 추가)
    get(slot, style, reference = null, extended = true) {
        const slotHeight = FONT_SLOT_HEIGHTS[slot];
        if (!slotHeight) throw new RangeError(`유니코드 글꼴을 만들 수 없는 슬롯입니다: ${slot}`);
        const emSize = koreanEmSize(slotHeight);
        const whole = extended && koreanScale(emSize) >= 2;
        const key = `${slot}:${style}:${whole ? 'g' : 'n'}`;
        const cached = this.fonts.get(key);
        if (cached) return cached;
        const spec = {
            emSize,
            // 굵은체는 굵게 스타일(2)일 때만 쓴다. 슬롯 0·3 의 굵기 700 은 영어 Arial 의 획을 위한 것이고, 한글을 12~14픽셀 단색으로 그릴 때는
            // 작은 글자의 가독성을 위해 보통체를 쓴다(그림자 패스가 굵게 보이게 한다). (2026-10-31 WB03-N)
            bold: style === STYLE_BOLD,
            italic: style === STYLE_ITALIC,
            underline: style === STYLE_UNDERLINE,
            strikeout: style === STYLE_STRIKEOUT,
        };
        // 한글을 놓는 기준선은 원본 글꼴 기준선과 갈무리11 한글의 기준선 위 높이(배율 × 11줄) 중 큰 값이다. 원본 기준선에 그대로 맞추면 한글 윗부분(ㅈ·ㅎ·ㄹ 의 맨 위 가로획)이
        // 줄 맨 위 밖으로 나가 잘려 `잔`→`산`, `제`→`세` 처럼 읽힌다. 원본 BitmapFont.cpp 도 확장 글자가 기준선 위보다 크면 위쪽을 0 줄에 맞춘다(`shift = max(0, ascent - bearingY*scale)`).
        // 슬롯 0·5(원본 기준선 11)는 그대로이고 슬롯 1·6(원본 기준선 9~10)은 한글만 11 에 놓여 윗줄 잘림이 없어진다. 원본 글리프는 원본 기준선 위치 그대로 그린다.
        // (2026-11-23 WB04-E1259 큰 슬롯 보정, WB04-E1288: 작은 슬롯에도 적용해 슬롯 1·6 의 한글 윗줄 잘림을 고침)
        const ascent = Math.max(reference?.ascent ?? Math.round(emSize * 0.85), koreanAscentRows(emSize));
        const font = new UnicodeGlyphFont({
            rasterizer: this.rasterizer,
            spec,
            height: reference?.height ?? slotHeight,
            ascent,
            // Windows-1252 글자는 항상 원본 글리프를 쓴다. 큰 슬롯의 한글 섞인 문구에서는 원본 글리프를 한글 기준선까지 내려 문장 부호·숫자가 한글과 같은 줄에 앉게 한다
            // (갈무리11 24px 로 영문까지 그리면 폭이 넓어져 `NetStorm 11.01 패치 크레딧` 제목이 두 줄로 넘어갔다). (2026-11-23 WB04-E1288)
            native: reference,
            nativeOffset: whole && reference ? ascent - reference.ascent : 0,
            // 큰 슬롯 한글 문구는 기준선(24px 은 22줄)이 원본 슬롯 높이(제목 20)보다 커서 다음 줄과 겹치므로, 그 기준선에 원본 글꼴의 기준선 아래 높이(Arial 20 은 4줄)를 더한 최소 높이를 잰다.
            // 기준선까지만 잡으면 내린 쉼표 꼬리가 다음 줄 맨 위에 걸쳤다(실제 화면 확인). (2026-11-23 WB04-E1288)
            minimumLineHeight: whole ? ascent + Math.max(0, (reference?.height ?? slotHeight) - (reference?.ascent ?? ascent)) : 0,
        });
        this.fonts.set(key, font);
        return font;
    }
}
