// cpppj와 웹의 판번호 1 Session 덤프를 행별로 비교한다. 게임 실행이나 파일 수정은 하지 않는다. (2026-11-19 WB04-E870)
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

// 원본 판번호 1의 행별 열 수와 번호가 붙는 행이다.
const WIDTHS = Object.freeze({ netstorm_session_dump: 2, edition: 2, frame: 3, random: 3, world: 2, kernel: 3,
    process: 4, regular: 6, pool: 8, slot: 3, object: 9, player: 3, hidden_players: 2, alliances: 82, sp: 12,
    graph: 5, totals: 4, totalmade: 5, ai: 37, end: 1 });
const INDEXED = new Set(['process', 'regular', 'slot', 'object', 'player', 'graph', 'totalmade', 'ai']);

// 텍스트를 중복 없는 행 표와 순서로 해독한다. 알 수 없는 형식은 비교 전에 거부한다.
export function parseSessionDump(text) {
    if (typeof text !== 'string') throw new TypeError('Session 덤프는 문자열이어야 합니다');
    const lines = text.replaceAll('\r\n', '\n').split('\n');
    if (lines.at(-1) === '') lines.pop();
    const rows = new Map(), order = [];
    // 각 행의 종류·열 수·고유 번호를 확인하고 실제 순서를 보존한다.
    for (let index = 0; index < lines.length; index += 1) {
        const cells = lines[index].split('\t'), kind = cells[0];
        if (!Object.hasOwn(WIDTHS, kind) || cells.length !== WIDTHS[kind]) throw new Error(`Session 덤프 ${index + 1}행 형식 오류`);
        if (INDEXED.has(kind) && !/^\d+$/.test(cells[1])) throw new Error(`Session 덤프 ${index + 1}행 번호 오류`);
        const key = INDEXED.has(kind) ? `${kind}:${cells[1]}` : kind;
        if (rows.has(key)) throw new Error(`Session 덤프 중복 행: ${key}`);
        rows.set(key, cells); order.push(key);
    }
    if (order[0] !== 'netstorm_session_dump' || rows.get('netstorm_session_dump')?.[1] !== '1' || order.at(-1) !== 'end') throw new Error('Session 덤프 머리·판번호·끝 오류');
    // 실제 비교에 필요한 공통 머리는 월드 유무와 관계없이 존재해야 한다.
    for (const key of ['edition', 'frame', 'random', 'world', 'kernel']) if (!rows.has(key)) throw new Error(`Session 덤프 필수 행 누락: ${key}`);
    if (!['originals', 'originalCD'].includes(rows.get('edition')[1])) throw new Error('Session 덤프 판본 오류');
    if (!['0', '1'].includes(rows.get('world')[1])) throw new Error('Session 덤프 월드 값 오류');
    if (rows.get('world')[1] === '1') {
        // 월드가 있으면 고정 월드 장부 행도 누락할 수 없다.
        for (const key of ['pool', 'hidden_players', 'alliances', 'sp', 'totals']) if (!rows.has(key)) throw new Error(`Session 덤프 월드 행 누락: ${key}`);
    } else {
        // 월드가 없으면 공통 Kernel 이외의 월드 행은 기록될 수 없다.
        for (const key of order) if (['pool', 'slot', 'object', 'player', 'hidden_players', 'alliances', 'sp', 'graph', 'totals', 'totalmade', 'ai'].includes(key.split(':', 1)[0])) throw new Error(`Session 덤프의 월드 없는 행: ${key}`);
    }
    return { rows, order };
}

// 두 덤프의 모든 행과 공통 행 순서를 비교한다. SP 소유자 0 제외는 호출자가 명시해야 한다.
export function compareSessionDumps(expectedText, actualText, { ignoreSpOwnerZero = false } = {}) {
    const expected = parseSessionDump(expectedText), actual = parseSessionDump(actualText), differences = [];
    // 기준 행마다 누락과 값 차이를 구분한다.
    for (const [key, first] of expected.rows) {
        const second = actual.rows.get(key);
        if (!second) { differences.push({ key, kind: 'missing', expected: first, actual: null }); continue; }
        const left = first.slice(), right = second.slice();
        if (key === 'sp' && ignoreSpOwnerZero) { left[3] = '<ignored>'; right[3] = '<ignored>'; }
        if (left.some((value, index) => value !== right[index])) differences.push({ key, kind: 'changed', expected: first, actual: second });
    }
    // 기준에 없던 행도 별도로 보고한다.
    for (const [key, row] of actual.rows) if (!expected.rows.has(key)) differences.push({ key, kind: 'extra', expected: null, actual: row });
    const expectedOrder = expected.order.filter((key) => actual.rows.has(key));
    const actualOrder = actual.order.filter((key) => expected.rows.has(key));
    if (expectedOrder.some((key, index) => key !== actualOrder[index])) differences.push({ key: 'row-order', kind: 'order', expected: null, actual: null });
    return { differences, ignored: ignoreSpOwnerZero ? ['sp.owner0'] : [] };
}

// 명령줄은 비교 결과의 앞부분과 전체 건수를 출력하고 차이가 있으면 종료 코드 1을 반환한다.
function main(argv) {
    const ignore = argv.includes('--ignore-sp-owner0');
    const files = argv.filter((arg) => arg !== '--ignore-sp-owner0');
    if (files.length !== 2) throw new Error('사용: node session-dump-compare.mjs <기준> <대상> [--ignore-sp-owner0]');
    const result = compareSessionDumps(fs.readFileSync(files[0], 'utf8'), fs.readFileSync(files[1], 'utf8'), { ignoreSpOwnerZero: ignore });
    console.log(`차이 ${result.differences.length}개, 제외 ${result.ignored.join(', ') || '없음'}`);
    // 너무 큰 덤프도 터미널 전체를 채우지 않게 처음 30개 키만 보여 준다.
    for (const difference of result.differences.slice(0, 30)) console.log(`${difference.kind}\t${difference.key}`);
    return result.differences.length ? 1 : 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    try { process.exitCode = main(process.argv.slice(2)); }
    catch (error) { console.error(`Session 덤프 비교 오류: ${error.message}`); process.exitCode = 2; }
}
