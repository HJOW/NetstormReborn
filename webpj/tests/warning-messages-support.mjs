// WarningMessages 조립 검사가 쓰는 실제 Gump/Text/Style 처리기다. 메뉴 허브는 해당 API를 같은 실제 처리기에 위임하는 검사 어댑터다. (2026-11-14 WB04-E454)
import { scene as textScene } from './text-gump-support.mjs';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { bindStyleTextPalette } from '../www/ui/style-text-fragments.js';
import { setGumpDrawClip } from '../www/render/gump-draw.js';
import { WarningMessages } from '../www/ui/warning-messages.js';

// 같은 창 등록표/파생 공급자/실제 글자 처리기와 명시 벽시각·이름·음향 효과를 구성한다.
export function warningScene(options = {}) {
    const s = textScene();
    // 이전 다중 줄 장면의 두 비표시 기본 노드만 정리해 root 하나에서 시작한다.
    for (const token of [3, 4]) { s.lifetime.destroy(token); s.nodes.delete(token); }
    s.released.length = 0;
    const providers = new Map(); const state = { now: options.now ?? 0, sounds: 0, changed: 0, palettes: [], redraws: [], names: [] }; let next = 100; let messages;
    const originalGet = s.nodes.get.bind(s.nodes);
    // 기본/파생 처리가 실제 Warning 소유자의 같은 주소를 찾게 한다. 노드를 복사하지 않는다.
    s.nodes.get = (token) => originalGet(token) ?? providers.get(2)?.text?.(token);
    const lookup = (token) => s.nodes.get(token);
    const hooks = { node: lookup, redraw: (token, erase) => state.redraws.push([token, erase]), push: (token) => tree.push(token), attach: (...args) => tree.attach(...args), kill: (token) => s.lifetime.kill(token) };
    const tree = new GumpTree(s.create, new GumpTreeState(), hooks); s.lifeHooks.push = (token) => tree.push(token);
    const previousDestructor = s.lifeHooks.deletingDestructor;
    // 실제 Warning 가상 소멸자는 같은 TextGump의 scalar 몸체와 공통 free로 내려간다.
    s.lifeHooks.deletingDestructor = (token, flags) => { if (messages?.node(token)) messages.deleteScalar(token, flags); else previousDestructor(token, flags); };
    s.lifeHooks.release = (token) => { s.released.push(token); if (messages?.node(token)) messages.release(token); else s.nodes.delete(token); };
    const common = { create: s.create, life: s.lifeState, collection: s.collection, tree, lifetime: s.lifetime, token: () => next++, node: lookup, changed: () => { state.changed += 1; } };
    const menus = { textBackend: () => s.single, styleBackend: () => s.style, setExternalGumps: (slot, provider) => providers.set(slot, provider), setPalette: (palette) => { state.palettes.push(palette); bindStyleTextPalette(s.styleHooks, palette); } };
    const context = { challenge: options.challenge ?? (() => 0), playerName: (owner) => state.names.push(owner), translate: options.translate ?? ((text) => text), gameTimeSeconds: () => 123,
        battle: () => 1, playerText: (owner) => Uint8Array.from(Buffer.from(`P${owner}\0`)), challengePlayerText: (owner) => Uint8Array.from(Buffer.from(`C${owner}\0`)) };
    messages = new WarningMessages(common, menus, options.clock ?? (() => state.now), () => { state.sounds += 1; options.onSound?.(); }, context, options.suppressed);
    const palette = { basic: Uint32Array.from({ length: 8 }, (_, index) => 10 + index), named: [{ red: 51, green: 255, blue: 102, index: 31 }] };
    messages.setView(640, 480, palette); setGumpDrawClip(s.clip, { left: 0, top: 0, right: 640, bottom: 480 });
    // 원문 공통 idle 표의 현재 값으로 분배한다. 게임 시각을 대신 읽지 않는다.
    const idle = () => { for (let index = 0; index < common.life.idle.length; index += 1) { const token = common.life.idle[index]; if (token && messages.node(token)) messages.idle(token); } };
    const cleanup = () => s.lifetime.cleanup();
    const clear = () => { s.lifetime.killExceptType(() => -1); cleanup(); };
    // 보고값은 제품의 현재 표/노드에서 읽는다. 검사 전용 별도 표시 목록이 아니다.
    const rows = () => messages.report().split('\n').filter((line) => line.startsWith('warning\t')).map((line) => { const values = line.split('\t'); const serial = Number(values[2]); return { slot: Number(values[1]), serial, x: Number(values[3]), y: Number(values[4]), width: Number(values[5]), height: Number(values[6]), deadline: Number(values[7]), deadlineText: values[7], source: values.slice(8).join('\t'), token: messages.lookup(serial).token }; });
    // 혼합 형제/재진입 검사에서 같은 실제 기본 생성자와 트리를 사용하는 비메시지 Gump를 만든다.
    const sibling = () => { const token = next++; const node = new GumpCreateNode(); s.nodes.set(token, node); s.factory.construct(token, 0, 20, 10, 1); tree.attach(token, 0, 0, s.create.entries[0], 0); return token; };
    return { ...s, common, menus, providers, state, messages, palette, context, tree, lookup, idle, cleanup, clear, rows, sibling };
}
