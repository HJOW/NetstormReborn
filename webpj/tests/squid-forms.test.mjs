// 공통 Form 층(squid-forms.js)의 원본 10.78 관측 대조와 부분 쓰기·현재 VT·GameAuxState 연결 계약을 검사한다(WB04-D3).
//  - 원본 관측(cpppj/tests/fixtures, 10.78 행만): form-constructor 28행·form-attach-current 432행·form-pop-current 1280행·form-unpop-current 864행·formtake 300행.
//    각 행의 합성 입력·명시 장치는 cpppj 검사(FormConstructorTests·FormAttachCurrentTests·FormPopCurrentTests·FormUnpopCurrentTests·FormTakeTests)와 같고 기대값은 fixture 에서 읽는다.
//  - 계약: cpppj 의 SquidForms 단독 검사(Take 통지 시점·풀 재확보, Unpop 통지 부재·진단 뒤 현재 VT, Pop 장치 부재, 부착 꼬리 진단·이웃 VT 부재·혼합 링크 WORD)를 옮겼다.
//    그 검사의 Factory.Create(74) 부모/이웃은 D4 범위라 같은 할당과 base 생성자 VT(0x501dd0)·TYPE 74 만 쓰는 합성 준비로 대신한다(링크 WORD 만 관측한다).
//    CreateGumpForm/CreateContained 는 Form 생성 경로가 VT→TYPE 만 쓰므로 SquidForms.create(3/6) 와 바이트가 같다.
//  - GameAuxState 연결: 부착 꼬리 진단과 contained 종단이 0054107c·00541088·005427dc 를 호출 시점마다 읽는지 확인한다(웹 단위 계약, 원본 관측 아님).
// 실제 게임·브라우저 실행이 아니다. (2026-11-06 WB04-D3: 공통 Form 층 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SQUID_BASE_VTABLE, SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import {
    RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetParent, commonContainedSetPrevious,
} from '../www/core/squid-contained-unpop.js';
import {
    SquidForms, attachCommonForm, popCommonForm, squidFormGlobals, formConstructorVtable, SQUID_FORM_TYPES,
} from '../www/core/squid-forms.js';

// 원본 관측 파일과 10.78 행이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-06 WB04-D3)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
// fixture 의 10.78 행만 고른다. (2026-11-06 WB04-D3)
const patchRows = (name) => readRows(path.join(FIXTURES, name))?.filter((row) => row[0] === 'originals') ?? null;
const constructorRows = patchRows('form-constructor-x86.tsv');
const attachRows = patchRows('form-attach-current-x86.tsv');
const popRows = patchRows('form-pop-current-x86.tsv');
const unpopRows = patchRows('form-unpop-current-x86.tsv');
const takeRows = patchRows('formtake-x86.tsv');

// 원문 Squid.cpp 진단 파일 이름이다. (2026-11-06 WB04-D3)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
// 원본 관측기가 명시 가상 장치에 준 VT 토큰 시작값이다. (2026-11-06 WB04-D3)
const INPUT_VT = 0x14000000;

// 바이트 배열의 소문자 16진 문자열이다(fixture 슬롯 표기). (2026-11-06 WB04-D3)
function hex(bytes) {
    return Buffer.from(bytes).toString('hex');
}

// 16진 문자열을 바이트 배열로 바꾼다. (2026-11-06 WB04-D3)
function unhex(text) {
    return Uint8Array.from(Buffer.from(text, 'hex'));
}

// 관측기와 같은 합성 payload 로 슬롯을 채운다(기대값은 계산하지 않는다). (2026-11-06 WB04-D3)
function fillPattern(raw, sid) {
    // 각 바이트에 (sid*50+offset)*37+19 의 하위 BYTE 를 둔다.
    for (let offset = 0; offset < raw.length; offset += 1) raw[offset] = ((sid * raw.length + offset) * 37 + 19) & 0xff;
}

// D4 Factory.Create(74) 대신 같은 할당과 base 생성자 VT·TYPE 74 만 쓰는 합성 부모/이웃이다(링크 WORD 관측용). (2026-11-06 WB04-D3)
function syntheticAsset(pool, flags = 0) {
    const sid = pool.allocate(flags);
    const raw = pool.physical(sid);
    put(raw, 0, SQUID_BASE_VTABLE);
    raw[10] = 74;
    return sid;
}

// 이미 부착된 단일 내용물 Form 의 명시 링크 입력이다(cpppj ContentAttachmentInput). (2026-11-06 WB04-D3)
function contentAttachmentInput(pool, form, parent) {
    const raw = pool.physical(form);
    raw[11] = 8;
    put(raw, 4, 0, 2);
    put(raw, 14, 0, 2);
    put(raw, 16, parent, 2);
    put(pool.physical(parent), 6, form, 2);
}

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-06 WB04-D3)
function throws(action) {
    try {
        action();
    } catch {
        return true;
    }
    return false;
}

// 원본 28행: 실제 생성자는 지정 물리 슬롯(0·예약·free·dead·끝)에 VT DWORD 하나만 쓰고 나머지 payload·풀 카운터를 보존한다.
test('SquidForms.constructVtableAt: 원본10.78 생성자 28행과 같다', { skip: !constructorRows && '원본 fixture 없음' }, () => {
    // 행마다 새 client 풀(65535)에서 실행한다.
    for (const row of constructorRows) {
        assert.equal(row.length, 6);
        const pool = new SidPool(65535, { server: false });
        const type = Number(row[1]);
        const sid = Number(row[2]);
        const before = unhex(row[3]);
        const raw = pool.physical(sid);
        assert.equal(before.length, raw.length);
        raw.set(before);
        const free = pool.freeCount;
        new SquidForms(pool).constructVtableAt(type, sid);
        assert.equal(hex(raw), row[4], `type ${type} sid ${sid}`);
        assert.equal(pool.freeCount, free);
        assert.equal(raw[10], 74);
        assert.deepEqual([...raw.subarray(4)], [...before.subarray(4)]);
        assert.equal(Number(row[5]), sid * pool.layout.stride);
    }
    assert.equal(constructorRows.length, 28);
});

// 원본 432행: 공통 부착 부모의 genus/setter/Pop/진단 호출 순서·정상 복귀 뒤 현재 VT/TYPE·저장 머리와 네 슬롯 전체.
test('attachCommonForm: 원본10.78 공통 부착 432행과 같다', { skip: !attachRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 풀과 명시 장치로 실행한다.
    for (const row of attachRows) {
        assert.equal(row.length, 5);
        const [parent, number, genus, head, profile, flags] = row[1].split(',').map(Number);
        let content = 6;
        const pool = new SidPool(32768);
        // 0~15 슬롯에 합성 payload 를 둔다.
        for (let sid = 0; sid < 16; sid += 1) fillPattern(pool.physical(sid), sid);
        // 세 명시 가상 장치의 초기 VT 토큰이다(5·7·9).
        for (let index = 0; index < 3; index += 1) put(pool.physical(5 + index * 2), 0, INPUT_VT + index * 0x100);
        const raw = pool.physical(5);
        raw[10] = number;
        raw[11] = 4;
        put(pool.physical(parent), 6, head, 2);
        const events = [];
        const hooks = {
            genus: (current) => {
                events.push(`G${current}`);
                return genus;
            },
            contentType: () => content,
            // 정상 복귀 뒤 현재 VT/TYPE 와 저장 머리 차이를 만드는 관측기 입력이다.
            assertion: (expression, file, line) => {
                events.push(`D${expression}|${file}|${line}`);
                if (profile === 1) {
                    put(raw, 0, INPUT_VT + 0x100);
                    raw[10] = 69;
                }
                if (profile === 3 && expression === 'n != getSid()') {
                    put(pool.physical(parent), 6, 7, 2);
                    put(raw, 16, 11, 2);
                }
                if (profile === 5 && expression === 'getType()!=contentFormType') raw[10] = 71;
            },
            setParent: (sid, table, value) => {
                events.push(`P${sid},${table},${value}`);
                put(pool.physical(sid), 16, value, 2);
                if (profile === 2) {
                    put(raw, 0, INPUT_VT + 0x200);
                    put(pool.physical(parent), 6, 7, 2);
                    put(raw, 16, 11, 2);
                }
            },
            setPrevious: (sid, table, value) => {
                events.push(`R${sid},${table},${value}`);
                put(pool.physical(sid), 14, value, 2);
                if (profile === 4 && value === 5) {
                    put(raw, 0, INPUT_VT + 0x100);
                    put(raw, 16, 11, 2);
                    put(pool.physical(parent), 6, 13, 2);
                }
            },
            pop: (sid, value) => {
                events.push(`O${value},${raw[11]},${get(raw, 4, 2)},${get(raw, 14, 2)},${get(raw, 16, 2)},${get(pool.physical(parent), 6, 2)}`);
                raw[11] &= ~4 & 0xff;
                if (profile === 5) {
                    raw[10] = 70;
                    content = 70;
                }
            },
        };
        attachCommonForm(pool, 5, parent, flags, hooks);
        assert.equal(events.join(';'), row[2], row[1]);
        assert.equal(String(content), row[3], row[1]);
        assert.equal([0, 5, 7, 9].map((sid) => hex(pool.slot(sid))).join(';'), row[4], row[1]);
    }
    assert.equal(attachRows.length, 432);
});

// 원본 1280행: 공통 Pop 부모의 진단/flags 정규화·부분 상태(void·xmit)·현재 권한/VT·공유 깊이와 this 슬롯 전체.
test('popCommonForm: 원본10.78 공통 Pop 1280행과 같다', { skip: !popRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 풀과 같은 {value} 권한 소유자로 실행한다.
    for (const row of popRows) {
        assert.equal(row.length, 6);
        const [sid, state, flags, server, initialDepth, profile] = row[1].split(',').map(Number);
        const owner = { value: server !== 0 };
        let depth = initialDepth >>> 0;
        const pool = new SidPool(32768, { serverOwner: owner });
        const raw = pool.physical(sid);
        fillPattern(raw, sid);
        put(raw, 0, INPUT_VT);
        raw[11] = state;
        const events = [];
        const hooks = {
            server: () => owner.value,
            depth: () => depth,
            setDepth: (value) => { depth = value >>> 0; },
            // 진단 복귀가 바꾼 현재 권한/상태/VT/깊이를 다음 명령이 읽어야 한다.
            assertion: (expression, file, line) => {
                events.push(`D${expression}|${file}|${line},${raw[11]},${owner.value ? 1 : 0},${depth}`);
                if (profile === 1) {
                    raw[11] = 8;
                    put(raw, 0, INPUT_VT + 0x100);
                    owner.value = !owner.value;
                    depth = 0xffffffff;
                }
            },
            // 실제 패킷 작성/송신이 아닌 명시 자식 관측이다.
            transmit: (actual, x, y, z, value) => {
                assert.equal(actual, sid);
                events.push(`T${x},${y},${z},${value},${raw[11]}`);
                if (profile === 2) {
                    raw[11] |= 0x40;
                    put(raw, 0, INPUT_VT + 0x100);
                    depth = 7;
                }
            },
            // 정상 감소 또는 잘못 반납한 깊이 진단 입력이다.
            postPop: (actual, table, value) => {
                assert.equal(actual, sid);
                events.push(`P${table},${value},${raw[11]},${depth}`);
                depth = (depth + (profile === 3 ? 1 : 0xffffffff)) >>> 0;
            },
        };
        popCommonForm(pool, sid, flags, hooks);
        assert.equal(events.join(';'), row[2], row[1]);
        assert.equal(String(owner.value ? 1 : 0), row[3], row[1]);
        assert.equal(String(depth), row[4], row[1]);
        assert.equal(hex(pool.slot(sid)), row[5], row[1]);
    }
    assert.equal(popRows.length, 1280);
});

// 원본 864행: Form Unpop 의 세 진단·void 쓰기·현재 VT 의 부모 getter·부모 free 관문(getter 뒤 재조회)·분리 호출과 두 슬롯 전체.
test('SquidForms.unpop: 원본10.78 Form Unpop 864행과 같다', { skip: !unpopRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 풀과 명시 장치로 실행한다.
    for (const row of unpopRows) {
        assert.equal(row.length, 4);
        const [sid, number, state, parentState, profile] = row[1].split(',').map(Number);
        const pool = new SidPool(32768);
        // 0~15 슬롯에 합성 payload 를 둔다.
        for (let index = 0; index < 16; index += 1) fillPattern(pool.physical(index), index);
        const raw = pool.physical(sid);
        put(raw, 0, INPUT_VT);
        raw[10] = number;
        raw[11] = state;
        pool.physical(9)[11] = parentState;
        const events = [];
        const hooks = {
            // 정상 복귀와 외부 변경 입력이다.
            assertion: (expression, file, line) => {
                events.push(`D${expression}|${file}|${line}`);
                if (profile === 1) {
                    raw[10] = 6;
                    raw[11] = 8;
                    put(raw, 0, INPUT_VT + 0x100);
                }
            },
            // getter 뒤 바뀐 현재 부모 free 를 본문이 다시 읽어야 한다.
            parent: (current, table) => {
                assert.equal(current, sid);
                events.push(`P${table}`);
                if (profile === 2) pool.physical(9)[11] ^= 1;
                return 9;
            },
            // 실제 종속 자식이 아닌 관측 장치이다.
            detach: (current) => {
                assert.equal(current, sid);
                events.push(`U${pool.slot(current)[11]}`);
            },
        };
        new SquidForms(pool, { unpop: hooks }).unpop(sid);
        assert.equal(events.join(';'), row[2], row[1]);
        assert.equal(`${hex(pool.slot(sid))};${hex(pool.slot(9))}`, row[3], row[1]);
    }
    assert.equal(unpopRows.length, 864);
});

// 원본 300행: 생성·지정 Take·Unpop·풀 재확보의 반환값·통지 당시 Form/부모 전체 바이트·관측 슬롯·풀 카운터/머리/꼬리·전체 풀 Adler-32.
test('SquidForms: 원본10.78 Create/Take/Unpop/Reset 300행과 같다', { skip: !takeRows && '원본 fixture 없음' }, () => {
    // 행마다 새 풀에서 실행한다.
    for (const row of takeRows) {
        assert.equal(row.length, 17);
        const pool = new SidPool(32768, { server: row[4] === '1' });
        // fixture 의 초기 오염 바이트만 복원한다(free 슬롯 포함).
        if (row[5]) {
            for (const slot of row[5].split(';')) {
                const [sid, bytes] = slot.split(':');
                pool.physical(Number(sid)).set(unhex(bytes));
            }
        }
        const notices = [];
        const forms = new SquidForms(pool, {
            // 부모 +0x24 경계에서 당시 Form 과 부모 전체 바이트를 기록한다.
            notice: (parent, form) => notices.push(`${parent}:${form}:2:${hex(pool.slot(form))}:${hex(pool.slot(parent))}`),
        });
        const number = Number(row[2]);
        const arg = Number.parseInt(row[3], 10);
        let result = 0;
        if (row[1] === 'Create') result = forms.create(number, arg >>> 0);
        else if (row[1] === 'Take') result = forms.take(number, arg & 0xffff);
        else if (row[1] === 'Unpop') forms.unpop(arg & 0xffff);
        else if (row[1] === 'Reset') pool.reset(arg);
        else assert.fail(`fixture 연산 오류 ${row[1]}`);
        const label = `${row[1]} ${row[2]} ${row[3]}`;
        assert.equal(result, Number(row[7]), label);
        assert.equal(notices.join(';'), row[16], label);
        // 관측 슬롯은 체크섬이 아닌 전체 바이트를 비교한다.
        if (row[6]) {
            for (const slot of row[6].split(';')) {
                const [sid, bytes] = slot.split(':');
                assert.equal(hex(pool.slot(Number(sid))), bytes, `${label} slot ${sid}`);
            }
        }
        const actual = [pool.capacity, pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.tail(true),
            pool.firstFree(false), pool.tail(false), adler32(pool.bytes())];
        assert.deepEqual(actual, row.slice(8, 16).map(Number), label);
    }
    assert.equal(takeRows.length, 300);
});

// 타입·생성자 표: 네 Form 타입만 지원하고 나머지는 할당 전에 거부한다.
test('SquidForms: 네 내장 Form 생성자 VT 와 타입 거부', () => {
    assert.deepEqual(SQUID_FORM_TYPES, [1, 2, 3, 6]);
    assert.deepEqual(SQUID_FORM_TYPES.map(formConstructorVtable),
        [SQUID_FORM_VTABLES.depend, SQUID_FORM_VTABLES.process, SQUID_FORM_VTABLES.gump, SQUID_FORM_VTABLES.content]);
    const pool = new SidPool(32768);
    const forms = new SquidForms(pool);
    const free = pool.freeCount;
    // 네 타입 밖 번호는 할당 전에 거부한다.
    for (const type of [0, 4, 5, 7, 74, 1.5]) assert.ok(throws(() => forms.create(type, 2)), `type ${type}`);
    assert.equal(pool.freeCount, free);
    // 다른 판본/객체 풀은 첫 쓰기 전에 거부한다.
    assert.ok(throws(() => new SquidForms({ edition: 'cd1072' })));
    const gump = forms.create(3, 2);
    assert.equal(forms.parent(gump), 0);
    assert.equal(forms.previous(gump), 0);
    // 예약/free 슬롯과 Form 이 아닌 현재 VT 는 부모/이전 getter 가 거부한다.
    assert.ok(throws(() => forms.parent(4)));
    assert.ok(throws(() => forms.parent(gump + 1)));
    put(pool.physical(gump), 0, SQUID_BASE_VTABLE);
    assert.ok(throws(() => forms.previous(gump)));
});

// Take 통지 시점: 부모 알림은 링크/contained 해제 뒤에 오며 알림이 고친 payload 가 Take 생성 뒤에도 남는다(cpppj FormTake_ContentNoticeTimingAndPayloadMutation).
test('SquidForms.take: 내용물 부모 통지 시점과 payload 보존', () => {
    const pool = new SidPool(32768);
    const forms = new SquidForms(pool);
    const parent = forms.create(1, 2);
    const form = forms.create(6, 0);
    put(pool.allocated(parent), 6, form, 2);
    put(pool.allocated(form), 16, parent, 2);
    pool.allocated(form)[11] = 8;
    // 원문은 부모 알림 전에 void/contained 와 부모 머리를 이미 쓴다. 나머지 바이트는 보존한다.
    const detached = pool.slot(form);
    detached[11] = 4;
    assert.ok(throws(() => forms.take(6, form)));
    assert.equal(hex(pool.slot(form)), hex(detached));
    assert.equal(get(pool.slot(parent), 6, 2), 0);
    put(pool.allocated(parent), 6, form, 2);
    pool.allocated(form)[11] = 8;
    let notices = 0;
    // 부모 +0x24 경계에서 현재 해제 상태를 읽고 payload 를 바꾼다.
    const bound = new SquidForms(pool, {
        notice: (notifiedParent, notifiedForm) => {
            notices += 1;
            assert.equal(notifiedParent, parent);
            assert.equal(notifiedForm, form);
            assert.equal(pool.slot(form)[11] & 12, 4);
            assert.equal(get(pool.slot(parent), 6, 2), 0);
            put(pool.allocated(form), 18, 0xfedc5678, 4);
        },
    });
    assert.equal(bound.take(6, form), form);
    assert.equal(notices, 1);
    assert.equal(get(pool.slot(form), 18, 4), 0xfedc5678);
    assert.equal(get(pool.slot(form), 0), SQUID_FORM_VTABLES.content);
    assert.equal(pool.slot(form)[11], 4);
    // 이미 void 인 Form 은 Unpop 진단(미연결)에서, 다른 TYPE 은 Take 입구에서, client 번호는 범위에서 멈춘다.
    assert.ok(throws(() => bound.take(6, form)));
    assert.ok(throws(() => bound.take(3, form)));
    assert.ok(throws(() => bound.take(6, parent)));
    const free = pool.freeCount;
    assert.ok(throws(() => bound.create(4, 2)));
    assert.equal(pool.freeCount, free);
});

// 풀 재확보: 양수 reset 뒤에도 같은 처리기를 쓰고 삭제 기록·권한을 보존하며 잘못된 크기는 기존 상태를 바꾸지 않는다(cpppj FormTake_ResizePreservesRecordsAndRejectsBadCapacity).
test('SquidForms: 풀 재확보 뒤 같은 처리기와 삭제 기록 보존', () => {
    const pool = new SidPool(32768, { server: false });
    const forms = new SquidForms(pool);
    const sid = forms.create(3, 2);
    pool.release(sid);
    const deletion = pool.deletions(true)[0];
    assert.deepEqual(deletion, { sid, type: 3 });
    pool.reset(32768 + 17);
    assert.equal(pool.isServer, false);
    assert.equal(pool.capacity, 32768 + 17);
    assert.deepEqual(pool.deletions(true)[0], deletion);
    assert.equal(forms.create(1, 2), 5);
    pool.reset(-1);
    assert.equal(pool.capacity, 32768 + 17);
    assert.equal(pool.firstFree(true), 5);
    const before = pool.bytes();
    assert.ok(throws(() => pool.reset(1)));
    assert.ok(throws(() => pool.reset(65536)));
    assert.deepEqual(pool.bytes(), before);
    assert.deepEqual(pool.deletions(true)[0], deletion);
});

// Unpop 통지 부재: 선택된 부모 알림이 없으면 앞선 링크/void 쓰기를 남긴 채 멈춘다(cpppj FormUnpopCurrent_MissingNoticeKeepsCompletedLinkWrites).
test('SquidForms.unpop: 통지 부재는 완료된 링크 쓰기를 남긴다', () => {
    const pool = new SidPool(32768);
    const parent = syntheticAsset(pool);
    const forms = new SquidForms(pool);
    const form = forms.create(6, 0);
    contentAttachmentInput(pool, form, parent);
    put(pool.physical(form), 18, 0xabcdef01);
    const free = pool.freeCount;
    assert.ok(throws(() => forms.unpop(form)));
    assert.equal(pool.slot(form)[11], 4);
    assert.equal(get(pool.slot(parent), 6, 2), 0);
    assert.equal(get(pool.slot(form), 18), 0xabcdef01);
    assert.equal(pool.freeCount, free);
});

// Unpop 진단 복귀: 잘못된 TYPE 진단이 고친 현재 VT/상태를 실제 공통 getter/분리가 쓰고, 부모 free 는 분리를 생략하며 dead 는 분리 안에서 링크를 보존한다
// (cpppj FormUnpopCurrent_ActualGetterUsesRepairedVtableAfterDiagnostic).
test('SquidForms.unpop: 진단 뒤 현재 VT 와 부모 free/dead 분기', () => {
    const pool = new SidPool(32768);
    const parent = syntheticAsset(pool);
    const forms = new SquidForms(pool);
    const form = forms.create(3, 0);
    forms.attach(form, parent, 0x10);
    const raw = pool.physical(form);
    raw[10] = 70;
    put(raw, 0, 0xdeadbeef);
    let diagnostics = 0;
    // 고정 TYPE 의 VT 추정을 넣으면 이 정상 가상 진입을 잃는다.
    forms.bindUnpop({
        assertion: (expression, file, line) => {
            assert.deepEqual([expression, file, line], ['isForm()', SQUID_CPP, 0xa59]);
            diagnostics += 1;
            raw[10] = 69;
            put(raw, 0, SQUID_FORM_VTABLES.gump);
        },
    });
    forms.unpop(form);
    assert.equal(diagnostics, 1);
    assert.equal(raw[11], 4);
    assert.equal(get(pool.slot(parent), 6, 2), 0);
    // 부모 free(1·3)는 분리 자체를 생략하고 dead(2)는 분리 안에서 링크를 보존한다.
    for (const parentState of [1, 2, 3]) {
        raw[11] = 8;
        put(pool.physical(parent), 6, form, 2);
        pool.physical(parent)[11] = parentState;
        new SquidForms(pool).unpop(form);
        assert.equal(raw[11], 12, `parent state ${parentState}`);
        assert.equal(get(pool.slot(parent), 6, 2), form);
    }
});

// Pop 장치 부재: 선택된 전송/알 수 없는 후처리 실패는 앞선 void/xmit 비트/깊이 쓰기를 남긴다(cpppj FormPopCurrent_SelectedMissingDevicesKeepVoidAndDepthPrefix).
test('SquidForms.pop: 선택된 장치 부재는 void·깊이 쓰기를 남긴다', () => {
    const pool = new SidPool(32768);
    const forms = new SquidForms(pool);
    let depth = 7;
    forms.bindPop({ depth: () => depth, setDepth: (value) => { depth = value; } });
    const form = forms.create(3, 0);
    const raw = pool.physical(form);
    const free = pool.freeCount;
    // 서버 번호의 전송 경로에서 전송 장치가 없다.
    assert.ok(throws(() => forms.pop(form, 5)));
    assert.deepEqual([raw[11], depth, pool.freeCount], [0x10, 7, free]);
    // 미지원 실제 VT+20 은 깊이 증가 뒤 선택된다.
    raw[11] = 4;
    put(raw, 0, 0xdeadbeef);
    assert.ok(throws(() => forms.pop(form, 0x10)));
    assert.deepEqual([raw[11], depth, pool.freeCount], [0, 8, free]);
    // 연결된 전송은 정규화 flags(서버 번호 → |8)를 받고 끝나면 xmit 비트를 끈다.
    const sent = [];
    const transmitting = new SquidForms(pool, { transmit: (sid, flags) => sent.push([sid, flags, pool.slot(sid)[11]]) });
    put(raw, 0, SQUID_FORM_VTABLES.gump);
    raw[11] = 4;
    transmitting.pop(form, 5);
    assert.deepEqual(sent, [[form, 13, 0x10]]);
    assert.equal(raw[11], 0);
});

// 부착 부분 쓰기: 늦은 꼬리 진단과 이웃 VT 미복원은 앞선 링크·Pop 쓰기를 남긴다(cpppj FormAttachCurrent_LateContentDiagnosticAndMissingNeighborKeepPrefix).
test('SquidForms.attach: 늦은 꼬리 진단·이웃 VT 부재는 앞선 쓰기를 남긴다', () => {
    const pool = new SidPool(32768);
    const forms = new SquidForms(pool);
    const parent = syntheticAsset(pool, 2);
    const content = forms.create(6, 2);
    const free = pool.freeCount;
    // TYPE 6 == 독립 contentType 6 이라 꼬리 진단이 선택되는데 진단 장치가 없다.
    assert.ok(throws(() => forms.attach(content, parent, 0x10)));
    assert.equal(pool.slot(content)[11], 8);
    assert.equal(commonContainedParent(pool, content), parent);
    assert.equal(get(pool.slot(parent), 6, 2), content);
    assert.equal(pool.freeCount, free);
    const form = forms.create(3, 2);
    const neighbor = syntheticAsset(pool, 2);
    const raw = pool.physical(form);
    put(pool.physical(parent), 6, neighbor, 2);
    put(pool.physical(neighbor), 0, 0xdeadbeef);
    // 이웃의 현재 VT+c 가 미복원이라 그 위치에서 멈춘다.
    assert.ok(throws(() => forms.attach(form, parent, 0x10)));
    assert.deepEqual([get(raw, 16, 2), get(raw, 4, 2), raw[11], get(pool.slot(parent), 6, 2), get(raw, 14, 2)], [parent, neighbor, 4, neighbor, 0]);
});

// 혼합 링크 WORD: 바뀐 raw TYPE 과 무관하게 자산/Form 의 현재 VT 가 정한 서로 다른 WORD 위치를 쓴다(cpppj FormAttachCurrent_ActualVtableSelectsMixedLinkWords).
test('SquidForms.attach: 현재 VT 가 자산/Form 링크 WORD 위치를 고른다', () => {
    const pool = new SidPool(32768);
    const forms = new SquidForms(pool);
    const parent = syntheticAsset(pool, 2);
    const asset = syntheticAsset(pool, 2);
    const form = forms.create(3, 2);
    const assetRaw = pool.physical(asset);
    const formRaw = pool.physical(form);
    assetRaw[10] = 3;
    formRaw[10] = 7;
    put(assetRaw, 14, 0x12345678);
    put(pool.physical(parent), 6, asset, 2);
    forms.attach(form, parent, 0x10);
    assert.deepEqual([get(assetRaw, 14, 2), get(assetRaw, 16, 2), get(formRaw, 4, 2), get(formRaw, 14, 2), get(formRaw, 16, 2), formRaw[11]],
        [0x5678, form, asset, 0, parent, 8]);
    commonContainedSetParent(pool, asset, 9);
    assert.equal(get(assetRaw, 14, 2), 9);
    assert.equal(commonContainedPrevious(pool, asset), form);
    // 직접 setter 는 raw TYPE 의 Form/자산 분류를 강제하지 않는다.
    formRaw[10] = 74;
    commonContainedSetParent(pool, form, 7);
    commonContainedSetPrevious(pool, form, 9);
    assert.deepEqual([commonContainedParent(pool, form), commonContainedPrevious(pool, form), get(formRaw, 4, 2), formRaw[11]], [7, 9, asset, 8]);
});

// GameAuxState 연결1: 부착 꼬리 진단이 현재 TYPE genus 와 00541088 contentFormType 을 호출마다 다시 읽는다(다시 바인딩하지 않음).
test('SquidForms.attach: GameAuxState 의 현재 contentFormType 을 호출마다 읽는다', () => {
    const pool = new SidPool(32768);
    const aux = new GameAuxState();
    const globals = squidFormGlobals(aux);
    assert.deepEqual([globals.contentFormType(), globals.gumpFormType(), globals.uiEvent()], [6, 3, 0x02000039]);
    const flags2 = new Uint32Array(188);
    const forms = new SquidForms(pool);
    const parent = syntheticAsset(pool, 2);
    let raw = null;
    const lines = [];
    forms.bindAttach({
        genus: (type) => flags2[type],
        contentType: globals.contentFormType,
        // 같은 진단 장치가 정상 복귀 뒤 현재 슬롯을 고친다.
        assertion: (expression, file, line) => {
            assert.equal(file, SQUID_CPP);
            lines.push(line);
            if (expression === '!(getGenus() & (gEMPLACEMENT|gVORTEX |gFACTORY))') {
                assert.equal(raw[11], 4);
                put(raw, 0, SQUID_FORM_VTABLES.depend);
                raw[10] = 69;
            } else if (expression === 'getType()!=contentFormType') {
                assert.equal(raw[11], 8);
                raw[10] = 70;
            } else {
                assert.equal(expression, 'isForm()');
                raw[10] = 1;
            }
        },
    });
    const first = forms.create(3, 2);
    raw = pool.physical(first);
    raw[10] = 7;
    flags2[7] = 0x44200;
    aux.contentFormType = 69;
    forms.attach(first, parent, 0x10);
    assert.deepEqual(lines, [0x3d0, 0x3e0, 0x3e3]);
    assert.deepEqual([raw[10], raw[11], commonContainedParent(pool, first), get(pool.slot(parent), 6, 2)], [1, 8, parent, first]);
    // aux 쓰기만으로 다음 부착의 비교값이 바뀐다.
    aux.contentFormType = 3;
    lines.length = 0;
    const second = forms.create(3, 2);
    raw = pool.physical(second);
    forms.attach(second, parent, 0x10);
    assert.deepEqual(lines, [0x3e0, 0x3e3]);
    aux.contentFormType = 6;
    lines.length = 0;
    const third = forms.create(3, 2);
    raw = pool.physical(third);
    forms.attach(third, parent, 0x10);
    assert.deepEqual(lines, []);
    // 머리 앞 삽입 순서와 이웃의 현재 VT(Depend 로 바뀐 first 포함) 이전 WORD 이다.
    assert.deepEqual([get(pool.slot(parent), 6, 2), get(pool.slot(third), 4, 2), get(pool.slot(second), 4, 2), commonContainedPrevious(pool, second), commonContainedPrevious(pool, first)],
        [third, second, first, third, second]);
    assert.ok(throws(() => squidFormGlobals({ contentFormType: 6 })));
});

// GameAuxState 연결2: Form Unpop 이 빌린 contained 종단이 00541088(부모 통지)·0054107c(UI 검색)·005427dc(사건)를 호출 시점마다 읽는다.
test('SquidForms.unpop: 빌린 contained 종단이 GameAuxState 의 현재 Form 전역을 쓴다', () => {
    const pool = new SidPool(32768);
    const aux = new GameAuxState();
    const notices = [];
    const commands = [];
    const contained = new RawSquidContainedUnpop(pool, squidFormGlobals(aux), {
        parent: (sid) => commonContainedParent(pool, sid),
        previous: (sid) => commonContainedPrevious(pool, sid),
        setPrevious: (sid, previous) => commonContainedSetPrevious(pool, sid, previous),
        parentNotice: (owner, sid, reason) => notices.push([owner, sid, reason, pool.slot(sid)[11]]),
        command: (event, object, value) => commands.push([event, object, value]),
    });
    const forms = new SquidForms(pool);
    forms.bindUnpop({ detach: (sid) => contained.detach(sid) });
    const parent = syntheticAsset(pool, 0);
    const form = forms.create(6, 0);
    // TYPE 7 이 현재 내용물 TYPE 이면 부모 통지(이유 2)가 분리 뒤 온다.
    pool.physical(form)[10] = 7;
    aux.contentFormType = 7;
    contentAttachmentInput(pool, form, parent);
    forms.unpop(form);
    assert.deepEqual(notices, [[parent, form, 2, 4]]);
    assert.equal(get(pool.slot(parent), 6, 2), 0);
    // aux 를 되돌리면 같은 TYPE 7 은 Form 범위라 통지하지 않는다.
    aux.contentFormType = 6;
    contentAttachmentInput(pool, form, parent);
    forms.unpop(form);
    assert.equal(notices.length, 1);
    // 같은 종단을 Take 도 쓴다(Unpop → 통지 → VT/TYPE/void).
    pool.physical(form)[10] = 6;
    contentAttachmentInput(pool, form, parent);
    assert.equal(forms.take(6, form), form);
    assert.deepEqual(notices[1], [parent, form, 2, 4]);
    assert.equal(commands.length, 0);
    // UI 검색은 현재 gumpFormType 과 사건 DWORD 를 읽는다.
    const gump = forms.create(3, 2);
    new SquidForms(pool).attach(gump, parent, 0x10);
    contained.notifyUi(parent);
    aux.contentUnpopEvent = 0x02000040;
    contained.notifyUi(parent);
    aux.gumpFormType = 9;
    contained.notifyUi(parent);
    assert.deepEqual(commands, [[0x02000039, parent, 0], [0x02000040, parent, 0]]);
});
