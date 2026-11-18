// 31h 타입·그래프 진단을 현재 Session의 TYPE·그림·그래프 기록에 연결한다.
// 사용: executeDeveloperReports(text, core, graph, trace) → 처리한 명령이면 true, 다른 명령이면 false.
import { firstDeveloperInteger, parseDeveloperCommand } from './raw-campaign-developer-settings.js';

// 원문 수준29 로그를 실제 선택된 추적 장치에 전달한다.
function log(trace, text) { if (typeof trace !== 'function') throw new Error('개발자 명령의 실제 추적 장치 미연결'); trace(29, text); }

// 현재 TYPE·그림과 그래프 레코드를 원문 순서로 진단 출력한다. (2026-11-18 WB04-E743)
export function executeDeveloperReports(text, core, graph, trace) {
    const { key, rest } = parseDeveloperCommand(text);
    // 호출 직전의 TYPE·그림을 읽고 원문의 512BYTE 출력 경계를 검사한다.
    const dumpType = (type) => {
        if (type >= core.types.length || type >= core.shapes.length) throw new RangeError('개발자 TYPE 덤프의 현재 물리 범위');
        const record = core.types[type], shape = core.shapes[type], name = record.name.split('\0', 1)[0];
        const message = `${String(type | 0).padStart(2, '0')}:${name}, flags=${record.flags1 | 0}, depth=${record.zOrder | 0} numFrames=${shape.frameCount | 0}\n`;
        if (message.length >= 512) throw new RangeError('개발자 TYPE 덤프 로그 물리 범위');
        log(trace, message);
    };
    if (key === 'dumptypes' || key === 'dts') {
        firstDeveloperInteger(rest);
        // 원문은 타입0을 제외하고 다음 행 직전에 현재 표 길이를 다시 읽는다.
        for (let type = 1; type < core.types.length; type += 1) dumpType(type);
        return true;
    }
    if (key === 'dumptype') { dumpType((firstDeveloperInteger(rest) ?? 0) >>> 0); return true; }
    if (key === 'dumpgraph' || key === 'dg') {
        const value = firstDeveloperInteger(rest);
        if (value === null) { log(trace, 'bad command\n'); return true; }
        const index = value >>> 0;
        if (index >= graph.records.length) throw new RangeError('개발자 graph 덤프의 실제255행 물리 범위');
        const record = graph.records[index];
        log(trace, `graph index: ${value} numSurface: ${(record.surfaces << 16) >> 16} inUse: ${(record.inUse << 16) >> 16}\n`);
        return true;
    }
    return false;
}
