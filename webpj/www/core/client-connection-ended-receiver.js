// Client 장기 연결·상태·Player를 2번 연결 종료 기본 수신기에 연결한다.
// 사용: installClientConnectionEndedReceiver(client, dispatch). Session을 선택한 효과에서만 현재 Runtime을 빌린다.
import { receiveConnectionEnded } from './raw-connection-ended.js';
import { releaseRuntimePin } from './runtime-pin.js';

// 현재 서버 전역이 상자 또는 직접 값으로 제공되어도 호출 시점 값을 읽는다.
function current(value) { return value && typeof value === 'object' ? value.value : value; }

// 004b6660의 Client 기본 본문을 설치하며 기존 관찰기·버전·길이는 변경하지 않는다.
export function installClientConnectionEndedReceiver(client, dispatch) {
    const game = client.gameplayGlobals(), players = client.playerRecords(), registry = client.connectionGlobals();
    const local = client.localStateGlobals(), activity = client.connectionActivityGlobals(), state = local.state(), clock = local.clockSynchronization();
    const globals = {
        server: () => current(game.server()),
        synchronization: { get value() { return clock.connection; }, set value(value) { clock.connection = value >>> 0; } },
        phase: () => state.phase[0], nextPhase: { get value() { return state.next[0]; }, set value(value) { state.next[0] = value >>> 0; } },
        frames: activity.frames(), players: () => players.records(),
    };
    // 선택된 하위 효과마다 현재 Session의 전체 Runtime만 정상·예외 복귀까지 보유한다.
    const callSession = (operation, ...args) => {
        const session = game.requireSession(), pin = game.pinCurrentRuntime(session);
        try { session[operation](...args); } finally { releaseRuntimePin(pin); }
    };
    const hooks = {
        saveServerFailure: () => callSession('saveServerFailure'),
        disconnect: (sender) => registry.disconnect(sender),
        broadcastPlayer: (owner) => callSession('broadcastConnectionPlayer', owner),
    };
    dispatch.registerDirect(2, (sender, connection) => receiveConnectionEnded(sender | 0, connection ? registry.tokenFor(connection) : 0, globals, hooks));
}
