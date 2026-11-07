// 생산 조각 문자 키 분배(Q/W/A/S/Z·Y/X/E/D)를 원본 10.78 관측으로 대조한다(WB04-E36, cpppj ProductionShortcutsTests 이식).
//  - production-shortcuts-x86.tsv 8040행 중 Guide/Gump/뗌/콘솔에서 끝나지 않은 4280행만(나머지 3760행은 원본의 앞선 관문에서 끝남) 같은 40칸 입력 큐 위에서 대조한다: 예약 인자 순서·사건별 횟수·현재 명령 조회 수·남은 활성 큐.
//  - 계약: 키 구간 앞의 뗌/정지 관문과 비선택 문자는 장치를 요구하지 않고, 큐 중단 뒤에도 앞선 예약은 남는다.
// D/d 최근 항목(repeatCommand)은 원본 관측 행이 없어 계약 검사로만 확인한다. (2026-11-08 WB04-E36: 생산 단축키 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { InputCode, InputQueue } from '../www/core/input-queue.js';
import { PRODUCTION_BRIDGE_COMMAND, PRODUCTION_REPEAT_COMMAND, queueProductionShortcut } from '../www/core/production-shortcuts.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E36)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'production-shortcuts-x86.tsv'));
// 원문 DWORD 읽기(감김)와 signed 변환, 인자 비트이다. (2026-11-08 WB04-E36)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const bits = (value) => value >>> 0;
const RING = 40;

// 각 Guide 진입을 사건 경계로 사용해 실제 queue CALL 인자와 사건별 횟수를 읽는다. (2026-11-08 WB04-E36)
function nativeAttempts(text) {
    const result = { trace: [], counts: [], pausedCalls: 0 };
    // 이미 관측한 경계만 선택하며 키 매핑을 다시 계산하지 않는다.
    for (const item of text.split(';')) {
        if (item.startsWith('guide:')) result.counts.push(0);
        else if (item.startsWith('paused:')) result.pausedCalls += 1;
        else if (item.startsWith('queue:')) {
            assert.ok(result.counts.length > 0, 'Guide 사건 없는 원문 예약');
            result.counts[result.counts.length - 1] += 1;
            result.trace.push(item);
        }
    }
    return result;
}

// 실제 InputQueue 의 공개 API 로 독립 입력의 활성 고리와 현재 명령 장치를 준비한다. (2026-11-08 WB04-E36)
class ShortcutScene {
    constructor(row) {
        this.queue = new InputQueue();
        this.command = number(row[6]);
        this.trace = [];
        this.commandReads = 0;
        const read = number(row[7]);
        const filled = number(row[8]);
        assert.ok(read < RING && filled < RING, '원문 입력 고리 범위');
        // 빈 큐의 두 커서를 공개 push/pop 만으로 독립 입력 위치까지 전진한다.
        for (let index = 0; index < read; index += 1) { this.queue.push(0x2001000 + index, 0, 0); this.queue.pop(false); }
        // oracle 이 명시한 기존 활성 슬롯의 입력 표식을 그대로 공급한다.
        for (let index = 0; index < filled; index += 1) {
            const physical = (read + index) % RING;
            this.queue.push(0x2000300 + physical, 100 + physical, 200 + physical);
        }
    }

    // 실제 452fe0 대응 API 만 호출하며 기대 슬롯이나 성공 결과를 대신 만들지 않는다.
    hooks() {
        return {
            command: () => { this.commandReads += 1; return this.command; },
            queue: (code, first, second) => { this.trace.push(`queue:${code}:${bits(first)}:${bits(second)}`); this.queue.pushCommand(code, first, second); },
        };
    }

    // 두 커서 사이의 활성 사건만 공개 pop 으로 관측한다.
    activeQueue() {
        const items = [];
        // 최대 39개의 실제 활성 사건을 순서대로 꺼낸다.
        while (!this.queue.empty) {
            assert.ok(items.length + 1 < RING, '생산 단축키 활성 큐 상한 오류');
            const event = this.queue.pop(false);
            items.push(`${event.code}:${bits(event.x)}:${bits(event.y)}`);
        }
        return items.length === 0 ? '-' : items.join(';');
    }
}

// 전체 8040 중 Guide/Gump/뗌/콘솔에서 끝나지 않은 4280행만 같은 실제 helper/큐에 대조한다.
test('생산 단축키: 원본10.78 8040행 중 도달 4280행(예약 인자·사건별 횟수·명령 조회 수·활성 큐)', { skip: !rows && '원본 fixture 없음' }, () => {
    let nativeRows = 0;
    let compared = 0;
    let gateRows = 0;
    for (const row of rows) {
        if (row[0] === 'name') continue;
        assert.equal(row.length, 17);
        nativeRows += 1;
        assert.equal(row[9], '1');
        assert.ok(number(row[16]) > 0);
        assert.equal(row[13].length, RING * 12 * 2);
        const events = row[1].split(';').map((item) => {
            const fields = item.split(':');
            assert.equal(fields.length, 3);
            return { code: number(fields[0]), x: number(fields[1]) | 0, y: number(fields[2]) | 0 };
        });
        const expected = nativeAttempts(row[14]);
        assert.equal(expected.counts.length, events.length);
        let reachable = number(row[2]) === 0 && number(row[3]) === 0 && number(row[5]) === 0;
        // 원본은 뗌을 Guide/Gump 뒤, 일반 키 구간 앞에서 거부한다.
        for (const event of events) if ((event.code & InputCode.release) !== 0) reachable = false;
        assert.equal(reachable, expected.pausedCalls === events.length, `행 ${nativeRows} 도달 여부 ${row[0]}`);
        if (!reachable) { gateRows += 1; continue; }
        compared += 1;
        const scene = new ShortcutScene(row);
        const hooks = scene.hooks();
        // 실제 460de0 의 signed 깊이 > 0 이다.
        const paused = (number(row[4]) | 0) > 0;
        // 같은 큐를 유지하며 원문과 같은 사건 순서로 예약한다.
        events.forEach((event, index) => {
            assert.equal(queueProductionShortcut(event, paused, hooks), expected.counts[index] !== 0, `행 ${nativeRows} 사건 ${index}`);
        });
        assert.equal(scene.trace.join(';'), expected.trace.join(';'), `행 ${nativeRows} ${row[0]} 예약 인자`);
        // 실제 관측한 예약 수와 현재 명령 조회 수를 대조한다.
        assert.equal(scene.commandReads, expected.counts.reduce((sum, count) => sum + count, 0));
        assert.equal(scene.activeQueue(), row[12], `행 ${nativeRows} 활성 큐`);
    }
    assert.deepEqual([nativeRows, compared, gateRows], [8040, 4280, 3760]);
});

// 키 구간 앞의 뗌/정지 관문과 비선택 문자가 실제 예약 장치를 요구하지 않는지 확인한다.
test('생산 단축키: 뗌·정지·비선택 문자는 장치 비요구, 큐 중단 뒤 앞선 예약 보존', () => {
    for (const key of 'qWaSzXyEdD') {
        const code = key.charCodeAt(0);
        assert.equal(queueProductionShortcut({ code: (code | InputCode.release) >>> 0, x: 0, y: 0 }, false, {}), false);
        assert.equal(queueProductionShortcut({ code, x: 0, y: 0 }, true, {}), false);
    }
    assert.equal(queueProductionShortcut({ code: 0x3f, x: 0, y: 0 }, false, {}), false);
    // E 는 매번 현재 명령을 읽어 여섯 슬롯을 예약하며 세 번째 큐 경계 중단 전의 두 예약은 남는다.
    let command = PRODUCTION_BRIDGE_COMMAND;
    let reads = 0;
    let calls = 0;
    const observed = [];
    const partial = {
        command: () => { reads += 1; return command; },
        queue: (code, slot, zero) => {
            assert.equal(slot, calls);
            assert.equal(zero, 0);
            calls += 1;
            if (calls === 3) throw new Error('명시 세 번째 큐 경계 중단');
            observed.push(code);
            command += 1;
        },
    };
    assert.throws(() => queueProductionShortcut({ code: 0x45, x: 0, y: 0 }, false, partial), /세 번째/);
    assert.deepEqual([reads, calls, observed], [3, 3, [0x020000f4, 0x020000f5]]);
    // D/d 는 별도 최근 항목 명령을 소비 직전에 읽어 (명령, 0, 0)으로 예약하고 다리 슬롯 명령은 읽지 않는다.
    const sent = [];
    assert.equal(queueProductionShortcut({ code: 0x64, x: 0, y: 0 }, false, {
        repeatCommand: () => PRODUCTION_REPEAT_COMMAND,
        command: () => assert.fail('다리 슬롯 명령을 읽으면 안 됨'),
        queue: (code, first, second) => sent.push([code, first, second]),
    }), true);
    assert.deepEqual(sent, [[0x02000107, 0, 0]]);
    // 선택된 문자의 명령 조회/큐 누락은 거부한다.
    assert.throws(() => queueProductionShortcut({ code: 0x71, x: 0, y: 0 }, false, {}), TypeError);
    assert.throws(() => queueProductionShortcut({ code: 0x71, x: 0, y: 0 }, false, { command: () => 1 }), TypeError);
    assert.throws(() => queueProductionShortcut({ code: 0x44, x: 0, y: 0 }, false, { command: () => 1, queue: () => {} }), TypeError);
});
