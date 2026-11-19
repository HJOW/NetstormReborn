// 실제 Client 글꼴·StyleText·그림 조각을 다중 줄 버퍼 수명에 연결한다.
import { GumpText } from '../render/gump-text.js';
import { bindStyleTextBufferFonts, bindStyleTextBufferMarkup } from './style-text-buffer.js';
import { StyleTextBufferStore } from './scrolling-text-gump-widgets.js';

// Gump 등록 ID와 구별되는 실제 다중 줄 버퍼 토큰을 소유한다.
export class ClientStyleTextBuffers {
    // 같은 글자 조각 저장소·그림 객체와 실제 버퍼 수명이다.
    #pictures; #store; #tokens = new Set(); #next = 1;
    // 글자 Gump와 같은 FontSet·그림 Markup을 빌려 복제·측정한다.
    constructor(text, fonts, pictures) {
        if (!text?.styleState || !text?.styleEngine || !fonts || !pictures) throw new TypeError('Client 글자 버퍼의 실제 조각·글꼴 누락');
        this.#pictures = pictures; const hooks = {}; bindStyleTextBufferFonts(hooks, new GumpText(fonts)); bindStyleTextBufferMarkup(hooks, pictures);
        this.#store = new StyleTextBufferStore(text.styleState(), text.styleEngine(), hooks, (token) => { if (!this.#tokens.delete(token)) throw new Error('Client 글자 버퍼 중복 반납'); });
    }
    // 실제 파생 그림 조각 생성기를 같은 버퍼 파서에 빌려준다.
    pictures() { return this.#pictures; }
    // 등록 Gump 수와 별개인 새 버퍼를 생성한다.
    create(maxLines) { if (this.#next > 0xffffffff) throw new RangeError('Client 글자 버퍼 토큰 소진'); const token = this.#next++; this.#store.construct(token, maxLines); this.#tokens.add(token); return token; }
    // 현재 실제 버퍼 상태와 처리기를 빌린다.
    binding(token) { return this.#store.binding(token); }
    // 소유 줄·복제 그림과 버퍼를 원본 순서로 반납한다.
    destroy(token) { this.#store.destroy(token); }
}
