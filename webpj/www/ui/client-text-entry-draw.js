// TextEntry의 실제 배경·선택 사각형과 혼합 글꼴 글리프를 같은 Gump 픽셀 장치에 출력한다.
// 사용: createClientTextOutput(owner, fonts, backgroundDraw, target, colors)를 ClientTextEntryControls 장치에 합친다.
import { TextEntryGumpNode } from './text-entry-gump.js';
import { GumpTextDraw, decodeGumpTextContext, makeGumpGlyphPixelHooks } from '../render/gump-text-draw.js';
import { drawBitmapGlyphText } from '../render/glyph-text.js';
import { logicalHeightOfSlot } from '../assets/font-set.js';
import { displayTextCodec } from '../assets/display-text.js';
import { hasExtendedDisplayText } from '../core/display-text-input.js';

// 현재 같은 등록표와 글꼴·배경·표면·색표를 빌린다.
export function createClientTextOutput(owner, fonts, background, target, colors) {
    if (!owner || typeof fonts?.drawFont !== 'function' || typeof background?.draw !== 'function' || typeof target?.fill !== 'function' || typeof target?.image !== 'function' || !colors) throw new TypeError('TextEntry 실제 출력 장치 필요');
    const glyphHooks = makeGumpGlyphPixelHooks(target, colors), text = new GumpTextDraw(fonts, glyphHooks);
    return {
        // 현재 파생 배경을 같은 실제 배경 처리기로 그린다.
        drawBackground: (token) => background.draw(token),
        // 현재 기본 또는 선택 문맥과 같은 Gump 상대 위치를 사용한다.
        drawText: (token, bytes, offset, explicit) => {
            const node = owner.node(token); if (!(node instanceof TextEntryGumpNode)) throw new Error('TextEntry 출력 노드 없음');
            if (!(bytes instanceof Uint8Array)) throw new TypeError('TextEntry 출력 바이트 필요');
            const end = bytes.indexOf(0); if (end < 0) throw new TypeError('Gump 글자 출력 범위에 NUL 종료 필요');
            const visible = bytes.subarray(0, end), context = decodeGumpTextContext(explicit ?? node.textContext);
            if (!hasExtendedDisplayText(visible)) { text.drawGump(owner.collection(), token, bytes, offset, node.textContext, context); return; }
            logicalHeightOfSlot(context.font | 0);
            const nativeFont = fonts.drawFont(context, bytes), unicodeFont = fonts.drawFont(context, new Uint16Array(0));
            const glyphs = [];
            // 원본 바이트와 네 바이트 확장 문자를 각각 실제 글꼴의 한 글리프로 연결한다.
            for (let at = 0; at < visible.length;) {
                const part = visible.subarray(at, at + 4);
                if (part.length === 4 && hasExtendedDisplayText(part)) { glyphs.push([unicodeFont, displayTextCodec.decode(part).charCodeAt(0)]); at += 4; }
                else { glyphs.push([nativeFont, visible[at]]); at += 1; }
            }
            if (glyphs.length > 65535) throw new RangeError('TextEntry 혼합 글리프 수 범위 초과');
            const indices = Uint16Array.from(glyphs, (_, index) => index), absent = { present: false };
            const proxy = { glyph: (index) => { const [font, code] = glyphs[index]; return font ? font.glyph(code) : absent; } };
            const point = owner.collection().position(token);
            drawBitmapGlyphText(indices, proxy, { x: (point.x + offset.x) | 0, y: (point.y + offset.y) | 0 }, context, glyphHooks, indices.length);
        },
        // 선택과 캐럿은 현재 클립을 공유하는 픽셀 표면에 채운다.
        fill: (rect, color) => target.fill(rect, color),
    };
}
