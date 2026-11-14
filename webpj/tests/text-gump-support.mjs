// TextGump/ScrollingTextGump/ScrollingTextGumpMarkup 시험이 공유하는 실제 Gump 장치 장면이다(글자 폭 1·줄 높이 10+글꼴·상승 8+글꼴인 독립 글꼴 장치). (2026-11-14 WB04-E424: E423 시험에서 분리)
import { TextGump, TextGumpNode, TextGumpState, ScrollingTextGump, ScrollingTextGumpNode, bindTextGumpLifetime } from '../www/ui/text-gump.js';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpDrawState } from '../www/render/gump-draw.js';
import { StyleText, StyleTextState, StyleTextFragmentKind as K } from '../www/ui/style-text-fragments.js';
import { StyleTextBuffer, StyleTextBufferState, bindStyleTextBufferMarkup } from '../www/ui/style-text-buffer.js';
import { Markup, MarkupState, bindMarkup } from '../www/ui/markup-picture.js';

// 문자열은 바이트 하나가 문자 하나이며 NUL 로 끝낸다. 8DWORD 문맥은 글꼴 번호만 context[1] 에 둔다.
export const bytes = (text) => Uint8Array.from(Buffer.from(text + '\0', 'latin1'));
export const words = (font = 0, color = 0) => Uint32Array.from([0, font, color, 0, 0, 0, 0, 0]);
export const textOf = (node) => Buffer.from(node.text.subarray(0, node.text.indexOf(0))).toString('latin1');

// 글자 폭 1·높이 10+글꼴·상승 8+글꼴 인 독립 글꼴 장치로 한 줄/다중 줄 처리기를 같은 안정 노드에 묶은 장면이다.
export function scene(options = {}) {
    const create = new GumpCreateState(); create.screenWidth = 640; create.screenHeight = 480;
    const nodes = new Map([[1, new GumpCreateNode()], [2, new TextGumpNode()], [3, new ScrollingTextGumpNode()], [4, new GumpCreateNode()]]); const lookup = (token) => nodes.get(token);
    const factory = new GumpCreate(create, { node: lookup, allocate: () => 1 }); factory.initializeRoot();
    const fragments = new StyleTextState(); const events = []; const draws = []; const scaleOnMeasure = { value: null };
    const measure = (text, context, length) => { const nul = text.indexOf(0); const limit = nul < 0 ? text.length : nul; return [length < 0 ? limit : Math.min(length, limit), 10 + context[1]]; };
    const styleHooks = { color: (command) => command.charCodeAt(0), textMeasure: (text, context, output) => { const size = measure(text, context, -1); output.width = size[0]; output.height = size[1]; if (scaleOnMeasure.value !== null) nodes.get(2).scale = scaleOnMeasure.value; }, textMetrics: (context) => [10 + context[1], 8 + context[1]], textDraw: (text, position, context) => draws.push([Buffer.from(text.subarray(0, text.indexOf(0))).toString('latin1'), { ...position }, context[1]]), release: (token) => events.push(['release', token]) };
    const pictureState = new MarkupState(); pictureState.typeCount = 200; const pictures = new Markup(fragments, pictureState, { type: () => ({ token: 50, shape: 500, frameCount: 100, hotspotX: 0, hotspotY: 0 }), frame: () => ({ width: 30, height: 12, hotspotX: 0, hotspotY: 0, extraX: 0, extraY: 0 }) }); bindMarkup(styleHooks, pictures);
    const style = new StyleText(fragments, styleHooks);
    const lifeState = new GumpLifetimeState(); const released = []; const lifeHooks = { node: lookup, release: (token) => released.push(token) };
    const lifetime = new GumpLifetime(create, lifeState, lifeHooks); lifeHooks.deletingDestructor = (token, flags) => lifetime.deleteScalar(token, flags);
    const collection = new GumpCollection(create.collection, { node: lookup }); const clip = new GumpDrawState(); const textState = new TextGumpState();
    const single = new TextGump(create, textState, factory, lifetime, collection, clip, style, { node: lookup, baseNode: lookup });
    // 연결 버퍼는 실제 StyleTextBuffer 이며 호출 기록을 남기는 얇은 대리 객체로 감싼다.
    const bufferState = new StyleTextBufferState(); const bufferHooks = { measureText: (text, context, length) => measure(text, context, length) }; bindStyleTextBufferMarkup(bufferHooks, pictures);
    const real = new StyleTextBuffer(bufferState, fragments, style, options.maxLines ?? 16, bufferHooks); const calls = [];
    const handler = { layout: (width) => { calls.push(['layout', width]); real.layout(width); }, totalHeight: () => real.totalHeight(), maximumWidth: () => real.maximumWidth(), appendText: (...args) => { calls.push(['append', args[2], args[3]]); real.appendText(...args); }, appendPicture: (...args) => { calls.push(['picture', args[3], args[4], args[5]]); real.appendPicture(...args); }, removeLine: (index) => { calls.push(['remove', index]); real.removeLine(index); } };
    const slider = []; const sliderState = { position: 0, override: null };
    const scrollHooks = { node: lookup, baseNode: lookup, buffer: (token) => { calls.push(['buffer', token]); return { state: bufferState, handler }; },
        sliderRange: (token, minimum, maximum) => slider.push(['range', token, minimum, maximum]), sliderPage: (token, page) => slider.push(['page', token, page]), sliderSetPosition: (token, position) => { slider.push(['set', token, position]); sliderState.position = position; }, sliderPosition: (token) => { slider.push(['get', token]); return sliderState.override ?? sliderState.position; }, redraw: (token, erase) => slider.push(['redraw', token, erase]) }; for (const key of options.omit ?? []) delete scrollHooks[key]; const scrolling = new ScrollingTextGump(create, collection, clip, fragments, style, scrollHooks);
    bindTextGumpLifetime(lifeHooks, (token) => token === 2 ? single : null);
    // 노드 2(한 줄)·3(다중 줄)·4(slider)를 실제 기본 생성으로 등록하고 루트(1) 아래 둔다.
    const root = nodes.get(1); void root; for (const [token, width, height] of [[3, 40, 25], [4, 9, 25]]) { factory.construct(token, 0, width, height, 1); nodes.get(token).parent = 1; }
    const multi = nodes.get(3); multi.x = 5; multi.y = 7; multi.slider = 4; multi.buffer = 9;
    return { create, nodes, factory, fragments, events, draws, style, styleHooks, lifetime, lifeState, lifeHooks, released, collection, clip, textState, single, scrolling, bufferState, real, handler, calls, slider, sliderState, pictures, pictureState, scaleOnMeasure, multi, slide: nodes.get(4), text: nodes.get(2) };
}
// 한 줄 노드(2)를 만든다.
export function made(s, width = 33, height = 17, font = 3, color = 9) { s.single.construct(2, width, height, font, color); s.text.parent = 1; s.text.x = 10; s.text.y = 20; return s.text; }
// 소유 체인의 문자열 목록이다.
export function chain(s, head) { const out = []; for (let token = head; token; token = s.fragments.fragments.get(token).next) { const node = s.fragments.fragments.get(token); out.push(node.kind === K.TEXT ? textOf(node) : '[pic]'); } return out; }
// 표시 줄 요약(센티널 순서)이다.
export function soft(s) { const out = []; for (let token = s.bufferState.sentinel.next; token; token = s.bufferState.softLines.get(token).next) { const line = s.bufferState.softLines.get(token); out.push({ token, y: line.y, height: line.height, width: line.width, pieces: chain(s, line.head), head: line.head }); } return out; }

// 표시 줄이 만들어진 다중 줄 장면이다(글자 폭 1·줄 높이 10: aaa/bbbb/ccccc 가 y 0/10/20, 전체 높이 30, 보기 40×25, 위치 5,7).
export function filled(s, text = 'aaa\nbbbb\nccccc\n') { s.scrolling.appendTextDefault(3, bytes(text)); s.scrolling.ensureLayout(3); s.calls.length = 0; s.slider.length = 0; return s; }
// 기록에서 한 종류만 거른다.
export const only = (log, name) => log.filter((entry) => entry[0] === name);

