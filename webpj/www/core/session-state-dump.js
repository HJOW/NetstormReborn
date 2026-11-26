// cpppj SessionStateDump 판번호 1의 순수 TSV 서식과 현재 Client·Session 상태 수집이다. (2026-11-19 WB04-E865~E867)
import { RegularProcess, SquidProcess } from './squid-process.js';
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 현행 cpppj SessionStateDump.cpp의 선택 AI 관측 DWORD 순서다. 원문 비트로 읽는다.
const AI_FIELDS = Object.freeze([0x258, 0x25c, 0x254, 0x264, 0x248, 0x2d8, 0x2d4, 0x290, 0x234, 0x2e4, 0x298, 0x2ac, 0x29c, 0x2a0, 0x294, 0x2a8, 0x2cc, 0x2d0, 0x394, 0x5a0, 0x5a4, 0x3cc, 0x3d8, 0x5f0]);
// AI 행동·생산·사제·검색·운반·건설·다리 재고의 실제 double 기한을 같은 QWORD 비트로 읽는다.
const AI_DEADLINES = Object.freeze([0x5a8, 0x5b0, 0x5b8, 0x5c0, 0x5c8, 0x5d0, 0x5d8, 0x5e0, 0x5e8, 0x630]);

// 풀 전체 원문의 FNV-1a 64비트 해시를 계산한다.
export function sessionDumpHash(bytes) {
    let hash = 0xcbf29ce484222325n;
    // 각 바이트에 XOR과 64비트 곱을 적용한다.
    for (const byte of bytes) hash = BigInt.asUintN(64, (hash ^ BigInt(byte)) * 0x100000001b3n);
    return hash;
}

// 분산 SP의 현재 비트만 읽는다. 저장소 초기화나 난수 소비는 하지 않는다.
export function decodeDumpScrambledSp(pools, permutation, offset, owner) {
    if (!Number.isInteger(owner) || owner < 0 || owner >= 9 || pools.length !== 288 || permutation.length !== 32) throw new RangeError('Session 덤프 분산 SP 입력 크기');
    let value = 0;
    // 원본 비트 순열과 offset을 이용해 소유자의 32비트를 복원한다.
    for (let bit = 0; bit < 32; bit += 1) {
        const index = permutation[bit];
        if (!Number.isInteger(index) || index < 0 || index >= 32) throw new RangeError('Session 덤프 분산 SP 순열 범위');
        if ((pools[index * 9 + owner] & (1 << ((offset + bit) & 31))) !== 0) value = (value | (1 << bit)) >>> 0;
    }
    return value >>> 0;
}

// 비어 있는 바이트열은 '-'로, 나머지는 소문자 16진으로 바꾼다.
function hex(bytes) {
    return bytes.length ? Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('') : '-';
}

// 현재 비정렬 원문의 지정 폭을 리틀 엔디언 부호 없는 정수로 읽는다.
function read(raw, offset, width) {
    if (offset < 0 || offset + width > raw.length) throw new RangeError('Session 덤프 슬롯 필드 범위');
    let value = 0;
    // 낮은 바이트부터 지정 폭을 조립한다.
    for (let index = 0; index < width; index += 1) value = (value | (raw[offset + index] << (index * 8))) >>> 0;
    return value;
}

// 게임 시각 double을 계산 없이 64비트 정수 비트로 바꾼다.
function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true);
}

// 장기 Client가 소유한 현재 시각·난수·Kernel을 읽어 월드 없는 덤프 입력을 만든다. 프로세스 실행과 난수 소비는 하지 않는다. (2026-11-19 WB04-E866)
export function captureSessionDumpBase(edition, frame, random, kernel) {
    if (edition !== 'patch1078' && edition !== 'cd1072') throw new RangeError('Session 덤프 판본 오류');
    if (!frame || !random?.game || !random?.crt || !kernel || typeof kernel.get !== 'function') throw new TypeError('Session 덤프의 현재 시계·난수·Kernel 필요');
    const input = { edition, frame: frame.number, gameTime: frame.game, gameRandom: random.game.state, crtRandom: random.crt.state,
        world: false, kernelSize: kernel.size, kernelCapacity: kernel.capacity, processes: [] };
    // PID 1부터 현재 슬롯을 읽는다. 같은 Kernel의 비 Squid는 Form과 부모를 0으로 둔다.
    for (let pid = 1; pid <= kernel.capacity; pid += 1) {
        const process = kernel.get(pid);
        if (!process) continue;
        const row = { pid, form: 0, parent: 0, regular: false };
        if (process instanceof SquidProcess) { row.form = process.form; row.parent = process.parent; }
        if (process instanceof RegularProcess) {
            row.regular = true; row.event = process.event; row.count = process.count;
            row.payloadBits = process.payloadBits; row.timeBits = doubleBits(process.time);
        }
        input.processes.push(row);
    }
    return input;
}

// 같은 Client가 소유한 현재 Session의 풀·Player·SP·Graph·장부를 공통 관측에 추가한다. 관측 사본을 만들어 이후 상태 변경과 구별한다. (2026-11-19 WB04-E867)
export function captureSessionStateDump(edition, frame, random, kernel, session = null, { includeAi = false } = {}) {
    if (typeof includeAi !== 'boolean') throw new TypeError('Session AI 관측 선택은 불리언이어야 합니다.');
    const input = captureSessionDumpBase(edition, frame, random, kernel);
    if (!session) return input;
    const core = session.core();
    if (core.kernel !== kernel || session.randomGlobals() !== random) throw new TypeError('Session 덤프의 같은 Kernel·난수 소유자 필요');
    const pool = core.pool, sp = session.spStorage(), book = core.bookkeeping;
    input.edition = pool.edition; input.world = true;
    input.poolBytes = pool.bytes(); input.poolCapacity = pool.capacity; input.stride = pool.layout.stride;
    input.freeCount = pool.freeCount; input.firstFreeServer = pool.firstFree(false); input.firstFreeClient = pool.firstFree(true);
    input.predictableCursor = pool.predictableCursor;
    input.players = session.playerRecords().slice(); input.hiddenPlayers = session.hiddenPlayers().slice();
    input.alliances = Array.from(session.alliances());
    input.spInitialized = sp.initialized; input.spOffset = sp.offset;
    if (input.spInitialized) { input.spPools = sp.pools(); input.spPermutation = sp.permutation(); }
    input.graph = core.graph.records().map((item) => ({ surfaces: item.surfaces, inUse: item.inUse, reserved: item.reserved }));
    input.totalCost = book.totalCost; input.postPopDepth = book.depth; input.productionDirty = book.productionDirty;
    input.totalLocal = book.localCounts().slice(); input.totalBuilt = book.localSecondaryCounts().slice(); input.totalGlobal = book.globalCounts().slice();
    if (includeAi) {
        input.ais = [];
        // 실제 Player가 소유한 살아 있는 AI 본문의 사본만 선택적으로 읽는다. 생성·명령·난수 소비는 없다.
        for (let owner = 1; owner < 9; owner += 1) {
            const token = session.participation().brain(owner);
            if (token) input.ais.push({ owner, token, body: session.aiMemory().bytes(token).slice() });
        }
    }
    return input;
}

// 명시된 관측값을 판번호 1 TSV로 변환한다. 숫자·행 순서는 cpppj FormatSessionStateDump와 같다.
export function formatSessionStateDump(input) {
    const patch = input.edition === 'patch1078';
    if (!patch && input.edition !== 'cd1072') throw new RangeError('Session 덤프 판본 오류');
    const rows = [
        'netstorm_session_dump\t1', `edition\t${patch ? 'originals' : 'originalCD'}`,
        `frame\t${input.frame}\t${doubleBits(input.gameTime)}`, `random\t${input.gameRandom}\t${input.crtRandom}`,
        `world\t${input.world ? 1 : 0}`, `kernel\t${input.kernelSize}\t${input.kernelCapacity}`,
    ];
    // 호출자가 관측한 PID 순서를 유지하고 정기 프로세스는 바로 뒤에 추가한다.
    for (const process of input.processes) {
        rows.push(`process\t${process.pid}\t${process.form}\t${process.parent}`);
        if (process.regular) rows.push(`regular\t${process.pid}\t${process.event}\t${process.count}\t${process.payloadBits}\t${process.timeBits}`);
    }
    if (!input.world) return `${rows.join('\n')}\nend\n`;
    const { poolBytes, poolCapacity, stride } = input;
    if (!Number.isInteger(stride) || stride < 12 || !Number.isInteger(poolCapacity) || poolCapacity < 0 || poolBytes.length !== poolCapacity * stride) throw new RangeError('Session 덤프 풀 원문 크기');
    rows.push(`pool\t${poolCapacity}\t${stride}\t${input.freeCount}\t${input.firstFreeServer}\t${input.firstFreeClient}\t${input.predictableCursor}\t${sessionDumpHash(poolBytes).toString(16)}`);
    // 예약 0~4번과 사용 중인 슬롯을 출력하고 자산은 판본에 맞는 읽기용 행도 만든다.
    for (let sid = 0; sid < poolCapacity; sid += 1) {
        const raw = poolBytes.subarray(sid * stride, (sid + 1) * stride);
        if (sid >= 5 && (raw[11] & 1)) continue;
        rows.push(`slot\t${sid}\t${hex(raw)}`);
        if (sid < 5 || raw[10] < 70) continue;
        rows.push(`object\t${sid}\t${raw[10]}\t${raw[11]}\t${raw[patch ? 34 : 32]}\t${read(raw, patch ? 36 : 34, patch ? 4 : 1)}\t${read(raw, 14, 4)}\t${read(raw, 18, 4)}\t${read(raw, 26, patch ? 4 : 2)}`);
    }
    if (input.players.length % 180) throw new RangeError('Session 덤프 Player 원문 크기');
    // Player와 숨김·동맹 행을 원래 번호 순서로 쓴다.
    for (let offset = 0; offset < input.players.length; offset += 180) rows.push(`player\t${offset / 180}\t${hex(input.players.subarray(offset, offset + 180))}`);
    rows.push(`hidden_players\t${hex(input.hiddenPlayers)}`);
    rows.push(['alliances', ...input.alliances].join('\t'));
    const sp = ['sp', input.spInitialized ? 1 : 0, input.spOffset];
    // 미초기화 상태는 분산 배열을 읽지 않고 아홉 개 '-'를 쓴다.
    for (let owner = 0; owner < 9; owner += 1) sp.push(input.spInitialized ? decodeDumpScrambledSp(input.spPools, input.spPermutation, input.spOffset, owner) : '-');
    rows.push(sp.join('\t'));
    // Graph 전체와 비용·깊이·생산 알림을 원래 순서로 쓴다.
    for (let index = 0; index < input.graph.length; index += 1) {
        const item = input.graph[index];
        rows.push(`graph\t${index}\t${item.surfaces}\t${item.inUse}\t${item.reserved}`);
    }
    rows.push(`totals\t${input.totalCost}\t${input.postPopDepth}\t${input.productionDirty}`);
    const { totalLocal, totalBuilt, totalGlobal } = input;
    if (totalLocal.length !== totalBuilt.length || totalLocal.length !== totalGlobal.length) throw new RangeError('Session 덤프 TotalMade 표 크기');
    // 하나라도 비영인 TYPE만 출력한다.
    for (let type = 0; type < totalLocal.length; type += 1) {
        if (totalLocal[type] || totalBuilt[type] || totalGlobal[type]) rows.push(`totalmade\t${type}\t${totalLocal[type]}\t${totalBuilt[type]}\t${totalGlobal[type]}`);
    }
    // 선택 관측은 일반 판번호1 덤프의 기존 행·바이트를 보존하면서 끝 직전에 덧붙인다.
    for (const ai of input.ais ?? []) {
        if (!patch || !Number.isInteger(ai.owner) || ai.owner < 1 || ai.owner >= 9 || !Number.isInteger(ai.token) || ai.token <= 0 || ai.token > 0xffffffff ||
            !(ai.body instanceof Uint8Array) || ai.body.length !== AI_BRAIN_BYTES || input.players.length !== 9 * 180 ||
            read(input.players, ai.owner * 180 + 0xb0, 4) !== ai.token || read(ai.body, 0x260, 4) !== ai.owner) throw new TypeError('Session AI 관측의 실제 본문/Player 계약 오류');
        const view = new DataView(ai.body.buffer, ai.body.byteOffset, ai.body.byteLength);
        rows.push(['ai', ai.owner, ai.token, ...AI_FIELDS.map((offset) => view.getUint32(offset, true)),
            ...AI_DEADLINES.map((offset) => view.getBigUint64(offset, true).toString())].join('\t'));
    }
    return `${rows.join('\n')}\nend\n`;
}
