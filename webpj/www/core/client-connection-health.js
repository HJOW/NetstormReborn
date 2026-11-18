// Client의 현재 연결 활동 감시와 로그인 타이머를 원본 프레임 순서로 연결한다.
// 사용: pollClientConnectionHealth(client, currentWorld, devices). 재접속과 경고 표시는 선택된 때만 장치를 요구한다.
import { pollConnectionHealth } from './raw-connection-health.js';

// 선택한 외부 효과가 없으면 해당 호출 위치에서 명시적으로 중단한다.
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`연결 감시의 실제 ${name} 미연결`);
    return device;
}

// 같은 장기 Client 전역과 현재 Session을 매 선택 시점에 빌려 감시 뒤 타이머를 실행한다.
export function pollClientConnectionHealth(client, currentWorld, devices = {}) {
    if (client.edition() !== 'patch1078') throw new Error('연결 감시 Client의 10.78 판본 필요');
    need(currentWorld, '현재 World 조회');
    const game = client.gameplayGlobals(), local = client.localStateGlobals(), activity = client.connectionActivityGlobals();
    const first = currentWorld();
    if (first && first.session().connectionCallbackActive()) throw new Error('연결 감시의 callback 재진입');
    const globals = {
        phase: local.state().phase, heartbeatEnd: activity.heartbeatEnd(), server: () => game.authority().server,
        localOwner: () => game.player.battleOwner, autoRestart: activity.autoRestart(), activity: activity.tables(),
        players: () => client.playerRecords().records(),
    };
    const hooks = {
        connectionExists: (id) => Boolean(client.connectionGlobals().cleanup().transceivers[id]),
        wallSeconds: () => client.wallSeconds(), frameWallSeconds: () => client.time().wall, frameNumber: () => client.time().number >>> 0,
        sendHeartbeat: (broadcast) => { const world = currentWorld(); if (!world) throw new Error('활동 송신의 실제 World 없음'); world.session().sendConnectionHeartbeat(broadcast); },
        battleFinished: () => { const world = currentWorld(); return Boolean(world && world.session().battleFinished()); },
        status: (kind, player) => need(devices.status, '연결 상태 안내')(kind, player),
        saveServerFailure: () => { const world = currentWorld(); if (!world) throw new Error('서버 장애 저장의 실제 World 없음'); world.session().saveServerFailure(); },
    };
    if (pollConnectionHealth(globals, hooks)) need(devices.reconnect, '재접속 부모')(false);
    const current = currentWorld(); if (current) current.session().runConnectionLoginTimer();
}
