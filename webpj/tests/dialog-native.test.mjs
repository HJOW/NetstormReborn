// HTML 대화상자 선택 명령(c79b0)의 발화 순서·정수 보존과 OnExit(c83b0) 해석/저장/소멸 예약을 독립 원본 관측 fixture 에 대조한다. 앞선 E435 이식은 소스 계약만 썼고(cpppj 시험이 있는 줄 몰랐다), 이 시험이 원본 252/204행을 처음 직접 연결한다. 원본게임을 실행하지 않는다. (2026-11-15 WB04-E467)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { fireDialogCommand } from '../www/ui/dialog-command-lifetime.js';
import { DialogExitCommandState, disposeDialogExitCommand, resolveDialogExitCommand } from '../www/ui/dialog-exit-command.js';
import { ISSUE_CHEAT_COMMAND, ISSUE_FLASH_COMMAND, invokeIssueCommand, registerIssueCommands } from '../www/ui/issue-commands.js';

// 탭 구분 fixture 의 주석 줄을 건너뛰고 열 목록으로 읽는다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t')); }
// 원문 16진 캡처 바이트를 문자열로, 문자열을 같은 16진 효과 기록으로 바꾼다('-' 는 빈 입력).
const decode = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1')); const textHex = (value) => Buffer.from(value, 'latin1').toString('hex');
// 독립 PE 관측이 현재 부모에 부여한 명시 토큰이다.
const PARENT = 0x12401000;

test('DialogCommandLifetime 원본 258행(발화 252행): 세 정수 예약 뒤 원래 부모 Kill 순서와 입력 종류별 비교 수가 모두 일치한다', () => {
    const table = rows('dialog-command-lifetime-x86.tsv'); let fired = 0; let instant = 0; let released = 0;
    for (const row of table) {
        assert.equal(row.length, 13); if (!Number(row[10])) continue; // helper 앞의 선택/fallback 은 이 core API 의 범위가 아니다.
        const selected = { command: Number(row[6]), parameter: Number(row[7]) }; let queued = ''; let trace = '';
        fireDialogCommand(PARENT, selected, (command, first, second) => { queued = `${command >>> 0},${first >>> 0},${second >>> 0}`; trace = `Queue,${queued}`; }, (parent) => { trace += `;Kill,${parent}`; });
        assert.deepEqual([Number(row[9]), Number(row[10]), queued, trace], [1, 1, row[11], row[12]], row.join('|'));
        fired += 1; if (Number(row[5])) instant += 1; if (Number(row[1]) & 0x80000000) released += 1;
    }
    assert.deepEqual([table.length, fired, instant, released], [258, 252, 126, 126]);
});

// 새 독립 원문 고리의 첫 실제 할당 주소와 같은 명시 토큰이다.
const STRING_TOKEN = 0x12000000;
// 새 모듈이 재사용하는 숫자 CRT 내부 관측(Digit/Atol)만 빼고 외부 효과를 원래 순서로 비교한다.
const externalTrace = (source) => source.split(';').filter((item) => !item.startsWith('Digit,') && !item.startsWith('Atol,')).join(';');

test('DialogExitCommand 원본 204행: regex 캡처 뒤 해석된 필드·문자열 고리 진행·소멸 예약 순서가 두 정밀도에서 모두 일치한다', () => {
    const table = rows('dialog-exit-command-x86.tsv'); assert.equal(table.length, 204); let index = 0;
    for (const row of table) {
        assert.equal(row.length, 8, `행 ${index}`); const command = decode(row[0]); const argument = decode(row[1]); const configZero = Number(row[2]) !== 0; const state = new DialogExitCommandState(); const trace = []; let ring = 0; let queued = 0;
        const hooks = {
            commands: {
                lookup: (name) => { trace.push(`Lookup,${textHex(name)}`); if (name === 'QuitApp') return 0x02000001; if (name === 'Tell') return 0x02000095; if (name === 'Config') return configZero ? 0 : 0x0200006d; if (name === 'Pause') return 0x020000cf; return 0xffffffff; },
                unknown: (name) => { trace.push(`Diagnostic,${textHex(name)}`); },
            },
            storeText: (text) => { trace.push(`String,${textHex(text)}`); ring += 1; return STRING_TOKEN; },
        };
        resolveDialogExitCommand(state, command, argument, hooks);
        assert.deepEqual([state.command, state.parameter, ring], [Number(row[3]), Number(row[4]), Number(row[5])], `행 ${index} ${row[0]} ${row[1]}`);
        disposeDialogExitCommand(state, (code, first, second) => { queued += 1; trace.push(`Queue,${code >>> 0},${first >>> 0},${second >>> 0}`); });
        assert.deepEqual([state.command, state.parameter, queued, trace.join(';')], [0, 0, Number(row[6]), externalTrace(row[7])], `행 ${index}`); index += 1;
    }
});

test('IssueCommands 원본 122행: 초기 등록 네 호출과 flash/cheatorama 의 NULL/부호/32비트 감김·인용·78BYTE 토큰·중복 등록·효과 순서가 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/issue-commands-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t')); assert.equal(table.length, 122);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 6, `행 ${index}`); const [operation, hex, before, returned, after, expected] = row; const bytes = hex === '-' || hex === '~' ? '' : decode(hex); const cheat = { value: Number(before) >>> 0 }; const events = [];
        if (operation === 'Init') registerIssueCommands((name, callback, flag) => { events.push(`R,${textHex(name)},${callback},${flag}`); });
        else {
            const hooks = { flash: (value) => { events.push(`F,${value >>> 0}`); }, sound: (name) => { events.push(`S,${textHex(name)},0,0,0,0,0`); } };
            const result = invokeIssueCommand(operation === 'Flash' ? ISSUE_FLASH_COMMAND : ISSUE_CHEAT_COMMAND, hex === '-' ? null : bytes, cheat, hooks); assert.equal(result, Number(returned), `행 ${index} 반환`);
        }
        assert.equal(cheat.value >>> 0, Number(after) >>> 0, `행 ${index} 치트`); assert.equal(events.length ? events.join(';') : '-', expected, `행 ${index} ${operation} ${hex}`);
    }
});
