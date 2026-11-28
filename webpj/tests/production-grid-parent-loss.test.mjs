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
        lose() { destroy.destroy(oldParent, 0, forms.hooks()); },
        reconnect() { technology = factory.create(80, 2); panel.handleHudMode(3); return technology; },
    };
}

// 두 격자의 연결 플래그·부모·Form을 읽기만 한다.
function states(s) { return s.tokens.map(token => { const node = s.root.node(token); return [node.flags & 0x40, node.object, node.form]; }); }

// 부모 소실 뒤 즉시 Close의 결과를 고정한다. E1491은 수정 전 결함 재현이며 다음 단위에서 올바른 계약으로 바꾼다.
test('생산 격자 부모 소실: 즉시 닫기의 수정 전 상태', () => {
    const s = scene(); s.lose();
    assert.deepEqual(states(s), s.oldForms.map(form => [0x40, s.oldParent, form]));
    assert.ok(s.oldForms.every(form => s.pool.slot(form)[11] & 1));
    assert.doesNotThrow(() => s.panel.close()); s.root.dispose();
    console.log('부모 소실 즉시 닫기: 예외 없음, 통지', JSON.stringify(s.notices));
});

// 실제 FIFO 번호를 모두 보유해 반납 Form 번호가 다른 TYPE에 넘어간 뒤 Close를 시도한다.
test('생산 격자 부모 소실: 반납 번호 재사용 뒤 닫기 결함 재현', () => {
    const s = scene(); s.lose(); const reused = new Set();
    // 중간 할당을 반납하지 않아 동일 슬롯을 인위적으로 지정하지 않고 FIFO 재사용에 도달한다.
    for (let count = 0; count < s.pool.capacity && reused.size < s.oldForms.length; count++) {
        const sid = s.factory.create(80, 2); if (s.oldForms.includes(sid)) reused.add(sid);
    }
    assert.equal(reused.size, 2); const before = s.oldForms.map(form => s.pool.slot(form).slice());
    assert.throws(() => s.panel.close(), /Client Gump의 실제 Form 삭제 장치 누락/);
    assert.deepEqual(s.oldForms.map(form => s.pool.slot(form)), before);
    console.log('반납 번호 재사용 뒤 닫기: Form 삭제 장치 오류, 재사용 슬롯 보존');
});

// 새 기술 부모의 HUD 모드 연결이 오래된 0x40에 막히는 경계를 실제 처리기로 재현한다.
test('생산 격자 부모 소실: 새 기술 부모 연결 결함 재현', () => {
    const s = scene(); s.lose();
    assert.throws(() => s.reconnect(), /Client Gump 객체 부착의 현재 수명 오류/);
    assert.deepEqual(states(s), s.oldForms.map(form => [0x40, s.oldParent, form]));
    s.panel.close(); s.root.dispose();
    console.log('새 기술 부모 연결: 현재 수명 오류');
});
