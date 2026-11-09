// 0044dab0 의 가상 진단(+0x70) 호출과 모든 Form 재귀, 현재 들여쓰기 전역을 옮긴 모듈이다(cpppj `o/RawObjectTrace.cpp/.h`, 10.78 전용).
//  - traceRawObject(pool, globals, hooks, sid): sid 가 0 이면 아무 장치도 부르지 않고 돌아온다. 슬롯이 free 이면 `sid <번호> is free` 한 줄만 기록한다. 그 밖에는 객체의 현재 가상 +0x70(`hooks.trace`)을 부르고 →
//    재귀 깊이 +3·들여쓰기 설정 → 진단 checking 이 켜져 있고 메타맵이 아니며 슬롯이 dead 이고 boss 가 아니면 dead 부모 assert 보고(복귀) → 부모 head 부터 next 고리를 따라 TYPE 0..69 후보만(70 이상은 건너뜀)
//    논리 끝 번호(count) 미만이면 자식으로 재귀(free 자식도 개수에 포함) → 깊이 −3·들여쓰기 복구 → 자식이 있었으면 `~1~yEND <4자리>` 줄을 기록한다.
//    next 는 외부 효과 전에 저장하고 가상 호출·로그가 바꾼 뒤의 head/깊이/count/checking 은 소비 위치마다 다시 읽는다.
//  - 원문은 재귀/종속 고리에서 끝없이 실행한다. 호스트만 현재 경로의 고리(부모 재방문)와 형제 고리를 명시 오류로 거부한다. 전역 방문 집합으로 다른 부모가 다시 참조한 정상 후보를 생략하지 않는다.
// globals = { depth: { value }(0055a4a0 재귀 깊이, DWORD 감김), count: () => 정수(005c847c 논리 끝 번호), checkingDead: () => 정수(005e4794, 전체 DWORD), metamap: () => 정수(00594fc8), boss: () => boolean(00540bc4) },
// hooks = { trace(sid)(가상 +0x70), indent(깊이)(004c2600), log(level, line)(004c2630, 레벨 29), deadParent()(004b22d0 dead 부모 assert 보고) }. 선택된 장치 누락은 TypeError 로 앞선 출력/깊이 변경을 유지한 채 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E236: 객체 trace 재귀 이식으로 추가)

// 후보 TYPE 상한(이하만 방문)과 깊이 증분, 로그 레벨, 슬롯 필드 위치(TYPE +10·상태 +11·next +4·head +6)이다. (2026-11-10 WB04-E236)
const LAST_VISITED_TYPE = 69;
const DEPTH_STEP = 3;
const LOG_LEVEL = 29;
const TYPE_AT = 10;
const STATE_AT = 11;
const NEXT_AT = 4;
const HEAD_AT = 6;
const FREE = 1;
const DEAD = 2;

// 선택된 실제 장치가 없으면 앞선 출력/깊이 변경을 유지한 채 거부한다. (2026-11-10 WB04-E236)
function need(device) {
    if (typeof device !== 'function') throw new TypeError('객체 trace 실제 장치 미연결');
    return device;
}

// 원문의 DWORD 번호를 줄이기 전에 물리 슬롯 범위를 확인한 슬롯 사본이다. (2026-11-10 WB04-E236)
function rawOf(pool, sid) {
    if (sid >= pool.capacity) throw new RangeError('객체 trace 물리 SID 범위');
    return pool.slot(sid);
}

// 정렬되지 않은 head/next WORD 를 현재 물리 바이트에서 읽는다. (2026-11-10 WB04-E236)
const wordOf = (raw, offset) => raw[offset] | (raw[offset + 1] << 8);

// 한 객체의 진단과 자식 재귀이다. active 는 현재 경로의 부모 집합(고리 거부용)이다. (2026-11-10 WB04-E236)
function trace(pool, globals, hooks, sid, active) {
    if (!sid) return;
    if (rawOf(pool, sid)[STATE_AT] & FREE) { need(hooks.log)(LOG_LEVEL, `sid ${sid} is free\n`); return; }
    if (active.has(sid)) throw new Error('객체 trace 재귀 종속 고리');
    active.add(sid);
    need(hooks.trace)(sid);
    globals.depth.value = (globals.depth.value + DEPTH_STEP) >>> 0;
    need(hooks.indent)(globals.depth.value);
    let children = 0; // free 자식도 방문 횟수에 포함하며 END 출력 조건으로만 쓴다.
    if (globals.checkingDead() !== 0 && globals.metamap() === 0 && (rawOf(pool, sid)[STATE_AT] & DEAD) && !globals.boss()) need(hooks.deadParent)();
    let next = wordOf(rawOf(pool, sid), HEAD_AT);
    const chain = new Set(); // finder 는 부모 head 부터 시작하고 각 후보 next 를 먼저 저장한다.
    while (next) { // 현재 모든 Form 탐색기는 TYPE 0..69 를 통과시키고 나머지 TYPE 만 건너뛴다.
        const current = next;
        const raw = rawOf(pool, current);
        if (chain.has(current)) throw new Error('객체 trace 형제 종속 고리');
        chain.add(current);
        next = wordOf(raw, NEXT_AT);
        if (raw[TYPE_AT] > LAST_VISITED_TYPE) continue;
        if (current >= globals.count()) break;
        children += 1;
        trace(pool, globals, hooks, current, active);
    }
    globals.depth.value = (globals.depth.value - DEPTH_STEP) >>> 0;
    need(hooks.indent)(globals.depth.value);
    if (children) need(hooks.log)(LOG_LEVEL, `~1~yEND ${String(sid).padStart(4, '0')}\n`);
    active.delete(sid);
}

// 0044dab0: 최초 SID 0 은 판본/장치 검사 없이 반환하고 PATCH 의 실제 함수 경계만 지원한다. (2026-11-10 WB04-E236)
export function traceRawObject(pool, globals, hooks, sid) {
    if (!sid) return;
    if (pool?.edition !== 'patch1078') throw new TypeError('객체 trace PATCH 풀 필요');
    trace(pool, globals, hooks, sid, new Set());
}
