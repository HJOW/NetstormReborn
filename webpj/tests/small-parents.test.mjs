// 소형 부모 다섯 개(제단 해방·Tutorial 소멸·다리 낙하 진단·사제 회복 HP·Boardcoord 각도)를 원본 10.78 관측으로 대조한다(WB04-E15,
// cpppj AltarReleaseParentTests·TutorialMissionReleaseTests·BridgeFallParentTests·PriestRegenHitPointCurrentTests·BoardAngleCheckingTests 이식).
//  - altar-release-parent-hardware-x86.tsv 1596행: 첫/두 번째 방향·현재 checking/범위·저장 SID·현재 P 프레임과 가상 요청. bridge-fall-parent-x86.tsv 200행: 현재 checking/genus 진단·복귀 뒤 원래 좌표 비트.
//  - priest-regen-hit-points-current-x86.tsv 의 originals 행: Getter/번호/HP trace 와 Setter 인자. board-angle-hardware-x86.tsv 의 originals 행: 실제 CRT fmod 와 현재 checking 의 이진 double 결과.
//  - Tutorial 소멸은 원본 관측이 없고 PE 정적 확인 계약(직접 전역 0 쓰기·기본 Mission 한 번 정리)을 독립 검사한다.
// CD 10.72 행과 cpppj 의 Client/Host 통합 검사는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E15: 소형 부모 다섯 개 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { runAltarReleaseParent } from '../www/core/raw-altar-release-parent.js';
import { releaseTutorialMission } from '../www/core/raw-tutorial-mission-release.js';
import { runBridgeFallParent } from '../www/core/raw-bridge-fall-parent.js';
import { readPriestRegenHitPoints } from '../www/core/raw-priest-regen-hit-points.js';
import { normalizeBoardAngle } from '../www/core/raw-board-angle.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E15)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const altarRows = fixture('altar-release-parent-hardware-x86.tsv');
const fallRows = fixture('bridge-fall-parent-x86.tsv');
const regenAll = fixture('priest-regen-hit-points-current-x86.tsv');
const angleAll = fixture('board-angle-hardware-x86.tsv');

// 입력 DWORD 를 32비트로 감아 읽고 바이트를 소문자 hex 로 표시한다. (2026-11-08 WB04-E15)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const textHex = (text) => Buffer.from(text, 'latin1').toString('hex');

// 같은 제단 50 BYTE 와 두 현재 방향/검색/범위/프레임 입력을 한 호출 동안 유지하는 장면이다(cpppj AltarReleaseScene). (2026-11-08 WB04-E15)
class AltarReleaseScene {
    constructor(row) {
        this.first = Number(row[1]) | 0;
        this.second = Number(row[2]) | 0;
        this.checking = number(row[3]);
        this.found = number(row[4]);
        this.capacity = number(row[5]);
        this.frameBase = number(row[6]);
        this.action = number(row[7]);
        this.directions = 0;
        this.events = '';
        this.raw = new Uint8Array(50);
        // 예약 바이트를 정규화하지 않는다.
        for (let byte = 0; byte < 50; byte += 1) this.raw[byte] = (byte * 7 + 19) & 0xff;
        put(this.raw, 0, 0x61f000);
        this.raw[10] = 164;
        this.raw[11] = 0;
        put(this.raw, 14, 0x3fa00000);
        put(this.raw, 18, 0xc0200000);
        put(this.raw, 36, 0x55667788);
        this.hooks = {
            findDais: (altar) => {
                assert.equal(altar, 50);
                const result = this.found;
                this.events += `F,${get(this.raw, 14)},${get(this.raw, 18)},165,${result};`;
                if (this.action & 1) { this.checking = 0x80000000; put(this.raw, 14, 0x40b00000); put(this.raw, 18, 0x40d00000); }
                return result;
            },
            direction: (altar) => {
                assert.equal(altar, 50);
                const result = this.directions ? this.second : this.first;
                this.events += `G,${this.directions},${result};`;
                if (!this.directions && (this.action & 2)) this.checking = 0;
                this.directions += 1;
                return result;
            },
            assertion: (expression, file, line) => {
                this.events += `A,${line}:${expression}:${file};`;
                if (this.action & 4) { this.checking = 0; this.capacity = 2; this.raw[10] = 165; put(this.raw, 14, 0x40700000); put(this.raw, 18, 0x40900000); }
                if (this.action & 32) this.found = this.found ? 0 : 127;
            },
            valid: (dais) => dais > 0 && dais < this.capacity,
            release: (dais) => {
                this.events += `R,${dais};`;
                if (this.action & 8) { this.raw[10] = 166; put(this.raw, 14, 0x40900000); this.capacity = 1; this.found = 0; }
            },
            unpop: (altar, flags) => {
                assert.deepEqual([altar, flags], [50, 0]);
                this.events += 'U,50,0;';
                if (this.action & 16) { this.raw[10] = 167; put(this.raw, 18, 0xc0d00000); }
            },
            plainP: (altar) => {
                assert.equal(altar, 50);
                const type = this.raw[10];
                const frame = (this.frameBase + type * 17) >>> 0;
                this.events += `T,${type},${frame};`;
                put(this.raw, 36, frame);
            },
            repop: (altar, flags) => { assert.deepEqual([altar, flags], [50, 0]); this.events += 'P,50,0;'; },
        };
    }

    // 실제 부모 실행 뒤 메모리/요청만 출력한다.
    run() {
        runAltarReleaseParent(50, () => this.checking, this.hooks);
        return [hex(this.raw), String(this.checking), String(this.found), String(this.capacity), String(this.directions), this.events || '-'];
    }
}

// 첫/두 번째 방향·현재 checking/범위·저장 SID·현재 P 프레임과 가상 요청을 실제 CPU 와 비교한다.
test('제단 해방 부모: 원본10.78 1596행(방향·checking·범위·프레임·가상 요청)', { skip: !altarRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of altarRows) {
        assert.equal(row.length, 14);
        const actual = new AltarReleaseScene(row).run();
        assert.deepEqual(actual, row.slice(8), `행 ${row[0]} 입력 ${row.slice(1, 8).join(',')}`);
        count += 1;
    }
    assert.equal(count, 1596);
});

// 장치 누락/진단 중단은 이미 일어난 finder/방향/공간/raw 프레임을 되돌리지 않는다.
test('제단 해방 부모: 선택된 장치와 부분 효과', () => {
    const row = altarRows[0].slice();
    [row[1], row[2], row[3], row[4], row[5], row[6], row[7]] = ['12', '0', '0', '0', '128', '7', '0'];
    const skipped = new AltarReleaseScene(row);
    delete skipped.hooks.direction; delete skipped.hooks.assertion; delete skipped.hooks.release;
    skipped.run();
    assert.deepEqual([skipped.directions, get(skipped.raw, 36)], [0, 2795]);
    row[3] = '2147483648';
    const missing = new AltarReleaseScene(row);
    delete missing.hooks.assertion;
    assert.throws(() => missing.run(), /선택된 실제 경계 미연결/);
    assert.deepEqual([missing.directions, get(missing.raw, 36), missing.events.includes('U,')], [1, 0x55667788, false]);
    const stopped = new AltarReleaseScene(row);
    stopped.hooks.assertion = () => { stopped.checking = 0; stopped.raw[10] = 165; throw new Error('선택 진단 중단'); };
    assert.throws(() => stopped.run(), /선택 진단 중단/);
    assert.deepEqual([stopped.checking, stopped.raw[10], get(stopped.raw, 36)], [0, 165, 0x55667788]);
    row[3] = '0';
    const noRepop = new AltarReleaseScene(row);
    delete noRepop.hooks.repop;
    assert.throws(() => noRepop.run(), /선택된 실제 경계 미연결/);
    assert.equal(get(noRepop.raw, 36), 2795);
    assert.ok(noRepop.events.includes('U,50,0;') && !noRepop.events.includes('P,50,0;'));
});

// Tutorial 소멸 전역 여섯 개와 인접 상태를 가진 장면이다(cpppj ReleaseFields). (2026-11-08 WB04-E15)
function releaseFields() {
    return { denySalvage: -7, denyAscend: 9, missionNumber: 0x87654321, tutorialNumber: 0xfedcba98, stage: -65, stageDirty: 0x80000004,
        words() { return [this.denySalvage >>> 0, this.denyAscend >>> 0, this.missionNumber >>> 0, this.tutorialNumber >>> 0, this.stage >>> 0, this.stageDirty >>> 0]; } };
}

// 효과가 같은 현재 필드를 바꾸면 뒤 원문 store/조회에 반영되며 최종 복구 사본이 없어야 한다.
test('Tutorial 소멸: 현재 전역과 호출이 원문 store 순서를 따른다', () => {
    const fields = releaseFields();
    const initial = fields.words();
    const calls = [];
    releaseTutorialMission(fields, {
        setAllowAll: (enabled) => { assert.equal(enabled, 1); assert.deepEqual(fields.words(), initial); calls.push('allow'); fields.denySalvage = 47; fields.stageDirty = 0xffffffff; },
        removeAllBrains: () => { assert.deepEqual(fields.words(), [0, 0, 0, 0, 0, 0]); calls.push('brains'); fields.missionNumber = 71; },
        releaseBaseMission: () => { assert.deepEqual([fields.missionNumber, fields.denySalvage, fields.denyAscend, fields.tutorialNumber, fields.stage, fields.stageDirty], [71, 0, 0, 0, 0, 0]); calls.push('base'); fields.denyAscend = -19; },
    });
    assert.deepEqual(calls, ['allow', 'brains', 'base']);
    assert.deepEqual([fields.missionNumber, fields.denyAscend], [71, -19]);
});

// 허용 표 갱신 실패는 여섯 직접 store 에 도달하지 않고 기본 정리만 실행한다.
test('Tutorial 소멸: 허용 표 실패는 기본 Mission 정리만 실행한다', () => {
    const fields = releaseFields();
    const initial = fields.words();
    const calls = [];
    assert.throws(() => releaseTutorialMission(fields, {
        setAllowAll: () => { calls.push('allow'); fields.stageDirty = 23; throw new Error('allow interrupted'); },
        removeAllBrains: () => calls.push('unexpected brains'),
        releaseBaseMission: () => { calls.push('base'); const expected = initial.slice(); expected[5] = 23; assert.deepEqual(fields.words(), expected); fields.denyAscend = 29; },
    }), /allow interrupted/);
    assert.deepEqual(calls, ['allow', 'base']);
    assert.deepEqual([fields.denySalvage, fields.denyAscend, fields.missionNumber, fields.tutorialNumber, fields.stage, fields.stageDirty], [-7, 29, initial[2], initial[3], -65, 23]);
});

// 004c3268 실패 전 직접 0 쓰기와 AI 하위의 부분 변경을 base 정리와 호출자에게 보존한다.
test('Tutorial 소멸: AI 반납 실패는 직접 0 쓰기와 부분 변경을 보존한다', () => {
    const fields = releaseFields();
    const calls = [];
    assert.throws(() => releaseTutorialMission(fields, {
        setAllowAll: () => calls.push('allow'),
        removeAllBrains: () => { assert.deepEqual(fields.words(), [0, 0, 0, 0, 0, 0]); calls.push('brains'); fields.stage = -31; throw new Error('brains interrupted'); },
        releaseBaseMission: () => { assert.deepEqual([fields.denySalvage, fields.denyAscend, fields.missionNumber, fields.tutorialNumber, fields.stage, fields.stageDirty], [0, 0, 0, 0, -31, 0]); calls.push('base'); fields.stageDirty = 37; },
    }), /brains interrupted/);
    assert.deepEqual(calls, ['allow', 'brains', 'base']);
    assert.deepEqual([fields.stage, fields.stageDirty, fields.denySalvage, fields.denyAscend], [-31, 37, 0, 0]);
});

// 정상 꼬리의 기본 Mission 실패는 해제 재시도로 바꾸지 않고 한 번만 전파한다.
test('Tutorial 소멸: 기본 Mission 실패는 재시도하지 않는다', () => {
    const fields = releaseFields();
    let allow = 0;
    let brains = 0;
    let base = 0;
    assert.throws(() => releaseTutorialMission(fields, {
        setAllowAll: () => { allow += 1; },
        removeAllBrains: () => { brains += 1; },
        releaseBaseMission: () => { base += 1; assert.deepEqual(fields.words(), [0, 0, 0, 0, 0, 0]); throw new Error('base interrupted'); },
    }), /base interrupted/);
    assert.deepEqual([allow, brains, base], [1, 1, 1]);
});

// 모든 효과열/현재 DWORD 와 원래 좌표 비트를 최초 관측에 대조한다.
test('다리 낙하 부모: 원본10.78 200행(현재 checking·genus 진단·복귀 뒤 원래 좌표 비트)', { skip: !fallRows && '원본 fixture 없음' }, () => {
    const POOL = 0x12000000;
    const COORDS = [[0x80000000, 0x41780000], [0x42fa8000, 0xc0600000], [0x7fc12345, 0x7f800000]];
    let rows = 0;
    for (const row of fallRows) {
        assert.equal(row.length, 4);
        const p = row[0].split(',').map(number);
        assert.equal(p.length, 6);
        const [sid, initial, genus, coordinate, resume, mutate] = p;
        let word = initial | 0;
        let outcome = 'ok';
        const events = [];
        const hooks = {
            genus: (current) => { assert.equal(current, sid); events.push(`G:${POOL + current * 50}:${genus}`); if (mutate) word = 0; return genus; },
            assertion: (condition, source, line) => {
                events.push(`D:${textHex(condition)}:${textHex(source)}:${line}`);
                if (mutate) word = 0xabcdef10 | 0;
                if (!resume) throw new Error('assertion');
            },
            schedule: (current, x, y) => { assert.equal(current, sid); events.push(`F:${POOL + current * 50}:${floatToBits(x)}:${floatToBits(y)}`); },
        };
        const [xBits, yBits] = COORDS[coordinate];
        try { runBridgeFallParent(sid, bitsToFloat(xBits), bitsToFloat(yBits), () => word >>> 0, hooks); } catch (failure) { outcome = failure.message; }
        const label = `행 ${rows} ${row[0]}`;
        assert.equal(outcome, row[1], `${label} 결과`);
        assert.equal(String(word >>> 0), row[2], `${label} checking`);
        assert.equal(events.join(';'), row[3], `${label} 효과`);
        rows += 1;
    }
    assert.equal(rows, 200);
});

// 사제 회복 HP 부모의 10.78 Getter/번호/HP trace 와 Setter 인자를 원문과 대조한다.
test('사제 회복 HP: 원본10.78 originals 행(Getter·번호·HP 반복 읽기 순서)', { skip: !regenAll && '원본 fixture 없음' }, () => {
    const rows = regenAll.filter((row) => row[0] === 'originals');
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 6);
        const input = row[1].split(',');
        assert.equal(input.length, 7);
        let hp = Number(input[0]) | 0;
        let numberValue = 74;
        const actor = [Number(input[1]) | 0, Number(input[2]) | 0, Number(input[3]) | 0];
        const type = [Number(input[4]) | 0, Number(input[5]) | 0];
        const profile = number(input[6]);
        let actorCalls = 0;
        let typeCalls = 0;
        const trace = [];
        // Native 의 명시 자식 복귀 변경 입력을 그대로 공급한다.
        const mutate = (isActor, countValue) => {
            if (!profile) return;
            if (profile === 1) {
                hp = isActor ? 90 - countValue * 17 : -13 + countValue * 40;
                numberValue = (isActor ? 80 : 90) + countValue;
            } else {
                hp = isActor ? (0x7ffffff0 + countValue) | 0 : (-2147483648 + countValue) | 0;
                numberValue = (isActor ? 110 : 120) + countValue;
            }
        };
        const value = readPriestRegenHitPoints({
            current: () => { trace.push(`H${hp}`); return hp; },
            priestType: () => { trace.push(`N${numberValue}`); return numberValue; },
            maximum: () => { assert.ok(actorCalls < actor.length); const result = actor[actorCalls]; actorCalls += 1; trace.push(`A${result}`); mutate(true, actorCalls); return result; },
            typeMaximum: (selected) => { assert.ok(typeCalls < type.length); const result = type[typeCalls]; typeCalls += 1; trace.push(`T${selected}:${result}`); mutate(false, typeCalls); return result; },
        });
        const label = `행 ${count} ${row[1]}`;
        assert.equal(value === null ? '-' : String(value), row[2], `${label} 반환`);
        assert.equal(trace.join(';'), row[3], `${label} 호출`);
        assert.deepEqual([String(hp), String(numberValue)], [row[4], row[5]], `${label} 현재 값`);
        count += 1;
    }
    assert.ok(count > 0);
    assert.equal(count, rows.length);
});

// 회복 없는 분기는 TYPE 장치를 요구하지 않고 실제 실패 위치를 보존한다.
test('사제 회복 HP: 선택하지 않은 장치와 읽기 접두', () => {
    let trace = '';
    const hooks = { current: () => { trace += 'H'; return 50; }, maximum: () => { trace += 'A'; return 40; } };
    assert.equal(readPriestRegenHitPoints(hooks), null);
    assert.equal(trace, 'HA');
    trace = '';
    hooks.maximum = () => { trace += 'A'; return 100; };
    assert.throws(() => readPriestRegenHitPoints(hooks), /선택된 실제 장치 미연결/);
    assert.equal(trace, 'HA');
});

// 두 정밀도 CRT fmod 와 현재 checking 의 PATCH 결과를 이진 double 전체로 비교한다.
test('Boardcoord 각도: 원본10.78 originals 행(실제 fmod·현재 checking·이진 double)', { skip: !angleAll && '원본 fixture 없음' }, () => {
    const rows = angleAll.filter((row) => row[0] === 'originals');
    const doubleHex = (value) => { const view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, value, true); return hex(new Uint8Array(view.buffer)); };
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 8);
        let checking = number(row[3]);
        const action = number(row[4]);
        const events = [];
        const result = doubleHex(normalizeBoardAngle(bitsToFloat(number(row[1])), {
            checking: () => checking,
            assertion: (expression, file, line) => { events.push(`${line}:${expression}:${file}`); if (action) checking = 0; },
        }, Number(row[2])));
        const label = `행 ${count} ${row.slice(0, 5).join(',')}`;
        assert.equal(result, row[5], `${label} 결과`);
        assert.equal(checking, number(row[6]), `${label} checking`);
        assert.equal(events.length ? events.join('|') : '-', row[7], `${label} 진단`);
        count += 1;
    }
    assert.ok(count > 0);
    assert.equal(count, rows.length);
});

// 장치 누락은 원문 진단이 선택될 때만 실패한다.
test('Boardcoord 각도: 선택된 진단 장치 누락의 정확한 경계', () => {
    const diagnostics = { checking: () => 1 };
    assert.equal(normalizeBoardAngle(360.5, diagnostics, 720), Math.fround(359.9999));
    assert.throws(() => normalizeBoardAngle(361, diagnostics, 720), /진단 장치 미연결/);
    assert.equal(normalizeBoardAngle(0, diagnostics, 720), 0);
});
