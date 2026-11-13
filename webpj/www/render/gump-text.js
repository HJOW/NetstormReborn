// 실제 FontSet의 첫 폭 표/논리 슬롯 높이/캐시 메트릭을 원본 측정 부모와 연결한다(cpppj client/GumpText.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E414)
import { logicalHeightOfSlot } from '../assets/font-set.js';
import { measureGumpText } from '../core/gump-text-measure.js';
import { decodeGumpTextContext } from './gump-text-draw.js';

// 같은 실제 글꼴 저장소를 빌린다. 사용: new GumpText(fonts).measure(bytes,context,out?,length=-1).
export class GumpText {
    // 장면보다 오래 사는 실제 FontSet이다.
    #fonts;
    // 생성 자체에서는 글꼴을 만들거나 측정하지 않는다.
    constructor(fonts) { this.#fonts = fonts; }
    // 높이는 요청 슬롯을 먼저 검사하고 불법 스타일의 폭만 슬롯0 normal로 바꾼다.
    measure(bytes, context, output = { width: 0, height: 0 }, length = -1) {
        const height = logicalHeightOfSlot(context.font | 0); const font = this.#fonts.measureFont({ ...context, font: context.font >>> 0, style: context.style >>> 0 }, bytes);
        if (bytes instanceof Uint16Array) return measureGumpText(bytes, (code) => font ? font.glyph(code).advance : 0, height, context, output, length);
        const widths = new Int32Array(256);
        // 캐시 첫 폭 표만 사용하며 그리기 전진 폭/ABC A와 섞지 않는다.
        if (font) for (let i = 0; i < 256; i += 1) widths[i] = font.glyph(i).advance;
        return measureGumpText(bytes, widths, height, context, output, length);
    }
    // 불법 스타일은 슬롯0 메트릭을 쓰며 논리 높이/그림자 여백을 더하지 않는다.
    metrics(context) { const legal = (context.style >>> 0) <= 4; const selected = { ...context, font: legal ? context.font : 0, style: legal ? context.style : 0 }; logicalHeightOfSlot(selected.font | 0); const font = this.#fonts.measureFont(selected); return font ? [font.height | 0, font.ascent | 0, font.descent | 0] : [0, 0, 0]; }
}

// 돌 버튼의 기본8DWORD 문맥과 명시 길이를 같은 측정기에 전달한다. 버튼의 정상/눌림 Draw 문맥과 구별한다.
export function bindStoneButtonText(hooks, text) { hooks.measure = (token, label, context, output, length) => { void token; return text.measure(label, decodeGumpTextContext(context), output, length | 0); }; }
