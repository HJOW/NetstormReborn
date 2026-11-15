// 공통 객체 메뉴 Open raw-object-menu-open.js(RawObjectMenuOpen.open·position·openFactory·openPointerObjectMenu)를 원본 10.78 관측 788입력과 cpppj 계약으로 대조한다(WB04-E543).
//  - object-menu-open-x86.tsv 788행: Open(004adb30)·Position(00425f90) 두 부모의 입력 15 DWORD(모드·extra·TYPE flags·제단·위임 반환·변경 경계·내용물·사정거리·메뉴 폭·화면 폭)에서
//    반환값·효과 순서와 전체 인자·콜백 뒤 현재 슬롯/사건/화면 폭 필드.
//  - 계약: 조기 거부는 GUI 연결을 요구하지 않고 누락된 경계는 그 위치에서 거부, 판본/TYPE 수 거부, 포인터 열기의 소리 한 번.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E543)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawObjectMenuOpen, openPointerObjectMenu } from '../www/core/raw-object-menu-open.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E543)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'object-menu-open-x86.tsv'));
// signed 좌표도 unsigned 십진수로 읽어 상위 비트를 유지한다. (2026-11-15 WB04-E543)
const numbers = (text) => text.split(',').map((value) => Number(value) >>> 0);
// 비정렬 값을 낮은 바이트부터 쓴다. (2026-11-15 WB04-E543)
function put(raw, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 하나의 실제 풀/현재 TYPE/사건을 여러 독립 입력에서 재사용하는 장면이다. (2026-11-15 WB04-E543)
class OpenScene {
    // 클라이언트 번호 5~77 을 실제 free 목록에서 확보한다.
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.input = { code: 0x00020000, x: 0, y: 0 };
        this.row = [];
        this.screen = 0;
        this.events = '';
        for (let sid = 5; sid <= 77; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // 각 독립 입력의 현재 슬롯/TYPE/사건만 초기화하며 기대값은 읽지 않는다.
    prepare(values) {
        assert.equal(values.length, 15);
        this.row = values;
        this.events = '';
        this.screen = values[14];
        this.input = { code: 0x00020000, x: values[7] | 0, y: values[8] | 0 };
        const raw = this.pool.allocated(50);
        raw.fill(0);
        raw[10] = 100;
        raw[40] = values[1];
        put(raw, 14, floatToBits(12.5));
        put(raw, 18, floatToBits(-3.25));
        put(this.pool.allocated(77), 18, 102, 2);
        this.types[100].flags2 = values[2];
        this.types[101].flags2 = values[6];
    }

    // 외부 효과 입력의 변경만 공급하며 제품의 부모 분기를 대신 실행하지 않는다.
    mutate(boundary) {
        const row = this.row;
        if (row[5] === boundary) this.pool.allocated(50)[10] = 101;
        if (row[5] === 4 && boundary === 3) { this.input.x = 99; this.input.y = -33; }
        if (row[5] === 5 && boundary === 2) this.pool.allocated(50)[40] = 1;
        if (row[5] === 6 && boundary === 6) this.screen = 400;
    }

    // 명시 경계의 전체 DWORD 인자를 원본 관찰과 동일한 문자열로 직렬화한다.
    event(letter, values = []) {
        this.events = `${this.events}${this.events ? ';' : ''}${letter}${values.map((value) => `:${value >>> 0}`).join('')}`;
    }

    // 실제 모듈이 선택한 경계의 인자/반환/변경 입력만 연결한다.
    hooks() {
        const row = this.row;
        return {
            altar: (sid) => { this.event('D', [sid]); return row[3]; },
            delegate: (sid, event) => { this.event('V', [sid, event.code, event.x, event.y]); return row[4]; },
            beginFancy: () => { this.event('B'); this.mutate(1); },
            summary: (sid) => { this.event('S', [sid]); this.mutate(2); },
            populate: (sid) => { this.event('M', [sid]); this.mutate(3); },
            project: (x, y) => { this.event('P', [floatToBits(x), floatToBits(y)]); return [row[9] | 0, row[10] | 0]; },
            content: (sid) => { this.event('C', [sid, 1, 0, 0]); return row[11]; },
            range: (type) => { this.event('R', [type]); return bitsToFloat(row[12]); },
            menuWidth: () => { this.event('W'); this.mutate(6); return bitsToFloat(row[13]); },
            screenWidth: () => this.screen | 0,
            attach: (sid, x, y) => this.event('A', [sid, x, y]),
        };
    }

    // 같은 현재 슬롯/사건/화면 폭의 원문 비트를 읽는다.
    current() {
        const raw = this.pool.slot(50);
        return [raw[10], raw[40], this.input.x >>> 0, this.input.y >>> 0, this.screen >>> 0];
    }
}

test('공통 객체 메뉴 Open: 원본 10.78 788입력의 두 부모 반환·효과 순서·현재 필드가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new OpenScene();
    let count = 0;
    // 모든 입력에서 반환/효과 순서/현재 필드를 대조한다.
    for (const fields of rows) {
        assert.equal(fields.length, 4);
        scene.prepare(numbers(fields[0]));
        const open = new RawObjectMenuOpen(scene.pool, scene.types, scene.hooks());
        if (scene.row[0] === 0) assert.equal(open.open(50, scene.input), Number(fields[1]) >>> 0, `행 ${count}: ${fields[0]}`);
        else open.position(50);
        assert.equal(scene.events || '-', fields[2], `행 ${count}: ${fields[0]}`);
        assert.deepEqual(scene.current(), numbers(fields[3]), `행 ${count}: ${fields[0]}`);
        count += 1;
    }
    assert.equal(count, 788);
});

test('공통 객체 메뉴 계약: 조기 거부는 GUI 연결을 요구하지 않고 누락된 현재 경계는 그 위치에서 거부하며 판본/TYPE 수를 거부한다', () => {
    const scene = new OpenScene();
    const raw = scene.pool.allocated(50);
    raw[10] = 100;
    raw[40] = 1;
    const blocked = new RawObjectMenuOpen(scene.pool, scene.types, {});
    assert.equal(blocked.open(50, { code: 0, x: 0, y: 0 }), 0);
    assert.equal(blocked.openFactory(50, { code: 0, x: 0, y: 0 }), 0);
    raw[40] = 0;
    assert.throws(() => blocked.open(50, { code: 0, x: 0, y: 0 }), /미연결/);
    // 실제 선행 효과(생성/제목)를 실행한 뒤 populate 의 누락을 확인한다.
    let begins = 0;
    let summaries = 0;
    const incomplete = new RawObjectMenuOpen(scene.pool, scene.types, { beginFancy: () => { begins += 1; }, summary: (sid) => { assert.equal(sid, 50); summaries += 1; } });
    assert.throws(() => incomplete.open(50, { code: 0, x: 0, y: 0 }), /미연결/);
    assert.ok(begins === 1 && summaries === 1);
    // 풀 범위와 TYPE 수는 실행 전에 거부한다.
    assert.throws(() => new RawObjectMenuOpen(scene.pool, scene.types.slice(0, 187), {}), TypeError);
    assert.throws(() => blocked.open(0, { code: 0, x: 0, y: 0 }), RangeError);
});

test('공통 객체 메뉴 계약: 사정거리 상한 2·내용물 kind 는 WORD·Factory 는 사건 좌표를 x,y 순서로 부착한다', () => {
    // 사정거리(float)가 2 이상이면 모두 2 로 제한되어 같은 부착 x 가 나온다(여백 (2+1)<<4).
    const attachX = (rangeValue) => {
        const scene = new OpenScene();
        const raw = scene.pool.allocated(50);
        raw[10] = 100;
        put(raw, 14, floatToBits(12.5));
        put(raw, 18, floatToBits(-3.25));
        put(scene.pool.allocated(77), 18, 102, 2);
        let attached = null;
        const open = new RawObjectMenuOpen(scene.pool, scene.types, {
            project: () => [100, 50], content: () => 77, range: () => rangeValue, menuWidth: () => 10, screenWidth: () => 1000, attach: (sid, x, y) => { attached = [sid, x, y]; },
        });
        open.position(50);
        return attached;
    };
    assert.deepEqual(attachX(5), attachX(2));
    assert.notDeepEqual(attachX(1), attachX(2));
    assert.deepEqual(attachX(2).slice(2), [50]);
    // 내용물 kind 는 +0x12 의 WORD 전체이며 TYPE 수(188) 이상이면 거부한다(하위 BYTE 만 읽으면 통과해 버린다).
    const wide = new OpenScene();
    put(wide.pool.allocated(77), 18, 0x0166, 2);
    const reject = new RawObjectMenuOpen(wide.pool, wide.types, { project: () => [0, 0], content: () => 77 });
    assert.throws(() => reject.position(50), RangeError);
    // Factory +ac 는 생성 → 제목 → 파생 작성 → 사건 좌표 부착 순서이며 x,y 를 바꾸지 않는다.
    const factory = new OpenScene();
    const trace = [];
    const open = new RawObjectMenuOpen(factory.pool, factory.types, {
        beginFancy: () => trace.push('B'), summary: (sid) => trace.push(`S${sid}`), populate: (sid) => trace.push(`M${sid}`), attach: (sid, x, y) => trace.push(`A${sid},${x},${y}`),
    });
    assert.equal(open.openFactory(50, { code: 0, x: 123, y: -45 }), 1);
    assert.deepEqual(trace, ['B', 'S50', 'M50', 'A50,123,-45']);
});

test('포인터 객체 메뉴 열기: 가상 +ac 가 비영을 돌려줄 때만 opengump.wav 를 한 번 재생하고 원래 반환값을 보존한다', () => {
    const sounds = [];
    assert.equal(openPointerObjectMenu(() => 0, (name) => sounds.push(name)), 0);
    assert.deepEqual(sounds, []);
    assert.equal(openPointerObjectMenu(() => 0x123, (name) => sounds.push(name)), 0x123);
    assert.deepEqual(sounds, ['opengump.wav']);
    // 열기 장치가 없으면 거부하고, 열렸는데 소리 장치가 없으면 성공으로 숨기지 않는다.
    assert.throws(() => openPointerObjectMenu(undefined, (name) => sounds.push(name)), /열기 미연결/);
    assert.throws(() => openPointerObjectMenu(() => 1, undefined), /음향 미연결/);
    assert.equal(openPointerObjectMenu(() => 0, undefined), 0);
});
