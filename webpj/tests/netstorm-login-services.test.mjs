// 앱의 Login 설정·시계·Player 관문이 현재 Client 소유자를 사용하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { ClientFrameClock } from '../www/core/client-frame-clock.js';
import { ClientTimeSource } from '../www/core/client-time-source.js';

// 지정 포트는 장기 전역, 다른 키는 기본값을 보존하는 설정에서 읽는다.
test('Netstorm Login 장치: 현재 설정·시계·표시 갱신', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false);
    let milliseconds = 1000;
    app._gameClock = new ClientFrameClock(new ClientTimeSource({ hostMilliseconds: () => milliseconds, hostUnixSeconds: () => 1700000000 }), app._clientLocal.gameTicks());
    const values = new Map([['timeout', 47]]);
    app._gameData = { readInt: (key, fallback) => ({ value: values.has(key) ? values.get(key) : fallback }) };
    const login = app._campaignLoginServices();
    app._clientGameplay.gameServerPort().value = 0xfffffffe;
    assert.equal(login.configInt('gameServerPort', 30), -2); assert.equal(login.configInt('timeout', 30), 47); assert.equal(login.configInt('missing', 30), 30);
    values.set('timeout', 18); assert.equal(login.configInt('timeout', 30), 18);
    milliseconds = 1125; assert.equal(login.clock(), 1125); assert.equal(login.wallSeconds(), 0.125); assert.equal(login.gameSeconds(), 0.125);
    app._gameClock.pushGamePause(); milliseconds = 2125; assert.equal(login.wallSeconds(), 1.125); assert.equal(login.gameSeconds(), 0.125);
    app._mainMenu = { dirty: false }; app._displayDirty = false; login.refresh();
    assert.equal(app._mainMenu.dirty, true); assert.equal(app._displayDirty, true);
});

// 원본 전투 완료 관문은 단독 미션·비전투·대본 중단에서 Session을 조회하지 않는다.
test('Netstorm Login 장치: Player UI의 현재 전투 완료 관문', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false);
    const login = app._campaignLoginServices();
    assert.throws(() => login.playerUiBusy(), /Client 수명/);
    app._surfaceStorage = { ownerMode: { battle: false } }; app._clientGameplay.world.missionPost = false;
    app._clientLocal.scriptBreak().value = 0;
    assert.equal(login.playerUiBusy(), false);
    app._surfaceStorage.ownerMode.battle = true; app._clientGameplay.world.missionPost = true;
    assert.equal(login.playerUiBusy(), false);
    app._clientGameplay.world.missionPost = false; app._clientLocal.scriptBreak().value = 1;
    assert.equal(login.playerUiBusy(), false);
    app._clientLocal.scriptBreak().value = 0; assert.throws(() => login.playerUiBusy(), /World 누락/);
    let finished = false; app._campaignWorld = { session: () => ({ battleFinished: () => finished }) };
    assert.equal(login.playerUiBusy(), false); finished = true; assert.equal(login.playerUiBusy(), true);
});
