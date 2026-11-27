// 독립 typed World 삭제의 실제 래퍼/현재 가상 자격·부모 분배 계약이다(WB04-E614).
// 정적 슬롯 입력만 사용하며 실제 미션/공중3종 생성·행동/교전은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { SQUID_DESTROY_EFFECT as E } from '../www/core/squid-destroy.js';
import { readData } from '../tests/helpers.mjs';
import { realCaptureAssets } from '../tests/capture-assets-support.mjs';
import { put } from '../tests/raw-support.mjs';
// 미선택 외부 서비스는 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자료와 실제 팔레트를 공유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 경계만 명시적으로 공급한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 원문 생성 순서만 진행하며 유닛/생산 프로세스를 만들지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); o.connectArcSpire(); o.connectFlyer(); o.connectFence(); o.connectGemReward(); }
// 같은 실제 몸체를 조립하며 World/Session은 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectWorldDeletion(); return o; }
// 정적 가상 선택 입력을 같은 실제 풀에 쓴다. Pop/생산/게임 행동은 실행하지 않는다.
function image(o, sid, type, vt) { const raw = o.core.pool.physical(sid); raw[10] = type; put(raw, 0, vt, 4); return raw; }


// 같은 실제 Session 부모와 두 장기 목록의 Outpost 삭제 라우팅을 확인한다. 실제 플레이/승리 시험은 아니다.
test('Session Outpost: 실제 두 목록 제거/지역 통지/피해 부모와 후처리 순서',()=>{
 const o=scene();try{const raw=image(o,50,126,0x509210);raw[11]=4;put(raw,14,0x41a00000);put(raw,18,0x41f00000);
  const first=o.state.assetState.currentOutposts(),second=o.state.assetState.currentResidences();first.entries=[9,50,50,11];first.count=4;second.entries=[50,12,50];second.count=3;const calls=[];
  o.ownership.updateRegion=(x,y)=>calls.push(['region',x,y,first.items(),second.items()]);o.damageablePre.preDestroy=(sid,flags)=>calls.push(['parent',sid,flags]);
  o.deletionModules.emit({effect:E.preDestroy,sid:50,flags:0x31});assert.deepEqual(calls,[['region',20,30,[9,11],[12]],['parent',50,0x31]]);
  assert.deepEqual(first.entries,[9,11,50,11]);assert.deepEqual(second.entries,[12,12,50]);
  o.damageableModules.preparePost=()=>calls.push(['preparePost']);o.damageablePost.postDestroy=(sid,flags)=>calls.push(['post',sid,flags]);o.deletionModules.emit({effect:E.postDestroy,sid:50,flags:7});assert.deepEqual(calls.slice(-2),[['preparePost'],['post',50,7]]);
 }finally{o.dispose();}
});
test('Session Outpost: abstract 목록 생략과 현재 VT 자격/표시 TYPE 변경',()=>{
 const o=scene();try{const raw=image(o,50,75,0x509210);raw[11]=4;raw[40]=1;const first=o.state.assetState.currentOutposts(),second=o.state.assetState.currentResidences();first.entries=[50];first.count=1;second.entries=[50];second.count=1;const calls=[];o.ownership.updateRegion=forbidden;o.damageablePre.preDestroy=(sid,flags)=>calls.push([sid,flags]);
  o.deletionModules.emit({effect:E.preDestroy,sid:50,flags:9});assert.deepEqual(calls,[[50,9]]);assert.deepEqual(first.items(),[50]);assert.deepEqual(second.items(),[50]);assert.equal(o.deletionModules.kinds.outpost(50),true);
 }finally{o.dispose();}
});
test('Session Outpost: 지역 예외 전에 두 목록 제거를 보존하고 부모는 호출하지 않는다',()=>{
 const o=scene();try{const raw=image(o,50,126,0x509210);raw[11]=4;const first=o.state.assetState.currentOutposts(),second=o.state.assetState.currentResidences();first.entries=[50,9];first.count=2;second.entries=[50,11];second.count=2;o.ownership.updateRegion=()=>{throw new Error('지역 실패');};o.damageablePre.preDestroy=forbidden;
  assert.throws(()=>o.deletionModules.emit({effect:E.preDestroy,sid:50,flags:0}),/지역 실패/);assert.deepEqual(first.items(),[9]);assert.deepEqual(second.items(),[11]);
 }finally{o.dispose();}
});
