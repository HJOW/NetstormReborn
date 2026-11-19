// 실제 공통 Gump 저장소와 Uber 루트의 현재 HUD 필드 뷰를 함께 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { HudDividerNode, HudDock, HudDockPart } from '../www/ui/hud-dock.js';
import { UberRootNode, uberRootDockView } from '../www/ui/uber-root-gump.js';

// 루트의 필드를 호출 때마다 다시 읽고 같은 파생·기본 주소를 요구한다.
test('Uber 루트 HUD 뷰: 같은 실제 노드와 현재 자식 필드', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), root = new UberRootNode();
    const token = owner.allocateDerived(root, () => 0, (value, flags) => owner.lifetime().deleteScalar(value, flags));
    owner.factory().construct(token, 0, 0, 0, 1); owner.attach(token, 0, 0);
    const view = uberRootDockView(token, { node: (value) => owner.node(value), base: (value) => owner.node(value) });
    root.vertical = 17; assert.equal(view(HudDockPart.root), token); assert.equal(view(HudDockPart.vertical), 17);
    root.vertical = 29; assert.equal(view(HudDockPart.vertical), 29); assert.throws(() => view(99), /종류/u);
    assert.throws(() => uberRootDockView(token, { node: () => root, base: () => new GumpCreateNode() }), /불일치/u);
    owner.dispose(); assert.throws(() => view(HudDockPart.root), /반납/u);
});

// 기존 HudDock의 원본 등록 부모가 현재 Uber 자식들의 같은 실제 노드를 바꾼다.
test('Uber 루트 HUD 뷰: 실제 공통 트리에서 표준 배치 등록', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), root = new UberRootNode();
    // 공통 공장과 파생 소멸자에 동일한 노드를 등록한다.
    const allocate = (node) => {
        const token = owner.allocateDerived(node, () => 0, (value, flags) => owner.lifetime().deleteScalar(value, flags));
        owner.factory().construct(token, 1, 10, 10, 1); return token;
    };
    const rootToken = allocate(root); owner.attach(rootToken, 0, 0);
    // 세 구분선과 네 일반 패널을 실제 같은 트리에 등록한다.
    for (const key of ['vertical', 'minimapDivider', 'chatDivider']) { root[key] = allocate(new HudDividerNode()); owner.attach(root[key], 0, 0, rootToken); }
    for (const key of ['minimap', 'production', 'chat', 'knowledge']) { root[key] = owner.createBase(1, 10, 10, 1, () => 0); owner.attach(root[key], 0, 0, rootToken); }
    // 표준 배치에서 호출하지 않는 장치는 선택되면 명시적으로 중단한다.
    const missing = () => { throw new Error('도달하지 않는 실제 장치'); };
    // 실제 공통 주소와 구분선 파생 주소를 각각 조회한다.
    const node = (value) => owner.node(value), divider = (value) => { const current = node(value); return current instanceof HudDividerNode ? current : null; };
    const dock = new HudDock(uberRootDockView(rootToken, { node, base: node }), { screenWidth: 100, screenHeight: 80 }, () => 55,
        { node, divider, baseAttach: missing, push: missing, attach: missing, move: missing, resolveSerial: missing, bubble: missing, closePopups: missing });
    dock.registerNormalLayout();
    assert.deepEqual(divider(root.vertical).slaves.slice(0, 6), [root.minimapDivider, root.chatDivider, root.minimap, root.production, root.knowledge, root.chat].map((value, index) => [node(value).serial, [1, 4, 1, 1, 1, 4][index]]));
    assert.deepEqual([divider(root.vertical).minX, divider(root.vertical).maxX, divider(root.minimapDivider).minY, divider(root.chatDivider).maxY], [60, 130, -43, 55]);
    owner.dispose();
});
