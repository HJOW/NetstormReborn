// RawUberRootControls 조회가 실제 Client 등록표와 삭제 수명을 따르는지 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { UberRootNode } from '../www/ui/uber-root-gump.js';
import { RawUberRootLookup } from '../www/ui/raw-uber-root-lookup.js';

// 실제 공통 루트 대신 등록된 Uber 파생 루트를 공개한다.
function makeScene(read = null) {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {}), root = new UberRootNode();
    const token = owner.allocateDerived(root, () => 0, (value, flags) => owner.lifetime().deleteScalar(value, flags));
    owner.factory().construct(token, 1, 100, 80, 1);
    const state = owner.createState(); state.entries[root.id] = 0; root.id = 0; owner.replaceRoot(token);
    return { owner, root, token, lookup: new RawUberRootLookup(owner, read) };
}

// 실제 등록과 반납으로 만든 노드만 serial 순서대로 검색한다.
test('Uber HUD 참조: 루트·죽은 노드·빈 주소를 제외하고 현재 등록 순서를 따른다', () => {
    const { owner, root, lookup } = makeScene();
    const a = owner.createBase(1, 1, 1, 1, () => 0), b = owner.createBase(1, 1, 1, 1, () => 0);
    const first = owner.node(a), second = owner.node(b), state = owner.createState();
    first.serial = second.serial = root.serial; assert.equal(lookup.reference(root.serial), a);
    first.flags |= 0x10000; assert.equal(lookup.reference(root.serial), b); first.flags &= ~0x10000;
    const index = first.id; state.entries[index] = 0xffffffff; assert.equal(lookup.reference(root.serial), b); state.entries[index] = a;
    assert.equal(lookup.reference(0xffffffff), 0); owner.remove(a); assert.equal(lookup.reference(root.serial), b);
    owner.remove(b); assert.equal(lookup.reference(root.serial), 0); owner.dispose();
    assert.throws(() => lookup.reference(1), /반납/u);
});

// 필수 자식 및 미니맵의 현재 전역 교체를 실제 객체 변이로 확인한다.
test('Uber HUD 현재: 모든 자식과 미니맵 토글 예외를 실제 소유자에서 검사한다', () => {
    let currentMinimap = 0, address = 0;
    const { owner, root, lookup } = makeScene((value) => { address = value; return currentMinimap; });
    assert.equal(lookup.current(), false);
    const fields = ['vertical', 'minimapDivider', 'chatDivider', 'minimap', 'production', 'knowledge', 'chat'];
    // 실제 공통 등록표에 생성한 각 자식을 루트의 해당 필드에 보관한다.
    for (const field of fields) root[field] = owner.createBase(1, 1, 1, 1, () => 0);
    assert.equal(lookup.current(), true);
    const production = owner.node(root.production); production.flags |= 0x10000;
    assert.equal(lookup.current(), false); production.flags &= ~0x10000;
    const old = root.minimap; currentMinimap = old; owner.remove(old);
    assert.equal(lookup.current(), false); currentMinimap = 0; assert.equal(lookup.current(), true); assert.equal(address, 0x005949d0);
    root.minimap = 0; assert.equal(lookup.current(), false); root.minimap = old;
    assert.equal(new RawUberRootLookup(owner).current(), false);
    root.flags |= 0x10000; assert.equal(lookup.current(), false); root.flags &= ~0x10000;
    // 트리에 부착하지 않은 자식은 개별 삭제한 뒤 루트를 반납한다.
    for (const field of fields) if (field !== 'minimap') owner.remove(root[field]);
    owner.dispose(); assert.throws(() => lookup.current(), /반납/u);
});

// 기본 루트와 잘못된 계약은 완성 HUD로 오인하지 않는다.
test('Uber HUD 현재: 기본 루트와 잘못된 장치를 거부한다', () => {
    const owner = new ClientGumpRoot(100, 80, () => 0, () => {});
    assert.equal(new RawUberRootLookup(owner).current(), false);
    assert.throws(() => new RawUberRootLookup(null), /소유자/u);
    assert.throws(() => new RawUberRootLookup(owner, 3), /읽기/u); owner.dispose();
});
