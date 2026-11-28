// 실제 Client 생산 패널·격자와 FIFO SID 풀·Form 호스트·공통 삭제기로 부모 소실 경계를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { ProductionControlContext } from '../www/core/production-control-context.js';
import { CampaignSessionAssets } from '../www/core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { GumpFormHost } from '../www/core/gump-form-host.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { ProductionMaintenanceGlobals, ProductionMaintenancePanel } from '../www/core/raw-production-maintenance.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { ClientInventoryGrid } from '../www/ui/client-inventory-grid.js';
import { ClientGumpBackground } from '../www/ui/client-gump-background.js';
import { ClientHudDivider } from '../www/ui/client-hud-divider.js';
import { ClientProductionPanel } from '../www/ui/client-production-panel.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 원본 게임을 실행하거나 수정하지 않고 공개 게임 자료를 읽는다.
const data = realCaptureAssets(), game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));

// 생산 패널의 실제 두 격자를 TYPE80 부모에 붙인다. 화면·예산 출력은 검사하지 않는다.
function scene() {
    const root = new ClientGumpRoot(1024, 768, () => 0, () => {});
    const gameplay = new ClientGameplayGlobals(), players = new ClientPlayerRecords(); players.initialize(() => {});
    const assets = new CampaignSessionAssets(game, { read() { throw new Error('미선택 파일 읽기'); } });
    const geometry = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
    const storage = new RawSurfaceStorage(assets.types, assets.frames, geometry.shapes, { source: game, authority: gameplay.authority(), gameAux: gameplay.gameAux() });
    const context = new ProductionControlContext(storage, gameplay, players, new Uint8Array(87));
    const pool = storage.pool, factory = new SquidFactory(pool, storage.types);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, storage.hash, storage.spots), storage.types), notices = [];
    const forms = new GumpFormHost(pool, factory, storage.types, destroy, {
        // 통지 전후 모두 살아 있는 Form을 읽어 종속 삭제 순서까지 보존하는지 확인한다.
        notice(owner, parent) {
            const token = root.createState().entries[owner], node = root.node(token), form = node.form;
            const before = Boolean(pool.slot(form)[11] & 1); root.objectGone(owner, parent);
            notices.push({ parent, form, before, after: Boolean(pool.slot(form)[11] & 1), flags: node.flags & 0x40, object: node.object, currentForm: node.form });
        },
        fallback: { selected: () => 0, emit(event) {
            if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) destroy.completePreDestroy();
            else if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) destroy.completePostDestroy();
            else if (event.effect !== SQUID_DESTROY_EFFECT.transmit) throw new Error('미선택 삭제 효과');
        } },
    });
    root.setFormHost(forms);
    const grid = new ClientInventoryGrid(root, context, { cursorPoint: { x: 0, y: 0 }, cursorFlags: 0 }, {
        now: () => 0, baseHandle: () => 0, allocateItem() { throw new Error('빈 부모의 항목 생성'); },
    });
    const background = new ClientGumpBackground(root, game, () => 35, () => null, () => null, () => 0);
    const divider = new ClientHudDivider(root, { background, backgroundColor: () => 35, dividerColor: () => 55, closePopups() {} });
    // 예산 글자와 선택은 이번 수명 검사의 외부 경계다. 격자·패널은 제품 구현이다.
    const budget = { create(parent) { const token = root.createBase(0, 20, 10, 1, () => 0); root.attach(token, 0, 0, parent, 2); return token; } };
    let technology = factory.create(80, 2);
    const globals = new ProductionMaintenanceGlobals(); globals.backgroundType = 92;
    const panel = new ClientProductionPanel(root, context, { grid, background, divider, budget, selection: {}, now: () => 0, technology: () => technology, changed() {} }, globals, new ProductionMaintenancePanel());
    panel.construct(); panel.handleHudMode(3);
    const tokens = [panel.node().bridge, panel.node().teleport];
    const oldParent = technology, oldForms = tokens.map(token => root.node(token).form);
    return { root, pool, forms, factory, panel, tokens, notices, oldParent, oldForms,
        // 실제 공통 삭제가 부모 통지와 종속 반납을 순서대로 실행한다.
        lose() { destroy.destroy(oldParent, 0, forms.hooks()); },
        // HUD 모드 처리기를 통해 살아 있는 새 기술 부모를 연결한다.
        reconnect() { technology = factory.create(80, 2); panel.handleHudMode(3); return technology; },
    };
}

// 두 격자의 연결 플래그·부모·Form을 읽기만 한다.
function states(s) { return s.tokens.map(token => { const node = s.root.node(token); return [node.flags & 0x40, node.object, node.form]; }); }

// 부모 소실 통지에서는 소유 표시만 지우고 실제 Form은 공통 삭제 순서에서 반납해야 한다.
test('생산 격자 부모 소실: Form 삭제 순서와 object 보존 및 즉시 닫기', () => {
    const s = scene(); s.lose();
    assert.deepEqual(states(s), s.oldForms.map(() => [0, s.oldParent, 0]));
    assert.equal(s.notices.length, 2);
    // 통지 안에서는 살아 있는 Form을 먼저 삭제하지 않아야 한다.
    for (const notice of s.notices) assert.deepEqual([notice.before, notice.after, notice.flags, notice.object, notice.currentForm], [false, false, 0, s.oldParent, 0]);
    assert.ok(s.oldForms.every(form => s.pool.slot(form)[11] & 1));
    assert.doesNotThrow(() => s.panel.close()); s.root.dispose();
});

// 실제 FIFO 번호를 모두 보유해 반납 Form 번호가 다른 TYPE에 넘어간 뒤 Close를 시도한다.
test('생산 격자 부모 소실: 반납 번호 재사용 뒤 닫기가 새 슬롯을 보존한다', () => {
    const s = scene(); s.lose(); const reused = new Set();
    // 중간 할당을 반납하지 않아 동일 슬롯을 인위적으로 지정하지 않고 FIFO 재사용에 도달한다.
    for (let count = 0; count < s.pool.capacity && reused.size < s.oldForms.length; count++) {
        const sid = s.factory.create(80, 2); if (s.oldForms.includes(sid)) reused.add(sid);
    }
    assert.equal(reused.size, 2); const before = s.oldForms.map(form => s.pool.slot(form).slice());
    assert.doesNotThrow(() => s.panel.close());
    assert.deepEqual(s.oldForms.map(form => s.pool.slot(form)), before);
    s.root.dispose();
});

// 새 기술 부모의 실제 Form 부착과 패널 종료 시 반납이 모두 성공해야 한다.
test('생산 격자 부모 소실: 새 기술 부모 연결과 새 Form 반납', () => {
    const s = scene(); s.lose();
    const parent = s.reconnect(), newForms = s.tokens.map(token => s.root.node(token).form);
    assert.notEqual(parent, s.oldParent);
    assert.deepEqual(states(s), newForms.map(form => [0x40, parent, form]));
    // 새 연결은 두 실제 Form의 살아 있는 부모 체인에 기록돼야 한다.
    for (const form of newForms) { assert.equal(s.forms.parent(form), parent); assert.equal(s.pool.slot(form)[11] & 1, 0); }
    s.panel.close(); s.root.dispose();
    assert.ok(newForms.every(form => s.pool.slot(form)[11] & 1));
});

// 일반 Gump 부모 0 부착은 원본 무동작이고 격자의 실제 소실 통지만 보완해야 한다.
test('생산 격자 부모 소실: 일반 bindObject의 부모 0 무동작 보존', () => {
    const s = scene(), before = states(s);
    // 같은 격자라도 일반 API 호출은 소유 표시·부모·Form·할당 상태를 바꾸지 않는다.
    for (const token of s.tokens) s.root.bindObject(token, 0);
    assert.deepEqual(states(s), before); assert.ok(s.oldForms.every(form => !(s.pool.slot(form)[11] & 1)));
    s.panel.close(); s.root.dispose();
});
