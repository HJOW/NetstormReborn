// 원본 글꼴 슬롯(0~6)과 스타일(보통·기울임·굵게·취소선·밑줄)을 비트맵 글꼴 캐시(.chfnt) 파일에 대응시키는 모듈이다.
// 원본 FontStore(cpppj BitmapFont.cpp FontStore::Get)의 파일 이름 규칙 `d/!<글꼴>.<스타일>.<높이>.<굵기>.chfnt` 를 따른다.
// 슬롯 1 만 `Courier New`, 나머지는 `Arial` 이고, 슬롯 1·3·4 는 보통 스타일만 존재하며 슬롯 2 는 원본에서도 비어 있다.
// 글자 측정(원본 GumpText::Measure)과 글자 그리기(GumpTextDraw::Draw)가 쓰는 슬롯·스타일 선택 규칙도 여기에 모은다.
// (2026-10-28 WB03-D: Tell 대화상자의 제목(슬롯 3)·본문(슬롯 5)·굵게/기울임 스타일을 그리기 위해 추가)
import { BitmapFont, FONT_SLOT_HEIGHTS } from './bitmap-font.js';

// 원본 스타일 번호(0~4)에 대응하는 파일 이름 조각이다. 순서는 원본 kStyles 와 같다. (2026-10-28 WB03-D)
export const FONT_STYLE_NAMES = Object.freeze(['normal', 'italic', 'bold', 'strikeout', 'underline']);

// 슬롯별 CreateFontA 굵기이다(파일 이름의 마지막 숫자). 원본 kWeights 와 같다. (2026-10-28 WB03-D)
const FONT_SLOT_WEIGHTS = Object.freeze([700, 0, 0, 700, 0, 0, 0]);

// 10.78 에서 유효한 가장 큰 스타일 번호이다(CD 10.72 는 2). 이보다 큰 스타일은 불법이라 슬롯 0 보통으로 바꿔 쓴다. (2026-10-28 WB03-D)
const MAX_STYLE = 4;

// 슬롯·스타일 조합이 원본에 존재하는지 검사한다(원본 FontStore::Available). 슬롯 2 는 비어 있고 슬롯 1·3·4 는 보통 스타일만 있다.
// 사용: FontSet.isAvailable(5, 2) → true(굵게 14), FontSet.isAvailable(3, 2) → false. (2026-10-28 WB03-D)
export function isFontAvailable(slot, style) {
    return slot >= 0 && slot < 7 && slot !== 2 && style >= 0 && style < FONT_STYLE_NAMES.length && ((slot !== 1 && slot !== 3 && slot !== 4) || style === 0);
}

// 슬롯·스타일의 글꼴 캐시 파일 경로(데이터 폴더 기준)를 만든다. 조합이 없으면 null 이다. (2026-10-28 WB03-D)
export function fontCachePath(slot, style) {
    if (!isFontAvailable(slot, style)) return null;
    const face = slot === 1 ? 'Courier New' : 'Arial';
    return `d/!${face}.${FONT_STYLE_NAMES[style]}.${FONT_SLOT_HEIGHTS[slot]}.${FONT_SLOT_WEIGHTS[slot]}.chfnt`;
}

// 원본에 존재하는 모든 글꼴 캐시 파일 경로 목록이다(슬롯 0~6 × 스타일 0~4 중 유효한 조합). 자산 준비 도구가 쓴다. (2026-10-28 WB03-D)
export const ALL_FONT_PATHS = Object.freeze(
    (() => {
        const paths = [];
        // 슬롯마다 스타일을 훑어 존재하는 조합의 경로만 모은다.
        for (let slot = 0; slot < 7; slot += 1) {
            for (let style = 0; style < FONT_STYLE_NAMES.length; style += 1) {
                const path = fontCachePath(slot, style);
                if (path) paths.push(path);
            }
        }
        return paths;
    })(),
);

// 글자 문맥(createTextContext 결과)이 가리키는 글꼴 슬롯의 논리 높이를 돌려준다. 슬롯이 범위 밖이면 RangeError 다(원본은 assert). (2026-10-28 WB03-D)
export function logicalHeightOfSlot(slot) {
    if (!Number.isInteger(slot) || slot < 0 || slot >= FONT_SLOT_HEIGHTS.length) throw new RangeError(`글꼴 슬롯 범위 오류: ${slot}`);
    return FONT_SLOT_HEIGHTS[slot];
}

// 글꼴 캐시 파일 바이트(경로 → Uint8Array)로 슬롯·스타일별 글꼴을 지연 해독해 보관하는 묶음이다.
// 사용: const fonts = new FontSet(files); fonts.measure(bytes, context) → { width, height }, fonts.drawFont(context) → BitmapFont|null.
// 필요한 파일이 files 에 없으면 어떤 파일이 빠졌는지 알리는 오류를 던진다(자산 묶음 누락을 조기에 드러내기 위해). (2026-10-28 WB03-D)
export class FontSet {
    // 파일 Map(경로 → Uint8Array)을 받는다. 글꼴은 처음 쓸 때 해독한다.
    constructor(files) {
        // 글꼴 캐시 파일 바이트를 경로로 찾는 Map 이다.
        this.files = files;
        // 해독한 글꼴 캐시다. 키는 `슬롯:스타일` 이다.
        this.fonts = new Map();
    }

    // 슬롯·스타일의 글꼴을 돌려준다(없는 조합은 호출 전에 isFontAvailable 로 걸러야 한다). 처음 한 번만 해독한다.
    get(slot, style) {
        const key = `${slot}:${style}`;
        const cached = this.fonts.get(key);
        if (cached) return cached;
        const path = fontCachePath(slot, style);
        if (!path) throw new RangeError(`원본에 없는 글꼴 조합입니다: 슬롯 ${slot}, 스타일 ${style}`);
        const bytes = this.files.get(path);
        if (!bytes) throw new Error(`필수 자산이 없습니다: ${path}`);
        const font = BitmapFont.parse(bytes);
        this.fonts.set(key, font);
        return font;
    }

    // 글자 측정에 쓰는 글꼴을 고른다(원본 GumpText::Measure). 스타일이 불법이면 슬롯 0 보통으로 대체하고, 없는 조합은 null(폭 0)이다.
    measureFont(context) {
        const legal = context.style <= MAX_STYLE;
        const slot = legal ? context.font : 0;
        const style = legal ? context.style : 0;
        return isFontAvailable(slot, style) ? this.get(slot, style) : null;
    }

    // 글자 그리기에 쓰는 글꼴을 고른다(원본 GumpTextDraw::Draw). 규칙은 측정과 같다. 없는 조합은 null 이라 아무것도 그리지 않는다.
    drawFont(context) {
        return this.measureFont(context);
    }

    // 바이트 문자열의 폭·높이를 잰다(원본 GumpText::Measure). 폭은 글꼴의 측정 폭(첫 표) 합, 높이는 슬롯의 논리 높이이며,
    // 문맥 플래그 2(그림자)가 있으면 그림자 오프셋을 각각 더한다. 정수는 32비트로 감긴다.
    measure(bytes, context) {
        const font = this.measureFont(context);
        let width = font ? font.measure(bytes) : 0;
        let height = logicalHeightOfSlot(context.font);
        if (context.flags & 2) {
            width = (width + context.shadowX) | 0;
            height = (height + context.shadowY) | 0;
        }
        return { width, height };
    }

    // 문맥의 캐시 글꼴 높이와 기준선 위쪽 높이를 돌려준다(원본 GumpText::Metrics). 글꼴이 없으면 모두 0 이다.
    // 줄 배치에서 조각을 같은 기준선에 맞출 때 ascent 를 쓴다.
    metrics(context) {
        logicalHeightOfSlot(context.font);
        const font = this.measureFont(context);
        return font ? { height: font.height, ascent: font.ascent } : { height: 0, ascent: 0 };
    }
}
