// State 정리 흉내(`_clearPlayerSpForState`)가 SP 비우기와 함께 다리 붕괴 스캔을 초기화(Reset422490)하는지 검사한다.
// 원본은 미션 시작(CleanupNativeStateWorld(1))·메인 메뉴 설치 등 State 전체 정리에서 스캔 범위(15000..23001)·커서·다음 기한(주기 + 현재 게임 시각)을 다시 잡는다.
// 웹은 이 초기화가 없어 스캔 범위가 -1 로 남았고, 그 결과 매달린 다리의 수명 감소(금 가기)가 한 번도 일어나지 않았다(L3 Tutorial1 680프레임 비교에서 발견). (2026-11-24 WB04-E1301)
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';

// 스캔 상태와 주기·게임 시각만 가진 가짜 저장소·시계를 붙인 앱이다. SP 저장소는 호출 기록을 남긴다.
function appWith({ game = 9.2, sp = null } = {}) {
    const app = new Netstorm();
    app._surfaceStorage = { bridgeDecayScan: { first: -1, last: -1, cursor: 0, next: 0 }, bridgeDecayPeriod: 10 };
    app._gameClock = { time: () => ({ game }) };
    if (sp) app._clientSp = { store: () => sp };
    return app;
}

test('정리는 다리 붕괴 스캔의 범위·커서·다음 기한(주기 + 게임 시각)을 다시 잡는다', () => {
    const app = appWith({ game: 9.2 });
    app._clearPlayerSpForState();
    // 기한은 float 주기에 double 게임 시각을 더한 값이다(게임 시각을 float 로 줄이지 않는다).
    assert.deepEqual(app._surfaceStorage.bridgeDecayScan, { first: 15000, last: 23001, cursor: 15000, next: 10 + 9.2 });
    // 다시 정리하면 이미 진행한 커서도 처음으로 돌아가고 기한은 그때의 시각 기준이다.
    app._surfaceStorage.bridgeDecayScan.cursor = 20000;
    app._gameClock = { time: () => ({ game: 25 }) };
    app._clearPlayerSpForState();
    assert.deepEqual(app._surfaceStorage.bridgeDecayScan, { first: 15000, last: 23001, cursor: 15000, next: 35 });
});

test('SP 저장소가 있으면 소유자 1~8 의 SP 를 0 으로 쓰고 스캔도 함께 초기화한다', () => {
    const written = [];
    const app = appWith({ sp: { set: (owner, value) => written.push([owner, value]) } });
    app._clearPlayerSpForState();
    assert.deepEqual(written, [1, 2, 3, 4, 5, 6, 7, 8].map((owner) => [owner, 0]));
    assert.equal(app._surfaceStorage.bridgeDecayScan.first, 15000);
});

test('SP 저장소가 없어도 스캔은 초기화되고, 표면 저장소나 게임 시계가 없으면 아무 일도 하지 않는다', () => {
    const app = appWith();
    app._clearPlayerSpForState();
    assert.equal(app._surfaceStorage.bridgeDecayScan.last, 23001, 'SP 저장소가 없어도 스캔 초기화');
    // 저장소가 없는 앱(시작·인트로 초기)은 오류 없이 지나간다.
    assert.doesNotThrow(() => new Netstorm()._clearPlayerSpForState());
    const noClock = appWith(); noClock._gameClock = null;
    noClock._clearPlayerSpForState();
    assert.equal(noClock._surfaceStorage.bridgeDecayScan.first, -1, '게임 시계가 없으면 건드리지 않는다');
});
