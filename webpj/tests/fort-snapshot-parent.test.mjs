// 최초 PE2728관측의 전체 상태·헤더·호출 순서/부분 중단을 원문 네 부모에 대조한다(WB04-E627).
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put } from './raw-support.mjs';
import { snapshotFortTemplate, snapshotBattleTemplate, snapshotCurrentTemplate, snapshotCurrentTemplateIfPresent } from '../www/core/raw-fort-snapshot-parent.js';

// 최초 관측이 준비한 manager 포인터와 ID다. 포인터0도 독립 장치로 그대로 허용한다.
const MANAGERS = new Map([[0, 0], [11, 0x12000100], [22, 0x12000200]]);
// 제품의 오류/진단을 삼키지 않고 독립 관측의 emu_stop만 구별한다.
class SnapshotStop extends Error {}
// 관측 입력과 외부 child의 명시 응답만 구성한다. 원문 기대열은 실행 뒤에만 사용한다.
class SnapshotDevices {
    // 입력 kind/고정 슬롯과 현재 전역, 실제 manager 헤더·현재 선택·횟수·순서다.
    kind; slot; owner; fort; battle; mutation; activeSlot; parent = 111; created = 0;
    headers = new Uint8Array(240); currents = new Map([[0, 90], [11, 91], [22, 92]]); calls = new Map(); trace = [];
    // 실제12기록(-2..9)의 주변16BYTE도 최초 패턴대로 준비한다.
    constructor(input) {
        const fields = input.split(','); assert.equal(fields.length, 7);
        [this.kind, this.slot] = fields.slice(0, 2).map(Number); const manager = Number(fields[2]);
        [this.owner, this.fort, this.battle] = fields.slice(3, 6).map(Number); this.mutation = fields[6]; this.activeSlot = this.slot;
        // 미선택 기록도 같은 물리 패턴이며 제품의 순회/분기를 여기서 계산하지 않는다.
        for (let selected = -2; selected <= 9; selected++) {
            const at = (selected + 2) * 20;
            // 각 기록의 첫 DWORD 외 모든 BYTE를 비교 대상으로 남긴다.
            for (let byte = 0; byte < 20; byte++) this.headers[at + byte] = (selected + 2) * 13 + byte;
            put(this.headers, at, MANAGERS.get(selected % 2 ? 11 : 22), 4);
        }
        put(this.headers, (this.slot + 2) * 20, MANAGERS.get(manager), 4);
    }
    // 실제 child만1기준으로 세며 getter는 계수에 넣지 않는다.
    event(name, text) { this.trace.push(text); const count = (this.calls.get(name) ?? 0) + 1; this.calls.set(name, count); return name + count; }
    // 최초 PE에 공급한 외부 변경/중단 입력이다. 반환값 변경은 각 child가 별도로 처리한다.
    mutate(boundary) {
        const dash = this.mutation.lastIndexOf('-'); if (dash < 0 || this.mutation.slice(0, dash) !== boundary) return;
        const action = this.mutation.slice(dash + 1);
        if (action.startsWith('manager')) put(this.headers, (this.activeSlot + 2) * 20, MANAGERS.get(Number(action.slice(7))), 4);
        else if (action.startsWith('owner')) this.owner = Number(action.slice(5));
        else if (action.startsWith('fort')) this.fort = Number(action.slice(4));
        else if (action.startsWith('battle')) this.battle = Number(action.slice(6));
        else if (action.startsWith('parent')) this.parent = Number(action.slice(6));
        else if (action === 'current999') { for (const manager of this.currents.keys()) this.currents.set(manager, 999); }
        else if (action === 'stop') throw new SnapshotStop();
    }
    // 각 전역 getter의 실제 읽기 순서와 값만 기록한다.
    globals() {
        return { templateOwner: () => { this.trace.push('owner:' + this.owner); return this.owner; }, fort: () => { this.trace.push('fort:' + this.fort); return this.fort; }, battle: () => { this.trace.push('battle:' + this.battle); return this.battle; } };
    }
    // 고정 this child는 실제 manager getter가 선택한 같은 슬롯을 받아야 한다.
    slotChild(name, slot) { assert.equal(slot, this.activeSlot); this.mutate(this.event(name, name + ':' + slot)); }
    // 네 원문 부모가 호출하는 외부 장치다. 각 child의 전체 구현 성공을 가장하지 않는 관측 어댑터다.
    hooks() {
        const hooks = {
            manager: (slot) => { assert.ok(slot >= -2 && slot <= 9); this.activeSlot = slot; const pointer = get(this.headers, (slot + 2) * 20, 4); const entry = [...MANAGERS].find(([, value]) => value === pointer); assert.ok(entry); this.trace.push('manager:' + slot + ':' + entry[0]); return entry[0]; },
            assertion: (reason) => this.mutate(this.event('assert', 'assert:' + reason)),
            technologyParent: () => { const parent = this.parent; const boundary = this.event('parent', 'parent:' + parent); this.mutate(boundary); return this.mutation === boundary + '-return0' ? 0 : parent; },
            create: (manager, name) => { const token = ++this.created; const boundary = this.event('create', 'create:' + manager + ':' + name + ':' + token); assert.ok(this.currents.has(manager)); this.currents.set(manager, token); this.mutate(boundary); return this.mutation === boundary + '-return0' ? 0 : token; },
            technology: (parent, stream) => this.mutate(this.event('technology', 'technology:' + parent + ':' + stream)),
            money: (stream) => this.mutate(this.event('money', 'money:' + stream)), mission: (stream) => this.mutate(this.event('mission', 'mission:' + stream)),
        };
        // 같은 슬롯을 받는 일곱 child의 호출명만 관측 형식에 매핑한다.
        for (const [key, name] of [['typeNames', 'names'], ['deck', 'deck'], ['objects', 'objects'], ['territory', 'territory'], ['badges', 'badges'], ['coreData', 'core'], ['notify', 'notify']]) hooks[key] = (slot) => this.slotChild(name, slot);
        return hooks;
    }
    // 입력 kind에 해당하는 실제 제품 진입 하나를 실행한다.
    run(hooks = this.hooks()) {
        const globals = this.globals();
        if (this.kind === 0) snapshotFortTemplate(this.slot, globals, hooks);
        else if (this.kind === 1) snapshotBattleTemplate(this.slot, globals, hooks);
        else if (this.kind === 2) snapshotCurrentTemplate(globals, hooks);
        else if (this.kind === 3) snapshotCurrentTemplateIfPresent(globals, hooks);
        else throw new RangeError('관측 snapshot kind 범위');
    }
    // 실제 현재8DWORD의 순서이며 전체 헤더 비교와 별개다.
    state() { return [this.owner, this.fort, this.battle, this.parent, this.created, this.currents.get(0), this.currents.get(11), this.currents.get(22)].join(','); }
}

test('Fort snapshot 부모: 최초 원본2728관측의 고정 this·현재 전역·stream·정상/중단과 전체 헤더가 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-snapshot-parent-x86.tsv')); const unique = new Set(); let stopped = 0;
    // 기대는 실제 함수 실행 뒤에만 읽으며 두 정밀도와 모든 부분 중단을 빠짐없이 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 6); const key = row[0] + ':' + row[1]; assert.equal(unique.has(key), false); unique.add(key);
        const device = new SnapshotDevices(row[1]); let completed = true;
        try { device.run(); } catch (error) { if (!(error instanceof SnapshotStop)) throw error; completed = false; stopped++; }
        assert.equal(completed, row[2] === '1', key); assert.equal(device.state(), row[3], key); assert.equal(Buffer.from(device.headers).toString('hex'), row[4], key); assert.equal(device.trace.join(';'), row[5], key);
    }
    assert.equal(rows.length, 2728); assert.equal(stopped, 378);
});

test('Fort snapshot 부모: Fort/Battle의 미선택 child는 요구하지 않고 선택된 누락은 실제 접두를 보존한다', () => {
    const fort = new SnapshotDevices('0,1,11,1,1,1,none'); const first = fort.hooks(); delete first.coreData; fort.run(first); assert.equal(fort.calls.get('badges'), 1); assert.equal(fort.calls.has('core'), false);
    const battle = new SnapshotDevices('1,1,11,1,1,1,none'); const second = battle.hooks(); delete second.typeNames; delete second.badges; battle.run(second); assert.equal(battle.calls.has('names'), false); assert.equal(battle.calls.has('badges'), false); assert.equal(battle.calls.get('core'), 1);
    const missing = new SnapshotDevices('0,1,11,1,1,1,none'); const third = missing.hooks(); delete third.create; assert.throws(() => missing.run(third), /실제 경계 미연결/); assert.equal(missing.calls.get('names'), 1); assert.equal(missing.calls.get('parent'), 1); assert.equal(missing.created, 0);
    const absent = new SnapshotDevices('3,1,0,1,1,1,none'); absent.run({ manager: absent.hooks().manager }); assert.equal(absent.trace.join(';'), 'owner:1;manager:1:0');
});

test('Fort snapshot 주소: DWORD 상위 비트 별칭은 보존하고 현재20BYTE 경계 밖은 child 선택 전에 거부한다', () => {
    const alias = new SnapshotDevices('2,1,11,1073741825,1,1,none'); alias.run(); assert.equal(alias.calls.has('assert'), false); assert.equal(alias.calls.get('notify'), 2);
    // 0xffffffff는 원문 주소 계산에서 정상 슬롯-1이다. 20배가 DWORD를 넘겨 비정렬 delta가 되는 입력을 사용한다.
    let managers = 0; assert.throws(() => snapshotCurrentTemplateIfPresent({ templateOwner: () => 0x10000000 }, { manager: () => { managers++; } }), RangeError); assert.equal(managers, 0);
    snapshotCurrentTemplate({ battle: () => 0, fort: () => 0 }, {});
});
