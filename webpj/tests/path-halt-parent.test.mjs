// 공통 경로 Halt 부모 raw-path-halt-parent.js(runPathHaltParent)를 실제 CPU 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: path-halt-parent-hardware-x86.tsv 2288행 — 가상 Halt 결과(10/11/12)·checking·boss·Form 번호·부모 flags·TYPE genus·두 목표·재지정 find 응답·재지정 선택·부분 효과(action 비트)에서
//    정지 여부·경로 소멸·checking·boss·부모 번호 필드·Form·두 목표·부모 TYPE/flags·find 횟수·호출 순서(H Halt/F 내용물 조회/C·Y 후보 선택/R·G 이동/K kill/A 진단)가 같고
//    기존 실제 재지정 몸체(E103 RawPathRetarget)와 실제 경로 이미지(E98 RawPathRuntime)를 사용한다.
//  - 계약: 선택된 경계 누락은 거부, 삭제 뒤에는 현재 진단·수명 관찰만 쓰고 진단이 중단하면 그대로 전파, 결과 12 는 진단 없이 반환.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E144: 공통 경로 Halt 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathRetarget } from '../www/core/raw-path-retarget.js';
import { runPathHaltParent } from '../www/core/raw-path-halt-parent.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E144)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'path-halt-parent-hardware-x86.tsv')) ?? [];

// 관측 행 수, 풀 크기, 너겟 타입 번호이다. (2026-11-09 WB04-E144)
const ROWS = 2288;
const CAPACITY = 24000;
const NUGGET = 154;

// 예외가 나는지 본다. (2026-11-09 WB04-E144)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 원문과 같은 현재 raw 부모/경로/전역을 한 호출 동안 유지하는 장면이다. 입력은 행에서만 공급하고 결과는 fixture 에서 읽는다. (2026-11-09 WB04-E144)
class Scene {
    constructor(row) {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.runtime = new RawPathRuntime();
        this.result = Number(row[1]);
        this.checking = Number(row[2]);
        this.boss = Number(row[3]);
        this.form = Number(row[4]);
        this.genus = Number(row[6]);
        this.first = Number(row[8]);
        this.second = Number(row[9]);
        this.choice = Number(row[10]);
        this.action = Number(row[11]);
        this.finds = 0;
        this.alive = true;
        this.events = '';
        this.runtime.setAttachment(5, 50);
        put(this.runtime.bytes, 4, this.form);
        this.runtime.setTargets((Number(row[7]) & 1) ? 60 : 0, (Number(row[7]) & 2) ? 61 : 0);
        // 유효 부모와 죽은 두 대상의 명시 입력이다.
        for (const sid of [50, 51, 52, 60, 61]) {
            const raw = this.pool.physical(sid);
            raw.fill(0);
            raw[10] = 1;
            raw[11] = sid >= 60 ? 2 : 0;
        }
        this.pool.physical(50)[40] = Number(row[5]);
        this.hooks = this.makeHooks();
        const effects = {
            find: (parent, mask, type, index) => { this.events += `F,${parent},${mask},${type},${index};`; const value = this.finds > 0 ? this.second : this.first; this.finds += 1; return value; },
            chooseReturn: (parent) => { this.events += `C,${parent};`; return this.choice; },
            chooseGeyser: (parent) => { this.events += `Y,${parent};`; return this.choice; },
            goReturn: (parent, target) => this.go('R', parent, target),
            goGeyser: (parent, target) => this.go('G', parent, target),
        };
        const retarget = new RawPathRetarget(this.pool, () => NUGGET, effects);
        this.hooks.retarget = (parent) => retarget.update(this.runtime, parent);
    }

    // CPU 장치와 같은 명시 삭제 입력이다(실제 Host 삭제는 별도 검사). (2026-11-09 WB04-E144)
    go(name, parent, target) {
        this.events += `${name},${parent},${target};`;
        if (this.action & 128) this.alive = false;
    }

    // 원문 호출 순서와 같은 외부 응답/현재 전역을 공급하는 경계 묶음이다. (2026-11-09 WB04-E144)
    makeHooks() {
        const raw = (sid) => this.pool.physical(sid);
        return {
            parent: () => 50,
            alive: () => this.alive,
            halt: (parent) => {
                assert.equal(parent, 50);
                this.events += `H,50,${this.result};`;
                if (this.action & 1) this.alive = false;
                if (this.action & 2) this.checking = 0x80000000;
                if (this.action & 4) this.checking = 0;
                if (this.action & 8) this.boss = this.boss ? 0 : 1;
                if (this.action & 16) raw(50)[40] ^= 0x20;
                if (this.action & 32) raw(50)[10] = 2;
                if (this.action & 64) put(this.runtime.bytes, 8, 51);
                return this.result;
            },
            assertion: (expression, file, line) => {
                this.events += `A,${line}:${expression}:${file};`;
                if (this.action & 512) { this.checking = 0; this.boss = 0; put(this.runtime.bytes, 8, 52); }
            },
            forcedStop: (parent) => (raw(parent)[40] & 0x20) !== 0,
            mayKill: () => this.boss !== 0 || ((this.form | 0) > 4 && (this.form | 0) < 15000),
            kill: () => { this.events += `K,${this.form},0;`; this.alive = false; },
            boss: () => this.boss !== 0,
            carrier: (parent) => ((raw(parent)[10] === 1 ? this.genus : (this.genus ^ 0x30000) >>> 0) & 0x30000) !== 0,
            hasTargets: () => this.runtime.targetB !== 0 || this.runtime.targetA !== 0,
        };
    }

    // 호출하고 현재 출력과 실제 요청만 기록한다. (2026-11-09 WB04-E144)
    run() {
        const stop = runPathHaltParent(() => this.checking, this.hooks);
        return [String(stop ? 1 : 0), String(this.alive ? 0 : 1), String(this.checking >>> 0), String(this.boss), String(get(this.runtime.bytes, 8)), String(this.form),
            String(this.runtime.targetA), String(this.runtime.targetB), String(this.pool.slot(50)[10]), String(this.pool.slot(50)[40]), String(this.finds), this.events];
    }
}

test('경로 Halt 부모: 실제 CPU 관측 2288행의 정지·상태·호출 순서가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    // 저장 부모/현재 checking·권한·TYPE/강제 정지와 기존 재지정의 순서를 CPU 에 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 24);
        const actual = new Scene(row).run();
        assert.deepEqual(actual, row.slice(12), `행 ${row[0]}`);
    }
});

test('경로 Halt 부모: 삭제 뒤에는 현재 진단/수명 관찰만 쓰고 미선택 경계와 앞선 효과를 보존한다', () => {
    let checking = 0;
    let alive = true;
    const hooks = { parent: () => 50, halt: () => { alive = false; return 0; }, alive: () => alive };
    assert.equal(runPathHaltParent(() => checking, hooks), true);
    checking = 0x80000000;
    // 진단이 필요한데 장치가 없으면 거부한다.
    alive = true;
    assert.ok(throws(() => runPathHaltParent(() => checking, hooks)));
    hooks.assertion = (expression, file, line) => {
        assert.equal(expression, 'result == haltALREADY_DEAD');
        assert.equal(file, '\\Ns\\O\\Pathprocess.cpp');
        assert.equal(line, 0x2d5);
        checking = 0;
    };
    alive = true;
    assert.equal(runPathHaltParent(() => checking, hooks), true);
    assert.equal(checking, 0);
    // 진단이 중단하면 그대로 전파한다.
    checking = 1;
    alive = true;
    hooks.assertion = () => { checking = 0; throw new Error('원문 진단 중단'); };
    assert.ok(throws(() => runPathHaltParent(() => checking, hooks)));
    assert.equal(checking, 0);
    // 결과 12(이미 죽음)는 진단 없이 반환한다. 숫자 checking 도 허용한다.
    hooks.assertion = undefined;
    hooks.halt = () => { alive = false; return 12; };
    alive = true;
    assert.equal(runPathHaltParent(0xffffffff, hooks), true);
});
