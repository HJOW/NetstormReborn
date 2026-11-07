// 일반 입력 분배 세 개(Pause 키·회전 방향 C 키·URL 명령)를 원본 10.78 관측으로 대조한다(WB04-E20, cpppj PauseKeyInputTests·RotationKeyInputTests·UrlInputTests 이식).
//  - pause-key-input-x86.tsv 246행: 상위 비트/뗌/정지/선행 소비와 현재 명령/포화 결과를 실제 40칸 FIFO 위에서 대조한다.
//  - rotation-key-input-x86.tsv 264행: C/c·수정키·뗌·정지와 Config 재진입/중단. url-input-x86.tsv 96행: 두 번째 DWORD 전체 비트·뗌·수정키·모드 독립.
//  - 계약: 정확한 키 코드, 필요한 장치만 요구, 실패 시 앞선 효과 보존.
// cpppj 의 GameMenuActions 소비 시점 검사·실제 Client Config 연결·메타 resolver 검사는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E20: 일반 입력 분배 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { InputCode, InputQueue } from '../www/core/input-queue.js';
import { dispatchPauseKeyInput } from '../www/core/pause-key-input.js';
import { dispatchRotationKeyInput } from '../www/core/rotation-key-input.js';
import { dispatchUrlInput, URL_COMMAND } from '../www/core/url-input.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E20)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const pauseAll = fixture('pause-key-input-x86.tsv');
const rotationRows = fixture('rotation-key-input-x86.tsv');
const urlAll = fixture('url-input-x86.tsv');

// 원문 DWORD 의 전체 비트를 읽고(감김), 비트 열을 signed 로 되돌린다. (2026-11-08 WB04-E20)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const bits = (value) => value >>> 0;
const signed = (value) => value | 0;

// 독립 PE 가 결정한 상위 비트/뗌/정지/선행 소비와 현재 명령/포화 결과이다.
test('Pause 키: 원본10.78 246행(상위 비트·뗌·선행 소비·현재 명령·FIFO 포화)', { skip: !pauseAll && '원본 fixture 없음' }, () => {
    const rows = pauseAll.filter((row) => row[0] !== 'name');
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 17);
        count += 1;
        const queue = new InputQueue();
        const trace = [];
        const read = number(row[7]);
        const filled = number(row[8]);
        // 실제 두 커서를 공개 API 로만 전진한다.
        for (let index = 0; index < read; index += 1) { queue.push(1, 0, 0); queue.pop(false); }
        // 원문 입력의 현재 유효 슬롯 표식을 같은 순서로 채운다.
        for (let index = 0; index < filled; index += 1) {
            const physical = (read + index) % 40;
            queue.push(0x02000300 + physical, 100 + physical, 200 + physical);
        }
        const hooks = {
            command: () => number(row[6]),
            queue: (command, first, second) => { trace.push(`queue:${command}:${bits(first)}:${bits(second)}`); queue.pushCommand(command, first, second); },
        };
        for (const text of row[1].split(';')) {
            const fields = text.split(':');
            assert.equal(fields.length, 3);
            // 선행 Guide/Gump/Console 소비는 부모의 별도 관문이며 이 부분에 전달하지 않는다.
            if (number(row[2]) || number(row[3]) || number(row[5])) continue;
            dispatchPauseKeyInput({ code: number(fields[0]), x: signed(number(fields[1])), y: signed(number(fields[2])) }, hooks);
        }
        const active = [];
        while (!queue.empty) { const event = queue.pop(false); active.push(`${event.code}:${bits(event.x)}:${bits(event.y)}`); }
        const expectedTrace = row[14].split(';').filter((item) => item.startsWith('queue:')).join(';');
        const label = `행 ${count} ${row[0]} ${row[1]}`;
        assert.equal(trace.join(';'), expectedTrace, `${label} 예약`);
        assert.equal(active.join(';') || '-', row[12], `${label} FIFO`);
        assert.ok(row[9] === '1' && number(row[16]) > 0);
        for (const delta of row[15].split(';')) assert.equal(delta, '0');
    }
    assert.equal(count, 246);
});

// 뗌·글자 19·다른 가상키·일반명령은 현재 명령을 조회하지 않는다.
test('Pause 키: 다른 사건은 장치를 요구하지 않고 정확한 코드만 예약한다', () => {
    for (const code of [0x80130000, 0xb8130000, 0x13, 0x02130000, 0x120000, 0x140000]) assert.equal(dispatchPauseKeyInput({ code, x: 37, y: -19 }, {}), false);
    let reads = 0;
    let writes = 0;
    const hooks = { command: () => { reads += 1; return 0x020000cf; }, queue: (code, first, second) => { writes += 1; assert.deepEqual([code, first, second], [0x020000cf, 0, 0]); } };
    // outside/모든 수정키/pressed 는 원문에서 별도 거부하지 않는다.
    assert.equal(dispatchPauseKeyInput({ code: 0x7c130000, x: 37, y: -19 }, hooks), true);
    assert.deepEqual([reads, writes], [1, 1]);
});

// 실제 선택된 누락 장치는 빈 성공 대신 진단하며 앞선 조회 효과는 유지한다.
test('Pause 키: 예약 장치 누락은 현재 명령 조회를 보존한다', () => {
    assert.throws(() => dispatchPauseKeyInput({ code: 0x130000, x: 0, y: 0 }, {}), /현재 명령 조회 미연결/);
    let reads = 0;
    assert.throws(() => dispatchPauseKeyInput({ code: 0x130000, x: 0, y: 0 }, { command: () => { reads += 1; return 0x020000cf; } }), /일반 입력 큐 미연결/);
    assert.equal(reads, 1);
});

// 전체 일반 입력에서 동결한 C/c·수정키·뗌·정지와 Config 재진입/중단을 같은 분리에 재생한다.
test('회전 방향 키: 원본10.78 264행(C/c·수정키·뗌·정지·Config 재진입/중단)', { skip: !rotationRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rotationRows) {
        assert.equal(row.length, 10);
        const rotation = { mode: number(row[1]) };
        let assignment = '-';
        let atCall = 0xffffffff;
        let threw = false;
        let handled = false;
        const hooks = {
            paused: () => number(row[2]) !== 0,
            configure: (text) => {
                assignment = text;
                atCall = rotation.mode;
                // 마지막 8행만 callback 변경 입력이며 앞 256행은 기본 설정 장치이다.
                if (count >= 256) rotation.mode = number(row[3]);
                if (number(row[4])) throw new Error('원문 Config 명시 중단 입력');
            },
        };
        try { handled = dispatchRotationKeyInput({ code: number(row[0]), x: 37, y: -19 }, rotation, hooks); } catch (error) { if (error.message === '원문 Config 명시 중단 입력') threw = true; else throw error; }
        const label = `행 ${count} ${row.join(',')}`;
        assert.equal(rotation.mode, number(row[5]), `${label} 방향`);
        assert.equal(assignment, row[6], `${label} 할당문`);
        assert.equal(atCall, number(row[7]), `${label} 호출 시점`);
        assert.equal(threw, number(row[8]) === 0, `${label} 중단`);
        if (!threw) assert.equal(handled, (number(row[0]) & InputCode.release) === 0, `${label} 소비`);
        assert.ok(number(row[8]) === 0 || number(row[9]) === 0);
        count += 1;
    }
    assert.equal(count, 264);
});

// 키/정지 관문은 필요한 장치만 요구하고 설정 실패 시 먼저 쓴 반전값을 유지한다.
test('회전 방향 키: 정확한 코드와 부분 효과', () => {
    const rotation = { mode: 0 };
    // 가상키/명령을 같은 문자로 오인하지 않는다.
    for (const code of [(InputCode.release | 0x43) >>> 0, (InputCode.command | 0x43) >>> 0, 0x430000, 0x56]) {
        assert.equal(dispatchRotationKeyInput({ code, x: 0, y: 0 }, rotation, {}), false);
        assert.equal(rotation.mode, 0);
    }
    assert.equal(dispatchRotationKeyInput({ code: 0x43, x: 0, y: 0 }, rotation, { paused: () => true }), true);
    assert.equal(rotation.mode, 0);
    assert.throws(() => dispatchRotationKeyInput({ code: 0x63, x: 0, y: 0 }, rotation, { paused: () => false }), /Config 쓰기 미연결/);
    assert.equal(rotation.mode, 1);
});

// 두 번째 DWORD 의 전체 비트와 뗌·수정키·모드 독립을 원문 96입력에 직접 대조한다.
test('URL 입력: 원본10.78 96행(두 번째 DWORD 전체 비트·뗌·수정키·모드 독립)', { skip: !urlAll && '원본 fixture 없음' }, () => {
    const rows = urlAll.filter((row) => /^\d+$/.test(row[0]));
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 7);
        const event = { code: number(row[0]), x: signed(number(row[1])), y: signed(number(row[2])) };
        const mode = number(row[3]);
        assert.ok(mode < 4);
        let trace = '';
        let resolved = 0;
        const consumed = dispatchUrlInput(event, (token) => { resolved += 1; trace = `url:${token}`; return 'observed'; }, (text) => assert.equal(text, 'observed'));
        const label = `행 ${count} ${row.join(',')}`;
        assert.ok(consumed, `${label} 소비`);
        assert.equal(trace || '-', row[4], `${label} 토큰`);
        assert.equal(resolved, (event.code & InputCode.release) !== 0 ? 0 : 1, `${label} 조회`);
        assert.ok(row[5] === '1' && row[6] === '0');
        count += 1;
    }
    assert.equal(count, 96);
});

// 다른 명령은 소비하지 않고 뗌은 장치 없이 소비하며 도달한 장치 누락은 거부한다.
test('URL 입력: 소비 범위와 장치 누락', () => {
    assert.equal(dispatchUrlInput({ code: (URL_COMMAND | InputCode.release) >>> 0, x: 0, y: 0 }, null, null), true);
    assert.equal(dispatchUrlInput({ code: 0x02000089, x: 0, y: 0 }, null, null), false);
    assert.throws(() => dispatchUrlInput({ code: URL_COMMAND, x: 0, y: 0 }, null, null), /링크 분배기 미연결/);
    let activated = '';
    assert.equal(dispatchUrlInput({ code: URL_COMMAND, x: 0, y: -1 }, (token) => `tok${token}`, (text) => { activated = text; }), true);
    assert.equal(activated, 'tok4294967295');
});
