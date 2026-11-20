// 미션 F6 지식 창 연결(ClientKnowledgeMenu)의 열기 관문·장치 전달·열림 판정·닫기를 가짜 Session·작성 장치로 검사한다. 게임을 실행하지 않는다.
// 근거: cpppj `RawMenuControls::OpenKnowledge`/`KnowledgeOpen`(PlayerKnown·메타맵 관문, 첫 카드 직전 팝업 정리, 살아 있는 팝업만 열림으로 셈). (2026-11-20 WB04-E1088)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientKnowledgeMenu } from '../www/ui/client-knowledge-menu.js';
import { KnowledgeMenuKind } from '../www/core/raw-knowledge-menu.js';

// 호출 기록을 남기는 가짜 Session 과 작성 장치를 만든다. 노드 표는 토큰→{flags,parent}이다.
function scene({ available = true, tokens = [7, 8] } = {}) {
    const log = { shown: [], closed: 0, marks: [], stored: [] }, nodes = new Map();
    const types = [{ description: 'first' }, { description: 'second' }];
    const session = { knowledgeAvailable: () => available, core: () => ({ types }),
        showKnowledge: (state, hooks, kind) => { log.shown.push({ state: { ...state }, hooks, kind }); return tokens; } };
    const writer = { knowledgeWriter: (store) => { log.stored.push(store('themeHelp')); return { begin: () => 1, item: () => {} }; }, closePopups: () => { log.closed += 1; }, root: () => 3, nodeOf: (token) => nodes.get(token) ?? null };
    const devices = { translate: (text) => `T(${text})`, mark: (command) => log.marks.push(command), width: () => 1024, height: () => 768, language: () => 7, storeText: (value) => value.length };
    return { log, nodes, menu: new ClientKnowledgeMenu(session, writer, devices) };
}

test('지식 창 열기: 알려진 Player 의 Storm 카드는 화면·언어 상태와 번역·마크 장치를 Session 에 전달한다', () => {
    const { log, menu } = scene();
    assert.equal(menu.open(), true);
    assert.equal(log.shown.length, 1);
    const call = log.shown[0];
    assert.equal(call.kind, KnowledgeMenuKind.Storm); assert.equal(call.state.width, 1024); assert.equal(call.state.height, 768); assert.equal(call.state.language, 7);
    assert.equal(call.state.iconWidth, 65); assert.equal(call.state.iconHeight, 60); assert.equal(call.state.helpCommand, 0x02000017);
    // 작성 장치 hooks 에 번역·타입 설명·팝업 표식이 더해진다.
    assert.equal(call.hooks.translate('Wind'), 'T(Wind)'); assert.equal(call.hooks.description(1), 'T(second)');
    call.hooks.mark(0x020000a0); assert.deepEqual(log.marks, [0x020000a0]); assert.equal(call.hooks.begin(), 1);
    // 정적 문자열 저장 장치를 작성기에 그대로 전달한다.
    assert.deepEqual(log.stored, ['themeHelp'.length]);
    assert.deepEqual(menu.tokens(), [7, 8]);
});

test('지식 창 열기: World·선택 메뉴 종류를 그대로 전달하고 카드가 없으면 false 이다', () => {
    const world = scene(); assert.equal(world.menu.open(KnowledgeMenuKind.World), true); assert.equal(world.log.shown[0].kind, KnowledgeMenuKind.World);
    const side = scene(); assert.equal(side.menu.open(KnowledgeMenuKind.Side), true); assert.equal(side.log.shown[0].kind, KnowledgeMenuKind.Side);
    const none = scene({ tokens: [] }); assert.equal(none.menu.open(), false); assert.deepEqual(none.menu.tokens(), []);
});

test('지식 창 열기: Player 가 알려지지 않았거나 메타맵이면(knowledgeAvailable false) 만들지 않는다', () => {
    const { log, menu } = scene({ available: false });
    assert.equal(menu.open(), false); assert.deepEqual(log.shown, []); assert.deepEqual(log.stored, []);
});

test('지식 창 열림 판정: 표시 중이고 삭제 대기가 아닌 루트 직속 카드가 하나라도 있어야 열림이다', () => {
    const { nodes, menu } = scene(); menu.open();
    assert.equal(menu.isOpen(), false);
    nodes.set(7, { flags: 0x1, parent: 3 }); assert.equal(menu.isOpen(), true);
    // 삭제 대기·숨김·다른 부모 카드는 세지 않는다.
    nodes.set(7, { flags: 0x10001, parent: 3 }); assert.equal(menu.isOpen(), false);
    nodes.set(7, { flags: 0x0, parent: 3 }); assert.equal(menu.isOpen(), false);
    nodes.set(7, { flags: 0x1, parent: 9 }); assert.equal(menu.isOpen(), false);
    // 다른 카드가 살아 있으면 같은 창이 열려 있는 것이다.
    nodes.set(8, { flags: 0x1, parent: 3 }); assert.equal(menu.isOpen(), true);
});

test('지식 창 닫기: 공통 팝업 닫기를 요청하고 보관한 카드 토큰을 비운다', () => {
    const { log, nodes, menu } = scene(); menu.open(); nodes.set(7, { flags: 0x1, parent: 3 });
    menu.close(); assert.equal(log.closed, 1); assert.deepEqual(menu.tokens(), []); assert.equal(menu.isOpen(), false);
});

test('지식 창 장치 누락: 필수 Session·작성·화면 장치가 없으면 만들지 않는다', () => {
    const { menu: ok } = scene(); assert.ok(ok);
    assert.throws(() => new ClientKnowledgeMenu(null, {}, {}), /지식 창/u);
    assert.throws(() => new ClientKnowledgeMenu({ showKnowledge() {}, knowledgeAvailable() {}, core() {} }, { knowledgeWriter() {}, closePopups() {}, root() {} }, {}), /지식 창/u);
    assert.throws(() => new ClientKnowledgeMenu({ showKnowledge() {}, knowledgeAvailable() {}, core() {} }, { knowledgeWriter() {}, closePopups() {}, root() {}, nodeOf() {} }, { translate() {} }), /지식 창/u);
});
