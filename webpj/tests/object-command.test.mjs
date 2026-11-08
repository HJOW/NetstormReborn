// 공통 객체 명령 raw-object-command.js(RawObjectCommand)를 원본 10.78 관측 160행과 cpppj 계약으로 대조한다.
//  - 원본 관측: object-command-x86.tsv 의 `originals` 160행 — (명령 종류, 상위 비트, 편집기, 미션, 슬롯 HP, 로컬 소유자) 입력으로 반환(처리 1/미처리 0)과 가상 효과(owner/hit/destroy/describe) 인자 순서가 같고 슬롯 HP DWORD 는 변하지 않는다.
//    `originalCD` 행은 10.72(WORD HP/무조건 Hurt)라 사용하지 않는다.
//  - 계약: 제한된 피해/미처리 명령은 외부 효과를 요구하지 않고 같은 객체의 현재 모드를 매번 읽으며, 판본이 다르면 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E123: 공통 객체 명령 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawObjectCommand } from '../www/core/raw-object-command.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E123)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'object-command-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 원본 데이터 표와 미등록 명령이다. (2026-11-08 WB04-E123)
const COMMANDS = [0x030000af, 0x030000e2, 0x03000015, 0x0300006e, 0x030003fe];

test('ObjectCommand: 원본 10.78 160입력의 분기·가상 인자·미처리 반환이 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: true });
    const parent = 50;
    const raw = pool.physical(parent);
    let count = 0;
    // 기대 분기를 재계산하지 않고 원본 반환과 호출 인자를 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const input = row[1].split(',').map(Number);
        assert.equal(input.length, 6);
        raw.fill(0xa5);
        put(raw, 26, input[4]);
        // 이번 제품 호출이 만든 가상 효과 순서이다.
        let trace = '';
        const state = { localOwner: () => input[5], editor: () => input[2] !== 0, mission: () => input[3] !== 0 };
        const hooks = {
            owner: (sid, owner) => { assert.equal(sid, parent); trace = `owner:${owner}`; },
            hit: (sid, amount, owner, attacker) => { assert.equal(sid, parent); trace = `hit:${amount >>> 0}:${owner}:${attacker}`; },
            destroy: (sid, flags) => { assert.equal(sid, parent); trace = `destroy:${flags}`; },
            describe: (sid) => { assert.equal(sid, parent); trace = 'describe'; }
        };
        const command = new RawObjectCommand(pool, state, hooks);
        assert.equal(command.command(parent, (COMMANDS[input[0]] | input[1]) >>> 0), Number(row[2]), `객체 명령 행 ${count} ${row[1]}`);
        assert.equal(trace === '' ? '-' : trace, row[3], `객체 명령 행 ${count} 효과`);
        assert.equal(get(raw, 26), input[4]);
        count += 1;
    }
    assert.equal(count, 160);
});

test('ObjectCommand: 제한된 피해/미처리는 외부 효과를 요구하지 않고 같은 객체의 현재 모드를 매번 읽는다', () => {
    const pool = new SidPool(24000, { server: true });
    let editor = false;
    let mission = false;
    let missionReads = 0;
    const state = { editor: () => editor, mission: () => { missionReads += 1; return mission; } };
    const command = new RawObjectCommand(pool, state, {});
    assert.equal(command.command(0, 0xffffffff), 0);
    assert.equal(command.command(0, 0x030000e2), 1);
    assert.equal(missionReads, 1);
    editor = true;
    assert.throws(() => command.command(0, 0x030000e2), Error);
    assert.equal(missionReads, 1);
    editor = false;
    mission = true;
    assert.throws(() => command.command(0, 0x030000e2), Error);
    assert.equal(missionReads, 2);
    // 소유자 명령은 현재 로컬 소유자 조회와 가상 효과가 모두 있어야 한다.
    assert.throws(() => command.command(0, 0x030000af), Error);
    assert.throws(() => new RawObjectCommand({ edition: 'cd1072' }, state, {}), Error);
});
