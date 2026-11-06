// Form 호스트(Depend·Content)와 그 부모(감시 쌍 생성·정렬 Content 부착·Form 연결 검사)를 원본 10.78 관측과 대조한다(WB04-D5a).
//  - 원본 관측(cpppj/tests/fixtures, 10.78 행만): depend-create-parent-current 810행·content-attach-current 41행·form-linked-parent 120행·formlifecycle 456행.
//    각 행의 합성 입력·명시 장치는 cpppj 검사(DependCreateCurrentTests·ContentAttachCurrentTests·RawFormLinkedTests·FormLifecycleTests)와 같고 기대값은 fixture 에서 읽는다.
//  - form-linked-parent 는 판본 열이 없는 물리 입력 표이며 웹은 같은 체인 입력 12종으로 검사한다.
// 계약 검사는 tests/form-hosts-contract.test.mjs 에 있다. 실제 게임·브라우저 실행이 아니다. (2026-11-06 WB04-D5a: Form 호스트 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { createDependForms, DependFormHost } from '../www/core/depend-form-host.js';
import { attachSortedContent, ContentFormHost } from '../www/core/content-form-host.js';
import { checkRawFormChain } from '../www/core/raw-form-linked.js';

// 원본 관측 파일 폴더와 행 읽기이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-06 WB04-D5a)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
// 첫 열이 originals 인 행만 고른다. (2026-11-06 WB04-D5a)
const patchRows = (name) => readRows(path.join(FIXTURES, name))?.filter((row) => row[0] === 'originals') ?? null;
const dependCreateRows = patchRows('depend-create-parent-current-x86.tsv');
const contentAttachRows = readRows(path.join(FIXTURES, 'content-attach-current-x86.tsv'));
const formLinkedRows = readRows(path.join(FIXTURES, 'form-linked-parent-x86.tsv'))?.filter((row) => row[0] !== 'case') ?? null;
const lifecycleRows = patchRows('formlifecycle-x86.tsv');

// 원본 관측기가 명시 가상 장치에 준 VT 토큰 시작값이다. (2026-11-06 WB04-D5a)
const INPUT_VT = 0x14000000;

// 바이트 배열의 소문자 16진 문자열과 그 역변환이다. (2026-11-06 WB04-D5a)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// 관측기와 같은 합성 payload 로 슬롯을 채운다(기대값은 계산하지 않는다). (2026-11-06 WB04-D5a)
function fillPattern(raw, sid) {
    // 각 바이트에 (sid*50+offset)*37+19 의 하위 BYTE 를 둔다.
    for (let offset = 0; offset < raw.length; offset += 1) raw[offset] = ((sid * raw.length + offset) * 37 + 19) & 0xff;
}

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-06 WB04-D5a)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}));

// 원본 810행: 감시 쌍 생성 부모의 server 진단·두 Create(각각 현재 TYPE)·WORD 쓰기·저장/현재 VT·부착 순서와 두 슬롯(5·9) 전체.
test('createDependForms: 원본10.78 감시 쌍 생성 810행과 같다', { skip: !dependCreateRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 풀과 명시 장치로 실행한다.
    for (const row of dependCreateRows) {
        assert.equal(row.length, 6);
        const [watched, watcher, serverArg, profile] = row[1].split(',').map(Number);
        let server = serverArg !== 0;
        let number = 7;
        let creates = 0;
        const pool = new SidPool(32768);
        // 0~15 슬롯에 합성 payload 를 둔다.
        for (let slot = 0; slot < 16; slot += 1) fillPattern(pool.physical(slot), slot);
        const events = [];
        const pair = createDependForms(watched, watcher, {
            server: () => server,
            formType: () => number,
            sid: (token) => token,
            // 명시 Create 반환(5 다음 9)과 현재 번호 변경 입력이다.
            create: (type, flags) => {
                const sid = creates === 0 ? 5 : 9;
                events.push(`C${type},${flags},${sid}`);
                const raw = pool.physical(sid);
                put(raw, 0, INPUT_VT + (sid === 5 ? 0 : 0x100));
                raw[10] = type & 0xff;
                raw[11] = 4;
                if (profile === 1 && creates === 0) number = 0x12345678;
                creates += 1;
                return sid;
            },
            word: (sid, offset, value) => {
                events.push(`W${sid},${offset},${value}`);
                put(pool.physical(sid), offset, value, 2);
                if (profile === 3 && sid === 9 && offset === 18) put(pool.physical(5), 0, INPUT_VT + 0x200);
            },
            vtable: (sid) => get(pool.physical(sid), 0),
            // 첫 가상 부착이 바꾼 두 번째 VT 를 다음 호출이 읽어야 한다.
            attach: (sid, table, parent, flags) => {
                events.push(`A${sid},${table},${parent},${flags}`);
                if (profile === 4 && sid === 5) put(pool.physical(9), 0, INPUT_VT + 0x200);
            },
            // 정상 복귀 뒤에도 저장한 flags 0 을 다시 계산하지 않는다.
            assertion: (expression, file, line) => {
                events.push(`D${expression}|${file}|${line}`);
                if (profile === 2) {
                    number = 0xffffffff;
                    server = true;
                }
            },
        });
        assert.deepEqual([pair.watched, pair.watcher], [9, 5]);
        assert.deepEqual([String(number), server ? '1' : '0'], [row[2], row[3]], row[1]);
        assert.equal(events.join(';'), row[4], row[1]);
        assert.equal(`${hex(pool.slot(5))};${hex(pool.slot(9))}`, row[5], row[1]);
    }
    assert.equal(dependCreateRows.length, 810);
});

// 원본 41행: 정렬 Content 부착의 dead/free 진단 복귀·그룹 순서 삽입·저장 VT/현재 getter·늦은 Pop/통지 실패의 부분 효과와 다섯 슬롯 전체.
test('attachSortedContent: 원본10.78 정렬 부착 41행과 같다', { skip: !contentAttachRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 풀과 명시 장치로 실행한다.
    for (const row of contentAttachRows) {
        assert.equal(row.length, 6);
        const [layout, state, profile] = row.slice(0, 3).map(Number);
        const pool = new SidPool(32768);
        // 0~15 슬롯을 비운다.
        for (let sid = 0; sid < 16; sid += 1) pool.physical(sid).fill(0);
        let index = 0;
        // payload 까지 같은 네 입력을 두고 각 VT 만 구분한다.
        for (const sid of [5, 7, 9, 11]) {
            const raw = pool.physical(sid);
            raw.fill(0x71);
            put(raw, 0, INPUT_VT + index * 0x100);
            index += 1;
            // 같은 WORD 입력들이다(기대값 아님).
            for (const at of [4, 6, 14, 16, 20]) put(raw, at, 0, 2);
            raw[10] = 6;
            raw[11] = 4;
            put(raw, 18, 85, 2);
            raw[23] = 0;
        }
        const own = pool.physical(5);
        const first = pool.physical(7);
        const parent = pool.physical(9);
        const last = pool.physical(11);
        own[11] = state;
        if (layout) {
            put(parent, 6, 7, 2);
            put(first, 4, 11, 2);
            put(last, 14, 7, 2);
            first[23] = layout === 1 ? 2 : 0;
            last[23] = layout === 1 ? 3 : 1;
            if (layout === 3) {
                first[10] = 1;
                last[23] = 2;
            }
        }
        if (profile >= 3 && profile <= 5) {
            parent[11] = 6;
            own[11] = 6;
        }
        let content = 6;
        let cause = 'returned';
        const trace = [];
        try {
            const attached = attachSortedContent(pool, 5, 9, 0x10, {
                contentType: () => content,
                assertion: (expression, file, line) => {
                    trace.push(`D${line}`);
                    if (profile === 4 || profile === 5) {
                        pool.physical(line === 0xbe6 ? 9 : 5)[11] = 4;
                        if (profile === 5) {
                            content = 1;
                            first[10] = 1;
                            last[10] = 1;
                        }
                    }
                },
                previous: (sid, table) => {
                    trace.push(`G${sid},${table}`);
                    if (profile === 7) throw new Error('previous');
                    if (profile === 6) put(own, 0, INPUT_VT + 0x400);
                    return get(pool.physical(sid), 14, 2);
                },
                setPrevious: (sid, table, value) => {
                    trace.push(`R${sid},${table},${value}`);
                    put(pool.physical(sid), 14, value, 2);
                },
                setParent: (sid, table, value) => {
                    trace.push(`P${sid},${table},${value}`);
                    put(pool.physical(sid), 16, value, 2);
                },
                pop: (sid, flags) => {
                    trace.push(`O${flags}`);
                    if (profile === 1) throw new Error('pop');
                    own[11] &= ~4 & 0xff;
                },
                notice: (sid, child, reason) => {
                    trace.push(`N${sid},${child},${reason}`);
                    if (profile === 2) throw new Error('notice');
                },
            });
            if (attached) cause = 'attached';
        } catch (error) {
            cause = error.message;
        }
        const label = row.slice(0, 3).join(',');
        assert.equal(cause, row[3], label);
        assert.equal(trace.length ? trace.join(';') : '-', row[4], label);
        assert.equal([0, 5, 7, 9, 11].map((sid) => hex(pool.slot(sid))).join(';'), row[5], label);
    }
    assert.equal(contentAttachRows.length, 41);
});

// 원본 120행: Form 부모 체인 전체 검사는 자기를 찾은 뒤에도 끝까지 범위를 보고(논리 64 이상이면 false) 바이트를 바꾸지 않는다.
test('checkRawFormChain: 원본 Form 연결 검사 120행과 같다', { skip: !formLinkedRows && '원본 fixture 없음' }, () => {
    // 원문에 공급한 체인 입력 12종이다(논리 범위 64 전후·자기 뒤 실패 포함).
    const chains = [[], [20], [50], [20, 50], [50, 20], [20, 30, 50, 63], [20, 64], [50, 64], [64], [65535], [20, 50, 65535], [63, 30, 20]];
    // 행마다 독립 64슬롯 입력으로 실행한다.
    for (const row of formLinkedRows) {
        assert.equal(row.length, 4);
        const [form, parent, chainIndex] = row[0].split(',').map(Number);
        const chain = chains[chainIndex];
        const bytes = new Uint8Array(64 * 50);
        put(bytes, parent * 50 + 6, chain.length ? chain[0] : 0, 2);
        // 논리 밖 번호는 next/head 값으로만 둔다.
        chain.forEach((sid, index) => {
            if (sid < 64) put(bytes, sid * 50 + 4, index + 1 < chain.length ? chain[index + 1] : 0, 2);
        });
        const before = bytes.slice();
        const result = checkRawFormChain(bytes, 50, 64, form, parent);
        assert.equal(result ? '1' : '0', row[1], row[0]);
        assert.deepEqual([row[2], row[3]], [`P:${form}:${parent}`, '1']);
        assert.ok(Buffer.from(bytes).equals(Buffer.from(before)));
    }
    assert.equal(formLinkedRows.length, 120);
});

// 원본 456행: 감시 쌍(Pair/Destroy/Notify/Has/Remove)과 Content(Attach/Robust/Damage)의 결과·통지 당시 전체 바이트·관측 슬롯·풀/삭제 깊이.
test('DependFormHost/ContentFormHost: 원본10.78 Form 수명 456행과 같다', { skip: !lifecycleRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 풀·호스트로 실행한다.
    for (const row of lifecycleRows) {
        assert.equal(row.length, 12);
        const pool = new SidPool(32768, { server: row[5] === '1' });
        // fixture 의 초기 슬롯 바이트를 복원한다(free/서버 번호 포함).
        if (row[7]) {
            for (const item of row[7].split(';')) {
                const [sid, bytes] = item.split(':');
                pool.physical(Number(sid)).set(unhex(bytes));
            }
        }
        const types = formTypeInputs();
        const unpop = new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536));
        const destroy = new RawSquidDestroy(pool, unpop, types);
        const events = [];
        // 부모 알림을 대체한 원본 경계와 같은 전체 바이트를 기록하고 같은 명시 변경만 한다.
        const content = new ContentFormHost(pool, types, (parent, form, reason) => {
            events.push(`content:${parent}:${form}:${reason}:${hex(pool.slot(parent))}:${hex(pool.slot(form))}`);
            if (reason === 2 && row[6] !== '0') put(pool.allocated(form), 20, Number(row[6]), 2);
        });
        // 서버 Pop 과 삭제 전파도 같은 현재 슬롯을 기록한다(네트워크 없음).
        const transmit = (form, flags) => events.push(`transmit:${form}:${flags}:${hex(pool.slot(form))}`);
        const depend = new DependFormHost(pool, destroy, {
            notice: (watcher, watched, form) => events.push(`watch:${watcher}:${watched}:${form}:${hex(pool.slot(watcher))}:${hex(pool.slot(form))}`),
            fallback: {
                emit: (event) => {
                    if (event.effect !== SQUID_DESTROY_EFFECT.transmit) throw new Error('fixture 미선언 Form 효과');
                    transmit(event.sid, event.flags);
                },
            },
            transmit,
        });
        const first = Number(row[2]);
        const second = Number(row[3]);
        const flags = Number(row[4]) >>> 0;
        let result = 0;
        if (row[1] === 'Pair') depend.create(first, second);
        else if (row[1] === 'Destroy') depend.destroy(first, second);
        else if (row[1] === 'Notify') depend.notify(first, second);
        else if (row[1] === 'Has') result = depend.has(first, second) ? 1 : 0;
        else if (row[1] === 'Remove') depend.remove(first, second, flags);
        else if (row[1] === 'Attach') content.attach(first, second, flags);
        else if (row[1] === 'Robust') result = content.robust(first) ? 1 : 0;
        else if (row[1] === 'Damage') assert.equal(content.damage(first, second, bitsToFloat(flags)), 0);
        else assert.fail(`Form 수명 연산 오류 ${row[1]}`);
        const label = `${row[1]} ${row[2]} ${row[3]} ${row[4]}`;
        assert.equal(result, Number(row[9]), label);
        assert.equal(events.join(';'), row[11], label);
        // 관측 슬롯은 전체 바이트를 비교한다.
        for (const item of row[8].split(';')) {
            const [sid, bytes] = item.split(':');
            assert.equal(hex(pool.slot(Number(sid))), bytes, `${label} slot ${sid}`);
        }
        const actual = [pool.capacity, pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.tail(true), pool.firstFree(false), pool.tail(false),
            adler32(pool.bytes()), destroy.preDepth, destroy.postDepth, 0];
        assert.deepEqual(actual, row[10].split(',').map(Number), label);
    }
    assert.equal(lifecycleRows.length, 456);
});
