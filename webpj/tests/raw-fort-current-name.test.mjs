// 단독 미션의 Fort 현재 이름 선택을 보호 10.78 관측과 물리·호출 순서로 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { setCurrentFortName, setFortManagerFileName } from '../www/core/raw-fort-current-name.js';
import { REPO_ROOT } from './helpers.mjs';

// NUL 뒤 물리를 보존하면서 원본 고정 입력을 쓴다.
function input(target, text) { target.set(Buffer.from(text, 'latin1')); target[text.length] = 0; }
// 원본 장치처럼 첫 NUL 앞의 바이트만 읽는다.
function text(bytes) { const end = bytes.indexOf(0); if (end < 0) throw new Error('검사 입력 NUL 누락'); return Buffer.from(bytes.subarray(0, end)).toString('latin1'); }
// 원본 관측 표의 manager 없는 경로를 실제 몸체에 공급하고 이름·scratch·전역·통지를 비교한다.
test('Fort 현재 이름: 원본 관측의 manager 없는 경로와 편집 설정', () => {
    const file = path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-current-name-x86.tsv');
    const rows = fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t'));
    const selected = rows.filter((row) => { const f = row[1].split(','); return f[0] === 'current' && f[1] === '1' && f[2] === '0' && f[3] === '0' && f[4] === '0' && f[5] === '0' && Number(f[6]) < 8 && f[7] === '0' && f[12] === 'none'; });
    assert.equal(selected.length, 60);
    const names = ['New', 'Old', 'oLd', 'Bad.Name', '', 'M'.repeat(255), 'M'.repeat(256), 'New\0.after'];
    // 각 원본 행마다 독립된 현재 이름·입력·미작성 scratch로 시작한다.
    for (const row of selected) {
        const f = row[1].split(','), current = new Uint8Array(256).fill(0xa7), argument = new Uint8Array(512).fill(0xb6), scratch = new Uint8Array(256).fill(0xcd), header = new Uint8Array(20), calls = [];
        input(current, 'Old'); input(scratch, 'Prior'); const selectedName = names[Number(f[6])];
        if (Number(f[6]) === 7) argument.set(Buffer.from(selectedName, 'latin1')); else input(argument, selectedName);
        const globals = { owner: () => 1, fort: () => Number(f[8]), editor: () => Number(f[9]), mission: () => Number(f[10]), dirty: 0xfffffffe };
        const hooks = { header: () => header, currentName: () => current, assertion: (line) => calls.push(`A:${line}`), refreshTitle: () => calls.push(`T:${Buffer.from(text(current), 'latin1').toString('hex')}`),
            config: (key, value) => calls.push(`C:${key}:${Buffer.from(text(value), 'latin1').toString('hex')}`) };
        setCurrentFortName(argument, scratch, globals, hooks);
        const expected = row[4].split(':');
        assert.equal(Buffer.from(current).toString('hex'), expected[3], row[1]); assert.equal(Buffer.from(scratch).toString('hex'), expected[7], row[1]);
        assert.equal(globals.dirty, Number(row[3].split(',')[2]), row[1]); assert.equal(calls.join(';'), row[5], row[1]);
    }
});

// 첫 Snapshot 뒤 바뀐 owner와 고정 헤더를 재조회하고 원본 효과 순서를 지킨다.
test('Fort 현재 이름: Snapshot 뒤 owner 변경과 현재 manager 경로', () => {
    let owner = 1; const headers = [new Uint8Array(20), new Uint8Array(20), new Uint8Array(20)];
    new DataView(headers[1].buffer).setUint32(0, 11, true); new DataView(headers[2].buffer).setUint32(0, 22, true);
    const current = new Uint8Array(256), scratch = new Uint8Array(256), managers = new Map([[11, new Uint8Array(256)], [22, new Uint8Array(256)]]), calls = []; input(current, 'Old');
    const globals = { owner: () => owner, fort: () => 0, editor: () => 0, mission: () => 1, dirty: 4 };
    const hooks = { header: (slot) => headers[slot], currentName: () => current, managerName: (token) => managers.get(token), selectFileSpec: () => {},
        pathSpec: (name) => { const value = text(name); calls.push(`경로:${value}`); return Uint8Array.from(Buffer.from(`d/${value}.fort\0`, 'latin1')); },
        assertion: (line) => calls.push(`진단:${line}`), snapshot: () => { calls.push(`저장:${owner}`); owner = 2; }, notify: (slot) => calls.push(`통지:${slot}`),
        samePath: (old, next) => text(old).toLowerCase() === text(next).toLowerCase(), deleteRequest: (old) => calls.push(`삭제:${text(old)}`), refreshTitle: () => calls.push(`제목:${text(current)}`) };
    setCurrentFortName(Uint8Array.from(Buffer.from('New\0')), scratch, globals, hooks);
    assert.equal(text(managers.get(22)), 'd/New.fort'); assert.equal(globals.dirty, 5);
    assert.deepEqual(calls, ['경로:Old', '저장:1', '경로:New', '통지:2', '저장:2', '경로:New', '삭제:d/Old.fort', '제목:New']);
});

// 고정 manager 진단 뒤 같은 헤더를 다시 읽고 복사 실패 전까지의 물리 쓰기를 남긴다.
test('Fort manager 이름: 진단 뒤 현재 토큰과 부분 복사', () => {
    const header = new Uint8Array(20), target = new Uint8Array(2), calls = [];
    const hooks = { header: () => header, assertion: (line) => { calls.push(line); if (line === 429) new DataView(header.buffer).setUint32(0, 11, true); }, managerName: () => target, notify: () => calls.push('통지') };
    assert.throws(() => setFortManagerFileName(1, Uint8Array.of(65, 66, 0), hooks), /복사 물리 범위/);
    assert.deepEqual([...target], [65, 66]); assert.deepEqual(calls, [429]);
});
