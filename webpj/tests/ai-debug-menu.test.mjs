// AI 디버그 메뉴 raw-ai-debug-menu.js(openAiDebugMenu)를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - ai-debug-menu-x86.tsv 60행(원문 413dd0): 열 = 입력 20개(checking·owner·present·mutation·+0x258/+0x2d8/+0x2d4/+0xb00/+0x220/+0x228/+0x230/+0x234/+0x24c 필드 9개·진단 로그 전역 7개), 원문 메뉴 효과 trace(머리/서식/체크/begin/take/submenu 순서와 전체 인자).
//    각 행은 입력만 설치한다(mutation 은 효과 callback 이 Player+0xb0 또는 보관 본문을 바꾸는 독립 재진입 입력). 현재 Player 행과 brain 재조회/보관 본문의 수명, 단락 뒤 Player 조회 횟수를 함께 대조한다. 기대 trace 는 계산하지 않는다.
//  - cpppj 계약: 첫 효과 중단 뒤 후속 메뉴를 합성하지 않고 초기 단락은 미연결 GUI 를 요구하지 않음, 목표 배열 밖을 임의 문구로 바꾸지 않음.
// cpppj 의 AiDebugInput_* 시험(명령 입력/이름/Client 전역)은 `ai-debug-command-input.js`(WB04-E27) 등 다른 몸체의 검사 범위라 옮기지 않았다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E182: AI 디버그 메뉴 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { openAiDebugMenu } from '../www/core/raw-ai-debug-menu.js';
import { AI_REPORTING_COMMAND, DEBUG_SWITCH_COMMAND } from '../www/core/ai-debug-command-input.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E182)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ai-debug-menu-x86.tsv')) ?? [];

// 원문 필드의 정확한 폭/오프셋과 실제 메뉴 7행의 로그 주소 순서이다. (2026-11-09 WB04-E182)
const FIELDS = [0x258, 0x2d8, 0x2d4, 0xb00, 0x220, 0x228, 0x230, 0x234, 0x24c];
const DEBUG = [0x52f3cc, 0x52f3e4, 0x52f3d0, 0x52f3d4, 0x52f3d8, 0x52f3dc, 0x52f3e0];

// 원문 문자열을 재인코딩하지 않고 16진으로 나타낸다. (2026-11-09 WB04-E182)
const hex = (text) => Buffer.from(text, 'latin1').toString('hex');

// 선택된 미연결/잘못된 물리는 명시 실패여야 한다. (2026-11-09 WB04-E182)
function fails(action) {
    try { action(); } catch { return true; }
    return false;
}

// 독립 Player 한 행과 교체 전후의 정상 본문, 현재 입력, 효과 callback 의 명시 호출 번호이다. (2026-11-09 WB04-E182)
class MenuInput {
    // native 입력의 현재 필드와 정상 포인터 토큰을 같은 폭으로 복구한다.
    constructor(data) {
        this.player = new Uint8Array(180);
        this.first = new Uint8Array(0xb08).fill(0xa5);
        this.second = new Uint8Array(0xb08).fill(0x5a);
        this.debug = new Map();
        this.formats = 0;
        this.checks = 0;
        this.taken = 0;
        this.playerReads = 0;
        this.captured = 1;
        this.trace = [];
        this.stop = '';
        this.checking = Number(data[0]);
        this.owner = Number(data[1]);
        this.mutation = Number(data[3]);
        put(this.player, 0xb0, Number(data[2]) ? 1 : 0);
        // 원문 관측에 공급한 교체 본문의 독립 입력이다.
        const other = [1, 1234, 4, 9, 0x40c00000, 0x40800000, 9, 10, 30];
        FIELDS.forEach((offset, index) => {
            put(this.first, offset, Number(data[4 + index]));
            put(this.second, offset, other[index]);
        });
        DEBUG.forEach((address, index) => this.debug.set(address, Number(data[13 + index])));
    }

    // 포인터 토큰 1/2 만 살아 있으며 해제/NULL 은 미정 성공으로 만들지 않는다. (2026-11-09 WB04-E182)
    body(token) {
        if (token === 1) return this.first;
        if (token === 2) return this.second;
        throw new RangeError('명시 본문 토큰 누락');
    }

    // 선택한 효과를 기록한 다음 명시 중단을 전파한다. (2026-11-09 WB04-E182)
    emit(value) {
        this.trace.push(value);
        if (this.stop && value.startsWith(this.stop)) throw new Error('명시 메뉴 효과 중단');
    }

    // 실제 메뉴 부모가 필요한 현재 조회 경계만 공급한다. (2026-11-09 WB04-E182)
    reads() {
        return {
            checking: () => this.checking,
            player: (value) => { this.playerReads += 1; assert.equal(value, this.owner); return this.player; },
            body: (token) => this.body(token),
            debug: (address) => { if (!this.debug.has(address)) throw new RangeError('진단 로그 주소 누락'); return this.debug.get(address); },
            command: (address) => {
                if (address === 0x542a34) return AI_REPORTING_COMMAND;
                if (address === 0x542a38) return DEBUG_SWITCH_COMMAND;
                assert.equal(address, 0x5428d8);
                return 0x2000078;
            }
        };
    }

    // Native 와 같은 외부 효과/재진입 입력을 주며 실제 원문 서식기 몸체를 합성했다고 주장하지 않는다. (2026-11-09 WB04-E182)
    hooks() {
        return {
            header: (value) => { this.emit(`H:${hex(value)}`); if (this.mutation === 1 && value === 'NOT INITIALIZED\n') put(this.player, 0xb0, 2); },
            format: (limit, pattern, args) => {
                assert.ok(limit === 80 && args.length === 1);
                const value = typeof args[0] === 'string' ? hex(args[0]) : String(args[0] >>> 0);
                this.emit(`F:${limit}:${hex(pattern)}:${value}`);
                this.formats += 1;
                if (this.mutation === 2 && this.formats === 1) put(this.player, 0xb0, 2);
                return `F${this.formats}`;
            },
            checked: (on, label, command, parameter, include, enabled) => {
                this.emit(`C:${on}:${hex(label)}:${command}:${parameter}:${include}:${enabled}`);
                this.checks += 1;
                if (this.mutation === 4 && this.checks === 9) put(this.captured === 1 ? this.first : this.second, 0x220, 0x3f800000);
            },
            begin: (flags) => { this.emit(`B:${flags}`); },
            take: () => { this.taken += 1; this.emit(`T:${this.taken}`); return 0x700 + this.taken; },
            submenu: (label, child, enabled, checked) => {
                this.emit(`S:${hex(label)}:${child}:${enabled}:${checked}`);
                if (label === 'AI Debug') {
                    if (this.mutation === 3) put(this.player, 0xb0, 2);
                    this.captured = get(this.player, 0xb0);
                }
            }
        };
    }
}

test('AI 디버그 메뉴: 조건부 수명·순서·같은 현재 brain 재조회를 실제 PE 60행 전체와 대조한다', () => {
    assert.equal(rows.length, 60);
    // 새 고정 관측의 입력/출력을 분리해 읽는다.
    rows.forEach((row, index) => {
        assert.equal(row.length, 2);
        const data = row[0].split(',');
        assert.equal(data.length, 20);
        const input = new MenuInput(data);
        openAiDebugMenu(input.owner, input.reads(), input.hooks());
        assert.equal(input.trace.length === 0 ? '-' : input.trace.join(';'), row[1], `메뉴 행 ${index}`);
        assert.equal(input.playerReads, input.checking && ((input.owner >>> 0) | 0) > 0 ? 1 : 0, `Player 조회 횟수 행 ${index}`);
    });
});

test('AI 디버그 메뉴: 첫 효과 중단 뒤 후속 메뉴를 합성하지 않고 초기 단락은 미연결 GUI 를 요구하지 않는다', () => {
    const skipped = { checking: () => 0 };
    openAiDebugMenu(0, skipped, {});
    skipped.checking = () => 1;
    openAiDebugMenu(0xffffffff, skipped, {});
    const input = new MenuInput('1,1,1,0,0,30000,0,0,0,1073741824,3,6,8,99,29,99,29,99,99,99'.split(','));
    input.stop = 'F:';
    assert.ok(fails(() => openAiDebugMenu(1, input.reads(), input.hooks())));
    assert.ok(input.trace.join(';') === 'H:4e4f5420494e495449414c495a45440a;F:80:7e6b4d6f6e65793d25647e2e0a:30000' && !input.taken);
    input.trace.length = 0;
    input.stop = '';
    put(input.first, 0x2d4, 5);
    // 원문 목표 배열 밖을 임의 문구로 바꾸지 않는다.
    assert.ok(fails(() => openAiDebugMenu(1, input.reads(), input.hooks())));
    assert.ok(input.trace.join(';') === 'H:4e4f5420494e495449414c495a45440a;F:80:7e6b4d6f6e65793d25647e2e0a:30000;H:4631' && !input.taken);
});

test('AI 디버그 메뉴: owner 9 이상과 해제/NULL brain 재조회는 가짜 성공 없이 거부하고 선택되지 않은 하위는 요구하지 않는다', () => {
    const input = new MenuInput('1,9,1,0,0,30000,0,0,0,1073741824,3,6,8,99,29,99,29,99,99,99'.split(','));
    assert.ok(fails(() => openAiDebugMenu(9, input.reads(), input.hooks())));
    assert.equal(input.playerReads, 0);
    // brain 이 0 이면 어떤 효과도 선택하지 않아 하위 경계(format/header 등)를 요구하지 않는다.
    const absent = new MenuInput('1,1,0,0,0,30000,0,0,0,1073741824,3,6,8,99,29,99,29,99,99,99'.split(','));
    openAiDebugMenu(1, absent.reads(), {});
    assert.ok(absent.trace.length === 0 && absent.playerReads === 1);
    // 보관된 brain 토큰이 해제/미지(3)이면 본문 재조회가 거부된다.
    const stale = new MenuInput('1,1,1,0,1,30000,0,0,0,1073741824,3,6,8,99,29,99,29,99,99,99'.split(','));
    put(stale.player, 0xb0, 3);
    assert.ok(fails(() => openAiDebugMenu(1, stale.reads(), stale.hooks())));
    assert.equal(stale.trace.length, 0);
});
