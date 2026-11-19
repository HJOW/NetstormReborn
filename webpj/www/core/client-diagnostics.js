// Client 수명의 trace 상태와 현재 월드 객체 진단을 브라우저 console에 연결한다.
// 사용: const diagnostics = new ClientTraceGlobals(); writeClientDiagnosticLog(diagnostics, level, text); traceClientObject(client, diagnostics, sid).
import { ClientBuildReplayGlobals } from './client-build-replay.js';
import { TraceLogState, writeRawTraceLog } from './raw-trace-log.js';
import { traceRawObject } from './raw-object-trace.js';
import { traceRawObjectVirtual } from './raw-object-trace-main.js';
import { forwardTraceToChat } from '../ui/trace-chat.js';
import { PROCESS_TYPE_NAMES } from '../assets/type-flags.js';

// 패치판 프로세스 요약은 별도 포인터 표의 앞 열 이름과 이어지는 53개 전체 이름을 쓴다.
const SUMMARY_TYPE_NAMES = Object.freeze(['HealEffect', 'MeteorEffect', 'BOMBmeteor', 'bombIMano', 'bombIIMano',
    'bombIIIMano', 'bombLightingZap', 'bombLightingwave', 'RUIN', 'Monument', ...PROCESS_TYPE_NAMES]);

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

// 현재 Kernel 슬롯을 순서대로 다시 읽고 프로세스 종류·부모를 웹 출력 장치에 한 행씩 보낸다. (2026-11-19 WB04-E864)
export function writeClientProcessSummary(kernel, surface, write = (line) => console.error(line)) {
    const pool = surface.pool, types = surface.types;
    if (types.length !== 188) throw new RangeError('프로세스 요약의 패치판 TYPE 표 필요');
    if (typeof write !== 'function') throw new TypeError('프로세스 요약의 출력 장치 필요');
    // 0번은 원본처럼 제외하고 각 슬롯의 현재 프로세스와 Form TYPE를 읽는다.
    for (let slot = 1; slot <= kernel.capacity; slot += 1) {
        const process = kernel.get(slot);
        if (!process) continue;
        const form = process.form;
        if (!Number.isInteger(form) || form < 0 || form >= pool.capacity) throw new RangeError('프로세스 요약의 Form SID 범위');
        const name = SUMMARY_TYPE_NAMES[pool.slot(form)[10]];
        if (name === undefined) throw new RangeError('프로세스 요약의 이름 표 범위');
        const current = kernel.get(slot);
        if (!current) throw new Error('프로세스 요약 중 현재 슬롯이 비었습니다');
        const firstParent = current.parent;
        if (!Number.isInteger(firstParent)) throw new TypeError('프로세스 요약의 부모 필드 누락');
        const observedParent = firstParent | 0;
        if (observedParent === 0) {
            write(`pType=${name} sid=0, attachToType=NULL\n`);
        } else {
            const currentParent = current.parent;
            if (!Number.isInteger(currentParent)) throw new TypeError('프로세스 요약의 현재 부모 필드 누락');
            const parent = currentParent | 0;
            if (parent < 0 || parent >= pool.capacity) throw new RangeError('프로세스 요약의 부모 SID 범위');
            const type = pool.slot(parent)[10];
            if (type >= types.length) throw new RangeError('프로세스 요약의 부모 TYPE 범위');
            if (typeof types[type]?.name !== 'string') throw new TypeError('프로세스 요약의 부모 TYPE 이름 누락');
            const parentName = types[type].name.split('\0', 1)[0];
            write(`pType=${name} sid=${parent}, attachToType=${parentName}\n`);
        }
    }
}

// 같은 장기 Surface와 Kernel을 소진 진단에 연결한다. 요약은 웹 console로 보내고 상세·단언은 현재 Surface에 맡긴다.
export function bindClientKernelDiagnostics(kernel, surface, write) {
    kernel.bindDiagnostics({
        summary: () => writeClientProcessSummary(kernel, surface, write),
        detail: (text) => surface.recordAssertionDetail(text),
        assertion: (expression, file, line) => surface.assertDisplay(expression, file, line),
    });
}
