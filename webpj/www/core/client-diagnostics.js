// Client 수명의 trace 상태와 현재 월드 객체 진단을 브라우저 console에 연결한다.
// 사용: const diagnostics = new ClientTraceGlobals(); writeClientDiagnosticLog(diagnostics, level, text); traceClientObject(client, diagnostics, sid).
import { ClientBuildReplayGlobals } from './client-build-replay.js';
import { TraceLogState, writeRawTraceLog } from './raw-trace-log.js';
import { traceRawObject } from './raw-object-trace.js';
import { traceRawObjectVirtual } from './raw-object-trace-main.js';
import { forwardTraceToChat } from '../ui/trace-chat.js';

// 현재 Client 수명 동안 미션 교체와 무관하게 유지하는 원본 진단 전역이다.
export class ClientTraceGlobals {
    // 힙 진단과 원본 로그·재귀 깊이·덤프 억제·Chat 전달 관문이다.
    heapTrace = 0; heapAllocated = 0; log = new TraceLogState(); depth = { value: 0 }; suppress = 0; traceToChat = 0;
    // 건설 재생 전역과 단정도 기록 시각은 다른 Session과 공유한다.
    buildReplay = new ClientBuildReplayGlobals(); lastBuildRecordTime = 0;
}

// 선택된 Chat 전달과 stdout/stderr를 현재 브라우저 출력 장치로 보낸다.
function logHooks(state, devices) {
    return {
        // 원본 console callback은 설정된 경우에만 현재 Chat에 전달한다.
        console: (text) => forwardTraceToChat(state.traceToChat, text, devices.chat ?? {}),
        // 웹에는 FILE 대신 현재 console 출력 함수를 사용한다.
        write: (stream, text) => { const output = stream === 2 ? devices.error ?? console.error : devices.info ?? console.info; output(text); },
        // 브라우저 console에는 별도 flush가 없다.
        flush: () => {},
    };
}

// 같은 필터·들여쓰기·버퍼를 거친 실제 진단 문장을 출력한다.
export function writeClientDiagnosticLog(state, level, text, prefixBytes = 0, devices = {}) {
    writeRawTraceLog(state.log, level, text, prefixBytes, logHooks(state, devices));
}

// 현재 같은 Pool과 TYPE의 가상 진단·종속 재귀를 공유 로그로 보낸다.
export function traceClientObject(client, state, sid, devices = {}) {
    if (!sid) return;
    const surface = client.surfaceStorage(), game = client.gameplayGlobals(), pool = surface.pool;
    const write = (level, line) => {
        let prefix = 0;
        // 원본 고정 서식의 선행 색상 쌍만 들여쓰기 앞에 둔다.
        while (prefix + 1 < line.length && line[prefix] === '~') prefix += 2;
        writeClientDiagnosticLog(state, level, line, prefix, devices);
    };
    traceRawObject(pool, { depth: state.depth, count: () => pool.capacity, checkingDead: () => game.finder.currentChecking,
        metamap: () => Number(surface.ownerMode.challenge), boss: () => game.authority().boss }, {
        trace: (current) => traceRawObjectVirtual(pool, surface.types, () => state.suppress, current, write),
        indent: (value) => { state.log.indent = value; }, log: write,
        deadParent: () => { throw new Error('원문 객체 trace dead 부모 진단'); },
    }, sid);
}
