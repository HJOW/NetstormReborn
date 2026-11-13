// 원본 Renderer(00497580 Add·00497900 정렬·004994b0 변경 영역 합성·00499fe0 Present)의 장면 합성 본체를 옮긴 모듈이다(cpppj `client/Renderer.cpp/.h` 의 Renderer 클래스·`DrawIndexedImage`·`SortRenderSprites`, 10.78 전용).
// 이미 이식된 조각(변경 사각형 표 `dirty-regions.js`, 색 번호 그림 출력 `indexed-surface.js`, 16바이트 정렬 `draw-sort.js`, 글꼴 글자 출력 `glyph-text.js`, 셰이프 해독 `assets/shapes.js`)을 하나로 묶어
// "정렬된 스프라이트 → 변경 영역마다 배경·월드·미리보기·선택 표시·메뉴·글자·소프트웨어 커서" 순서로 합성하고, 그린 사각형만 Present 로 내보낸 뒤 변경 표를 비운다.
//  - 스프라이트(`RenderSprite` 와 같은 필드의 객체): { database(ShapeDatabase|null), block, frame, x, y, clip, order: { x, y, depth }, colors(Uint8Array 256|null), shadow, followingFrames([]), procedure(null | { bounds, draw(pixels, pitch, height, clip) }), sourceSid }.
//    SHP 스프라이트는 database/block/frame(+followingFrames)으로 그림을 정하고, 직접 그리기(procedure)는 같은 정렬 위치에서 현재 표면에 직접 그린다(입자 본체 `raw-campaign-particles.js` 의 sprites() 가 만드는 항목). 둘은 함께 쓸 수 없다.
//  - 글자(`RenderText`): { font, bytes, x, y, color = 255, shadowColor = 0, shadowOffset: { x, y }, shadow, outline, clip|null }. font 는 `glyph(code)` 를 가진 글꼴이다(`glyph-text.js` 규약).
//  - new Renderer(width, height, edition = 'patch1078'): 첫 화면은 전체 갱신이다. setScene(sprites, text, sort)·setExtraCollection(collect)·setBackground(image)·setOverlay(image)·setOverlayProcedure(procedure)·sceneImage()·
//    invalidateAll()·clearDirty()·invalidate(rect, flags)·fullRedrawPending()·setSoftwareCursor(image, position, force)·cameraRedraw/backgroundRedraw(`{ value }` 상자)·cameraDirtyEntries()·
//    draw(pixels, pitch, background, world)→그린 사각형 목록·present(update)·drawCount/presentCount. world = { collect(clip, flags)→덮임, background(clip, flags, 덮임), submit(pixels, pitch, height, clip)? } 는 실제 Native 프레임이 지형/배경을 같은 dirty 안에서 연결하는 장치이다.
//  - sortRenderSprites(sprites): 원본 16바이트 키(깊이 내림차순, y·x 오름차순)로 제자리 정렬한다(스프라이트를 원래 인덱스와 함께 옮김). drawIndexedImage 는 `indexed-surface.js` 의 같은 함수를 다시 내보낸다.
// 웹 대체: 셰이프 해독 캐시는 `ShapeDatabase.decode` 가 소유하므로 `clearDecodedImages()` 는 비울 렌더러 캐시가 없어 아무 일도 하지 않는다(소유자가 데이터베이스를 바꾸면 그 객체의 캐시가 함께 사라진다).
// 글자는 `drawGlyphText` 에 표면 대역(`drawImage`)을 넘겨 그린다. 웹은 10.78 전용이라 edition 은 'patch1078' 만 받는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E391: 5-A Renderer 본체 이식으로 추가, Renderer.cpp 의 DirtyRegions 는 E59 이식분을 그대로 사용)
import { DirtyRegions } from './dirty-regions.js';
import { clipRect, drawIndexedImage } from './indexed-surface.js';
import { sortDrawEntries } from './draw-sort.js';
import { createTextContext, drawGlyphText } from './glyph-text.js';

export { drawIndexedImage };

// 원본 변경 표 플래그: 버려진 항목(1)과 배경을 다시 지우지 않는 영역(4)이다. (2026-11-13 WB04-E391)
const DISCARDED = 1;
const OPAQUE = 4;
// 32비트 부호 있는 정수 범위이다. 그림 사각형 계산이 이를 넘으면 원본의 감김 대신 거부한다. (2026-11-13 WB04-E391)
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;

// 사각형이 양수 면적을 가지는지 본다. (2026-11-13 WB04-E391)
const nonempty = (rect) => rect.left < rect.right && rect.top < rect.bottom;

// 기준점에 상대 좌표(와 선택 여유)를 더한다. 범위 밖 정수는 감기지 않고 거부한다. (2026-11-13 WB04-E391)
function position(origin, offset, edge = 0) {
    const value = origin + offset + edge;
    if (value < INT_MIN || value > INT_MAX) throw new RangeError('Renderer coordinate overflow');
    return value;
}

// 16바이트 정렬 키로 스프라이트를 제자리 정렬한다: 원래 인덱스를 함께 정렬한 뒤 그 순서로 스프라이트를 옮긴다(DrawSort.cpp SortRenderSprites). (2026-11-13 WB04-E391)
export function sortRenderSprites(sprites) {
    // 원본 draw entry 에 해당하는 키와 이동 전 인덱스만 만든다.
    const entries = sprites.map((sprite, index) => ({ x: sprite.order.x, y: sprite.order.y, depth: sprite.order.depth, index }));
    sortDrawEntries(entries);
    const sorted = entries.map((entry) => sprites[entry.index]);
    // 제자리 정렬이므로 같은 배열에 되돌려 쓴다.
    for (let index = 0; index < sorted.length; index += 1) sprites[index] = sorted[index];
    return sprites;
}

// 이미지가 화면 크기의 올바른 색 번호 그림인지 검사한다(배경/선택 표시). (2026-11-13 WB04-E391)
function checkScreenImage(image, width, height, message) {
    if (image && (image.width !== width || image.height !== height || image.indices.length !== image.width * image.height || image.opacity.length !== image.indices.length)) throw new RangeError(message);
}

// 장면 합성기이다. 입력 스프라이트/그림/글꼴은 이 객체보다 오래 살아야 한다. 사용: const renderer = new Renderer(1024, 768); renderer.setScene(sprites); renderer.draw(pixels, 1024); renderer.present(update). (2026-11-13 WB04-E391)
export class Renderer {
    // 화면 크기와 판본, 변경 사각형 표, 카메라/배경 다시 그리기 요청 상자이다.
    #width;
    #height;
    #dirty;
    #cameraRedraw = { value: 0 };
    #backgroundRedraw = { value: 0 };
    // 정렬된 장면, 정렬 전 원본 제출 순서, dirty 마다 부르는 추가 수집기와 마지막 추가 수집 결과, 글자이다.
    #sprites = [];
    #sourceSprites = [];
    #extraCollection = null;
    #extraCache = [];
    #text = [];
    // 배경·선택 표시 그림, 현재 메뉴 직접 그리기, 이번 Draw 에서 그린 사각형/플래그이다.
    #background = null;
    #overlay = null;
    #overlayProcedure = null;
    #painted = [];
    #paintFlags = [];
    // 소프트웨어 커서 그림과 위치, 누적 그리기/출력 횟수이다.
    #cursor = null;
    #cursorPosition = { x: 0, y: 0 };
    #draws = 0;
    #presents = 0;

    // 화면 크기로 만들고 첫 화면을 전체 갱신으로 시작한다. 웹은 10.78 만 지원한다. (2026-11-13 WB04-E391)
    constructor(width, height, edition = 'patch1078') {
        if (edition !== 'patch1078') throw new RangeError('Renderer 는 10.78(patch1078) 판본만 지원합니다.');
        this.#width = width;
        this.#height = height;
        this.#dirty = new DirtyRegions(width, height);
        this.#dirty.invalidateAll();
    }

    // 같은 객체가 제출한 모든 그림의 합집합(화면 클립 적용)이다. 특수 프레임은 그림 범위가 없다. (2026-11-13 WB04-E391)
    #bounds(sprite) {
        if (sprite.procedure) {
            if (sprite.database || typeof sprite.procedure.draw !== 'function') throw new TypeError('직접 표시 항목의 실제 출력 경계 오류');
            return clipRect(sprite.procedure.bounds, sprite.clip);
        }
        if (!sprite.database) throw new TypeError('Null sprite database');
        if (sprite.block >= sprite.database.blocks.length) throw new RangeError('Invalid sprite block');
        let bounds = null;
        // 각 그림의 압축 영역을 같은 화면 기준점에 옮겨 합친다.
        const include = (number) => {
            const frame = sprite.database.frame(sprite.block, number);
            if (frame.special) return;
            const rect = clipRect({ left: position(sprite.x, frame.rect.left), top: position(sprite.y, frame.rect.top), right: position(sprite.x, frame.rect.right, 1), bottom: position(sprite.y, frame.rect.bottom, 1) }, sprite.clip);
            if (!nonempty(rect)) return;
            bounds = bounds ? { left: Math.min(bounds.left, rect.left), top: Math.min(bounds.top, rect.top), right: Math.max(bounds.right, rect.right), bottom: Math.max(bounds.bottom, rect.bottom) } : rect;
        };
        include(sprite.frame);
        // 레이어를 별도 정렬하지 않고 전체 출력 영역에만 포함한다.
        for (const number of sprite.followingFrames ?? []) include(number);
        return bounds ?? { left: 0, top: 0, right: 0, bottom: 0 };
    }

    // 본 그림과 후속 그림을 하나의 정렬 항목 안에서 같은 위치/색/클립으로 그린다(부분 갱신과 sceneImage 가 같은 순서를 쓴다). (2026-11-13 WB04-E391)
    #drawSprite(pixels, pitch, clip, sprite) {
        const cut = clipRect(clip, sprite.clip);
        if (sprite.procedure) {
            sprite.procedure.draw(pixels, pitch, this.#height, cut);
            return;
        }
        const draw = (number) => {
            const frame = sprite.database.frame(sprite.block, number);
            if (frame.special) return;
            drawIndexedImage(pixels, pitch, this.#height, sprite.database.decode(sprite.block, number), position(sprite.x, frame.rect.left), position(sprite.y, frame.rect.top), cut, sprite.colors ?? null, Boolean(sprite.shadow));
        };
        draw(sprite.frame);
        // 0번 기반 그림 뒤 현재 물결과 상태별 그림을 원본 호출 순서로 덮는다.
        for (const number of sprite.followingFrames ?? []) draw(number);
    }

    // 현재 변경 영역 안에 바이트 글자를 원본 전진 폭으로 그린다(그림자/외곽선/본문 패스). (2026-11-13 WB04-E391)
    #drawText(pixels, pitch, clip, text) {
        const area = text.clip ? clipRect(clip, text.clip) : clip;
        if (!text.font) throw new TypeError('Null bitmap font');
        const surface = {
            bounds: { left: 0, top: 0, right: pitch, bottom: this.#height },
            drawImage: (image, left, top, cut, colorMap) => drawIndexedImage(pixels, pitch, this.#height, image, left, top, cut, colorMap),
        };
        const context = createTextContext({
            flags: (text.shadow ? 2 : 0) | (text.outline ? 4 : 0), color: text.color ?? 255, shadowColor: text.shadowColor ?? 0,
            shadowX: text.shadowOffset?.x ?? 0, shadowY: text.shadowOffset?.y ?? 0,
        });
        drawGlyphText(surface, text.font, text.bytes, text.x, text.y, context, area);
    }

    // 이동 전후 그림을 같은 변경 표에 넣어 사라진 그림의 배경도 복구한다. 정렬은 입력 배열을 바꾸지 않고 사본에 한다. (2026-11-13 WB04-E391)
    setScene(sprites, text = [], sort = true) {
        // 이전 스프라이트의 범위.
        for (const sprite of this.#sprites) this.#dirty.add(this.#bounds(sprite));
        // 새 스프라이트의 범위와 유한 좌표 검사.
        for (const sprite of sprites) {
            if (!Number.isFinite(sprite.order.x) || !Number.isFinite(sprite.order.y)) throw new RangeError('Nonfinite draw order');
            this.#dirty.add(this.#bounds(sprite));
        }
        // dirty 별 재정렬도 같은 원본 입력 순서에서 시작한다.
        const list = sprites.slice();
        this.#sourceSprites = sprites.slice();
        if (sort) sortRenderSprites(list);
        // 글자 이동·삭제는 현재 전체를 무효화한다. 후속 Gump 가 전용 변경 범위를 등록한다.
        if (this.#text.length > 0 || text.length > 0) this.#dirty.invalidateAll();
        this.#sprites = list;
        this.#text = text.slice();
    }

    // 셰이프 해독 캐시는 ShapeDatabase 가 소유하므로 비울 렌더러 캐시가 없다(웹 대체). (2026-11-13 WB04-E391)
    clearDecodedImages() {}

    // 화면 모드 변경·전체 다시 그리기를 요청한다. (2026-11-13 WB04-E391)
    invalidateAll() {
        this.#dirty.invalidateAll();
    }

    // 원문 force redraw 가 기존 100칸을 비운 뒤 전체 사각형을 추가할 때 같은 표를 비운다. (2026-11-13 WB04-E391)
    clearDirty() {
        this.#dirty.clear();
    }

    // dirty 마다 추가 수집(미리보기 등)을 부르는 수집기를 교체한다. 이전 캐시를 비우고 전체 갱신을 요청한다. (2026-11-13 WB04-E391)
    setExtraCollection(collect) {
        if (collect !== null && collect !== undefined && typeof collect !== 'function') throw new TypeError('추가 수집기는 함수여야 합니다.');
        this.#extraCollection = collect ?? null;
        this.#extraCache = [];
        this.#dirty.invalidateAll();
    }

    // 실제 수집기가 연결되어 있는지 본다. (2026-11-13 WB04-E391)
    hasExtraCollection() {
        return this.#extraCollection !== null;
    }

    // 메뉴가 합성한 화면 크기의 8비트 배경을 소유하고 전체 갱신을 요청한다. (2026-11-13 WB04-E391)
    setBackground(image) {
        checkScreenImage(image, this.#width, this.#height, 'Invalid renderer background');
        this.#background = image ?? null;
        this.#dirty.invalidateAll();
    }

    // 선택 표시처럼 스프라이트 위에 오는 투명 UI 픽셀을 소유하고 전체 갱신을 요청한다. (2026-11-13 WB04-E391)
    setOverlay(image) {
        checkScreenImage(image, this.#width, this.#height, 'Invalid renderer overlay');
        this.#overlay = image ?? null;
        this.#dirty.invalidateAll();
    }

    // 현재 메뉴 Gump 를 월드/선택 뒤, 소프트웨어 커서 앞의 같은 dirty clip 에 직접 그린다. 비우면 닫힌 것이다. (2026-11-13 WB04-E391)
    setOverlayProcedure(procedure) {
        if (procedure && typeof procedure.draw !== 'function') throw new TypeError('현재 UI 표면 출력 경계 누락');
        if (!procedure && !this.#overlayProcedure) return;
        this.#overlayProcedure = procedure ?? null;
        this.#dirty.invalidateAll();
    }

    // 정지한 미션 위에 대화상자를 합성할 배경을 만든다: 배경 → 마지막 표시 목록(추가 수집 포함) → 선택 표시. 변경 표·출력 상태·커서·메뉴 글자는 건드리지 않는다. (2026-11-13 WB04-E391)
    sceneImage() {
        const size = this.#width * this.#height;
        const result = { width: this.#width, height: this.#height, indices: new Uint8Array(size), opacity: new Uint8Array(size).fill(255) };
        const all = { left: 0, top: 0, right: this.#width, bottom: this.#height };
        if (this.#background) drawIndexedImage(result.indices, this.#width, this.#height, this.#background, 0, 0, all);
        // 추가 수집의 SP/피드백을 다시 실행하지 않고 마지막 표시 목록만 같은 정렬로 합성한다.
        const combined = (this.#extraCache.length === 0 ? this.#sprites : this.#sourceSprites).slice();
        combined.push(...this.#extraCache);
        if (this.#extraCache.length > 0) sortRenderSprites(combined);
        // Renderer 장면 순서를 보존하며 메뉴 글자와 하드웨어 커서는 합성하지 않는다.
        for (const sprite of combined) {
            const cut = clipRect(all, sprite.clip);
            if (!nonempty(clipRect(this.#bounds(sprite), cut))) continue;
            this.#drawSprite(result.indices, this.#width, cut, sprite);
        }
        if (this.#overlay) drawIndexedImage(result.indices, this.#width, this.#height, this.#overlay, 0, 0, all);
        return result;
    }

    // 오브젝트 측의 변경 알림을 변경 표에 추가한다. (2026-11-13 WB04-E391)
    invalidate(rect, flags = 0) {
        this.#dirty.add(rect, flags);
    }

    // 표시 억제/변경 표 넘침에 대응하는 현재 전체 갱신 상태이다. (2026-11-13 WB04-E391)
    fullRedrawPending() {
        return this.#dirty.fullRedraw;
    }

    // 소프트웨어 커서의 이전·새 범위를 모두 무효화한다. 움직이지 않았으면(force 아님) 새 변경 영역을 만들지 않고, 그림이 null 이면 표시를 지운다. (2026-11-13 WB04-E391)
    setSoftwareCursor(image, at, force = false) {
        if (!force && this.#cursor === image && at.x === this.#cursorPosition.x && at.y === this.#cursorPosition.y) return;
        // 이전·새 프레임의 사각형을 각각 등록한다.
        const invalidate = (item, where) => {
            if (item) this.#dirty.add({ left: where.x, top: where.y, right: position(where.x, item.width), bottom: position(where.y, item.height) });
        };
        invalidate(this.#cursor, this.#cursorPosition);
        invalidate(image, at);
        this.#cursor = image ?? null;
        this.#cursorPosition = { x: at.x, y: at.y };
    }

    // 카메라 이동(Renderer+5c)과 배경 시차(Renderer+60)의 별도 다시 그리기 요청 `{ value }` 상자이다. 부분 Draw 뒤 present 가 소비한다. (2026-11-13 WB04-E391)
    get cameraRedraw() {
        return this.#cameraRedraw;
    }

    get backgroundRedraw() {
        return this.#backgroundRedraw;
    }

    // present 가 비운 뒤 커서가 추가하는 같은 실제 표(살아 있는 배열)이다. clear 뒤에도 배열 자체는 바뀌지 않는다. (2026-11-13 WB04-E391)
    cameraDirtyEntries() {
        return this.#dirty.cameraEntries();
    }

    // 변경 영역마다 배경 → 깊이 순 스프라이트 → 선택 표시 → 메뉴 → 글자 → 커서 순으로 합성한다. 반환 사각형은 present 이전까지 유효하다. (2026-11-13 WB04-E391)
    draw(pixels, pitch, background = 0, world = null) {
        if (this.#painted.length > 0) throw new Error('Present must follow Draw');
        if (pitch < this.#width || pixels.length < pitch * this.#height) throw new RangeError('Invalid renderer surface');
        const whole = { left: 0, top: 0, right: this.#width, bottom: this.#height };
        // force 전체 재합성은 원문처럼 Count0 뒤 flags0 인 실제 한 항목을 만든다.
        if (this.#dirty.fullRedraw) {
            this.#dirty.clear();
            this.#dirty.add({ ...whole });
        }
        // 원문 445290 은 같은 표의 좌표를 제한한다. 빈 유효 항목은 뒤 present 표에 남기되 그리지는 않는다.
        for (const entry of this.#dirty.cameraEntries()) {
            if ((entry.flags & DISCARDED) !== 0) continue;
            entry.rect = clipRect(entry.rect, whole);
            if (nonempty(entry.rect)) {
                this.#painted.push(entry.rect);
                this.#paintFlags.push(entry.flags);
            }
        }
        // 각 변경 영역을 같은 장면에서 다시 합성한다.
        for (let index = 0; index < this.#painted.length; index += 1) {
            const clip = this.#painted[index];
            const flags = this.#paintFlags[index];
            if (world && (!world.collect || !world.background)) throw new Error('Native Renderer의 현재 지형/배경 장치 누락');
            // 실제 지형 수집 뒤 커서 kind4 미리보기를 수집한다.
            const covered = world ? world.collect(clip, flags) : 0;
            if (!world) {
                // 독립 표시의 합성은 추가 수집보다 먼저 배경을 복원한다(불투명 영역은 다시 지우지 않음).
                if ((flags & OPAQUE) === 0) {
                    for (let y = clip.top; y < clip.bottom; y += 1) pixels.fill(background, y * pitch + clip.left, y * pitch + clip.right);
                }
                if (this.#background) drawIndexedImage(pixels, pitch, this.#height, this.#background, 0, 0, clip);
            }
            // 정적 월드와 dirty 별 미리보기를 같은 원본 작업 순서로 병합한다.
            let combined = (this.#extraCollection ? this.#sourceSprites : this.#sprites).slice();
            if (this.#extraCollection && (!world || !world.submit)) {
                const extra = this.#extraCollection(clip, flags);
                if (!(flags & 4)) this.#extraCache = extra;
                combined.push(...extra);
                sortRenderSprites(combined);
            }
            if (world) {
                world.background(clip, flags, covered);
                // UI 전용 4 의 원문은 월드/미리보기 표시를 제출하지 않는다.
                if (flags & OPAQUE) combined = [];
            }
            if (world && world.submit) {
                // 같은 5,000칸의 원문 종류 0..4 를 한 번만 출력한다.
                world.submit(pixels, pitch, this.#height, clip);
                if (this.#cursor) drawIndexedImage(pixels, pitch, this.#height, this.#cursor, this.#cursorPosition.x, this.#cursorPosition.y, clip);
                continue;
            }
            // 원본 깊이 키로 정렬한 모든 월드/미리보기 항목을 현재 dirty 안에서 출력한다.
            for (const sprite of combined) {
                const cut = clipRect(clip, sprite.clip);
                if (!nonempty(clipRect(this.#bounds(sprite), cut))) continue;
                this.#drawSprite(pixels, pitch, cut, sprite);
            }
            if (this.#overlay && (!world || !(flags & OPAQUE))) drawIndexedImage(pixels, pitch, this.#height, this.#overlay, 0, 0, clip);
            if (this.#overlayProcedure) {
                // 현재 dirty 범위 밖의 세계 픽셀을 건드리지 않는다.
                const cut = clipRect(clip, this.#overlayProcedure.bounds);
                if (nonempty(cut)) this.#overlayProcedure.draw(pixels, pitch, this.#height, cut);
            }
            // UI 글자는 지형 위에 낸다. Gump 깊이 통합은 메뉴 복원 단계에서 진행한다.
            for (const text of this.#text) this.#drawText(pixels, pitch, clip, text);
            if (this.#cursor) drawIndexedImage(pixels, pitch, this.#height, this.#cursor, this.#cursorPosition.x, this.#cursorPosition.y, clip);
        }
        if (this.#painted.length > 0) this.#draws += 1;
        return this.#painted;
    }

    // 그린 영역만 내보내고 성공했을 때 변경 표를 비운다. 카메라/배경 다시 그리기 요청이 있으면 전체 한 항목으로 바꾼다(첫 flags 는 보존). (2026-11-13 WB04-E391)
    present(update) {
        if (this.#cameraRedraw.value || this.#backgroundRedraw.value) {
            this.#cameraRedraw.value = 0;
            this.#backgroundRedraw.value = 0;
            const flags = this.#dirty.firstFlags;
            const entries = this.#dirty.cameraEntries();
            entries.length = 0;
            entries.push({ rect: { left: 0, top: 0, right: this.#width, bottom: this.#height }, flags });
        }
        // 원본과 같이 표의 순서로 내보낸다.
        for (const entry of this.#dirty.entries) {
            if (entry.flags & DISCARDED) continue;
            update({ ...entry.rect });
            this.#presents += 1;
        }
        this.#painted = [];
        this.#paintFlags = [];
        this.#dirty.clear();
    }

    // 변경 영역을 그린 프레임 수이다. (2026-11-13 WB04-E391)
    get drawCount() {
        return this.#draws;
    }

    // 내보낸 사각형 수이다. (2026-11-13 WB04-E391)
    get presentCount() {
        return this.#presents;
    }
}
