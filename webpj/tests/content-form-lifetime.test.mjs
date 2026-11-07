// Content 삭제 수명(ContentFormLifetime)과 Form 분배기(SquidFormDispatch)를 원본 10.78 관측과 cpppj 계약 검사로 대조한다(WB04-D5b).
//  - 원본 관측(cpppj/tests/fixtures, 10.78 행만): contentlifetime-x86 198행(cpppj ContentLifetime_IndependentPeFixture)·content-cursor-width-x86(CursorPreviewDword_ContentComparisonNative56KeepsAllBits).
//    입력·명시 장치는 cpppj 검사와 같고 기대값(효과 당시 전체 슬롯·통지·관측 슬롯·풀/깊이)은 fixture 에서 읽는다.
//  - 계약: cpppj ContentLifetimeTests(실제 SP/지갑 통합·boss 재조회·너겟 번호 재조회·혼합 Form 분배), FormTakeTests 의 FormClassLifetime_CurrentVtableKeepsGumpAndContentEffects,
//    DependCreateCurrentTests 의 DependCreateCurrent_UnregisteredClassDoesNotUseDifferentRawTypeRoute. 혼합 분배는 Process(자기 삭제 프로세스·Kernel)까지 포함한다(WB04-D5c).
// 실제 게임·브라우저 실행이 아니다. (2026-11-07 WB04-D5b: Content 수명·Form 분배기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidForms } from '../www/core/squid-forms.js';
import { SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { ContentFormHost } from '../www/core/content-form-host.js';
import { DependFormHost } from '../www/core/depend-form-host.js';
import { GumpFormHost } from '../www/core/gump-form-host.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { ContentFormLifetime, ContentLifetimeState } from '../www/core/content-form-lifetime.js';
import { SquidFormDispatch } from '../www/core/squid-form-dispatch.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';

// 원본 관측 파일 폴더와 10.78 행 읽기이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-07 WB04-D5b)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const patchRows = (name) => readRows(path.join(FIXTURES, name))?.filter((row) => row[0] === 'originals') ?? null;
const lifetimeRows = patchRows('contentlifetime-x86.tsv');
const cursorRows = patchRows('content-cursor-width-x86.tsv');

// 바이트 배열의 소문자 16진 문자열과 그 역변환이다. (2026-11-07 WB04-D5b)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
// float 비트의 소문자 16진(리틀엔디언 4바이트)이다(cpppj Hex(cost)). (2026-11-07 WB04-D5b)
const floatHex = (value) => {
    const bytes = new Uint8Array(4);
    new DataView(bytes.buffer).setUint32(0, floatToBits(value), true);
    return hex(bytes);
};
// 예외가 나는지 본다. (2026-11-07 WB04-D5b)
const throws = (action) => {
    try {
        action();
        return false;
    } catch {
        return true;
    }
};
// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-07 WB04-D5b)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
// 같은 풀의 공통 삭제 장치(해시·spot·Unpop·Destroy)이다. (2026-11-07 WB04-D5b)
const destroyFor = (pool, types) => new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
// 장부(SquidPostPopState 의 보상 소비 칸)이다. (2026-11-07 WB04-D5b)
const books = (localOwner = 0) => ({ localOwner, aiAttached: new Array(9).fill(false) });

// 원본 198행: 커서 취소·로컬/원격 알림·지갑·SP·위치 소리 당시 전체 슬롯/전역/비용과 명시 변경, 부모 제거 통지, 관측 슬롯 전체 바이트, 풀/공통 깊이.
test('ContentFormLifetime: 원본10.78 Content 수명 198행과 같다', { skip: !lifetimeRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 입력 상태에서 시작한다.
    for (const row of lifetimeRows) {
        assert.equal(row.length, 14);
        const pool = new SidPool(32768, { server: row[2] === '1' });
        // 합성 입력 슬롯의 모든 바이트를 놓는다(free/서버 번호 포함).
        for (const item of row[8].split(';')) {
            const [sid, bytes] = item.split(':');
            const raw = pool.physical(Number(sid));
            assert.equal(bytes.length, raw.length * 2);
            raw.set(unhex(bytes));
        }
        const types = formTypeInputs();
        types[154].cost = bitsToFloat(Number(row[5]));
        types[90].footX = 3;
        types[90].footY = 5;
        const destroy = destroyFor(pool, types);
        const form = 80;
        const parent = get(pool.slot(form), 16, 2);
        const state = new ContentLifetimeState({ cursor: Number(row[6]) >>> 0, nuggetType: 154, localOwner: Number(row[3]), notice: 0x1234, boss: row[4] === '1' });
        const mutation = Number(row[7]);
        const effects = [];
        const notices = [];
        // 효과 당시 Form/부모 전체 바이트·localOwner·boss·너겟 번호·현재 비용 비트이다.
        const snapshot = () => `${hex(pool.slot(form))}:${hex(pool.slot(parent))}:${state.localOwner}:${state.boss ? 1 : 0}:${state.nuggetType}:${floatHex(types[state.nuggetType].cost)}`;
        let inBattle = false;
        if (mutation >= 100) {
            inBattle = ((mutation - 100) & 4) !== 0;
            types[80].flags2 = ((mutation - 100) & 2) ? 0x200000 : 0;
            types[90].flags2 = ((mutation - 100) & 1) ? 0x10000 : 0;
        }
        const forms = new ContentFormHost(pool, types, (notifiedParent, notifiedForm, reason) => {
            notices.push(`content:${notifiedParent}:${notifiedForm}:${reason}:${hex(pool.slot(notifiedParent))}:${hex(pool.slot(notifiedForm))}`);
        }, { state: { inBattle }, captured: (object, previousOwner) => effects.push(`capture:${object}:${previousOwner}:${snapshot()}`) });
        const hooks = {
            // 취소 경계는 원본 관찰에서 명시한 현재 cursor 0/kind 변경만 한다.
            cancelCursor: () => {
                effects.push(`cancel:${snapshot()}`);
                state.cursor = 0;
                if (mutation === 1) put(pool.physical(form), 18, 154, 2);
            },
            // 로컬 알림이 바꾼 paid/owner/dead/kind 는 뒤의 몸체가 다시 읽는다.
            localNotice: (command) => {
                effects.push(`local:${command}:${snapshot()}`);
                if (mutation === 2) pool.physical(form)[22] = 1;
                else if (mutation === 3) pool.physical(parent)[34] = 3;
                else if (mutation === 7) pool.physical(parent)[11] = 2;
                else if (mutation === 8) put(pool.physical(form), 18, 153, 2);
            },
            // 원격 송신의 실제 인자와 당시 상태를 기록한다(네트워크 없음).
            remoteNotice: (owner) => effects.push(`remote:${owner}:${snapshot()}`),
            // 선지급 지갑 경계는 cost/owner/localOwner 만 합성 입력대로 바꾼다.
            wallet: (owner, amount) => {
                effects.push(`wallet:${owner}:${amount >>> 0}:${snapshot()}`);
                if (mutation === 4) {
                    pool.physical(parent)[34] = 2;
                    types[154].cost = 125.75;
                } else if (mutation === 5) state.localOwner = 2;
            },
            // SP 요청은 로컬/다른 소유자 경로를 구분하고 뒤의 중심 계산 좌표 입력을 바꾼다.
            addSp: (owner, amount, local) => {
                effects.push(`sp:${owner}:${floatToBits(amount)}:${local ? 1 : 0}:${snapshot()}`);
                if (mutation === 6) {
                    put(pool.physical(parent), 14, floatToBits(200.5));
                    put(pool.physical(parent), 18, floatToBits(60.25));
                }
            },
            // 실제 중심 계산 결과 float 비트를 비교한다.
            sound: (x, y) => effects.push(`sound:${floatToBits(x)}:${floatToBits(y)}:${snapshot()}`),
        };
        const lifetime = new ContentFormLifetime(pool, types, forms, destroy, state, hooks);
        let result = 0;
        if (row[1] === 'Destroy') lifetime.destroy(form, 0x10);
        else if (row[1] === 'Linked') result = forms.linked(form) ? 1 : 0;
        else if (row[1] === 'Attach') forms.attach(form, parent, 0x10);
        else assert.fail(`Content fixture 연산 오류 ${row[1]}`);
        const label = `${row[1]} server${row[2]} owner${row[3]} boss${row[4]} cursor${row[6]} mutation${row[7]}`;
        assert.equal(result, Number(row[10]), label);
        assert.equal(effects.join(';'), row[12], label);
        assert.equal(notices.join(';'), row[13], label);
        // 관측 슬롯은 체크섬이 아니라 전체 원본 바이트와 직접 비교한다.
        for (const item of row[9].split(';')) {
            const [sid, bytes] = item.split(':');
            assert.equal(hex(pool.slot(Number(sid))), bytes, `${label} slot ${sid}`);
        }
        const actual = [pool.capacity, pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.tail(true), pool.firstFree(false), pool.tail(false),
            adler32(pool.bytes()), destroy.preDepth, destroy.postDepth, 0];
        assert.deepEqual(actual, row[11].split(',').map(Number), label);
    }
    assert.equal(lifetimeRows.length, 198);
});

// 원본 10.78 커서 폭 관측: 현재 커서 DWORD 전체(높은 WORD 포함)와 부모 자기 WORD 비교, 취소 호출 수·취소 뒤 커서. 값형 필드와 현재 getter 두 경로로 재생한다.
// cpppj 검사는 슬롯 VT 를 0 으로 두지만 기준선 isContentForm 은 현재 VT 로 판정하므로, 웹은 실제 Content 생성자 VT 를 넣어 같은 커서 비교를 관측한다(문서 4절).
test('ContentFormLifetime.preDestroy: 원본10.78 커서 DWORD 전체 비교와 같다', { skip: !cursorRows && '원본 fixture 없음' }, () => {
    let observations = 0;
    // 행마다 값형/현재 getter 두 경로를 실행한다.
    for (const row of cursorRows) {
        assert.equal(row.length, 7);
        for (const getter of [false, true]) {
            const pool = new SidPool(32768);
            const types = formTypeInputs();
            const destroy = destroyFor(pool, types);
            const forms = new ContentFormHost(pool, types, () => {});
            const form = 50;
            const raw = pool.physical(form);
            raw.fill(0);
            put(raw, 0, SQUID_FORM_VTABLES.content);
            raw[10] = 6;
            put(raw, 16, Number(row[3]), 2);
            put(raw, 18, 85, 2);
            const state = new ContentLifetimeState({ cursor: Number(row[2]) >>> 0 });
            let cancelled = 0;
            if (getter) state.currentCursor = () => state.cursor;
            const lifetime = new ContentFormLifetime(pool, types, forms, destroy, state, { cancelCursor: () => { cancelled += 1; } });
            lifetime.preDestroy(form);
            assert.deepEqual([cancelled, state.cursor], [Number(row[4]), Number(row[5]) >>> 0], row.join(' '));
            observations += 1;
        }
    }
    assert.equal(observations, cursorRows.length * 2);
    assert.equal(cursorRows.length, 28);
});

// 너겟 삭제는 실제 SP 저장소/AI 지갑을 쓰고 패치판 선가산·위치 소리·SP 표시 갱신·부모 통지 순서를 유지한다(cpppj ContentLifetime_ActualSpAndWalletIntegration, 10.78).
test('ContentFormLifetime: 실제 SP 저장소·AI 지갑 통합', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    types[154].cost = 50.75;
    types[90].footX = 3;
    types[90].footY = 5;
    const factory = new SquidFactory(pool, types);
    const destroy = destroyFor(pool, types);
    const store = new ScrambledSpStore(new GameRandom(7), () => 42);
    store.initialize();
    store.set(1, floatToBits(1000));
    const book = books(1);
    book.aiAttached[1] = true;
    const rewardState = new SquidRewardState();
    let refreshed = 0;
    const reward = new SquidReward(pool, types, book, rewardState, store, { refreshSp: () => { refreshed += 1; } });
    const parent = factory.create(90, 2);
    const parentRaw = pool.allocated(parent);
    parentRaw[34] = 1;
    put(parentRaw, 14, floatToBits(20));
    put(parentRaw, 18, floatToBits(30));
    const reasons = [];
    const forms = new ContentFormHost(pool, types, (object, _form, reason) => {
        assert.equal(object, parent);
        reasons.push(reason);
    });
    const form = factory.createContained(2);
    put(pool.allocated(form), 18, 154, 2);
    put(pool.allocated(form), 20, 0x20, 2);
    forms.attach(form, parent, 0x10);
    let notified = 0;
    let sounded = 0;
    const hooks = {
        localNotice: () => { notified += 1; },
        sound: (x, y) => {
            sounded += 1;
            assert.deepEqual([x, y], [18.5, 27.5]);
        },
    };
    new ContentFormLifetime(pool, types, forms, destroy, new ContentLifetimeState(), hooks, {}, reward).destroy(form, 0x10);
    assert.equal(bitsToFloat(store.get(1)), 1050.75);
    assert.deepEqual([rewardState.aiWallet[1], notified, sounded, refreshed], [100, 1, 1, 1]);
    assert.deepEqual(reasons, [1, 2]);
    assert.equal(get(pool.slot(parent), 6, 2), 0);
    assert.equal(pool.deletions(true)[0].type, 6);
});

// 실제 Content 삭제가 커서 취소 뒤 현재 boss 를 읽어 원격 수입 알림을 고른다(cpppj ContentLifetime_SharedBossRecheckedAfterCursorCancel).
test('ContentFormLifetime: 커서 취소 뒤 현재 boss 를 다시 읽는다', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    const destroy = destroyFor(pool, types);
    const parent = factory.create(90, 2);
    pool.allocated(parent)[34] = 2;
    const forms = new ContentFormHost(pool, types, () => {});
    let boss = true;
    let nextBoss = false;
    const state = new ContentLifetimeState({ localOwner: 1, currentBoss: () => boss });
    let cancelled = 0;
    let remote = 0;
    const lifetime = new ContentFormLifetime(pool, types, forms, destroy, state, {
        cancelCursor: () => {
            cancelled += 1;
            boss = nextBoss;
        },
        remoteNotice: (owner) => {
            assert.equal(owner, 2);
            remote += 1;
        },
    });
    // 두 실제 Form 을 차례로 붙이고 공통 삭제 깊이와 부모 링크까지 정상 반납한다.
    for (let index = 0; index < 2; index += 1) {
        const form = factory.createContained(2);
        const raw = pool.allocated(form);
        put(raw, 18, 154, 2);
        put(raw, 20, 0x20, 2);
        raw[22] = 1;
        forms.attach(form, parent, 0x10);
        state.cursor = form;
        nextBoss = index !== 0;
        state.boss = !nextBoss;
        lifetime.destroy(form, 0x10);
        assert.deepEqual([remote, cancelled, get(pool.slot(parent), 6, 2), destroy.preDepth, destroy.postDepth], [index, index + 1, 0, 0, 0]);
    }
});

// 커서 취소/안내/지갑 뒤의 서로 다른 현재 너겟 번호가 kind 와 각 단가 조회에 반영된다(cpppj NuggetTypeNumber_ContentReadsCurrentKindAndEachIncomeCost).
test('ContentFormLifetime: 현재 너겟 번호를 kind·단가마다 다시 읽는다', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    types[71].cost = 99.5;
    types[72].cost = 125.75;
    types[90].footX = 2;
    types[90].footY = 4;
    const factory = new SquidFactory(pool, types);
    const destroy = destroyFor(pool, types);
    const parent = factory.create(90, 2);
    const parentRaw = pool.allocated(parent);
    parentRaw[34] = 1;
    put(parentRaw, 14, floatToBits(40.75));
    put(parentRaw, 18, floatToBits(41.25));
    const forms = new ContentFormHost(pool, types, () => {});
    let current = 154;
    const state = new ContentLifetimeState({ currentNuggetType: () => current });
    let events = '';
    const hooks = {
        cancelCursor: () => {
            events += 'C';
            current = 70;
        },
        localNotice: (command) => {
            assert.deepEqual([command, current], [state.notice, 70]);
            events += 'N';
            current = 71;
        },
        wallet: (owner, amount) => {
            assert.deepEqual([owner, amount], [1, 99]);
            events += 'W';
            current = 72;
        },
        addSp: (owner, amount, local) => {
            assert.deepEqual([owner, amount, local], [1, 125.75, true]);
            events += 'S';
        },
        sound: (x, y) => {
            assert.deepEqual([x, y], [39.75, 39.25]);
            events += 'A';
        },
    };
    const lifetime = new ContentFormLifetime(pool, types, forms, destroy, state, hooks);
    const form = factory.createContained(2);
    const raw = pool.allocated(form);
    put(raw, 18, 70, 2);
    put(raw, 20, 0x20, 2);
    raw[22] = 0;
    forms.attach(form, parent, 0x10);
    state.cursor = form;
    lifetime.destroy(form, 0x10);
    assert.deepEqual([events, current, state.nuggetType, pool.slot(form)[11] & 1, get(pool.slot(parent), 6, 2), destroy.preDepth, destroy.postDepth], ['CNWSA', 72, 154, 1, 0, 0, 0]);
});

// 부모 삭제 중 자신의 Form 을 같은 분배기로 지우는 프로세스이다. 객체를 삭제한 뒤 멤버를 다시 읽지 않는다. counts: [부모 사망 통지, form 삭제 통지, 소멸] 세 카운터. (2026-11-08 WB04-D5c)
class SelfDeletingProcess extends SquidProcess {
    constructor(dispatch, counts) {
        super();
        this.dispatch = dispatch;
        this.counts = counts;
    }

    // Kernel 의 실제 객체 소멸도 확인한다.
    destroy() { this.counts[2] += 1; }

    // 같은 Form 의 가상 preDestroy 로 들어온 통지를 센다.
    onFormDestroy() { this.counts[1] += 1; }

    // 상대 부모 release 중 현재 프로세스만 중첩 삭제한다. 부모가 선저장한 다음 종속은 지우지 않는다.
    onParentDying() {
        const form = this.form;
        this.counts[0] += 1;
        this.dispatch.destroy(form, 0x10);
    }
}

// 한 부모의 감시·Gump·프로세스·Content Form 이 자기 알림에서 삭제돼도 같은 저장소/깊이/선저장 next 를 공유한다(cpppj ContentLifetime_SharedMixedFormDispatch).
test('SquidFormDispatch: 혼합 Form 종속 체인의 중첩 삭제', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    const destroy = destroyFor(pool, types);
    const root = factory.create(90, 2);
    const watcher = factory.create(91, 2);
    let selected = root;
    let assetRoots = 0;
    let dependNotices = 0;
    let gumpNotices = 0;
    let contentRemovals = 0;
    const assets = {
        selected: () => selected,
        emit: (event) => {
            if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) destroy.completePreDestroy();
            else if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) destroy.completePostDestroy();
            else if (event.effect === SQUID_DESTROY_EFFECT.clearSelection) selected = 0;
            else assert.equal(event.effect, SQUID_DESTROY_EFFECT.transmit);
        },
    };
    const dispatch = new SquidFormDispatch(pool, destroy, assets, (sid, flags, hooks) => {
        assetRoots += 1;
        destroy.destroy(sid, flags, hooks);
        return true;
    });
    const content = new ContentFormHost(pool, types, (_parent, _form, reason) => { if (reason === 2) contentRemovals += 1; });
    const lifetime = new ContentFormLifetime(pool, types, content, destroy, new ContentLifetimeState(), {}, dispatch.hooks());
    dispatch.bind(lifetime);
    const counts = [0, 0, 0];
    const kernel = new Kernel();
    const processes = new SquidProcessHost(pool, types, kernel, destroy, new SquidProcessState(), { asset: dispatch.hooks() });
    dispatch.bind(processes);
    let gumpForm = 0;
    const gumps = new GumpFormHost(pool, factory, types, destroy, {
        notice: (owner, object) => {
            assert.deepEqual([owner, object], [0xabcdef12, root]);
            gumpNotices += 1;
            dispatch.destroy(gumpForm, 0x10);
        },
        fallback: dispatch.hooks(),
    });
    dispatch.bind(gumps);
    const depends = new DependFormHost(pool, destroy, {
        notice: (notifiedWatcher, watched, form) => {
            assert.deepEqual([notifiedWatcher, watched], [watcher, root]);
            dependNotices += 1;
            dispatch.destroy(form, 0x10);
        },
        fallback: dispatch.hooks(),
    });
    dispatch.bind(depends);
    const value = factory.createContained(2);
    put(pool.allocated(value), 18, 80, 2);
    content.attach(value, root, 0x10);
    const process = processes.attach(new SelfDeletingProcess(dispatch, counts), 46, root, 0x10);
    const processForm = process.form;
    gumpForm = gumps.create(2);
    gumps.setOwner(gumpForm, 0xabcdef12);
    gumps.attach(gumpForm, root);
    const pair = depends.create(root, watcher);
    assert.equal(dispatch.destroy(root, 0x10), true);
    assert.deepEqual([assetRoots, selected, kernel.size, counts, dependNotices, gumpNotices, contentRemovals, get(pool.slot(watcher), 6, 2)], [1, 0, 0, [1, 1, 1], 1, 1, 0, 0]);
    // 모든 Form 과 루트가 같은 SID 반납/삭제 기록에 들어간다.
    for (const sid of [root, value, processForm, gumpForm, pair.watchedForm, pair.watcherForm]) assert.equal(pool.slot(sid)[11] & 3, 3, `sid ${sid}`);
    assert.deepEqual([destroy.preDepth, destroy.postDepth], [0, 0]);
    assert.ok(throws(() => dispatch.bind(gumps)));
    assert.ok(throws(() => dispatch.bind({ pool, hooks: () => ({}) })));
});

// TYPE 별칭에도 현재 Gump/Content VT 의 부모 삭제 알림·커서 취소·공통 깊이 반납을 보존한다(cpppj FormClassLifetime_CurrentVtableKeepsGumpAndContentEffects, 10.78).
test('SquidFormDispatch: 현재 VT 가 Gump/Content 효과를 고른다', () => {
    // Gump 의 owner 알림과 Content 의 빈 알림/커서 취소를 구별한다.
    for (const formType of [3, 6]) {
        // 변경 가능한 TYPE 이 현재 클래스를 대신할 수 없다.
        for (const sourceType of [3, 6, 7]) {
            const pool = new SidPool(32768, { server: false });
            const types = formTypeInputs();
            const factory = new SquidFactory(pool, types);
            const destroy = destroyFor(pool, types);
            const assets = { selected: () => 0, emit: () => { throw new Error('Form 삭제 검사에서 자산 효과 선택'); } };
            const dispatch = new SquidFormDispatch(pool, destroy, assets);
            let parent = 0;
            let form = 0;
            let notices = 0;
            let cancels = 0;
            const gumps = new GumpFormHost(pool, factory, types, destroy, {
                notice: (owner, notifiedParent) => {
                    assert.deepEqual([owner, notifiedParent], [0x12345678, parent]);
                    notices += 1;
                },
                fallback: dispatch.hooks(),
            });
            const content = new ContentFormHost(pool, types, () => { throw new Error('삭제 중 Content 부모 통지 선택'); });
            const state = new ContentLifetimeState();
            const lifetime = new ContentFormLifetime(pool, types, content, destroy, state, {
                // 원본 Content preDestroy 는 dead 설정 뒤 실제 커서와 비교한다.
                cancelCursor: () => {
                    assert.equal(pool.slot(form)[10], sourceType);
                    assert.ok(pool.slot(form)[11] & 2);
                    cancels += 1;
                },
            }, dispatch.hooks());
            dispatch.bind(gumps);
            dispatch.bind(lifetime);
            parent = factory.createGumpForm(2);
            form = factory.create(formType, 2);
            state.cursor = form;
            if (formType === 3) gumps.setOwner(form, 0x12345678);
            else put(pool.allocated(form), 18, 75, 2);
            put(pool.allocated(parent), 6, form, 2);
            put(pool.allocated(form), 16, parent, 2);
            pool.allocated(form)[11] = 8;
            pool.allocated(form)[10] = sourceType;
            const free = pool.freeCount;
            const label = `form${formType} type${sourceType}`;
            assert.equal(dispatch.destroy(parent, 0), true, label);
            assert.deepEqual([notices, cancels], [formType === 3 ? 1 : 0, formType === 6 ? 1 : 0], label);
            assert.ok((pool.slot(parent)[11] & 1) && (pool.slot(form)[11] & 1), label);
            assert.deepEqual([pool.freeCount, destroy.preDepth, destroy.postDepth], [free + 2, 0, 0], label);
            // 반납 뒤 타입 기록 조회는 현재 클래스 판정과 별도 계약이다.
            assert.deepEqual([gumps.isGumpForm(form), lifetime.isContentForm(form)], [sourceType === 3, sourceType === 6], label);
        }
    }
});

// 감시 VT 가 선택되면 raw TYPE 3 의 Gump 처리기로 상대 삭제를 생략하지 않고, 이미 수행한 공통 접두(dead·pre 깊이)를 보존한 채 미등록 오류다
// (cpppj DependCreateCurrent_UnregisteredClassDoesNotUseDifferentRawTypeRoute, 10.78).
test('SquidFormDispatch: 미등록 현재 클래스는 다른 TYPE 경로로 숨기지 않는다', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    const destroy = destroyFor(pool, types);
    const dispatch = new SquidFormDispatch(pool, destroy, { selected: () => 0, emit: () => {} });
    const gumps = new GumpFormHost(pool, factory, types, destroy, { fallback: dispatch.hooks() });
    dispatch.bind(gumps);
    const parent = factory.create(74, 2);
    const source = gumps.create(2);
    const target = gumps.create(2);
    gumps.attach(source, parent);
    const raw = pool.allocated(source);
    put(raw, 0, new SquidForms(pool).vtable(1));
    put(raw, 20, target, 2);
    const free = pool.freeCount;
    assert.ok(throws(() => dispatch.destroy(source, 0x10)));
    assert.deepEqual([pool.slot(source)[11] & 3, pool.freeCount, get(pool.slot(parent), 6, 2), pool.slot(target)[11] & 3, destroy.preDepth], [2, free, source, 0, 1]);
});

// 생성 인자 계약: 다른 풀·불완전 자산 경계·판본 크기·형식 오류를 생성 때 거부하고, Content 아닌 슬롯은 preDestroy/삭제를 거부한다.
test('ContentFormLifetime/SquidFormDispatch: 연결 계약', () => {
    const pool = new SidPool(32768);
    const types = formTypeInputs();
    const destroy = destroyFor(pool, types);
    const forms = new ContentFormHost(pool, types, () => {});
    const other = new SidPool(32768);
    assert.ok(throws(() => new ContentFormLifetime(other, types, forms, destroy, new ContentLifetimeState())));
    assert.ok(throws(() => new ContentFormLifetime(pool, types.slice(0, 187), forms, destroy, new ContentLifetimeState())));
    assert.ok(throws(() => new ContentFormLifetime(pool, types, forms, destroy, {})));
    const foreignReward = new SquidReward(other, types, books(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    assert.ok(throws(() => new ContentFormLifetime(pool, types, forms, destroy, new ContentLifetimeState(), {}, {}, foreignReward)));
    assert.ok(throws(() => new ContentLifetimeState({ unknown: 1 })));
    const lifetime = new ContentFormLifetime(pool, types, forms, destroy, new ContentLifetimeState());
    const factory = new SquidFactory(pool, types);
    const asset = factory.create(90, 2);
    assert.ok(!lifetime.isContentForm(asset));
    assert.ok(!lifetime.isContentForm(0) && !lifetime.isContentForm(pool.layout.predictableFirst));
    assert.ok(throws(() => lifetime.preDestroy(asset)));
    assert.ok(throws(() => lifetime.destroy(asset)));
    // 다른 타입 효과는 fallback 이 없으면 명시 오류다.
    assert.ok(throws(() => lifetime.hooks().emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid: asset, parent: 0, flags: 0 })));
    assert.ok(throws(() => new SquidFormDispatch(other, destroy, { selected: () => 0, emit: () => {} })));
    assert.ok(throws(() => new SquidFormDispatch(pool, destroy, { selected: () => 0 })));
    const dispatch = new SquidFormDispatch(pool, destroy, { selected: () => 0, emit: () => {} });
    assert.ok(throws(() => dispatch.setAssets({ selected: () => 0, emit: () => {} }, null)));
    assert.ok(throws(() => dispatch.bind(new ContentFormLifetime(other, types, new ContentFormHost(other, types, () => {}), destroyFor(other, types), new ContentLifetimeState()))));
    // 등록되지 않은 Process VT 의 Unpop 은 미등록 오류다.
    const process = factory.create(2, 2);
    assert.ok(throws(() => dispatch.hooks().unpopForm(process)));
});
