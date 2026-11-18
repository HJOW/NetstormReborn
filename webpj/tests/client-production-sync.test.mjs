// Session 없는 실제 Client Pool·Player·Deck과 대여 관문을 생산 동기화로 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { synchronizeProductionControlContents } from '../www/core/client-production-sync.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { CampaignSessionAssets } from '../www/core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get } from './raw-support.mjs';

// 보호된 10.78 자료는 읽기만 하고 실제 창이나 World를 생성하지 않는다.
const data = realCaptureAssets(), game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 같은 Client 수명의 현재 Player·게임 전역·표면 저장소를 조립한다.
function scene() {
    const gameplay = new ClientGameplayGlobals(), players = new ClientPlayerRecords(); players.initialize(() => {});
    const assets = new CampaignSessionAssets(game, { read: () => { throw new Error('검사에서 선택하지 않은 파일'); } });
    const metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
    const storage = new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game, authority: gameplay.authority(), gameAux: gameplay.gameAux() });
    return { storage, gameplay, players };
}
// 현재 소유자 Player+68의 Deck 주소를 조회한다.
function token(context, owner = 1) { return get(context.players.records(), owner * 180 + 0x68); }

// 월드 없는 현재 기록의 Deck만 할당하고 재호출 시 같은 블록을 보존한다.
test('Client 생산 동기화: 월드 없이 현재 Player Deck 할당·재사용·소유자 전환', () => {
    const context = scene(), { storage, gameplay, players } = context, before = storage.pool.bytes(), memory = players.productionDeckMemory();
    assert.equal(token(context), 0); synchronizeProductionControlContents(context); const first = token(context); assert.ok(first);
    const deck = memory.bytes(first); assert.equal(deck.length, 352);
    // 생성자는 원본의 다섯 DWORD만 초기화한다.
    for (const offset of [0x14c, 0x150, 0x154, 0x158, 0x15c]) assert.equal(get(deck, offset), 0);
    deck[0] = 0x5a; synchronizeProductionControlContents(context); assert.equal(token(context), first); assert.equal(memory.bytes(first)[0], 0x5a);
    gameplay.player.templateOwner = 2; synchronizeProductionControlContents(context); const second = token(context, 2);
    assert.ok(second && second !== first); assert.equal(token(context), first); assert.equal(deck[0], 0x5a); assert.deepEqual(storage.pool.bytes(), before);
});

// SID0도 실제 finder가 순회하며 선택된 삭제는 가짜 성공 대신 Runtime을 요구한다.
test('Client 생산 동기화: SID 0 실제 순회와 선택된 Runtime 누락', () => {
    const context = scene(), { storage, gameplay, players } = context, zero = storage.pool.physical(0);
    put(zero, 6, 65535, 2); assert.throws(() => synchronizeProductionControlContents(context), /체인 손상/); const first = token(context); assert.ok(first); put(zero, 6, 0, 2);
    assert.equal(storage.types[83].flags2 & 4, 0); const deck = players.productionDeckMemory().bytes(first); put(deck, 0x14c, 1); deck[0] = 83; deck[2] = 1;
    const sid = storage.pool.allocate(2), form = storage.pool.allocated(sid); form[10] = 6; put(form, 4, 0, 2); put(form, 18, 83, 2); put(form, 20, 8, 2); put(zero, 6, sid, 2);
    synchronizeProductionControlContents(context); assert.equal(token(context), first); assert.equal(form[20], 8);
    deck[2] = 0; put(form, 20, 0x108, 2); assert.throws(() => synchronizeProductionControlContents(context), /현재 게임 Runtime 미연결/); assert.equal(get(form, 20, 2), 0x108);
    put(zero, 6, 0, 2); zero[11] |= 2; gameplay.writeDebugWord(0x5e4794, 1); gameplay.boss().value = false;
    assert.throws(() => synchronizeProductionControlContents(context), /dead 부모/); zero[11] &= ~2; gameplay.writeDebugWord(0x5e4794, 0); gameplay.boss().value = true;
});

// 실제 어느 한 소유자라도 대여 중이면 월드 없는 경로로 우회하지 않는다.
test('Client 생산 동기화: 공개 전 Runtime 대여 관문', () => {
    const context = scene(); const gameUse = context.gameplay.acquirePrepared();
    try { assert.throws(() => synchronizeProductionControlContents(context), /현재 게임 Runtime 미연결/); assert.equal(token(context), 0); } finally { gameUse.release(); }
    const storageUse = context.storage.acquirePrepared();
    try { assert.throws(() => synchronizeProductionControlContents(context), /현재 게임 Runtime 미연결/); assert.equal(token(context), 0); } finally { storageUse.release(); }
    synchronizeProductionControlContents(context); assert.ok(token(context)); assert.throws(() => synchronizeProductionControlContents({}), /문맥 미연결/);
});
