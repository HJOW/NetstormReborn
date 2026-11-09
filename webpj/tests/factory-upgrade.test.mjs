// Factory 업그레이드 요청/수신 raw-factory-upgrade-command.js(RawFactoryUpgradeCommand)와 예측 그림/완료/종속 그림 정리 raw-factory-upgrade.js(RawFactoryUpgrade)를
// cpppj `FactoryUpgradeCommandTests` 8건·`FactoryUpgradeTests` 7건(손 시험, 원본 x86 fixture 없음)으로 대조한다.
//  - 요청: 대기 20초(event 4)와 wire 완료 15초의 구분·16 BYTE 송신·예약 실패에도 송신 계속·부착 효과 뒤 현재 시각.
//  - 수신: 비용→로그→event 3 예약→원문 16 BYTE 방송, 기한 전/같음/미래/unordered(NaN)의 최소 1초, 상태/검사/genus 관문과 진단 뒤 현재 genus 재판정,
//    비용/예약/부착 효과 뒤의 현재 owner·시각·boss, 예약 실패/선택된 누락/짧은 패킷, 연구 수신 부모가 kind 0·mode 1 을 소유자/sender 조건 없이 업그레이드 수신으로 넘김.
//  - 진행/완료: begin 의 원본 순서와 raw 필드, nextLevel 재조회/상한, finish 순서와 현재 소유자, 아웃포스트 완료 생략, 완료가 실제 Host 의 event 4/3 을 모두 제거하고 다른 사건은 보존, 예측 그림 필터와 저장 next, 진단/누락 하위.
// 같은 PC 의 cpppj 통합 시험 실행 파일(`netstorm_tests.exe --filter FactoryUpgrade`, 게임 실행 아님)에서 18건(관련 CampaignSession 포함) 통과를 확인했다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-10 WB04-E218: Factory 업그레이드 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { RawFactoryResearch } from '../www/core/raw-factory-research.js';
import { RawFactoryUpgradeCommand } from '../www/core/raw-factory-upgrade-command.js';
import { RawFactoryUpgrade } from '../www/core/raw-factory-upgrade.js';

// 예외가 나는지 본다. (2026-11-10 WB04-E218)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 Factory 원문과 명시 비용/예약/전송 장치를 조립한다(cpppj UpgradeInput). (2026-11-10 WB04-E218)
class CommandInput {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.boss = true;
        this.checking = 0;
        this.reserve = true;
        this.now = 16777216.25;
        this.trace = [];
        this.timers = [];
        this.money = [];
        this.sent = [];
        this.broadcasts = [];
        this.factory = this.pool.allocate(2);
        const raw = this.pool.allocated(this.factory);
        raw.fill(0);
        put(raw, 0, 0x509148);
        raw[10] = 85;
        raw[34] = 2;
        this.types[85].flags2 = 0x4000;
        this.hooks = {
            cost: (type) => { assert.equal(type, 85); this.trace.push('cost'); return 500; },
            addSp: (owner, delta) => { this.trace.push('money'); this.money.push([owner, delta]); },
            reserveShared: () => { this.trace.push('reserve'); return this.reserve; },
            addShared: (parent, event, delay) => { assert.equal(parent, this.factory); this.trace.push('timer'); this.timers.push([parent, event, delay]); },
            send: (target, packet) => { assert.equal(target, 9); this.trace.push('send'); this.sent.push(Uint8Array.from(packet)); },
            broadcast: (packet) => { this.trace.push('broadcast'); this.broadcasts.push(Uint8Array.from(packet)); },
            log: (kind, text) => { assert.ok(kind === 4 && text === 'new ftUPGRADE sharedRegPro\n'); this.trace.push('log'); },
            assertion: (text) => { assert.equal(text, 'factory.getGenus() & gFACTORY'); this.trace.push('assert'); },
        };
    }

    // 구현 helper 와 별도로 같은 wire 필드를 직접 쓴 독립 입력/기대 원문이다. (2026-11-10 WB04-E218)
    packet(due) {
        const raw = new Uint8Array(16);
        raw[0] = 1;
        raw[1] = 0x58;
        put(raw, 2, 16, 2);
        put(raw, 4, this.factory, 2);
        raw[7] = 1;
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, due, true);
        put(raw, 8, view.getUint32(0, true));
        put(raw, 12, view.getUint32(4, true));
        return raw;
    }

    // 같은 현재 시각/권한/실제 슬롯을 빌리는 요청/수신 몸체이다. (2026-11-10 WB04-E218)
    make() {
        return new RawFactoryUpgradeCommand(this.pool, this.types, { boss: () => this.boss, checking: () => this.checking, now: () => this.now }, { ...this.hooks });
    }
}

test('업그레이드 요청: 대기 20초와 wire 완료 15초를 구분하며 정확히 16바이트를 송신한다', () => {
    const input = new CommandInput();
    input.make().request(input.factory);
    assert.deepEqual(input.trace, ['reserve', 'timer', 'send']);
    assert.deepEqual(input.timers, [[input.factory, 4, 20]]);
    assert.deepEqual(input.sent, [input.packet(input.now + 15)]);
    assert.deepEqual(input.money, []);
});

test('업그레이드 요청: 할당 실패도 송신은 계속하고 예약/부착 효과 뒤 현재 시각으로 메시지를 만든다', () => {
    const missing = new CommandInput();
    missing.reserve = false;
    delete missing.hooks.addShared;
    missing.make().request(missing.factory);
    assert.deepEqual([missing.timers, missing.sent.length, missing.trace], [[], 1, ['reserve', 'send']]);
    const changed = new CommandInput();
    changed.hooks.addShared = (parent, event, delay) => { changed.timers.push([parent, event, delay]); changed.now += 3.25; };
    changed.make().request(changed.factory);
    assert.deepEqual(changed.sent[0], changed.packet(changed.now + 15));
});

test('업그레이드 수신: 실제 비용→로그→할당/서버 event 3→원문 방송이며 뒤의 호스트 버퍼는 방송하지 않는다', () => {
    const input = new CommandInput();
    const first = input.packet(input.now + 15);
    const packet = new Uint8Array(24).fill(0xa7);
    packet.set(first);
    input.make().receive(packet);
    assert.deepEqual(input.trace, ['cost', 'money', 'log', 'reserve', 'timer', 'broadcast']);
    assert.deepEqual(input.money, [[2, -500]]);
    assert.deepEqual(input.timers, [[input.factory, 3, 15]]);
    assert.deepEqual(input.broadcasts, [first]);
});

test('업그레이드 수신: 기한 전/같음/미래와 unordered(NaN)를 구별하며 NaN 을 1초로 바꾸지 않는다', () => {
    for (const delta of [-1, 0, 1, 2.25, Number.NaN]) {
        const input = new CommandInput();
        input.make().receive(input.packet(input.now + delta));
        assert.equal(input.timers.length, 1);
        const actual = input.timers[0][2];
        if (Number.isNaN(delta)) assert.ok(Number.isNaN(actual));
        else assert.equal(actual, Math.fround(delta < 1 ? 1 : delta));
    }
});

test('업그레이드 수신: 상태/모드/genus 관문은 선택되지 않은 효과를 요구하지 않고 진단 뒤에는 현재 genus 를 다시 읽는다', () => {
    // free(1)·dead(2)·둘 다(3) 어느 비트든 부모가 무효이면 아무 하위도 선택하지 않는다.
    for (const state of [1, 2, 3]) {
        const dead = new CommandInput();
        dead.pool.allocated(dead.factory)[11] = state;
        dead.hooks = {};
        dead.make().receive(dead.packet(dead.now + 15));
        assert.deepEqual(dead.trace, []);
    }
    const plain = new CommandInput();
    plain.types[85].flags2 = 0;
    plain.hooks = {};
    plain.make().receive(plain.packet(plain.now + 15));
    const checked = new CommandInput();
    checked.checking = 1;
    checked.types[85].flags2 = 0;
    checked.hooks.assertion = () => { checked.trace.push('assert'); checked.types[85].flags2 = 0x4000; };
    checked.make().receive(checked.packet(checked.now + 15));
    assert.equal(checked.trace[0], 'assert');
    assert.equal(checked.money.length, 1);
    assert.equal(checked.timers.length, 1);
});

test('업그레이드 수신: 비용·할당·부착 효과 뒤의 현재 owner/시각/boss 를 원본 위치에서 다시 읽는다', () => {
    const input = new CommandInput();
    input.hooks.cost = (type) => { assert.equal(type, 85); input.pool.allocated(input.factory)[34] = 3; return 500; };
    input.hooks.reserveShared = () => { input.now += 1; return true; };
    input.hooks.addShared = (parent, event, delay) => { input.timers.push([parent, event, delay]); input.boss = false; };
    input.make().receive(input.packet(input.now + 15));
    assert.deepEqual(input.money[0], [3, -500]);
    assert.equal(input.timers[0][2], 14);
    assert.deepEqual(input.broadcasts, []);
});

test('업그레이드 수신: 할당 실패는 비용/방송을 되돌리지 않고 선택된 누락도 앞선 실제 효과를 보존한다', () => {
    const failed = new CommandInput();
    failed.reserve = false;
    delete failed.hooks.addShared;
    failed.make().receive(failed.packet(failed.now + 15));
    assert.deepEqual([failed.money.length, failed.timers.length, failed.broadcasts.length], [1, 0, 1]);
    const missing = new CommandInput();
    delete missing.hooks.reserveShared;
    assert.ok(throws(() => missing.make().receive(missing.packet(missing.now + 15))));
    assert.equal(missing.money.length, 1);
    assert.deepEqual(missing.trace, ['cost', 'money', 'log']);
    const shortPacket = new CommandInput();
    const packet = shortPacket.packet(shortPacket.now + 15);
    assert.ok(throws(() => shortPacket.make().receive(packet.subarray(0, 8))));
    // 8 BYTE 본문은 비용 차감까지 진행한 뒤 시각 필드(+8, 8 BYTE) 읽기에서 거부되므로 차감은 남고 예약은 없다.
    assert.equal(shortPacket.money.length, 1);
    assert.equal(shortPacket.timers.length, 0);
});

test('업그레이드 수신: 연구 수신 부모는 kind 0/mode 1 을 owner/sender 조건 없이 실제 수신으로 넘기며 미연결 호출은 거부한다', () => {
    const input = new CommandInput();
    const command = input.make();
    const state = { loadingDepth: () => 0, productionDirty: { value: 0 }, boss: () => input.boss, now: () => input.now, outpostType: () => 126 };
    const research = new RawFactoryResearch(input.pool, input.types, state, { upgradeReceive: (packet) => command.receive(packet) });
    const packet = input.packet(input.now + 15);
    research.receive(0xffffffff, packet);
    assert.equal(input.money.length, 1);
    assert.equal(input.timers.length, 1);
    const absent = new RawFactoryResearch(input.pool, input.types, state, {});
    assert.ok(throws(() => absent.receive(2, packet)));
});

// 같은 원문/Host 와 명시 가상·장치 효과를 준비한다(cpppj UpgradeScene). (2026-11-10 WB04-E218)
class UpgradeScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.types = realPlayerAssets().types.map((record) => ({ ...record }));
        this.spots = new Uint8Array(65536);
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.process = new SquidProcessState();
        // ProcessForm 에는 실제 Host 가상 삭제를 쓴다.
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.process, {
            asset: {
                emit: (event) => {
                    if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) this.destroy.completePreDestroy();
                    else if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) this.destroy.completePostDestroy();
                },
                selected: () => 0,
            },
        });
        this.outpost = 126;
        this.dependType = 1;
        this.local = 2;
        this.checking = 0;
        this.challenge = false;
        this.boss = true;
        this.ai = false;
        this.parent = 0;
        this.image = 0;
        this.trace = [];
        this.watchers = new Map();
        this.removed = [];
        this.assertions = [];
        this.parent = this.fresh();
        const raw = this.pool.allocated(this.parent);
        raw[10] = 85;
        raw[34] = 2;
        put(raw, 0, 0x509148);
        put(raw, 14, floatToBits(7.25));
        put(raw, 18, floatToBits(8.5));
        this.hooks = {
            level: (sid) => { assert.equal(sid, this.parent); this.trace.push('level'); return this.pool.slot(sid)[11] >> 5; },
            setLevel: (sid, level) => { this.trace.push('set'); const value = this.pool.allocated(sid); value[11] = (value[11] & 31) | (level << 5); },
            maxHp: (sid) => { assert.ok(sid === this.parent && (this.pool.slot(sid)[11] >> 5) === 1); this.trace.push('hp'); return 321; },
            adjustFrame: (sid) => { assert.equal(get(this.pool.slot(sid), 26), 321); this.trace.push('frame'); },
            display: () => this.trace.push('display'),
            soundAt: (x, y, name) => { assert.ok(x === 7.25 && y === 8.5 && name === 'upgradeDone.wav'); this.trace.push('position'); },
            sound: (name) => { assert.equal(name, 'upgradeComplete.wav'); this.trace.push('sound'); },
            flare: (sid, count, event) => { assert.ok(sid === this.parent && count === 6 && event === 2); this.trace.push('flare'); },
            create: (type, flags) => {
                assert.ok(type === 85 && flags === 2);
                this.trace.push('create');
                this.image = this.fresh();
                const value = this.pool.allocated(this.image);
                value[10] = 85;
                value[11] = 4;
                put(value, 12, 0xa5a5, 2);
                put(value, 36, 77);
                return this.image;
            },
            owner: (sid, owner) => {
                assert.ok(this.pool.slot(sid)[40] === 9 && get(this.pool.slot(sid), 12, 2) === 0 && get(this.pool.slot(sid), 36) === 77);
                this.trace.push('owner');
                this.pool.allocated(sid)[34] = owner;
            },
            pop: (sid, x, y, flags) => {
                assert.ok(sid === this.image && x === 7.25 && y === Math.fround(8.5 + 0.0010000000474974513) && flags === 0 && this.pool.slot(sid)[34] === 2);
                this.trace.push('pop');
            },
            construction: (sid, duration) => { assert.ok(sid === this.image && duration === 15); this.trace.push('construction'); },
            depend: (watched, watcher) => { assert.ok(watched === this.parent && watcher === this.image); this.trace.push('depend'); this.link(watcher); },
            watcher: (form) => { this.trace.push('watcher'); return this.watchers.get(form); },
            destroy: (sid, flags) => { assert.equal(flags, 0); this.removed.push(sid); this.trace.push('destroy'); },
            aiPresent: (sid) => { assert.equal(sid, this.parent); this.trace.push('ai'); return this.ai; },
            notifyAi: (owner, sid) => { assert.ok(sid === this.parent && owner === this.pool.slot(this.parent)[34]); this.trace.push('notify'); },
            log: (kind, text) => { assert.ok((kind === 3 && text === 'Upgrading!\n') || (kind === 4 && text === 'making the client upgrade picture')); this.trace.push('log'); },
            assertion: (line) => this.assertions.push(line),
        };
    }

    // 합성 입력은 할당 상태와 무관한 payload 를 명시적으로 0 으로 준비한다. (2026-11-10 WB04-E218)
    fresh(flags = 2) {
        const sid = this.pool.allocate(flags);
        this.pool.allocated(sid).fill(0);
        return sid;
    }

    // 원본처럼 부모 head 에 삽입하며 감시자 반환은 별도 명시 입력이다. (2026-11-10 WB04-E218)
    link(watched, type = 1) {
        const form = this.fresh();
        const raw = this.pool.allocated(form);
        raw[10] = type;
        put(raw, 4, get(this.pool.slot(this.parent), 6, 2), 2);
        put(this.pool.allocated(this.parent), 6, form, 2);
        this.watchers.set(form, watched);
        return form;
    }

    // 현재 참조 전역과 같은 실제 Host 를 매번 빌린다(호출 시점의 hooks 사본). (2026-11-10 WB04-E218)
    make() {
        return new RawFactoryUpgrade(this.pool, this.host, {
            outpost: () => this.outpost, dependType: () => this.dependType, localPlayer: () => this.local,
            checking: () => this.checking, challenge: () => this.challenge, boss: () => this.boss,
        }, { ...this.hooks });
    }
}

test('Factory 업그레이드: begin 은 생성→두 번 레벨 조회→가상 상태→abstract/state/owner→Pop→건설→감시 순서이다', () => {
    const input = new UpgradeScene();
    input.make().begin(input.parent, 15);
    assert.deepEqual(input.trace, ['create', 'level', 'level', 'set', 'owner', 'pop', 'log', 'construction', 'depend']);
    assert.equal(input.pool.slot(input.image)[11] >> 5, 1);
    assert.ok(input.make().hasPicture(input.parent));
});

test('Factory 업그레이드: 상한에서는 두 번째 가상 조회가 없고 하위가 바뀌면 원본 재조회 결과를 따른다', () => {
    const input = new UpgradeScene();
    input.pool.allocated(input.parent)[11] = 96;
    assert.equal(input.make().nextLevel(input.parent), 3);
    assert.equal(input.trace.length, 1);
    let calls = 0;
    input.hooks.level = () => (++calls === 1 ? 0 : 2);
    assert.equal(input.make().nextLevel(input.parent), 3);
    assert.equal(calls, 2);
});

test('Factory 업그레이드: 최대 HP 쓰기와 프레임/표시/소리·플레어·현재 AI 통지 순서를 확인한다', () => {
    const input = new UpgradeScene();
    input.ai = true;
    input.hooks.soundAt = () => { input.trace.push('position'); input.pool.allocated(input.parent)[34] = 3; input.local = 3; };
    assert.equal(input.make().finish(input.parent), -1);
    assert.equal(get(input.pool.slot(input.parent), 26), 321);
    assert.deepEqual(input.trace, ['log', 'level', 'level', 'set', 'hp', 'frame', 'display', 'position', 'sound', 'flare', 'ai', 'notify']);
});

test('Factory 업그레이드: 아웃포스트의 완료는 무효이며 finish 의 후속 AI 검사/통지는 별도로 남는다', () => {
    const input = new UpgradeScene();
    input.outpost = 85;
    input.ai = true;
    input.make().complete(input.parent);
    assert.deepEqual(input.trace, []);
    assert.equal(input.make().finish(input.parent), -1);
    assert.deepEqual(input.trace, ['log', 'ai', 'notify']);
});

test('Factory 업그레이드: 완료는 같은 실제 Host 의 모든 event 4/3 예약을 제거하되 다른 사건은 보존한다', () => {
    const input = new UpgradeScene();
    const other = input.host.addRegular(input.parent, 8, 3);
    input.host.addRegular(input.parent, 4, 20);
    input.host.addSharedRegular(input.parent, 4, 20);
    input.host.addSharedRegular(input.parent, 3, 15);
    input.host.addRegular(input.parent, 3, 15);
    assert.ok(input.make().isUpgrading(input.parent));
    input.make().complete(input.parent);
    assert.ok(!input.host.findEvent(input.parent, 4) && !input.host.findEvent(input.parent, 3));
    assert.equal(input.host.findEvent(input.parent, 8), other);
    assert.equal(input.kernel.size, 1);
    assert.ok(!input.make().isUpgrading(input.parent));
});

test('Factory 업그레이드: 다음 링크를 먼저 보관하고 같은 타입/extra 8/client 인 감시자만 지운다', () => {
    const input = new UpgradeScene();
    const first = input.fresh();
    const second = input.fresh();
    const server = input.fresh(0);
    const foreign = input.fresh();
    // 각 감시자의 명시 TYPE/extra 입력을 공급한다.
    for (const sid of [first, second, server, foreign]) {
        input.pool.allocated(sid)[10] = 85;
        input.pool.allocated(sid)[40] = 8;
    }
    input.pool.allocated(foreign)[10] = 87;
    input.link(server);
    input.link(foreign);
    input.link(second);
    const head = input.link(first);
    input.link(first, 70);
    input.hooks.destroy = (sid, flags) => {
        assert.equal(flags, 0);
        input.removed.push(sid);
        if (sid === first) put(input.pool.allocated(head), 4, 0, 2);
    };
    assert.ok(input.make().hasPicture(input.parent));
    input.make().complete(input.parent);
    assert.deepEqual(input.removed, [first, second]);
});

test('Factory 업그레이드: dead 부모/자기 자신 진단과 선택된 미연결 하위의 앞선 효과를 확인한다', () => {
    const input = new UpgradeScene();
    input.checking = 1;
    input.boss = false;
    input.pool.allocated(input.parent)[11] = 2;
    input.link(input.parent);
    assert.ok(!input.make().hasPicture(input.parent));
    assert.deepEqual(input.assertions, [0x158, 0xad]);
    input.challenge = true;
    input.assertions = [];
    assert.ok(!input.make().hasPicture(input.parent));
    assert.deepEqual(input.assertions, [0xad]);
    delete input.hooks.construction;
    assert.ok(throws(() => input.make().begin(input.parent, 15)));
    assert.equal(input.trace.at(-1), 'log');
    assert.equal(input.pool.slot(input.image)[40], 9);
});

test('Factory 업그레이드: 풀/Host 가 다르거나 종속 체인이 순환하면 거부한다', () => {
    const input = new UpgradeScene();
    const otherPool = new SidPool(32768, { server: true });
    assert.ok(throws(() => new RawFactoryUpgrade(otherPool, input.host, {}, input.hooks)));
    // 부모의 head 가 자기 자신을 가리키는 체인은 단계 상한에서 거부한다.
    const form = input.link(input.parent);
    put(input.pool.allocated(form), 4, form, 2);
    assert.ok(throws(() => input.make().hasPicture(input.parent)));
});
