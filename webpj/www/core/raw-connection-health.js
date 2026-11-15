// 연결 활동 송신·지연 안내·자동 서버 이전 관문(원본 004b9010)을 현재 연결 활동 표·벽시각·Player flags 에 연결하는 모듈이다(cpppj `o/RawConnectionHealth.cpp/.h`, 10.78 전용).
// 사용: const reconnect = pollConnectionHealth(globals, hooks)
//   globals = { phase, heartbeatEnd, server, localOwner, autoRestart, activity, players }
//     phase(005c8758)는 현재 상태 블록(Uint32Array 36칸, [0]=상태 번호·[33]=서버 이전 중 표시)이며 읽기마다 현재값이다. heartbeatEnd(005c8620)는 { value } 상자(double)이다.
//     server(00540bc0)·localOwner·autoRestart(00531908)는 읽기 전용으로 { value } 상자·현재 값을 돌려주는 함수·단순 값 중 하나이다.
//     activity = { frames, wallSeconds } 는 연결 활동 22DWORD 두 표(Uint32Array, wallSeconds 는 signed 로 해석한다)이다. players() 는 매 반복 현재 9행(180바이트) flags 를 읽는 함수이다.
//   hooks = { connectionExists(index), wallSeconds(), frameWallSeconds(), frameNumber(), sendHeartbeat(server), battleFinished(), status(kind, player), saveServerFailure() }
//     connectionExists 는 실제 객체 표의 NULL 여부(open 성공으로 바꾸지 않음), wallSeconds 는 새 벽시각(double), frameWallSeconds 는 프레임에 고정된 벽시각, frameNumber 는 현재 프레임 하위 DWORD,
//     sendHeartbeat 는 서버면 broadcast 아니면 9 에 길이 5 의 64h 송신, battleFinished/status 는 종류 1(서버 지연)·2(Player 지연) 안내 관문과 표시, saveServerFailure 는 flags8 의 같은 재접속 writer 이다.
// 반환: 시간 초과의 저장 부모가 복귀했을 때만 true 이며 호출자가 자동 004b8cb0 을 이어 실행한다.
// 순서: 상태 29/33 이고 연결 11 이 없으면 반환 → 상태가 14 가 아니면 다음 활동 끝을 벽시각+1.0 으로 두고 반환 → 활동 끝이 지났으면 송신 뒤 서버 권한을 다시 읽어(서버 3.0·클라이언트 10.0초) 다음 끝을 새 벽시각으로 계산
//   → 현재 owner·프레임 벽시각을 이 순서로 캡처 → 서버면 Player 1..8 의 flags 1·15초 초과·활동 프레임 있음·로컬 owner 아님·전투 종료 아님이면 종류 2 안내 → 클라이언트면(서버 이전 중이 아니면) 연결 9 의
//   8초 초과·활동 프레임 있음·전투 종료 아님이면 종류 1 안내, 연결 9 가 없거나 20초 초과이면서 프레임 차 20 초과(unsigned)이고 활동 프레임이 있고 자동 재시작이면 저장 뒤 true.
// 원문의 현재값 재조회·캐시된 프레임 벽시각·signed 활동 초와 unsigned 프레임 차를 유지한다. 자연 반환 가지는 쓰지 않는 장치를 요구하지 않는다.
// 원본 관측 fixture 가 없어 cpppj 소스 계약을 tests/raw-connection-health.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E559: 연결 감시 이식)

// 상태 블록의 상태 번호 칸(0)과 서버 이전 표시 칸(33), 감시하는 상태 번호(14)와 연결 확인 상태(29·33)·확인 연결 번호(11)이다. (2026-11-15 WB04-E559)
const PHASE_AT = 0;
const MIGRATING_AT = 33;
const PHASE_PLAYING = 14;
const PHASE_CONNECTING_A = 29;
const PHASE_CONNECTING_B = 33;
const CONNECTION_PROBE = 11;
// 서버 연결 번호(9)·Player 수(9)·Player 기록 폭(180)과 flags 위치(+0x50)이다. (2026-11-15 WB04-E559)
const SERVER_CONNECTION = 9;
const PLAYER_COUNT = 9;
const PLAYER_STRIDE = 180;
const PLAYER_FLAGS_AT = 0x50;
// 활동 지연 한계(초)와 프레임 차 한계, 다음 활동 간격(비활동 상태 1초·서버 3초·클라이언트 10초)이다. (2026-11-15 WB04-E559)
const PLAYER_STALL_SECONDS = 15.0;
const SERVER_STALL_NOTICE_SECONDS = 8.0;
const SERVER_STALL_RESTART_SECONDS = 20.0;
const SERVER_FRAME_GAP = 20;
const IDLE_RECHECK_SECONDS = 1.0;
const SERVER_HEARTBEAT_SECONDS = 3.0;
const CLIENT_HEARTBEAT_SECONDS = 10.0;
// 안내 종류: 1 은 서버(9) 지연, 2 는 Player 지연이다. (2026-11-15 WB04-E559)
const NOTICE_SERVER = 1;
const NOTICE_PLAYER = 2;
// 읽기 전용 현재 전역은 { value } 상자·현재 값을 돌려주는 함수·독립 입력의 단순 값 중 하나로 받아 호출 위치마다 다시 읽는다. (2026-11-15 WB04-E559)
const read = (source) => (typeof source === 'function' ? source() : (source !== null && typeof source === 'object' ? source.value : source));

// 선택된 실제 장치가 함수가 아니면 명시 오류를 낸다(자연 반환 가지에서는 쓰지 않는 장치를 요구하지 않는다). (2026-11-15 WB04-E559)
function need(device) {
    if (typeof device !== 'function') throw new Error('연결 감시 실제 장치 미연결');
    return device;
}

// 004b9010 전체이다. 시간 초과의 저장 부모가 복귀했을 때만 true 를 돌려준다. (2026-11-15 WB04-E559)
export function pollConnectionHealth(globals, hooks) {
    const phase = globals.phase;
    // 연결 확인 중인 상태에서 연결 11 이 없으면 아무 것도 하지 않는다.
    if ((phase[PHASE_AT] === PHASE_CONNECTING_A || phase[PHASE_AT] === PHASE_CONNECTING_B) && !need(hooks.connectionExists)(CONNECTION_PROBE)) return false;
    // 진행 상태(14)가 아니면 다음 활동 끝만 새 벽시각+1 로 미룬다.
    if (phase[PHASE_AT] !== PHASE_PLAYING) {
        globals.heartbeatEnd.value = need(hooks.wallSeconds)() + IDLE_RECHECK_SECONDS;
        return false;
    }
    // 활동 끝이 지났으면 송신하고, 송신 효과 뒤 서버 권한을 다시 읽어 다음 간격을 새 벽시각에서 계산한다.
    if (globals.heartbeatEnd.value <= need(hooks.wallSeconds)()) {
        need(hooks.sendHeartbeat)(Boolean(read(globals.server)));
        const server = Boolean(read(globals.server));
        globals.heartbeatEnd.value = need(hooks.wallSeconds)() + (server ? SERVER_HEARTBEAT_SECONDS : CLIENT_HEARTBEAT_SECONDS);
    }
    // 송신 뒤 현재 owner 와 프레임 벽시각을 이 순서로 캡처한다.
    const owner = read(globals.localOwner) >>> 0;
    const wall = need(hooks.frameWallSeconds)();
    // 활동 초는 signed DWORD 로 해석한 캡처 벽시각과의 차이다(표는 읽을 때마다 현재값).
    const elapsed = (id) => wall - (globals.activity.wallSeconds[id] | 0);
    if (read(globals.server)) {
        // Player 1..8: 현재 flags 1·활동 프레임 있음·로컬 owner 아님·전투 종료 아님일 때만 지연 안내(종류 2)를 낸다.
        for (let player = 1; player < PLAYER_COUNT; player += 1) {
            const rows = need(globals.players)();
            if (rows.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new RangeError('연결 감시 Player9 물리 누락');
            if ((rows[player * PLAYER_STRIDE + PLAYER_FLAGS_AT] & 1) && elapsed(player) > PLAYER_STALL_SECONDS && globals.activity.frames[player] && player !== owner && !need(hooks.battleFinished)()) need(hooks.status)(NOTICE_PLAYER, player);
        }
        return false;
    }
    // 서버 이전 중(33번 칸)이 아니면 연결 9 의 지연 안내와 자동 서버 이전 관문을 검사한다.
    if (!phase[MIGRATING_AT]) {
        if (elapsed(SERVER_CONNECTION) > SERVER_STALL_NOTICE_SECONDS && globals.activity.frames[SERVER_CONNECTION] && !need(hooks.battleFinished)()) need(hooks.status)(NOTICE_SERVER, 0);
        const exists = need(hooks.connectionExists)(SERVER_CONNECTION);
        if ((!exists || (elapsed(SERVER_CONNECTION) > SERVER_STALL_RESTART_SECONDS && ((need(hooks.frameNumber)() - globals.activity.frames[SERVER_CONNECTION]) >>> 0) > SERVER_FRAME_GAP)) && globals.activity.frames[SERVER_CONNECTION] && read(globals.autoRestart)) {
            need(hooks.saveServerFailure)();
            return true;
        }
    }
    return false;
}
