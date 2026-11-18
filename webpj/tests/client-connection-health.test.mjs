// Client 연결 감시가 현재 World를 다시 조회하고 동기 수신 중 재진입을 거부하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { pollClientConnectionHealth } from '../www/core/client-connection-health.js';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { ClientConnectionActivityGlobals } from '../www/core/client-connection-activity-globals.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';

// 상태 안내가 World를 바꾸면 원본처럼 후속 타이머는 변경된 World를 읽는다.
test('Client 연결 감시: 안내 뒤 현재 World 재조회와 타이머 순서', () => {
    const local = new ClientLocalStateGlobals(), activity = new ClientConnectionActivityGlobals(), gameplay = new ClientGameplayGlobals(), calls = [];
    local.state().phase[0] = 14; gameplay.authority().server = false; activity.heartbeatEnd().value = 200; activity.autoRestart().value = 0; activity.frames()[9] = 1;
    const first = { session: () => ({ connectionCallbackActive: () => false, battleFinished: () => false, runConnectionLoginTimer: () => calls.push('old') }) };
    const second = { session: () => ({ runConnectionLoginTimer: () => calls.push('new') }) }; let current = first;
    const client = { edition: () => 'patch1078', gameplayGlobals: () => gameplay, localStateGlobals: () => local, connectionActivityGlobals: () => activity,
        connectionGlobals: () => ({ cleanup: () => ({ transceivers: new Uint32Array(22) }) }), playerRecords: () => ({ records: () => new Uint8Array(9 * 180) }),
        wallSeconds: () => 100, time: () => ({ wall: 100, number: 50 }) };
    pollClientConnectionHealth(client, () => current, { status: (kind, owner) => { calls.push([kind, owner]); current = second; } });
    assert.deepEqual(calls, [[1, 0], 'new']);
    current = { session: () => ({ connectionCallbackActive: () => true }) }; activity.heartbeatEnd().value = 200;
    assert.throws(() => pollClientConnectionHealth(client, () => current), /callback 재진입/); assert.equal(activity.heartbeatEnd().value, 200);
});
