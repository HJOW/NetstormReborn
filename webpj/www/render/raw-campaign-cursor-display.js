// 실제 커서 전역의 kind4 수집/현재 색·글자·SHP를 Renderer dirty 표면에 연결한다(cpppj client/RawCampaignCursorDisplay.cpp/.h).
// 사용: new RawCampaignCursorDisplay(core, source, assets, sp, playerKnownBox). core는 RawSurfaceWorld.core()의 pool/types/spots/processState를 빌린다.
// setPalette는 Screen.palette()의 BGRX1024BYTE 또는 ScreenColor256객체를 받는다. setView는 전체 월드 viewport이며 마지막 dirty clip과 구별한다.
// source/자산/SP/시간/known 상자는 표시기보다 오래 살아야 한다. 실제 앱/Session 수명 조립과 구별한다. (2026-11-14 WB04-E407)
import { PreviewCollection, PreviewCollectionState, makePreviewSpHooks, bindPreviewCollection } from './preview-collection.js';
import { PreviewDraw, PreviewDrawState, PreviewPixelTarget, PreviewColors, makeCursorPreviewAssetShapes } from './preview-draw.js';
import { captureCursorPreviewSource } from './cursor-preview-source.js';
import { collectDrawEntries } from './draw-submission.js';
import { buildStandardOwnerColorMaps, buildSpecialOwnerColorMaps } from '../assets/owner-colors.js';
import { buildPaletteColorTable } from '../core/palette-colors.js';
import { buildPreviewColorMaps } from '../core/preview-color-maps.js';
import { colorSortLogicalInput } from '../core/client-color-sort-input.js';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from './gump-draw.js';
import { GumpTextDraw, GumpGlyphColorState, makeGumpGlyphPixelHooks, decodeGumpTextContext } from './gump-text-draw.js';

// 실제5325cc 번역 키와 최초 일반12/전용5 TYPE 목록이다.
const MESSAGES = Object.freeze(["Priest can't get here.", 'Must be placed over constructing artifact', 'Cannot place here', 'Energy not satisfied', 'Only on Controlled Island']);
const STANDARD = Object.freeze([82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126]);
const SPECIAL = Object.freeze([111, 141, 137, 94, 97]);

// 현재 시각을 원본 float32의 DWORD 비트로 복사한다.
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }

// 화면의 BGRX와 계산기의 논리 RGBX를 같은 색 번호 순서로 연결한다. 예약BYTE도 보존한다.
export function cursorDisplayLogicalPalette(palette) {
    const result = new Uint32Array(256);
    if (palette instanceof Uint8Array && palette.length === 1024) {
        // 실제 Screen.palette의 물리 색 순서를 바꾸지 않는다.
        for (let index = 0; index < 256; index += 1) { const at = index * 4; result[index] = palette[at + 2] | (palette[at + 1] << 8) | (palette[at] << 16) | (palette[at + 3] << 24); }
    } else if (palette?.length === 256 && typeof palette[0] === 'object') {
        // C++ ScreenColor 입력을 받는 독립 검사/호스트용 동일 변환이다.
        for (let index = 0; index < 256; index += 1) { const c = palette[index]; result[index] = c.red | (c.green << 8) | (c.blue << 16) | ((c.reserved ?? 0) << 24); }
    } else throw new TypeError('커서 실제 파일 팔레트256색 계약');
    return result;
}

// 장기 게임 저장소는 빌리고 표시 옵션·피드백·독립 색표만 소유한다.
export class RawCampaignCursorDisplay {
    // 원문 참조 구조체처럼 연결 순간의 참조 대상만 고정한다.
    #core; #source; #assets; #sp; #known;
    // 현재 경고/비용 색과 원본 표시 상태다.
    #textColors = new Uint32Array(2); #feedback = new PreviewCollectionState(); #settings = new PreviewDrawState();
    // 마지막 dirty와 별개인 전체 viewport 및 실제 팔레트 준비/세 색표다.
    #viewport = { left: 0, top: 0, right: 0, bottom: 0 }; #paletteReady = false; #colors = null;
    // 독립 입력의 타입별9표와 실제 Client의 공유 현재 표 소유자다.
    #ownerColors = new Map(); #sharedOwner = null;
    // 현재 부모/확장 색표/실제 장기 글꼴/번역 장치다.
    #depth = null; #specializedOwner = null; #fonts = null; #translate = null;
    // 같은 원본 수집/부분 기록/피드백 본체다.
    #collection;

    // 실제 단일 PATCH 풀·TYPE/spot/시간과 현재 known DWORD 상자를 빌린다.
    constructor(core, source, assets, sp, playerKnown) {
        if (core?.pool !== source?.pool || core?.pool?.edition !== 'patch1078' || assets?.typeTable?.definitions?.length !== 116 || core.types?.length !== 188) throw new TypeError('실제 커서 표시 판본/풀 계약 오류');
        if (!playerKnown || !('value' in playerKnown)) throw new TypeError('커서 현재 PlayerKnown DWORD 상자 필요');
        this.#core = { ...core }; this.#source = captureCursorPreviewSource(core.pool, source, '실제 커서 표시'); this.#assets = assets; this.#sp = sp; this.#known = playerKnown;
        const s = this.#settings; s.ownerMaps = new Array(core.types.length).fill(false); s.overlayTrigger = 99; s.overlayType = 123; s.labelType = 103; s.priceType = source.lifecycle.costType;
        const letters = 'wrtsWRTS.?>';
        // BSS0 문자 표에서 지정11개만 물리 프레임 순서로 쓴다.
        for (let index = 0; index < letters.length; index += 1) s.labelFrames[letters.charCodeAt(index)] = index;
        // 일반 표는 즉시 만들며 전용 표의 실제 명도 계산은 SetPalette까지 기다린다.
        for (const type of STANDARD) { s.ownerMaps[type] = true; this.#ownerColors.set(type, buildStandardOwnerColorMaps(type, source.surfaces.bridgeType)); }
        // 전용5종의 원본 존재 표는 팔레트 전에도 유지한다.
        for (const type of SPECIAL) s.ownerMaps[type] = true;
        const hooks = makePreviewSpHooks(sp, { gumpDepth: (parent) => { if (typeof this.#depth !== 'function') throw new Error('실제 커서 부모 Gump 깊이 미연결'); return this.#depth(parent); } });
        hooks.now = () => floatBits(this.#core.processState.now); hooks.querySp = () => this.#known.value >>> 0;
        hooks.scrambledOffset = () => this.#sp.offset; hooks.copiedOffset = () => this.#sp.trackingOffset >>> 0; hooks.firstPool = () => this.#sp.firstPoolReady; hooks.firstIndex = () => this.#sp.permutation()[0];
        this.#collection = new PreviewCollection(core.pool, core.types, core.spots, this.#feedback, hooks); this.#collection.bindCursor(this.#source);
    }

    // 현재 타입/소유자의 원본 표나 선택된 확장 장치만 반환한다.
    #color(request) {
        if (request.colors === PreviewColors.destroy) return this.#colors.destroy;
        if (request.colors === PreviewColors.illegal) return this.#colors.illegal;
        if (request.colors === PreviewColors.labelShadow) return this.#colors.labelShadow;
        const surfaces = this.#source.surfaces;
        if (this.#sharedOwner || this.#ownerColors.has(request.type)) {
            if (request.owner >= surfaces.ownerColors.length) throw new RangeError('커서 현재 owner 색 번호 범위');
            const color = surfaces.ownerColors[request.owner]; if (color < 0 || color > 8) throw new RangeError('커서 현재 owner 색표 범위');
            return this.#sharedOwner ? this.#sharedOwner.color(request.type, color) : this.#ownerColors.get(request.type)[color];
        }
        if (typeof this.#specializedOwner !== 'function') throw new Error(`커서 확장 소유자 색표 미연결 TYPE${request.type}`);
        return this.#specializedOwner(request.type, request.owner);
    }

    // 번역 전에 현재 문맥을 저장하고 실제 FontSet/글리프/dirty clip으로 출력한다.
    #text(request, pixels, pitch, height, clip) {
        if (!this.#fonts || typeof this.#translate !== 'function') throw new Error('실제 커서 글꼴/번역 미연결');
        const life = this.#source.lifecycle; const context = decodeGumpTextContext(request.index === 5 ? life.costContext : life.warningContext); let text;
        if (request.index === 5) text = life.costText;
        else { if (request.index >= MESSAGES.length) throw new RangeError('커서 경고 문구 번호'); text = this.#translate(MESSAGES[request.index]); }
        const state = new GumpDrawState(); setGumpDrawClip(state, clip); const target = new GumpPixelTarget(pixels, pitch, height, state); const colors = new GumpGlyphColorState();
        new GumpTextDraw(this.#fonts, makeGumpGlyphPixelHooks(target, colors)).draw(this.#fonts.encode(`${text}\0`), request.position, context);
    }

    // 캐시 수집 뒤 삭제/잠금이 바뀐 항목은 실제 raw로 다시 실행하지 않는다.
    #draw(entry, pixels, pitch, height, clip) {
        const source = this.#source; if (source.drop.lockDepth) return;
        if (!this.#paletteReady) throw new Error('실제 커서 파일 팔레트 미설치');
        const token = new DataView(entry.buffer, entry.byteOffset, entry.byteLength).getUint32(8, true); const sid = Math.trunc(token / 50) & 0xffff;
        if (!source.lifecycle.previews.includes(sid)) return;
        const target = new PreviewPixelTarget(pixels, pitch, height, clip, makeCursorPreviewAssetShapes(this.#assets, this.#settings.priceType, source.lifecycle), (request) => this.#color(request), (request) => this.#text(request, pixels, pitch, height, clip));
        const draw = new PreviewDraw(this.#core.pool, this.#core.types, this.#feedback, this.#settings, target.hooks()); draw.bindCursor(source); draw.draw(entry);
    }

    // 파일 팔레트 변경 때 명명색과 현재 세 색표를 갱신하고 독립 전용5표만 다시 만든다.
    setPalette(palette, colorSortLogical = [], colorMaps = null) {
        const logical = cursorDisplayLogicalPalette(palette); const named = buildPaletteColorTable(logical); this.#textColors.set([named.basic[7], named.basic[5]]);
        this.#colors = colorMaps ? { destroy: colorMaps[1].slice(), illegal: colorMaps[2].slice(), labelShadow: colorMaps[5].slice() } : buildPreviewColorMaps(colorSortLogicalInput(colorSortLogical, logical), named.basic[1]);
        this.#settings.goodColor = named.basic[7] & 255; this.#settings.badColor = named.basic[1] & 255;
        if (!this.#sharedOwner) {
            // 실제 Client 공통 표가 없을 때만 지역 명도 표를 갱신한다.
            for (const type of SPECIAL) this.#ownerColors.set(type, buildSpecialOwnerColorMaps(colorSortLogicalInput(colorSortLogical, logical), type));
        }
        this.#paletteReady = true;
    }
    // 전체 월드 clip은 마지막 작은 dirty와 분리해 보존한다.
    setView(view) { this.#viewport = { left: view.left, top: view.top, right: view.right, bottom: view.bottom }; }
    // 현재 배너 표의 주소를 빌리고 독립 캐시는 반납한다.
    bindOwnerColors(owner) { if (!owner?.ready) throw new TypeError('커서 공통 소유자 색표 최초 등록 누락'); this.#sharedOwner = owner; this.#ownerColors.clear(); this.#settings.sharedOwnerMaps = owner.tokens; }
    // 다음 출력부터 현재 장기 글꼴/번역 장치를 사용한다.
    setText(fonts, translate) { this.#fonts = fonts; this.#translate = translate; }
    // 선택된 부모 깊이와 기본17종 외 소유자 색표 장치를 교체한다.
    setExternal(depth, specializedOwner) { this.#depth = depth; this.#specializedOwner = specializedOwner; }
    // 원문 수집은 dirty마다 한 번이며 각 본체/보조 항목을 현재 Renderer 정렬 위치에 제출한다.
    sprites(clip, dirtyFlags) {
        const entries = new Uint8Array(60 * 16); const hooks = {}; bindPreviewCollection(hooks, this.#collection);
        const zero = { left: 0, top: 0, right: 0, bottom: 0 }; const count = collectDrawEntries({ screen: clip, visible: zero, unclipped: zero, dirtyFlags, groupRowLimit: 5000 }, entries, hooks).count; const result = [];
        // 항목 값과 전체 viewport를 복사하고 실제 출력은 해당 dirty 표면에서 수행한다.
        for (let index = 0; index < count; index += 1) {
            const entry = entries.slice(index * 16, (index + 1) * 16); const view = new DataView(entry.buffer); const bounds = { ...this.#viewport };
            result.push({ database: null, clip: { ...bounds }, order: { x: view.getFloat32(0, true), y: view.getFloat32(4, true), depth: view.getInt16(12, true) }, procedure: { bounds, draw: (pixels, pitch, height, cut) => this.#draw(entry, pixels, pitch, height, cut) } });
        }
        return result;
    }
    // 통합 정렬의 같은 작업 공간으로 한 번만 수집한다.
    collectDrawing(view, output, limit) { return this.#collection.collect(view, output, limit); }
    // 별도 스프라이트/두 번째 수집 없이 현재 kind4 항목을 직접 출력한다.
    drawEntry(entry, pixels, pitch, height, clip) { this.#draw(entry, pixels, pitch, height, clip); }
    // Take가 복사할 현재 두 색의 값 사본이다.
    textColors() { return this.#textColors.slice(); }
    // 실제 표시 설정/피드백 쓰기 상태를 같은 주소로 빌린다.
    settings() { return this.#settings; }
    // 원본 강조/현재 남은 횟수다.
    feedback() { return this.#feedback; }
}
