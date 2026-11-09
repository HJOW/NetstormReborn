// 객체 Main/Task trace 진단과 가상 +0x70 분배, 원본 CRT 좌표 서식 raw-object-trace-main.js(004b0fb0/00450d60)를 원본 10.78 관측과 cpppj `ObjectTraceMainTests` 계약으로 대조한다.
//  - object-trace-main-x86.tsv 373행: Main/Task 직접 본체의 입력(SID·덤프 모드·TYPE flags1/flags2·로그 직후 필드 변경·슬롯 raw 50 BYTE)에서 실제 로그 장치가 받은 레벨과 서식 바이트 전체와 수치 인자(double 승격 좌표 비트·부호 확장 BYTE·WORD)가 같다.
//  - object-trace-vtables-x86.tsv 87행: PE 에서 독립 추출한 +0x70 주소(자산 83개와 네 내장 Form)에 따라 같은 본체가 불리고(Dude 한 표만 Task 포함) 미등록 표는 로그 전에 거부된다.
//  - object-trace-coordinate-x86.tsv 887행: 0·음의 0·반올림 동률·전 지수·NaN/INF·고정 난수 float32 의 원본 CRT `%2.5f` 바이트가 같다.
// cpppj 의 네 번째 시험(재귀 분배: RawObjectTrace 의 TraceRawObject)은 그 단위가 미이식이라 옮길 때 대조한다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E235: 객체 Main/Task trace 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ObjectTraceFieldKind, traceRawObjectFields } from '../www/core/raw-object-trace-fields.js';
import { formatOriginalTraceCoordinate, formatOriginalTraceCoordinateBits, traceRawObjectMain, traceRawObjectVirtual } from '../www/core/raw-object-trace-main.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다. (2026-11-10 WB04-E235)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const mainRows = readRows(path.join(fixtures, 'object-trace-main-x86.tsv'));
const vtableRows = readRows(path.join(fixtures, 'object-trace-vtables-x86.tsv'));
const coordinateRows = readRows(path.join(fixtures, 'object-trace-coordinate-x86.tsv'));

// 문자열/바이트를 인코딩 변환 없이 16진수 열로 표현한다. (2026-11-10 WB04-E235)
const hexOfText = (text) => Array.from(text, (char) => char.charCodeAt(0).toString(16).padStart(2, '0')).join('');
// 64비트(BigInt)를 메모리 순서(little endian)의 16진수로 쓴다. (2026-11-10 WB04-E235)
const hexOfDouble = (bits) => Array.from({ length: 8 }, (_, index) => Number((bits >> BigInt(index * 8)) & 0xffn).toString(16).padStart(2, '0')).join('');

// 원문 50 BYTE 풀/현재 TYPE·모드와 명시 로그 콜백 변경을 보유한다. (2026-11-10 WB04-E235)
class MainScene {
    constructor(columns = null) {
        this.pool = new SidPool(24000, { server: true }); // 같은 물리 50 BYTE 슬롯이다.
        this.types = Array.from({ length: 188 }, () => ({ name: '', flags1: 0, flags2: 0 })); // 현재 TYPE 속성/이름 표이다.
        this.sid = 123; // 대상 슬롯이다.
        this.suppress = 0; // 덤프 모드이다(로그 장치가 바꾸므로 getter 로 넘긴다).
        this.mutation = 0;
        this.calls = 0;
        this.task = false;
        this.events = []; // 실제 최종 로그 바이트와 수치 인자의 별도 기록이다.
        this.arguments = '';
        this.types[82].name = 'assetName';
        this.types[46].name = 'Regular';
        this.types[6].name = 'ContentForm';
        if (columns) { // 독립 fixture 의 입력만 설치하고 기대 열은 읽지 않는다.
            this.task = columns[0] === 'Task';
            this.sid = Number(columns[1]);
            this.suppress = Number(columns[2]);
            this.types[82].flags1 = Number(columns[3]) >>> 0;
            this.types[82].flags2 = Number(columns[4]) >>> 0;
            this.mutation = Number(columns[5]);
            assert.equal(columns[6].length, 100);
            const raw = this.pool.physical(this.sid);
            for (let index = 0; index < 50; index += 1) raw[index] = parseInt(columns[6].slice(index * 2, index * 2 + 2), 16); // 원시 입력 50 BYTE 를 그대로 설치한다.
        }
    }

    // 지정 폭의 현재 원문 필드만 바꾼다(인접 필드는 보존). (2026-11-10 WB04-E235)
    put(number, offset, value, width) { put(this.pool.physical(number), offset, value, width); }

    // 원문과 같은 로그 반환 시점에만 다음 Main/Task 의 현재 필드를 바꾼다. (2026-11-10 WB04-E235)
    log() {
        return (level, line, numbers) => {
            this.events.push(`${level}:${hexOfText(line)}`);
            if (numbers) this.arguments = `${hexOfDouble(numbers.xBits)}${hexOfDouble(numbers.yBits)}:${numbers.highlight >>> 0},${numbers.quantity},${numbers.region},${numbers.z >>> 0}`;
            this.calls += 1;
            if (this.calls === 1 && this.mutation === 1) { this.put(this.sid, 40, 255, 1); this.put(this.sid, 26, 12345, 4); this.put(this.sid, 30, 254, 1); this.suppress = 1; this.put(this.sid, 12, 0x5106, 2); }
            if (this.calls === 2 && this.mutation === 2) this.put(this.sid, 12, 0x5104, 2);
        };
    }

    eventText() { return this.events.join(';'); }
}

test('객체 Main/Task trace: 원본 10.78 373행의 분기·수치 인자·원본 CRT 서식·로그 직후 필드 재조회가 모두 같다', { skip: !mainRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    for (const columns of mainRows) {
        assert.equal(columns.length, 8);
        const scene = new MainScene(columns);
        traceRawObjectMain(scene.pool, scene.types, () => scene.suppress, scene.sid, scene.task, scene.log());
        assert.equal(`${scene.eventText()}|${scene.arguments}`, columns[7], `행 ${rows + 1} ${columns[0]} sid ${columns[1]} 덤프 ${columns[2]} 변경 ${columns[5]}`);
        rows += 1;
    }
    assert.equal(rows, 373);
});

test('객체 Main/Task trace: PE 의 +0x70 가상 표 87개가 같은 실제 본체를 부르고 미등록 표는 로그 전에 거부된다', { skip: !vtableRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    let tasks = 0;
    for (const columns of vtableRows) {
        const table = parseInt(columns[0], 16);
        const entry = parseInt(columns[3], 16);
        const actual = new MainScene();
        const expected = new MainScene();
        actual.put(123, 0, table, 4);
        expected.put(123, 0, table, 4);
        actual.pool.physical(123)[10] = 82;
        expected.pool.physical(123)[10] = 82;
        traceRawObjectVirtual(actual.pool, actual.types, () => actual.suppress, 123, actual.log());
        if (entry === 0x4b0fb0 || entry === 0x450d60) {
            assert.ok(columns[1] === '004ad000' && columns[2] === '004ad010');
            const task = entry === 0x450d60;
            if (task) tasks += 1;
            traceRawObjectMain(expected.pool, expected.types, () => expected.suppress, 123, task, expected.log());
        } else {
            const kind = { 0x4ae1e0: ObjectTraceFieldKind.Form, 0x4ae3f0: ObjectTraceFieldKind.Process, 0x4ae530: ObjectTraceFieldKind.Depend, 0x4aea90: ObjectTraceFieldKind.Content }[entry];
            assert.notEqual(kind, undefined, columns[0]);
            traceRawObjectFields(expected.pool, expected.types, () => expected.suppress, 123, kind, (level, text) => expected.log()(level, text, null));
        }
        assert.ok(actual.eventText() === expected.eventText() && actual.arguments === expected.arguments, `표 ${columns[0]}`);
        rows += 1;
    }
    assert.ok(rows === 87 && tasks === 1);
    const unknown = new MainScene();
    assert.throws(() => traceRawObjectVirtual(unknown.pool, unknown.types, () => unknown.suppress, 123, unknown.log()), Error);
    assert.equal(unknown.calls, 0);
});

test('객체 Main/Task trace: 0·음의 0·반올림 동률·전 지수·NaN/INF·고정 난수 float32 887개의 원본 CRT 좌표 바이트가 같다', { skip: !coordinateRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    for (const columns of coordinateRows) {
        const bits = parseInt(columns[0], 16);
        assert.equal(hexOfText(formatOriginalTraceCoordinateBits(bits)), columns[1], `bits ${columns[0]}`);
        rows += 1;
    }
    assert.equal(rows, 887);
});

test('객체 Main/Task trace 계약: 수에서 만든 좌표 서식, 로그 장치 부재와 범위 밖 TYPE 의 거부', () => {
    assert.equal(formatOriginalTraceCoordinate(0), '0.00000');
    assert.equal(formatOriginalTraceCoordinate(-0), '0.00000');
    assert.equal(formatOriginalTraceCoordinate(1.5), '1.50000');
    assert.equal(formatOriginalTraceCoordinate(-20.75), '-20.75000');
    assert.equal(formatOriginalTraceCoordinate(Infinity), '1.#INF0');
    assert.equal(formatOriginalTraceCoordinate(0.000004), '0.00000');
    const scene = new MainScene();
    // 로그 장치가 없으면 첫 Base 줄 전에 거부한다.
    assert.throws(() => traceRawObjectMain(scene.pool, scene.types, 0, 123, false, undefined), TypeError);
    // 현재 TYPE 이 표 밖이면 Base 줄에서 범위 오류로 거부하고 Main 줄은 기록하지 않는다.
    scene.pool.physical(123)[10] = 200;
    assert.throws(() => traceRawObjectMain(scene.pool, scene.types, 0, 123, false, scene.log()), RangeError);
    assert.equal(scene.calls, 0);
});

test('객체 Main/Task trace 계약: 로그 직후 TYPE 이 표 밖이 되면 Main 줄에서 거부하고, float32→double 승격은 NaN 부호·가수를 보존하며 조용한 NaN 비트를 켜고, 연속 9 올림은 정수부로 전파된다', () => {
    // 첫 로그(Base 줄) 직후 현재 TYPE 이 표 밖(200)이 되면 두 번째(Main) 줄은 기록하지 않고 RangeError 이다.
    const scene = new MainScene();
    const baseLog = scene.log();
    assert.throws(() => traceRawObjectMain(scene.pool, scene.types, 0, 123, false, (level, line, numbers) => { baseLog(level, line, numbers); scene.pool.physical(123)[10] = 200; }), RangeError);
    assert.equal(scene.calls, 1);
    // 좌표 float 비트가 NaN 이면 승격 double 의 부호·가수 상위 비트를 보존하고 조용한 NaN 비트(51)를 켠다.
    const nan = new MainScene();
    for (const [bits, expected] of [[0x7fc00001, 0x7ff8000020000000n], [0xffc00001, 0xfff8000020000000n], [0x7f800001, 0x7ff8000020000000n], [0x3f800000, 0x3ff0000000000000n]]) {
        nan.put(123, 14, bits >>> 0, 4);
        let seen = null;
        traceRawObjectMain(nan.pool, nan.types, 0, 123, false, (_level, _line, numbers) => { if (numbers) seen = numbers; });
        assert.equal(seen.xBits, expected, bits.toString(16));
    }
    // 0.99999994 는 유지 자릿수가 모두 9 이고 다음 자리가 5 이상이라 올림이 맨 앞까지 전파되어 1.00000 이 된다.
    assert.equal(formatOriginalTraceCoordinate(0.99999994), '1.00000');
});

