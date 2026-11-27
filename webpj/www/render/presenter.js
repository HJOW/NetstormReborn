// 8비트 색 번호 표면을 팔레트로 RGB 로 바꿔 게임 Canvas 에 표시하는 모듈이다.
// 원본 DIB 화면(8비트 + 팔레트)의 표시 단계에 해당한다. 논리 크기(예: 1024×768)의 그림을 Canvas 의 기기 픽셀 크기로 늘린다.
// 확대 방식:
//  - 'sharp'(기본): 먼저 정수 배율로 최근접 확대한 뒤 남는 비율만 부드럽게 줄인다. 비정수 배율에서도 픽셀 굵기가 고르고 선명하다.
//  - 'nearest': 최근접 확대만 쓴다(비정수 배율에서 픽셀 굵기가 1~2 기기 픽셀로 들쭉날쭉할 수 있다).
//  - 'smooth': 브라우저의 부드러운 확대만 쓴다.
// 이 모듈은 Canvas 를 쓰므로 브라우저에서만 동작한다(색 변환 함수 convertIndexedToRgba 는 Node 에서도 쓸 수 있다).
// (2026-10-23 WB03-M1: 메인 메뉴를 원본 색 번호 합성으로 표시하기 위해 추가)
import { buildRgbaLookup } from '../assets/palette.js';

// 지원하는 확대 방식 이름 목록이다. 모르는 이름은 'sharp' 로 대체한다. (2026-10-23 WB03-M1)
export const SCALING_MODES = Object.freeze(['sharp', 'nearest', 'smooth']);

// 중간 확대 Canvas 의 정수 배율 상한이다. 아주 큰 화면에서 메모리가 과도해지지 않게 한다. (2026-10-23 WB03-M1)
const MAX_INTEGER_SCALE = 6;

// 배율이 정수와 이 값 이내로 같으면 정수 배율로 보고 최근접 확대만 한다. (2026-10-23 WB03-M1)
const INTEGER_SCALE_EPSILON = 1e-6;

// 색 번호 배열을 RGBA Uint32 배열로 바꾼다. lookup 은 buildRgbaLookup 결과(256개)다.
// 사용: 표시 전 변환, 검사에서 원본 스크린샷과 비교할 RGB 생성. (2026-10-23 WB03-M1)
export function convertIndexedToRgba(pixels, lookup, output) {
    // 각 픽셀의 색 번호를 RGBA 값으로 바꾼다.
    for (let i = 0; i < pixels.length; i += 1) output[i] = lookup[pixels[i]];
    return output;
}

// 바뀐 논리 사각형의 색 번호만 같은 전체 RGBA 예약에 쓴다. 바깥 픽셀은 보존한다.
function convertIndexedRectToRgba(pixels, lookup, output, width, rect) {
    // 각 행의 exclusive 오른쪽 끝까지 현재 팔레트를 적용한다.
    for (let y = rect.top; y < rect.bottom; y += 1) {
        const end = y * width + rect.right;
        // 현재 행에서 바뀐 픽셀에만 같은 팔레트 값을 쓴다.
        for (let index = y * width + rect.left; index < end; index += 1) output[index] = lookup[pixels[index]];
    }
}

// 확대 방식 이름을 지원하는 값으로 바꾼다. (2026-10-23 WB03-M1)
export function normalizeScaling(value) {
    return SCALING_MODES.includes(value) ? value : 'sharp';
}

// 8비트 표면 표시기이다. present(ctx, surface, layout) 를 프레임마다 불러도 바뀐 것이 없으면 변환·중간 확대를 다시 하지 않는다.
// 사용: const presenter = new IndexedPresenter(document); presenter.setPalette(palette.rgb); presenter.present(ctx, surface, layout, true)
// (2026-10-23 WB03-M1)
export class IndexedPresenter {
    // 그림 버퍼 Canvas 를 만들 문서를 받는다.
    constructor(doc, scaling = 'sharp') {
        // Canvas 를 만들 문서다.
        this.doc = doc;
        // 확대 방식이다('sharp' | 'nearest' | 'smooth').
        this.scaling = normalizeScaling(scaling);
        // 논리 크기 그림을 담는 Canvas 다(색 변환 결과).
        this.source = null;
        // source 의 2D 컨텍스트다.
        this.sourceContext = null;
        // source 에 쓸 ImageData 다.
        this.imageData = null;
        // imageData 의 Uint32 보기다.
        this.imageWords = null;
        // 정수 배율 최근접 확대 결과를 담는 Canvas 다('sharp' 방식에서만 쓴다).
        this.scaled = null;
        // scaled 의 2D 컨텍스트다.
        this.scaledContext = null;
        // scaled 에 그린 정수 배율이다. 0 이면 아직 그리지 않았다.
        this.scaledFactor = 0;
        // 현재 팔레트의 RGBA 값 표다.
        this.lookup = null;
        // 다음 present 에서 색 변환을 다시 해야 하는지 여부다.
        this.stale = true;
    }

    // 팔레트(RGB 768바이트)를 바꾼다. 다음 present 에서 화면 전체를 다시 변환한다.
    setPalette(rgb) {
        this.lookup = buildRgbaLookup(rgb);
        this.stale = true;
    }

    // 확대 방식을 바꾼다.
    setScaling(value) {
        this.scaling = normalizeScaling(value);
        this.scaledFactor = 0;
    }

    // 논리 크기에 맞는 그림 버퍼를 준비한다. 크기가 바뀌면 새로 만든다.
    ensureSource(width, height) {
        if (this.source && this.source.width === width && this.source.height === height) return;
        this.source = this.doc.createElement('canvas');
        this.source.width = width;
        this.source.height = height;
        this.sourceContext = this.source.getContext('2d');
        this.imageData = this.sourceContext.createImageData(width, height);
        this.imageWords = new Uint32Array(this.imageData.data.buffer);
        this.scaledFactor = 0;
        this.stale = true;
    }

    // 표면을 Canvas 에 표시한다. changed 가 true 이거나 크기·팔레트가 바뀌었으면 색 변환을 다시 한다.
    // layout 은 netstorm.js computeLayout 결과다. 선택적인 dirtyRect는 같은 논리 화면의 exclusive 변경 사각형이며 생략하면 전체를 갱신한다.
    present(ctx, surface, layout, changed, dirtyRect = null) {
        if (!this.lookup) return;
        this.ensureSource(surface.width, surface.height);
        // 새 크기/팔레트는 전체 변환한다. 기존 네 인자 호출도 전체 갱신 계약을 유지한다.
        const dirty = changed && !this.stale && dirtyRect ? {
            left: Math.max(0, Math.min(surface.width, dirtyRect.left | 0)), top: Math.max(0, Math.min(surface.height, dirtyRect.top | 0)),
            right: Math.max(0, Math.min(surface.width, dirtyRect.right | 0)), bottom: Math.max(0, Math.min(surface.height, dirtyRect.bottom | 0)),
        } : null;
        if (changed || this.stale) {
            if (dirty) {
                if (dirty.right <= dirty.left || dirty.bottom <= dirty.top) return;
                convertIndexedRectToRgba(surface.pixels, this.lookup, this.imageWords, surface.width, dirty);
                this.sourceContext.putImageData(this.imageData, 0, 0, dirty.left, dirty.top, dirty.right - dirty.left, dirty.bottom - dirty.top);
            } else {
                convertIndexedToRgba(surface.pixels, this.lookup, this.imageWords);
                this.sourceContext.putImageData(this.imageData, 0, 0);
            }
            this.stale = false;
            this.scaledFactor = 0;
        }
        const targetWidth = layout.canvasWidth;
        const targetHeight = layout.canvasHeight;
        const scale = Math.min(targetWidth / surface.width, targetHeight / surface.height);
        ctx.setTransform(1, 0, 0, 1, 0, 0);
        const nearestInteger = Math.round(scale);
        const isInteger = nearestInteger >= 1 && Math.abs(scale - nearestInteger) < INTEGER_SCALE_EPSILON;
        if (this.scaling === 'nearest' || (this.scaling === 'sharp' && isInteger)) {
            ctx.imageSmoothingEnabled = false;
            // 두 축이 정확한 정수 배율일 때만 부분 확대한다. 비정수 샘플링 위상은 기존 전체 표시로 보존한다.
            if (dirty && targetWidth === surface.width * nearestInteger && targetHeight === surface.height * nearestInteger) {
                const width = dirty.right - dirty.left, height = dirty.bottom - dirty.top;
                ctx.drawImage(this.source, dirty.left, dirty.top, width, height, dirty.left * nearestInteger, dirty.top * nearestInteger, width * nearestInteger, height * nearestInteger);
            } else ctx.drawImage(this.source, 0, 0, targetWidth, targetHeight);
            return;
        }
        if (this.scaling === 'smooth' || scale < 1) {
            ctx.imageSmoothingEnabled = true;
            ctx.imageSmoothingQuality = 'high';
            ctx.drawImage(this.source, 0, 0, targetWidth, targetHeight);
            return;
        }
        const factor = Math.min(MAX_INTEGER_SCALE, Math.max(1, Math.ceil(scale)));
        if (this.scaledFactor !== factor) {
            if (!this.scaled) {
                this.scaled = this.doc.createElement('canvas');
            }
            this.scaled.width = surface.width * factor;
            this.scaled.height = surface.height * factor;
            this.scaledContext = this.scaled.getContext('2d');
            this.scaledContext.imageSmoothingEnabled = false;
            this.scaledContext.drawImage(this.source, 0, 0, this.scaled.width, this.scaled.height);
            this.scaledFactor = factor;
        }
        ctx.imageSmoothingEnabled = true;
        ctx.imageSmoothingQuality = 'high';
        ctx.drawImage(this.scaled, 0, 0, targetWidth, targetHeight);
    }

    // 만든 Canvas 들을 놓아 메모리를 돌려준다. 이후 present 는 버퍼를 다시 만든다.
    release() {
        this.source = null;
        this.sourceContext = null;
        this.imageData = null;
        this.imageWords = null;
        this.scaled = null;
        this.scaledContext = null;
        this.scaledFactor = 0;
        this.stale = true;
    }
}
