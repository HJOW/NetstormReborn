// 실제 Client의 현재 보조 전역과 다음 Session의 같은 수명을 검사한다. 플레이/승패 상태를 만들지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { CampaignSessionAssets, RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { RawConstructionCommand } from '../www/core/raw-construction-command.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
// 보호 자료를 읽기 전용으로 공유하고 생성자에서 파일 읽기/외부 효과는 허용하지 않는다.
const data = realCaptureAssets(), game = new GameAssets(data.table, data.database);
const files = { read() { throw new Error('이 검사에서 파일 읽기는 선택되지 않는다'); } };
// 선택하지 않은 실제 효과를 성공 함수로 바꾸지 않는다.
function forbidden() { throw new Error('선택하지 않은 실제 효과'); }
// 실제 Client와 같은 장기 Storage/Gameplay owner를 조립한다.
function client() {
    const assets = new CampaignSessionAssets(game, files), metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
    const storage = new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game });
    const gameplay = new ClientGameplayGlobals(null, storage.authority, storage.gameAux);
    return { storage, gameplay, input: { surfaceStorage: storage, gameplayGlobals: gameplay } };
}
// 실제 같은 장기 owner 또는 독립 typed 입력의 짧은 Session을 만든다.
function session(extra = {}) { return new RawCampaignSessionOwners(game, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra }); }
// 실제 수신기가 현재 차단 DWORD를 본문보다 먼저 읽는지 확인한다.
test('Client GameAux 건설: 현재 차단값으로 실제 수신 본문 조회를 생략하고 독립 입력은 보존한다', () => {
    const c = client(), o = session(c.input);
    try {
        const state = o.state.constructionState.command, body = new RawConstructionCommand(o.core.types, state, { money: forbidden, shortfall: forbidden, request: forbidden, reject: forbidden });
        assert.throws(() => body.receive(1, new Uint8Array()), /본문 부족/);
        c.storage.gameAux.constructionBlocked = 0x80000000;
        assert.doesNotThrow(() => body.receive(1, new Uint8Array()));
        state.blocked = 0; assert.doesNotThrow(() => body.receive(1, null));
        c.storage.gameAux.constructionBlocked = 0; assert.throws(() => body.receive(1, null), /본문 부족/);
    } finally { o.dispose(); }
    const independent = session();
    try { independent.state.constructionState.command.blocked = 9; assert.equal(independent.state.constructionState.command.blocked, 9); } finally { independent.dispose(); }
});
// 실제 요청/건설/경로/충돌과 Player별 간헐천 소비자가 같은 현재 값을 다시 읽는다.
test('Client GameAux 경계: 같은 마스크/지역 요청 수의 재조회와 지역 fallback 대입 분리', () => {
    const c = client(), o = session(c.input);
    try {
        const aux = c.storage.gameAux, s = o.state;
        aux.constructionDisplaceGenus = 0x80000001;
        assert.deepEqual([s.constructionState.request.displaceGenus, s.buildingState.read('pushGenus'), s.process.currentBlockingGenus(), s.placement.collision.ignoredGenus, s.gameplayPlacement.collision.ignoredGenus], new Array(5).fill(0x80000001));
        s.constructionState.request.displaceGenus = 0; s.buildingState.pushGenus = 0;
        aux.constructionDisplaceGenus = 2; assert.equal(s.constructionState.request.displaceGenus, 2); assert.equal(s.buildingState.read('pushGenus'), 2);
        aux.firstLocalGeyserQuantity = -3; aux.lastLocalGeyserQuantity = 7;
        assert.deepEqual([s.constructionState.geyser.readFirstQuantity(), s.constructionState.geyser.readLastQuantity()], [-3, 7]);
        aux.firstLocalGeyserQuantity = 8; aux.lastLocalGeyserQuantity = 1;
        assert.deepEqual([s.constructionState.geyser.readFirstQuantity(), s.constructionState.geyser.readLastQuantity()], [8, 1]);
    } finally { o.dispose(); }
});
// 게임 보조 순환값은 같은 Client의 Session 반납/새 미션 뒤에도 유지하며 새 Client는0이다.
test('Client GameAux 공급자: 원래 signed 순환 저장소를 다음 Session에 보존한다', () => {
    const c = client(); c.storage.gameAux.dudeProviderIndex = 17;
    const first = session(c.input);
    try { assert.equal(first.dudeProviderIndex, 17); first.dudeProviderIndex = -2; assert.equal(c.storage.gameAux.dudeProviderIndex, -2); } finally { first.dispose(); }
    assert.equal(c.storage.borrowed, false);
    const second = session(c.input);
    try { assert.equal(second.dudeProviderIndex, -2); c.storage.gameAux.dudeProviderIndex = 5; assert.equal(second.dudeProviderIndex, 5); } finally { second.dispose(); }
    const other = client(), third = session(other.input);
    try { assert.equal(third.dudeProviderIndex, 0); } finally { third.dispose(); }
});
