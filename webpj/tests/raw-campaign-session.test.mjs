// 실제 외곽 Session 생성/공개·반납·부분 실패를 검사한다(WB04-E638). Load/Start/자연 플레이는 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawCampaignSession } from '../www/core/raw-campaign-session.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { ClientSubscriberGlobals } from '../www/core/client-subscriber-globals.js';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { releaseRuntimePin } from '../www/core/runtime-pin.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { readData } from './helpers.mjs';

// 같은 보호 자산 한 벌과 명시 빈 논리 Fort만 사용한다. 미션 저장 객체 적재 성공을 가장하지 않는다.
const data = realCaptureAssets(), game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 미선택 외부 효과는 호출 지점의 오류로 남긴다.
function forbidden() { throw new Error('외곽 검사에서 선택하지 않은 효과'); }
// ctor가 선택하지 않는 파일/오디오 효과만 명시 공급한다. 실제 부품은 외곽이 구성한다.
function session(extra = {}, fort = { sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }) {
    return new RawCampaignSession(game, { read: forbidden }, fort, 'thewarbegins', {
        unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra,
        sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden, ...extra.sound },
    });
}

// 외곽의 공개 보기는 같은 장기 옵션·참여 상태이며 미션 설치를 가장하지 않는다.
test('CampaignSession 공개 AI: 같은 옵션과 Load 전 관문', () => {
    const options = new Uint8Array(87), s = session({ missionOptions: options });
    try {
        s.hiddenPlayers()[2] = 17; assert.equal(options[0x40], 17); options[6] = 3; assert.equal(s.battleOption(6), 3);
        assert.equal(s.alliances(), s.alliances()); assert.equal(s.participation(), s.participation());
        assert.throws(() => s.battleOption(87), /범위/); assert.throws(() => s.createAi(1, new Uint8Array(600)), /설치/);
        assert.throws(() => s.crankAiMoney(), /미설치/); s.setAiParameter(0); assert.throws(() => s.prepareAi(1), /설치/);
        assert.throws(() => s.initializeAi(1), /설치/); assert.throws(() => s.refreshAiBridges(1), /설치/);
        assert.throws(() => s.computeAiDryCandidate({ x: 1, y: 1 }, { x: 2, y: 2 }), /설치/); assert.equal(s.loaded, false);
    } finally { s.dispose(); }
    assert.throws(() => s.alliances(), /반납/);
});

// cpppj TwoActualSessions의 Load 전 생성/공개·동일 전역 재사용 범위다. Load 뒤 비교는4D-2h·4D-10에 남긴다.
test('CampaignSharedGameplay_TwoActualSessionsPublishAndRetireSameGlobals: 실제 외곽 생성 범위', () => {
    const subscriberGlobals = new ClientSubscriberGlobals(), gameplayGlobals = new ClientGameplayGlobals(subscriberGlobals), playerRecords = new ClientPlayerRecords();
    const cursor = gameplayGlobals.cursor, energy = gameplayGlobals.energy, finder = gameplayGlobals.finder;
    energy.userCheat = 0x845fed;
    // 첫 외곽의 정상 반납 뒤 같은 Client 소유자들에 두 번째 실제 외곽을 구성한다.
    for (let generation = 0; generation < 2; generation++) {
        const s = session({ subscriberGlobals, gameplayGlobals, playerRecords });
        try {
            assert.equal(gameplayGlobals.requireSession(), s); assert.equal(gameplayGlobals.borrowed(), true); assert.equal(s.loaded, false);
            assert.notEqual(s.core(), s.core()); assert.equal(s.core().pool, s.world().core().pool); assert.equal(s.core().bookkeeping, s.world().core().bookkeeping); assert.equal(gameplayGlobals.cursor, cursor); assert.equal(gameplayGlobals.energy, energy); assert.equal(gameplayGlobals.finder, finder);
            assert.equal(energy.userCheat, 0x845fed);
            assert.throws(() => s.learnNewTechnology(77), /설치 전/); assert.throws(() => s.snapshotFort(1), /설치 전/);
            assert.throws(() => s.snapshotBattle(1), /설치 전/); assert.throws(() => s.generateWorldGeysers(), /설치 전/);
        } finally { s.dispose(); }
        s.dispose(); assert.equal(gameplayGlobals.borrowed(), false); assert.equal(cursor.lockDepth, 1); assert.equal(gameplayGlobals.player.playerKnown, 0);
        assert.throws(() => gameplayGlobals.requireSession(), /미연결/); assert.throws(() => s.core(), /반납한 외곽/); assert.throws(() => s.world(), /반납한 외곽/);
    }
});

// 다른 실제 Client 참조와 생성 중 Fort reader 실패는 공개/Player 대여를 남기지 않는다.
test('CampaignSharedGameplay_ForeignAndPartialConstructionRetireLease: 실제 외곽 생성 범위', () => {
    const subscriberGlobals = new ClientSubscriberGlobals(), gameplayGlobals = new ClientGameplayGlobals(subscriberGlobals), playerRecords = new ClientPlayerRecords();
    assert.throws(() => session({ gameplayGlobals, playerRecords }), /Subscriber 소유자 불일치/); assert.equal(gameplayGlobals.borrowed(), false); assert.equal(playerRecords.base(), 0);
    assert.throws(() => session({ gameplayGlobals, playerRecords, subscriberGlobals, missionFortImage: Uint8Array.of(1, 2) }));
    assert.equal(gameplayGlobals.borrowed(), false); assert.notEqual(playerRecords.base(), 0); assert.throws(() => gameplayGlobals.requireSession(), /미연결/);
    const s = session({ gameplayGlobals, playerRecords, subscriberGlobals }); s.dispose(); assert.equal(gameplayGlobals.borrowed(), false);
});

// 실제 발급한 전체 Runtime pin이 있는 동안 수동 반납도 거부하며 예외 뒤 현재 Session을 유지한다.
test('CampaignSession 외곽: 실제 Runtime pin·World 전달·배송 중 반납 거부', () => {
    const gameplayGlobals = new ClientGameplayGlobals(), s = session({ gameplayGlobals }), holder = { session: s };
    try {
        gameplayGlobals.bindRuntimeOwner(s, holder); const calls = [];
        s.bindRuntimePin(() => { const pin = gameplayGlobals.pinCurrentRuntime(s); calls.push(gameplayGlobals.runtimeCallActive(s)); return pin; });
        // 같은 실제 World의 Form152 Take가 외곽을 지나 발급한 pin을 정상/예외 복귀에서 모두 반납한다.
        const sid = s.world().create(152); assert.equal(s.core().factory.take(152, sid), sid); assert.ok(calls.length > 0); assert.ok(calls.every(Boolean));
        assert.throws(() => s.core().factory.take(999, sid)); assert.equal(gameplayGlobals.runtimeCallActive(s), false);
        const pin = gameplayGlobals.pinCurrentRuntime(s);
        try { assert.equal(pin.owner, holder); assert.throws(() => s.dispose(), /배송 종료 전/); assert.equal(gameplayGlobals.requireSession(), s); assert.equal(s.loaded, false); }
        finally { releaseRuntimePin(pin); }
    } finally { s.dispose(); }
    assert.equal(gameplayGlobals.borrowed(), false);
});

// Login을 만들지 않은 상태도 값형 전송 서비스의 반납을 실제 자식 해제 전에 선택한다.
test('CampaignSession 외곽: 같은 전송 서비스의 늦은 detach·함수 표 사본·멱등 반납', () => {
    const gameplayGlobals = new ClientGameplayGlobals(); let calls = 0, s;
    const transport = { detachSession: () => { calls++; assert.equal(gameplayGlobals.requireSession(), s); assert.equal(s.core().pool.edition, 'patch1078'); } };
    s = session({ gameplayGlobals, localLogin: { transport } }); transport.detachSession = forbidden;
    s.dispose(); s.dispose(); assert.equal(calls, 1); assert.equal(gameplayGlobals.borrowed(), false);
});
