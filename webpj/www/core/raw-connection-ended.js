// 연결 종료 통지(원본 004b6660)를 현재 동기화·서버 실패·Player flags·활동 프레임·다음 상태 예약에 연결하는 모듈이다(cpppj `o/RawConnectionEnded.cpp/.h`, 10.78 전용).
// 사용: receiveConnectionEnded(sender, connection, globals, hooks)
//   sender = 끝난 연결의 번호(signed, 서버 자기 끝 9·메타 10 포함), connection = 호스트 연결 token(같은 Registry 의 실제 노드 정체성, unsigned 32비트).
//   globals = { server, synchronization, phase, nextPhase, frames, players }
//     server(00540bc0)·phase(005c8758)는 읽기 전용으로 { value } 상자·현재 값을 돌려주는 함수·단순 값 중 하나이며 장치가 돌아온 뒤에도 각 원문 위치에서 다시 읽는다.
//     synchronization(005c8bf4)·nextPhase(005c8878)는 { value } 상자(실제 현재 DWORD)이다. frames 는 활동 22DWORD(Uint32Array) 중 선택된 양수 Player 한 칸만 0 으로 쓴다.
//     players() 는 flags 의 세 store 마다 현재 Player base(9행 180바이트 기록의 Uint8Array)를 다시 빌려 주는 함수이다.
//   hooks = { saveServerFailure(), disconnect(sender), broadcastPlayer(owner) } — 선택된 경로의 장치만 요구한다(미선택 경로는 요구하지 않는다).
//     saveServerFailure: 004b6560 의 flags8 정리와 같은 저장 writer, disconnect: 004e2940 의 현재 연결 지연 종료, broadcastPlayer: 00491270 의 현재 접속자별 Player 상태 송신.
// 순서: 연결이 현재 동기화 token 과 같으면 동기화를 0 → 서버가 아니면(9번이면 서버 실패 저장 뒤 서버가 되었을 때 즉시 반환) 연결 종료, 서버이고 9번이 아니면 연결 종료(서버의 9번 자기 끝은 닫지 않음)
//   → 서버이고 sender 가 1..8 이면 Player flags(+0x50) 에 0x100 설정·비트 0 해제·비트 0x40 해제를 세 번 따로 쓰고 Player 송신 뒤 그 활동 프레임을 0 → sender 가 10 이고 phase 23 이며 다음 상태가 24/12 가 아니면 다음 상태 20.
// 동기화 0 화/앞선 flags·종료/송신은 예외 때 되돌리지 않고, 송신이 실패하면 활동 프레임은 아직 지우지 않는다. 자연 반환에서 다음 상태도 쓰지 않는다. 이 모듈은 게임 상태를 즉시 전환하지 않고 다음 상태 DWORD 만 예약한다.
// 원본 관측 fixture 가 없어 cpppj 소스 계약을 tests/raw-connection-ended.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E559: 연결 종료 통지 이식)

// Player 기록의 폭·flags DWORD 위치·게임 중 서버 후보 Player 수(1..8)와 서버 자기 끝 번호(9)·메타 번호(10)이다. (2026-11-15 WB04-E559)
const PLAYER_STRIDE = 180;
const PLAYER_FLAGS_AT = 0x50;
const SERVER_SELF = 9;
const META_SENDER = 10;
// flags 비트: 종료 표시(0x100)·활동 비트(1)·보조 비트(0x40)와 다음 상태 번호들(연결 끊김 시 20, 현재 23, 제외 24/12)이다. (2026-11-15 WB04-E559)
const FLAG_ENDED = 0x100;
const FLAG_ACTIVE = 1;
const FLAG_HELPER = 0x40;
const PHASE_WAITING = 23;
const PHASE_EXCLUDED_A = 24;
const PHASE_EXCLUDED_B = 12;
const PHASE_RECONNECT = 20;
// 읽기 전용 현재 전역은 { value } 상자·현재 값을 돌려주는 함수·독립 입력의 단순 값 중 하나로 받아 호출 위치마다 다시 읽는다. (2026-11-15 WB04-E559)
const read = (source) => (typeof source === 'function' ? source() : (source !== null && typeof source === 'object' ? source.value : source));

// 선택된 실제 장치가 함수가 아니면 명시 오류를 낸다(미선택 장치는 호출하지 않으므로 요구하지 않는다). (2026-11-15 WB04-E559)
function need(device) {
    if (typeof device !== 'function') throw new Error('연결 종료 통지의 실제 장치 미연결');
    return device;
}

// 현재 Player base 에서 owner 의 flags DWORD 한 개만 읽어 (값 & ~clear) | set 으로 다시 쓴다(인접 Player 필드는 보존, 비정렬 위치도 바이트 단위로 처리). (2026-11-15 WB04-E559)
function changeFlags(players, owner, clear, set) {
    const bytes = need(players)();
    const at = owner * PLAYER_STRIDE + PLAYER_FLAGS_AT;
    if (at + 4 > bytes.length) throw new RangeError('연결 종료 Player flags 물리 부족');
    let value = 0;
    // 낮은 BYTE 부터 현재 DWORD 를 읽는다.
    for (let index = 0; index < 4; index += 1) value = (value | (bytes[at + index] << (index * 8))) >>> 0;
    value = (((value & ~clear) | set) >>> 0);
    // 같은 네 BYTE 에만 다시 쓴다.
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 0xff;
}

// 004b6660 전체이다. 각 원문 위치에서 서버 여부·단계를 다시 읽으며 선택된 장치만 사용한다. (2026-11-15 WB04-E559)
export function receiveConnectionEnded(sender, connection, globals, hooks) {
    // 현재 동기화 연결이 끝나면 동기화 DWORD 를 0 으로 되돌린다(장치 호출 전에 한다).
    if ((connection >>> 0) === (globals.synchronization.value >>> 0)) globals.synchronization.value = 0;
    if (!read(globals.server)) {
        // 클라이언트가 서버(9번) 끝을 받으면 서버 실패를 저장하고, 저장 중 서버가 되었다면 연결을 닫지 않고 반환한다.
        if (sender === SERVER_SELF) {
            need(hooks.saveServerFailure)();
            if (read(globals.server)) return;
        }
        need(hooks.disconnect)(sender);
    } else if (sender !== SERVER_SELF) need(hooks.disconnect)(sender); // 서버의 9번 자기 끝은 이 부모에서 닫지 않는다.
    // 서버이고 1..8 번 Player 연결이면 flags 세 번 store → 현재 접속자 송신 뒤에만 활동 프레임을 0 으로 지운다.
    if (read(globals.server) && sender > 0 && sender < SERVER_SELF) {
        const owner = sender >>> 0;
        changeFlags(globals.players, owner, 0, FLAG_ENDED);
        changeFlags(globals.players, owner, FLAG_ACTIVE, 0);
        changeFlags(globals.players, owner, FLAG_HELPER, 0);
        need(hooks.broadcastPlayer)(owner);
        globals.frames[owner] = 0;
    }
    // 메타 연결이 끝나고 대기 단계(23)에서 아직 종료/예약된 단계(24·12)가 아니면 다음 상태를 20 으로 예약만 한다.
    if (sender === META_SENDER && read(globals.phase) === PHASE_WAITING && globals.nextPhase.value !== PHASE_EXCLUDED_A && globals.nextPhase.value !== PHASE_EXCLUDED_B) globals.nextPhase.value = PHASE_RECONNECT;
}
