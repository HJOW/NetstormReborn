// 객체 소유자 명령 object-owner-commands.js(dispatchObjectOwner·followEditorObjectOwner·finishEditorPrimarySelection)를 원본 10.78 관측 256+256행과 cpppj `ObjectOwnerCommandsTests` 계약으로 대조한다(WB04-E531).
//  - 원본 관측 object-owner-command-x86.tsv 256행: 편집/미션 후처리/선택/새 owner/가상 74 뒤 변경(bit0 Editor 반전·bit1 선택 51 로 교체)/revision 입력에서 효과 순서(`O` 가상 74·`C` Player Core 선택)와 최종 상태
//    (revision,editor,selected,보관 SID 의 owner BYTE)를 대조하고 풀 전체가 보관 SID 의 owner BYTE 밖에서는 변하지 않음을 확인한다.
//  - 원본 관측 editor-primary-selection-x86.tsv 256행: 좌클릭 후처리(보관 SID·뒤늦은 owner·다리 순환)의 효과 순서(`F`·`O`·`B`·`C`)와 최종 상태.
//  - 계약: release 와 알 수 없는 명령은 하위를 요구하지 않고, 선택된 하위 누락은 앞선 효과를 보존한 채 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E531)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { InputCode } from '../www/core/input-queue.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SET_OBJECT_OWNER_COMMAND, dispatchObjectOwner, finishEditorPrimarySelection, followEditorObjectOwner } from '../www/core/object-owner-commands.js';

// 원본 관측 두 파일이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E531)
const commandRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'object-owner-command-x86.tsv'));
const primaryRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'editor-primary-selection-x86.tsv'));

// 효과 문자열을 세미콜론으로 이어 붙인다. (2026-11-15 WB04-E531)
const append = (trace, value) => (trace ? `${trace};${value}` : value);

// 원문 this 와 같은 클라이언트 물리 SID 50 을 가진 풀이다(SID 50 까지 할당). (2026-11-15 WB04-E531)
function makeScene() {
    const pool = new SidPool(24000, { server: true });
    for (let sid = 5; sid <= 50; sid += 1) assert.equal(pool.allocate(2), sid);
    return { pool, view: pool.allocated(50) };
}

test('SetOwner 명령: 원본 256행의 모드 단락·보관 SID·뒤늦은 Editor 조회·revision 감김이 같다', { skip: !commandRows && '원본 fixture 없음' }, () => {
    assert.equal(commandRows.length, 256);
    let count = 0;
    for (const row of commandRows) {
        assert.equal(row.length, 8);
        const { pool, view } = makeScene();
        view[34] = 2;
        const before = Uint8Array.from(pool.bytes());
        let editor = Number(row[0]);
        const mission = Number(row[1]);
        let selected = Number(row[2]);
        const revision = { value: Number(row[5]) };
        const mutation = Number(row[4]);
        let trace = '';
        const hooks = {
            editor: () => editor,
            missionPost: () => mission,
            selected: () => selected,
            owner: (sid, owner) => {
                trace = append(trace, `O,${sid},${owner}`);
                pool.allocated(sid)[34] = owner & 0xff;
                if (mutation & 1) editor = 1 - editor;
                if (mutation & 2) {
                    selected = 51;
                    view[34] = 6;
                }
            },
            follow: (sid) => { followEditorObjectOwner(pool, sid, revision, (owner, templateOwner) => { trace = append(trace, `C,${owner},${templateOwner}`); }); },
        };
        assert.equal(dispatchObjectOwner({ code: SET_OBJECT_OWNER_COMMAND, x: 0, y: Number(row[3]) | 0 }, revision, hooks), true);
        const label = `행 ${count}: ${row.slice(0, 6).join(',')}`;
        assert.equal(trace === '' ? '-' : trace, row[6], label);
        assert.equal(`${revision.value},${editor},${selected},${pool.slot(50)[34]}`, row[7], label);
        // 보관 SID 의 owner BYTE 밖에서는 풀이 변하지 않는다.
        const after = Uint8Array.from(pool.bytes());
        after[50 * 50 + 34] = before[50 * 50 + 34];
        assert.deepEqual(after, before, label);
        count += 1;
    }
    assert.equal(count, 256);
});

test('편집 좌클릭 후처리: 원본 256행의 보관 SID·뒤늦은 owner·다리 순환·효과 순서가 같다', { skip: !primaryRows && '원본 fixture 없음' }, () => {
    assert.equal(primaryRows.length, 256);
    let count = 0;
    for (const row of primaryRows) {
        assert.equal(row.length, 9);
        const { pool, view } = makeScene();
        let editor = Number(row[0]);
        let selected = Number(row[2]);
        const revision = { value: Number(row[6]) };
        const mutation = Number(row[5]);
        view[34] = Number(row[3]) & 0xff;
        let trace = '';
        const hooks = {
            editor: () => editor,
            selected: () => selected,
            localOwner: () => 1,
            readOwner: (sid) => pool.slot(sid)[34],
            flags2: (sid) => { trace = append(trace, `F,${sid}`); return Number(row[4]); },
            assignOwner: (sid, owner) => {
                trace = append(trace, `O,${sid},${owner}`);
                pool.allocated(sid)[34] = owner & 0xff;
                if (mutation & 1) editor = 1 - editor;
                if (mutation & 2) {
                    selected = 51;
                    view[34] = 6;
                }
            },
            cycleBridge: (sid) => { trace = append(trace, `B,${sid}`); },
            follow: (sid) => { followEditorObjectOwner(pool, sid, revision, (owner, templateOwner) => { trace = append(trace, `C,${owner},${templateOwner}`); }); },
        };
        finishEditorPrimarySelection(Number(row[1]) !== 0, hooks);
        const label = `행 ${count}: ${row.slice(0, 6).join(',')}`;
        assert.equal(trace === '' ? '-' : trace, row[7], label);
        assert.equal(`${revision.value},${editor},${selected},${pool.slot(50)[34]}`, row[8], label);
        count += 1;
    }
    assert.equal(count, 256);
});

test('편집 좌클릭 후처리 계약: 미선택 하위는 요구하지 않고 선택된 하위 누락은 앞선 효과를 보존한 채 거부한다', () => {
    const hooks = { editor: () => 0 };
    assert.doesNotThrow(() => finishEditorPrimarySelection(true, hooks));
    hooks.editor = () => 1;
    hooks.selected = () => 0;
    assert.doesNotThrow(() => finishEditorPrimarySelection(true, hooks));
    hooks.selected = () => 50;
    let followed = 0;
    let assigned = 0;
    hooks.follow = (sid) => { followed = sid; };
    finishEditorPrimarySelection(false, hooks);
    assert.equal(followed, 50);
    followed = 0;
    hooks.readOwner = () => 2;
    hooks.localOwner = () => 1;
    hooks.assignOwner = (sid, owner) => { assert.equal(sid, 50); assigned = owner; };
    hooks.follow = undefined;
    assert.throws(() => finishEditorPrimarySelection(true, hooks), /실제 하위 미연결/);
    assert.equal(assigned, 1);
    assert.equal(followed, 0);
});

test('SetOwner 명령 계약: release·다른 명령은 하위를 요구하지 않고 선택된 하위 누락은 거부하며 Follow 는 0 owner 가 무동작이고 풀 범위를 지킨다', () => {
    const revision = { value: 7 };
    const hooks = {};
    assert.equal(dispatchObjectOwner({ code: (SET_OBJECT_OWNER_COMMAND | InputCode.release) >>> 0, x: 0, y: 1 }, revision, hooks), true);
    assert.equal(revision.value, 7);
    assert.equal(dispatchObjectOwner({ code: 0x02000095, x: 0, y: 1 }, revision, hooks), false);
    hooks.editor = () => 0;
    hooks.missionPost = () => 0;
    assert.equal(dispatchObjectOwner({ code: SET_OBJECT_OWNER_COMMAND, x: 0, y: 1 }, revision, hooks), true);
    assert.equal(revision.value, 7);
    hooks.editor = () => 1;
    hooks.selected = () => 0;
    assert.equal(dispatchObjectOwner({ code: SET_OBJECT_OWNER_COMMAND, x: 0, y: 1 }, revision, hooks), true);
    assert.equal(revision.value, 8);
    // 선택된 가상 74 누락은 revision 전에 실패한다.
    hooks.selected = () => 50;
    assert.throws(() => dispatchObjectOwner({ code: SET_OBJECT_OWNER_COMMAND, x: 0, y: 1 }, revision, hooks), /실제 하위 미연결/);
    assert.equal(revision.value, 8);
    let changed = false;
    hooks.owner = () => { changed = true; };
    assert.throws(() => dispatchObjectOwner({ code: SET_OBJECT_OWNER_COMMAND, x: 0, y: 1 }, revision, hooks), /실제 하위 미연결/);
    assert.ok(changed);
    assert.equal(revision.value, 8);
    // revision DWORD 감김.
    const wrap = { value: 0xffffffff };
    hooks.owner = () => {};
    hooks.follow = () => {};
    dispatchObjectOwner({ code: SET_OBJECT_OWNER_COMMAND, x: 0, y: 1 }, wrap, hooks);
    assert.equal(wrap.value, 0);
    // Follow: owner 0 은 무동작, 비영은 선택 하위가 없으면 거부, SID 범위 밖은 거부.
    const { pool, view } = makeScene();
    view[34] = 0;
    followEditorObjectOwner(pool, 50, revision, undefined);
    assert.equal(revision.value, 8);
    view[34] = 2;
    assert.throws(() => followEditorObjectOwner(pool, 50, revision, undefined), /실제 하위 미연결/);
    assert.equal(revision.value, 8);
    assert.throws(() => followEditorObjectOwner(pool, pool.capacity, revision, () => {}), RangeError);
    assert.throws(() => followEditorObjectOwner({ edition: 'cd1072', capacity: 100 }, 5, revision, () => {}), RangeError);
});
