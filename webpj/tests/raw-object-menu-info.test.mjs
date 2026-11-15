// 객체 메뉴 제목/정보 raw-object-menu-info.js(RawObjectMenuInfo.describe·summary·captureStatus·levelName·themeName·bind)를 원본 10.78 관측 1,434입력과 cpppj 시험 계약으로 대조한다(WB04-E546).
//  - object-menu-info-x86.tsv 1,434행: 다섯 연산(0 Describe·1 Summary·2 CaptureStatus·3 LevelName·4 ThemeName)의 20 DWORD 독립 입력과 콜백 변경 조합에서 반환 문자열·번역/서식/GUI 효과 전체 순서·콜백 뒤 현재 필드.
//  - 계약: 거짓 492090 의 short-circuit·미션 비영 상태·미연결 제목 소비자·풀/표 크기 거부·컨트롤 위임.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E546)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawObjectMenuInfo, bindObjectMenuInfoControls, createObjectMenuInfoState } from '../www/core/raw-object-menu-info.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E546)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'object-menu-info-x86.tsv'));
// 바이트 문자열을 호스트 인코딩 변환 없이 원문 16진으로 직렬화한다. (2026-11-15 WB04-E546)
const infoHex = (value) => Buffer.from(value, 'latin1').toString('hex');
// fixture 의 20 DWORD 를 unsigned 십진수로 읽어 signed 좌표/마스크 비트를 보존한다. (2026-11-15 WB04-E546)
const infoNumbers = (text) => text.split(',').map((value) => Number(value) >>> 0);
// 비정렬 값을 낮은 바이트부터 쓴다. (2026-11-15 WB04-E546)
function put(raw, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}
// 단정도 비트 변환 도우미이다. (2026-11-15 WB04-E546)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 같은 실제 풀/현재 TYPE 와 명시 외부 경계 입력의 고정 소유자이다. (2026-11-15 WB04-E546)
class InfoScene {
    // 물리 슬롯을 실제 목록에서 확보하고 현재 값 getter 를 단 한 번 연결한다.
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.types = [];
        this.classes = [];
        this.alliances = new Uint32Array(81);
        this.row = [];
        this.local = 0; this.editor = 0; this.mission = 0; this.titles = 0; this.headers = 0;
        this.events = '';
        // 독립 실행이 사용할 50번까지 정상 클라이언트 영역을 확보한다.
        for (let sid = 5; sid <= 50; sid += 1) assert.equal(this.pool.allocate(2), sid);
        this.state = createObjectMenuInfoState({ localOwner: () => this.local, editor: () => this.editor, missionPost: () => this.mission });
    }

    // 이전 실행의 상태를 독립 입력으로 다시 설치하며 기대값은 참조하지 않는다.
    prepare(values) {
        assert.equal(values.length, 20);
        const row = values;
        this.row = row;
        this.events = '';
        this.local = row[7]; this.editor = row[8]; this.mission = row[12]; this.titles = 0; this.headers = 0;
        this.state.invisible = 117;
        this.state.paralyzed = 118;
        const raw = this.pool.allocated(50);
        raw.fill(0);
        put(raw, 0, 0x12200000);
        raw[10] = row[1];
        raw[34] = row[2];
        raw[40] = row[10];
        put(raw, 14, row[16]);
        put(raw, 18, row[17]);
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0, theme: 0 }));
        this.classes = Array.from({ length: 188 }, () => '');
        this.types[row[1]].flags2 = row[5];
        this.types[row[1]].theme = row[6] | 0;
        this.classes[row[1]] = row[4] ? 'Class % value' : '';
        this.alliances.fill(0);
        this.alliances[row[2] * 9 + row[7]] = row[14];
    }

    // 경계가 바꿀 현재 입력만 적용하며 제품의 부모 분기/문구 선택은 대신 실행하지 않는다.
    mutate(boundary) {
        const row = this.row;
        const raw = this.pool.allocated(50);
        const kind = row[1];
        if (row[15] === 1 && boundary === 'count') { raw[10] = 101; raw[34] = 2; raw[40] = 0x30; }
        if (row[15] === 2 && boundary === 'description') this.classes[kind] = '';
        if (row[15] === 3 && boundary === 'firstTitle') { this.classes[kind] = 'Changed'; this.types[kind].flags2 = 0x10000; this.types[kind].theme = 4; }
        if (row[15] === 4 && boundary === 'relation') { this.editor = 1; put(raw, 14, floatBits(-3.75)); put(raw, 18, floatBits(4.5)); }
        if (row[15] === 5 && boundary === 'subId') { put(raw, 14, floatBits(-17.5)); put(raw, 18, floatBits(21.5)); }
        if (row[15] === 6 && boundary === 'name') { this.local = 2; this.alliances[row[2] * 9 + 2] = 1; }
        if (row[15] === 7 && boundary === 'invisibleHeader') raw[40] = 0;
        if (row[15] === 8 && boundary === 'invisibleTranslation') { this.state.invisible = 99; raw[40] = 0x20; }
        if (row[15] === 9 && boundary === 'ground') { this.mission = 1; raw[34] = 2; }
        if (row[15] === 10 && boundary === 'alignment') this.types[kind].theme = 5;
        if (row[15] === 11 && boundary === 'class') this.classes[kind] = 'Replacement % token';
        if (row[15] === 12 && boundary === 'firstHeader') this.classes[kind] = '';
        if (row[15] === 13 && boundary === 'lastHeader') raw[40] = 0x30;
    }

    // 원본 관찰과 같은 구분 문자로 현재 경계의 인자만 직렬화한다.
    event(letter, argument) { this.events = `${this.events}${this.events ? ';' : ''}${letter}:${argument}`; }

    // 현재 번역 입력은 원본 바이트 그대로 반환하며 순서가 선택한 변경만 공급한다.
    translate(key) {
        this.event('L', infoHex(key));
        if (key === 'Owner' || key === 'Ally' || key === 'Enemy') this.mutate('relation');
        if (key === '~y~[I%d.*]Invisible!~.\n') this.mutate('invisibleTranslation');
        if (key === '\nAlignment: ~i%s~.') this.mutate('alignment');
        if (key === '\nClass: ~i%s~.') this.mutate('class');
        return (this.row[18] ? `${String.fromCharCode(0xe9)}:` : '') + key;
    }

    // 순수 고정 printf 경계만 실행한다. 번역/경고/소유자 분기는 제품 모듈에 있다.
    print(pattern, values) {
        let result = '';
        let argument = 0;
        // 현재 서식의 외부 인자를 원래 순서로 소비한다.
        for (let index = 0; index < pattern.length; index += 1) {
            if (pattern[index] !== '%') { result += pattern[index]; continue; }
            index += 1;
            assert.ok(index < pattern.length, '정보검사서식끝');
            if (pattern[index] === '%') { result += '%'; continue; }
            assert.ok(argument < values.length, '정보검사서식인자부족');
            if (pattern[index] === 's') { assert.equal(typeof values[argument], 'string'); result += values[argument]; } else { assert.equal(pattern[index], 'd'); assert.equal(typeof values[argument], 'number'); result += String(values[argument] | 0); }
            argument += 1;
        }
        assert.equal(argument, values.length);
        this.event('F', infoHex(result));
        return result;
    }

    // 실제 몸체가 선택한 외부 소비자의 반환/입력 변경만 제공한다.
    hooks() {
        const row = this.row;
        return {
            count: (sid) => { this.event('C', String(sid)); this.mutate('count'); return row[3]; },
            description: (kind) => { this.event('D', String(kind)); this.mutate('description'); return `Type${kind}`; },
            ownerName: (owner) => { this.event('P', String(owner)); this.mutate('name'); return row[19] ? `Player${owner}` : 'UNKNOWN'; },
            translate: (key) => this.translate(key),
            substitute: (value) => { this.event('S', infoHex(value)); return `[${value}]`; },
            print: (pattern, values) => this.print(pattern, values),
            title: (value) => { this.event('T', infoHex(value)); this.titles += 1; if (this.titles === 1) this.mutate('firstTitle'); },
            header: (value) => {
                this.event('H', infoHex(value));
                this.headers += 1;
                if (this.headers === 1) this.mutate('firstHeader');
                if (value.startsWith('\nSubID:')) this.mutate('subId');
                if (value.includes('Invisible!')) this.mutate('invisibleHeader');
                if (value === '\n') this.mutate('lastHeader');
            },
            alliance: (owner, current) => this.alliances[owner * 9 + current],
            groundImmobile: (sid) => { this.event('G', String(sid)); this.mutate('ground'); return row[11] !== 0; },
            ownerStatus: (owner) => { assert.ok(owner < 9); return row[13] & 0xff; },
        };
    }

    // 원본 반환 뒤 남은 현재 raw/전역/class/TYPE 필드만 출력한다.
    current() {
        const row = this.row;
        return `${Buffer.from(this.pool.slot(50)).toString('hex')}|${this.local},${this.editor},${this.mission},${this.state.invisible},${this.state.paralyzed}|${infoHex(this.classes[row[1]])}|${this.types[row[1]].flags2},${this.types[row[1]].theme >>> 0}`;
    }
}

test('객체 메뉴 정보: 원본 10.78 1,434입력의 반환 문자열·번역/서식/GUI 효과 순서·콜백 뒤 현재 필드가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new InfoScene();
    let count = 0;
    // 원문 행을 빠뜨리지 않고 반환 문자열/현재 입력과 전체 효과를 대조한다.
    for (const fields of rows) {
        assert.equal(fields.length, 4);
        scene.prepare(infoNumbers(fields[0]));
        const info = new RawObjectMenuInfo(scene.pool, scene.types, scene.classes, scene.state, scene.hooks());
        let result = '-';
        const row = scene.row;
        if (row[0] === 0) info.describe(row[1], row[3], row[2], row[9]);
        else if (row[0] === 1) info.summary(50);
        else if (row[0] === 2) info.captureStatus(50);
        else if (row[0] === 3) result = infoHex(RawObjectMenuInfo.levelName(row[3]));
        else result = infoHex(scene.translate(RawObjectMenuInfo.themeName(row[6] | 0)));
        const label = `행 ${count}: ${fields[0]}`;
        assert.equal(scene.events || '-', fields[1], label);
        assert.equal(result, fields[2], label);
        assert.equal(scene.current(), fields[3], label);
        count += 1;
    }
    assert.equal(count, 1434);
});

test('객체 메뉴 정보 계약: 거짓 492090 은 미션/Player 표를 읽지 않고 비영 미션은 Capturable, 미연결 제목 소비자·풀/표 크기 거부', () => {
    const scene = new InfoScene();
    scene.prepare(infoNumbers('0,100,1,2,0,0,0,1,0,50,0,0,0,0,0,0,0,0,0,0'));
    const types = Array.from({ length: 188 }, () => ({ flags2: 0, theme: 0 }));
    const classes = Array.from({ length: 188 }, () => '');
    // 492090 이 거짓이면 다른 상태 getter/표 조회가 하나도 필요 없다.
    new RawObjectMenuInfo(scene.pool, types, classes, createObjectMenuInfoState(), { groundImmobile: () => false }).captureStatus(50);
    // 참이면 미션 비영일 때 Capturable! 이다.
    let header = '';
    const state = createObjectMenuInfoState({ missionPost: () => 1 });
    const hooks = { groundImmobile: () => true, translate: (value) => value, header: (value) => { header = value; } };
    new RawObjectMenuInfo(scene.pool, types, classes, state, hooks).captureStatus(50);
    assert.equal(header, '~yCapturable!~.\n');
    // 필요한 제목 소비자가 없으면 거부하고 풀/표 크기를 거부한다.
    assert.throws(() => new RawObjectMenuInfo(scene.pool, types, classes, state, hooks).describe(100, 2, 1, 50), /미연결/);
    assert.throws(() => new RawObjectMenuInfo(scene.pool, types.slice(1), classes, state, hooks), TypeError);
    assert.throws(() => new RawObjectMenuInfo(scene.pool, types, classes.slice(1), state, hooks), TypeError);
    // 정보 로마숫자·원소 이름표는 번역 없이 원문 그대로이며 범위 밖 원소는 거부한다.
    assert.equal(RawObjectMenuInfo.levelName(0), 'I');
    assert.equal(RawObjectMenuInfo.themeName(4), 'Energy');
    assert.throws(() => RawObjectMenuInfo.themeName(8), RangeError);
    assert.throws(() => RawObjectMenuInfo.themeName(-1), RangeError);
});

test('객체 메뉴 정보 계약: Level 플래그 마스크·signed 소유자·이름/서식/치환/클래스 버퍼 한도', () => {
    const base = '0,100,1,2,1,0,0,1,0,50,0,0,0,0,0,0,0,0,0,1';
    // describe 를 실행해 title/header 호출을 모으는 도우미이다(번역은 원문 그대로, 클래스는 모든 입력에서 비어 있지 않게 만든다).
    const run = (adjust, call = (info) => info.describe(100, 2, 1, 0xffffffff)) => {
        const scene = new InfoScene();
        scene.prepare(infoNumbers(base));
        scene.classes[100] = 'Class % value';
        const hooks = scene.hooks();
        adjust(scene, hooks);
        call(new RawObjectMenuInfo(scene.pool, scene.types, scene.classes, scene.state, hooks));
        return scene;
    };
    // Level 줄은 flags2 의 0x80074000 비트 중 하나가 있어야 한다(비트 0 만 있는 값은 만들지 않는다).
    assert.ok(!run((scene) => { scene.types[100].flags2 = 1; }).events.includes(infoHex(' Level %s')));
    assert.ok(run((scene) => { scene.types[100].flags2 = 0x4000; }).events.includes(infoHex(' Level %s')));
    // 소유자는 signed 로 양수일 때만 이름 조회하고 0x80000001 은 Nobody 로 처리한다.
    const high = run(() => {}, (info) => info.describe(100, 2, 0x80000001, 0xffffffff));
    assert.ok(high.events.includes(infoHex('Nobody')) && !high.events.includes('P:'), high.events);
    // 이름 80·서식 256·치환 1024·클래스 23 바이트 한도(경계값은 통과하고 한도 이상은 거부).
    const name = (length) => run((scene, hooks) => { hooks.ownerName = () => 'n'.repeat(length); });
    assert.doesNotThrow(() => name(79));
    assert.throws(() => name(80), RangeError);
    const printed = (length) => run((scene, hooks) => { hooks.print = (pattern, values) => (pattern === '\n%s: ~i%s~.' ? 'p'.repeat(length) : scene.print(pattern, values)); });
    assert.doesNotThrow(() => printed(255));
    assert.throws(() => printed(256), RangeError);
    const substituted = (length) => run((scene, hooks) => { hooks.substitute = () => 's'.repeat(length); hooks.print = (pattern, values) => (pattern.startsWith(String.fromCharCode(10) + 'Class') ? 'short' : scene.print(pattern, values)); });
    assert.doesNotThrow(() => substituted(1023));
    assert.throws(() => substituted(1024), RangeError);
    const klass = (length) => run((scene) => { scene.classes[100] = 'c'.repeat(length); });
    assert.doesNotThrow(() => klass(23));
    assert.throws(() => klass(24), RangeError);
    // 편집 SID 가 16비트를 넘으면 거부한다.
    const editing = run(() => {}, () => {});
    editing.editor = 1;
    const info = new RawObjectMenuInfo(editing.pool, editing.types, editing.classes, editing.state, editing.hooks());
    assert.throws(() => info.describe(100, 2, 1, 65536), RangeError);
});

test('객체 메뉴 정보 연결: 같은 controlled 메뉴의 제목/머리 작성기에 위임한다', () => {
    const calls = [];
    const controls = { addTitle: (value) => calls.push(['addTitle', value]), addHeader: (value) => calls.push(['addHeader', value]) };
    const hooks = {};
    bindObjectMenuInfoControls(hooks, controls);
    hooks.title('T');
    hooks.header('H');
    assert.deepEqual(calls, [['addTitle', 'T'], ['addHeader', 'H']]);
});
