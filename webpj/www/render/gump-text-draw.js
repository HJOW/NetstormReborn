// 원본 Gump 글꼴 선택/기본 문맥/현재 위치와 실제 glyph→GumpPixelTarget을 연결한다(cpppj client/GumpTextDraw.cpp/.h).
// 사용: new GumpTextDraw(fonts, makeGumpGlyphPixelHooks(target, colors)).draw(bytesWithNul, point, context).
// 10.78 전용이다. 원문의4BYTE 한글 확장은 기존 웹 정책대로 UTF-16/D2Coding 입력으로 대체한다. (2026-11-14 WB04-E406)
import { logicalHeightOfSlot } from '../assets/font-set.js';
import { drawBitmapGlyphText } from './glyph-text.js';

// 실제32BYTE 문맥의8DWORD를 Position 경계 뒤 현재 값으로 해석한다.
export function decodeGumpTextContext(words) {
    if (words?.length !== 8) throw new TypeError('Gump 글자 문맥8DWORD 필요');
    return { flags: words[0] >>> 0, font: words[1] >>> 0, color: words[2] >>> 0, style: words[3] >>> 0, extra: words[4] >>> 0, shadowColor: words[5] >>> 0, shadowX: words[6] | 0, shadowY: words[7] | 0 };
}

// 현재 원본 비트맵/웹 유니코드 저장소와 같은 출력 장치를 빌린다.
export class GumpTextDraw {
    // 장기 글꼴과 실제 색/글리프 효과다.
    #fonts; #hooks;
    // 빈 문자열에서도 사용하는 경계를 생성할 때 모두 확인한다.
    constructor(fonts, hooks) { if (typeof hooks?.color !== 'function' || typeof hooks?.glyph !== 'function') throw new TypeError('Gump 색/글리프 출력 경계 필요'); this.#fonts = fonts; this.#hooks = hooks; }
    // 슬롯 assert를 스타일 fallback보다 먼저 검사하며 불법 스타일 로그는 선택 사항이다.
    draw(bytes, position, context, length = -1) {
        let slot = context.font | 0; logicalHeightOfSlot(slot); let style = context.style >>> 0;
        if (style > 4) { if (this.#hooks.illegalStyle) this.#hooks.illegalStyle(); slot = 0; style = 0; }
        const font = this.#fonts.drawFont({ ...context, font: slot, style }, bytes);
        drawBitmapGlyphText(bytes, font, position, context, this.#hooks, length);
    }
    // 실제 Position 투영/캐시 뒤 기본 문맥을 해석하고 상대 오프셋을 DWORD 덧셈한다.
    drawGump(collection, token, bytes, offset, defaultContext, context = null) {
        const position = collection.position(token); const selected = context ?? decodeGumpTextContext(defaultContext);
        this.draw(bytes, { x: (position.x + offset.x) | 0, y: (position.y + offset.y) | 0 }, selected);
    }
}

// 원본 글꼴 표와 마지막 VFX 설치 표를 같은 주소로 소유한다.
export class GumpGlyphColorState {
    // 색100만 패스마다 바꾸며 나머지255칸은 기존 상태를 유지한다.
    fontColors = new Uint8Array(256); vfxColors = new Uint8Array(256);
}

// 실제 글리프 그림은 상대 프레임 위치/현재 클립/투명도를 같은 픽셀 장치에 넘긴다.
export function makeGumpGlyphPixelHooks(target, state) {
    return {
        color: (color) => { state.fontColors[100] = color; state.vfxColors.set(state.fontColors); },
        // 특별 빈 프레임도 호출/폭은 유지하지만 픽셀은 출력하지 않는다.
        glyph: (font, code, at) => { const image = font.glyph(code).image; if (image) target.image(image, { x: (at.x + image.left) | 0, y: (at.y + image.top) | 0 }, state.vfxColors); },
    };
}
