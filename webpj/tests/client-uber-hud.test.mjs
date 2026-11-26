// 실제 Client Gump 루트에 Uber HUD 틀(세로·미니맵·채팅 구분선, 미니맵, 생산 패널)을 설치하고 도킹된 지도 영역과 재배치·정리를 검사한다. 게임을 실행하지 않는다.
// 근거: cpppj `UberHud.inc`의 `InstallHud`(Replace 뒤 HUD 모드 3)와 `RawUberRootControls::Reflow`. (2026-11-20 WB04-E1082)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { ClientHudDivider } from '../www/ui/client-hud-divider.js';
import { ClientUberHud } from '../www/ui/client-uber-hud.js';
import { UberRootNode } from '../www/ui/uber-root-gump.js';
import { HudDividerNode } from '../www/ui/hud-dock.js';

// 생산 패널·미니맵을 대신하는 파생 노드다. 실제 도킹이 크기와 위치를 정한다.
class PanelNode extends GumpCreateNode {}

// 1024×768 실제 루트와 구분선 공장, 자식 공장 두 개로 HUD를 구성한다.
function scene(width = 1024, height = 768) {
    const calls = [], root = new ClientGumpRoot(width, height, () => 0, () => {});
    const divider = new ClientHudDivider(root, { backgroundColor: () => 35, dividerColor: () => 55, closePopups: () => calls.push('popups'), baseHandle: () => 0, menuInstances: () => 0, poll: () => ({ code: 0 }) });
    // 호출자가 이미 만든 실제 파생 노드를 공통 등록표에 두는 공장이다.
    const make = (baseWidth, baseHeight) => {
        const token = root.allocateDerived(new PanelNode(), () => 0, (current, flags) => root.lifetime().deleteScalar(current, flags), null, () => {});
        root.factory().construct(token, 0, baseWidth, baseHeight, 1); return token;
    };
    const hud = new ClientUberHud(root, { assets: { find: () => ({ block: -70 }) }, panelGlobals: { backgroundType: 0, backgroundFrame: 0, panelFrame: 0 }, divider,
        fullDeck: () => ({ value: 0 }), mapRevision: () => ({ value: 0 }), createMinimap: () => make(128, 128), createProduction: () => make(40, height),
        closePopups: () => calls.push('popups'), baseHandle: () => 0, invalidate: (rect, flags) => calls.push(['invalidate', { ...rect }, flags]), finishResize: () => calls.push('finish') });
    return { root, hud, calls };
}

test('Uber HUD 설치: 모드 3 도킹이 구분선·생산 패널·미니맵을 원본 위치와 크기로 두고 지도 영역을 정한다', () => {
    const { root, hud, calls } = scene(); const token = hud.install();
    const create = root.createState(), uber = root.node(token);
    assert.equal(create.entries[0], token); assert.ok(uber instanceof UberRootNode);
    assert.ok(uber.vertical && uber.minimapDivider && uber.chatDivider && uber.minimap && uber.production); assert.equal(uber.knowledge, 0); assert.equal(uber.chat, 0);
    const vertical = root.node(uber.vertical), mini = root.node(uber.minimapDivider), chat = root.node(uber.chatDivider);
    assert.ok(vertical instanceof HudDividerNode);
    assert.deepEqual([vertical.x, vertical.y, vertical.baseWidth, vertical.baseHeight], [80, 0, 5, 768]);
    assert.deepEqual([mini.x, mini.y, mini.baseWidth, mini.baseHeight], [0, 688, 80, 5]);
    // 채팅 창이 숨겨진 상태이므로 채팅 구분선은 화면 아래 끝에 있다.
    assert.equal(chat.y, 768);
    const panel = root.node(uber.production), minimap = root.node(uber.minimap);
    assert.deepEqual([panel.x, panel.y, panel.baseWidth, panel.baseHeight], [0, 0, 80, 688]);
    assert.deepEqual([minimap.x, minimap.y, minimap.baseWidth, minimap.baseHeight], [0, 693, 80, 75]);
    assert.deepEqual({ ...hud.mapView() }, { left: 85, top: 0, right: 1024, bottom: 768 }); assert.equal(hud.mapView(), root.mapView());
    assert.ok(calls.includes('finish')); assert.ok(calls.some((entry) => Array.isArray(entry) && entry[0] === 'invalidate'));
    assert.deepEqual(hud.report().children, { vertical: uber.vertical, minimapDivider: uber.minimapDivider, chatDivider: uber.chatDivider, minimap: uber.minimap, production: uber.production });
    assert.throws(() => hud.install(), /중복/u); hud.close(); root.dispose();
});

test('Uber HUD 재배치: 화면 크기가 바뀌면 하단 간격과 지도 영역을 유지한다', () => {
    const { root, hud, calls } = scene(); const token = hud.install(), uber = root.node(token);
    calls.length = 0; root.resize(1366, 800); hud.reflow(1366, 800);
    assert.equal(root.node(uber.vertical).baseHeight, 800); assert.equal(root.node(uber.minimapDivider).y, 720); assert.equal(root.node(uber.chatDivider).y, 800);
    assert.deepEqual({ ...hud.mapView() }, { left: 85, top: 0, right: 1366, bottom: 800 });
    assert.equal(root.node(uber.production).baseHeight, 720); assert.equal(root.node(uber.minimap).y, 725); assert.equal(root.node(uber.minimap).baseHeight, 75);
    assert.deepEqual(calls.filter((entry) => entry === 'finish'), ['finish']);
    // 같은 크기를 다시 알려도 도킹 이동이나 후처리를 반복하지 않는다.
    calls.length = 0; hud.reflow(1366, 800); assert.deepEqual(calls, []);
    assert.throws(() => hud.reflow(0, 800), /크기/u); hud.close(); root.dispose();
});

test('Uber HUD 정리: 자식을 모두 지우고 새 루트로 다시 설치할 수 있다', () => {
    const { root, hud } = scene(); const first = hud.install(), uber = root.node(first), kids = [uber.vertical, uber.minimapDivider, uber.chatDivider, uber.minimap, uber.production];
    hud.close(); assert.equal(hud.token, 0); for (const token of kids) assert.equal(root.node(token), null);
    assert.ok(root.node(first) instanceof UberRootNode); hud.close();
    // 자식을 지운 빈 Uber 루트가 다음 설치 전까지 받는 HUD 모드 사건은 도킹하지 않고 기본 처리(0)로 돌아가며, 지도 영역은 다시 전체 화면이다.
    assert.equal(root.handle(first, 0x0200003d, 3, 0), 0); assert.deepEqual({ ...root.mapView() }, { left: 0, top: 0, right: 1024, bottom: 768 });
    // 새 설치는 원본처럼 새 Uber 루트로 교체하고 이전 루트를 반납한다.
    const second = hud.install(); assert.notEqual(second, first); assert.equal(root.node(first), null); assert.equal(root.createState().entries[0], second);
    hud.close(); root.dispose();
});

test('Uber HUD 장치 누락: 필수 실제 장치 없이는 만들지 않는다', () => {
    const root = new ClientGumpRoot(1024, 768, () => 0, () => {});
    assert.throws(() => new ClientUberHud(root, {}), /Uber HUD/u); assert.throws(() => new ClientUberHud(null, {}), /Uber HUD/u); root.dispose();
});

// State7 생성과 Ready 뒤 도킹 사이에 실제 자식의 정체·수명을 보존하고 중복 연결을 구별한다.
test('Uber HUD 두 단계 수명: 생성은 World 후처리를 미루고 최초 도킹은 같은 자식을 재사용한다', () => {
    const { root, hud, calls } = scene();
    assert.throws(() => hud.dockForWorld(), /생성/u);
    const token = hud.createForState(), node = hud.node();
    const children = [node.vertical, node.minimapDivider, node.chatDivider, node.minimap, node.production];
    const objects = children.map((child) => root.node(child));
    assert.equal(root.createState().entries[0], token); assert.ok(children.every(Boolean));
    assert.equal(calls.includes('finish'), false);
    assert.equal(hud.dockForWorld(), token);
    // 도킹이 같은 값의 새 노드로 바꾸지 않고 실제 생성 객체를 유지해야 한다.
    children.forEach((child, index) => assert.equal(root.node(child), objects[index]));
    assert.deepEqual({ ...hud.mapView() }, { left: 85, top: 0, right: 1024, bottom: 768 });
    assert.ok(calls.includes('finish')); calls.length = 0;
    assert.equal(hud.dockForWorld(), token); assert.deepEqual(calls, []);
    hud.close(); assert.throws(() => hud.dockForWorld(), /생성/u);
    const next = hud.createForState(); assert.notEqual(next, token);
    assert.equal(hud.dockForWorld(), next);
    hud.close(); root.dispose();
});
