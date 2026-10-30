// 시작 창(환영 창·"Did You Know?" 팁 창)과 그 바탕 기능을 검사한다: 스크립트 명령 토큰(원본 x86 기대값 대조), Tell 이름의 지역 인자(원본 x86 기대값 1,070행),
// 팁 명령(원본 x86 기대값 388행), Tell 창 공장의 BLANK·param 수명·이전 이름 기록·조건 태그·인라인 명령, `{@tell.tipN}` 조회, 실제 Tips·Welcome 본문이다.
// 기대값 파일(cpppj/tests/fixtures/script-command-x86.tsv·tell-parameters-x86.tsv·tell-tip-x86.tsv)은 cpppj 가 보호된 원본 10.78 의 해당 함수를 격리 실행해 만든 것이며 읽기 전용으로만 쓴다.
// 행 수를 고정해 두어 cpppj 쪽 파일이 바뀌면 이 검사가 실패해 다시 확인하게 한다.
// (2026-10-30 WB03-J: 시작 창과 Tell 지역 인자 이식 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, readDataMap } from './helpers.mjs';
import { GameData, TELL_NAME_BYTES } from '../www/assets/game-data.js';
import { Config, configParseLong } from '../www/assets/config.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { readRawTextToken } from '../www/ui/html-text.js';
import { applyScriptConfig, parseScriptCommand } from '../www/ui/script-command.js';
import { TELL_SOURCE_BYTES, parseTellParameters } from '../www/ui/tell-parameters.js';
import { INPUT_RELEASE, TELL_TIP_COMMAND, dispatchTellTip } from '../www/ui/tell-tip.js';
import { createTellWindow } from '../www/ui/tell-window.js';

// cpppj 기대값 파일의 폴더이다. (2026-10-30 WB03-J)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');

// 메뉴 묶음 파일 전부를 읽어 자원·게임 데이터를 만든다(검사마다 공유). (2026-10-30 WB03-J)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const resources = buildMenuResources(files);

// 논리 화면 크기이다(4:3). (2026-10-30 WB03-J)
const SCREEN = Object.freeze({ width: 1024, height: 768 });

// 새 게임 데이터를 만든다. extra 는 추가·대체할 느슨한 파일(경로 → 바이트)이고 options 는 설정 값이다. (2026-10-30 WB03-J)
function newData({ extra = [], options = {} } = {}) {
    return GameData.build({ files: new Map([...files, ...extra]), language: 'english', options: { soundVolume: 3, musicVolume: 2, ...options } });
}

// 16진수 칸을 바이트 하나가 문자 하나인 문자열로 바꾼다. '-' 는 빈 문자열이다. (2026-10-30 WB03-J)
function unhex(field) {
    return field === '-' ? '' : Buffer.from(field, 'hex').toString('latin1');
}

// 바이트 문자열을 16진수로 바꾼다(빈 문자열은 빈 문자열). (2026-10-30 WB03-J)
function hex(text) {
    return Buffer.from(text, 'latin1').toString('hex');
}

// 기대값 파일의 데이터 줄(주석·머리글 제외)을 칸으로 나눠 읽는다. 파일이 없으면 null 이다. (2026-10-30 WB03-J)
function readRows(name) {
    const filePath = path.join(FIXTURES, name);
    if (!fs.existsSync(filePath)) return null;
    const lines = fs.readFileSync(filePath, 'utf8').split('\n').filter((line) => line.length > 0 && line[0] !== '#');
    // `#` 주석이 아닌 머리글 줄(script-command 파일의 `operation` 줄)은 건너뛴다.
    if (lines.length > 0 && lines[0].startsWith('operation\t')) lines.shift();
    return lines.map((line) => line.replace(/\r$/, '').split('\t'));
}

// 기대값 파일에서 구분자 칸이 `DEFAULT`(16진수 44454641554c54)이면 원본 토큰 helper 의 기본 구분자(space·tab·LF·쉼표)를 뜻한다. (2026-10-30 WB03-J)
const DEFAULT_DELIMITER_FIELD = '44454641554c54';
const DEFAULT_DELIMITERS = ' \t\n,';

// 스크립트 명령 기대값 128+18+31행: 토큰 helper(구분자·따옴표·78글자 제한·공백)와 Config 명령·인자 분류가 원본 기계어 결과와 같다. (2026-10-30 WB03-J)
test('스크립트 명령: 토큰·Config 키/값·명령 인자가 원본 x86 기대값과 같다', (t) => {
    const rows = readRows('script-command-x86.tsv');
    if (!rows) {
        t.skip('cpppj/tests/fixtures/script-command-x86.tsv 가 없는 환경');
        return;
    }
    const counts = { Token: 0, Config: 0, Command: 0 };
    // 기대값 행을 하나씩 해당 종류의 검사로 보낸다. 대조한 행만 종류별로 센다.
    for (const row of rows) {
        const [operation, sourceField, delimiterField, , valueField, nextField, queueField, , eventsField] = row;
        if (operation === 'Token') {
            // 구분자는 16진수 바이트열(예: 3d = '=')이며 `DEFAULT` 칸은 기본 구분자이다.
            const token = readRawTextToken(unhex(sourceField), 0, delimiterField === DEFAULT_DELIMITER_FIELD ? DEFAULT_DELIMITERS : unhex(delimiterField));
            assert.equal(hex(token.text), valueField === '-' ? '' : valueField, `토큰 ${sourceField}/${delimiterField}`);
            assert.equal(token.next, Number(nextField), `다음 위치 ${sourceField}/${delimiterField}`);
        } else if (operation === 'Config') {
            // 원본이 설정을 쓴 경우는 첫 사건이 `S,키,값` 이고, 키가 비면 쓰지 않는다(로그만).
            const written = [];
            const wrote = applyScriptConfig(unhex(sourceField), (key, value) => written.push([hex(key), hex(value)]));
            const first = eventsField.split(';')[0].split(',');
            if (first[0] === 'S') {
                assert.equal(wrote, true, `쓰기 ${sourceField}`);
                assert.deepEqual(written, [[first[1], first[2] ?? '']], `키/값 ${sourceField}`);
            } else {
                assert.equal(wrote, false, `키 없음 ${sourceField}`);
                assert.deepEqual(written, []);
            }
        } else if (operation === 'Command' && queueField !== '-') {
            // 큐에 들어간 항목은 `코드,s문자열(16진수)` 또는 `코드,n정수`이다. 인자가 숫자 글자로 시작하면 정수, 아니면 문자열이다.
            const parsed = parseScriptCommand(unhex(sourceField));
            const detail = queueField.split(',')[1];
            if (detail[0] === 'n') {
                assert.equal(parsed.number, Number(detail.slice(1)), `정수 인자 ${sourceField}`);
            } else {
                assert.equal(parsed.number, null, `문자열 인자 ${sourceField}`);
                assert.equal(hex(parsed.argument), detail.slice(1), `문자열 값 ${sourceField}`);
            }
        } else {
            continue;
        }
        counts[operation] += 1;
    }
    // 큐에 들어간 Command 행은 31행 중 명령 표에 없는 이름(MISSING) 7행을 뺀 24행이다.
    assert.deepEqual(counts, { Token: 128, Config: 18, Command: 24 });
});

// 사용자 시작 흐름이 실제로 쓰는 명령 문자열의 분리이다. (2026-10-30 WB03-J)
test('스크립트 명령: 본문 인라인 명령과 버튼 인자의 분리', () => {
    assert.deepEqual(parseScriptCommand('config,welcomeGiven=1>rest'), { name: 'config', argument: 'welcomeGiven=1', number: null });
    assert.deepEqual(parseScriptCommand('Config,DoneTutorial1=1>'), { name: 'Config', argument: 'DoneTutorial1=1', number: null });
    // 인자 안의 쉼표는 남고, 숫자로 시작하는 인자는 정수이다.
    assert.deepEqual(parseScriptCommand('Warning,"a, b">'), { name: 'Warning', argument: 'a, b', number: null });
    assert.equal(parseScriptCommand('TellTip,3>').number, 3);
    // 명령 이름의 구분자는 `#,;>`, 인자의 구분자는 `;>` 이다(원본 004c9090, cpppj ScriptCommand.cpp 와 docs/exe/cpp-script-command-20261015.md). 기대값 파일의 명령 행은 이 구분자를 쓰지 않아 직접 확인한다.
    assert.deepEqual(parseScriptCommand('Warning#x>'), { name: 'Warning', argument: 'x', number: null });
    assert.deepEqual(parseScriptCommand('Warning;x>'), { name: 'Warning', argument: 'x', number: null });
    assert.equal(parseScriptCommand('Warning,a,b>').argument, 'a,b');
    assert.equal(parseScriptCommand('Warning,a;b>').argument, 'a');
    // Config 값은 줄 끝까지이며 값 안의 '=' 와 ';' 는 자르지 않는다.
    const written = [];
    assert.equal(applyScriptConfig('tellTips=0', (key, value) => written.push([key, value])), true);
    assert.equal(applyScriptConfig('a=b=c;d', (key, value) => written.push([key, value])), true);
    assert.equal(applyScriptConfig('=x', (key, value) => written.push([key, value])), false);
    assert.deepEqual(written, [['tellTips', '0'], ['a', 'b=c;d']]);
});

// Tell 이름의 지역 인자: 원본 004c8ea0 을 격리 실행한 1,070관측과 같은 이름·호출 순서·param 줄이다. (2026-10-30 WB03-J)
test('Tell 지역 인자: 이름·치환·param 줄이 원본 x86 기대값 1,070행과 같다', (t) => {
    const rows = readRows('tell-parameters-x86.tsv');
    if (!rows) {
        t.skip('cpppj/tests/fixtures/tell-parameters-x86.tsv 가 없는 환경');
        return;
    }
    assert.equal(rows.length, 1070);
    // 행마다 새 치환 프로필·param 줄 저장소로 독립 실행해 반환 이름과 호출 순서를 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const [, sourceHex, profileField, expected] = row;
        const profile = Number(profileField);
        const values = new Map();
        const trace = [];
        // 기대값 생성 도구와 같은 치환 프로필이다(0: 그대로, 1: 대괄호, 2: 앞 인자 노출, 3: 따옴표·개행 포함, 4: NUL 포함).
        const substitute = (text) => {
            trace.push(`S,${hex(text)}`);
            if (profile === 1) return `[${text}]`;
            if (profile === 2) return text === '{param.1}' ? values.get(1) ?? '' : text;
            if (profile === 3) return text === 'EMPTY' ? '' : `Q";=\n${text}`;
            if (profile === 4) return 'N\0TAIL';
            return text;
        };
        // param 객체에 줄을 덧붙이는 장치이다. 번호와 값을 저장해 뒤 인자의 치환(프로필 2)이 앞 인자를 보게 한다.
        const append = (line) => {
            trace.push(`A,${hex(line)}`);
            const equal = line.indexOf('=');
            values.set(Number(line.slice(0, equal)), line.slice(equal + 2, line.length - 1));
        };
        const name = parseTellParameters(unhex(sourceHex), { substitute, append });
        assert.equal(`${hex(name)}|${trace.join(';')}`, expected, `입력 ${row[0]}`);
    }
});

// 원본 지역 입력 범위(NUL 포함 1024바이트)를 넘으면 쓰기 전에 거부하고, 첫 NUL 뒤는 입력이 아니며, 필요 없는 훅은 요구하지 않는다. (2026-10-30 WB03-J)
test('Tell 지역 인자: 범위·NUL·훅 요구 경계', () => {
    assert.equal(parseTellParameters('About'), 'About');
    assert.equal(parseTellParameters('Foo("",next)'), 'Foo');
    assert.throws(() => parseTellParameters('Foo(value)'), /연결/);
    assert.throws(() => parseTellParameters('A'.repeat(TELL_SOURCE_BYTES)), RangeError);
    assert.equal(parseTellParameters(`About\0${'X'.repeat(TELL_SOURCE_BYTES)}`), 'About');
    // 입력은 토큰마다 78글자까지만 읽으므로 구분자 없는 긴 이름은 이름(78글자)과 이어지는 인자들로 나뉜다. 원본 동작이며 고치지 않는다(1023글자 = 78 + 78×12 + 9).
    const appended = [];
    const longName = parseTellParameters('A'.repeat(TELL_SOURCE_BYTES - 1), { substitute: (text) => text, append: (line) => appended.push(line) });
    assert.equal(longName.length, 78);
    assert.equal(appended.length, 13);
});

// 팁 명령 원본 388관측: 번호 증감·부호 있는 32비트 하한·서식·설정·Tips 재요청 순서가 같다. (2026-10-30 WB03-J)
test('팁 명령: 번호·호출 순서가 원본 x86 기대값 388행과 같다', (t) => {
    const rows = readRows('tell-tip-x86.tsv');
    if (!rows) {
        t.skip('cpppj/tests/fixtures/tell-tip-x86.tsv 가 없는 환경');
        return;
    }
    assert.equal(rows.length, 388);
    let dispatched = 0;
    // 행마다 새 호출 기록으로 명령을 분배하고 갱신된 번호와 호출 순서를 비교한다.
    for (const row of rows) {
        assert.ok(row.length === 8 || row.length === 10);
        const event = { code: Number(row[0]), y: Number(row[2]) | 0 };
        const number = Number(row[3]);
        const trace = [];
        const hooks = {
            format: (value) => {
                trace.push(`format:${value}`);
                return String(value);
            },
            setConfig: (key, value) => trace.push(`set:${key}:${value}`),
            tell: (section) => trace.push(`tell:${section}`),
        };
        // 기존 안내/Gump 소비 단계가 이미 처리한 입력(열 9·10 이 0 이 아님)은 다시 분배하지 않는다.
        const consumedEarlier = row.length === 10 && (Number(row[8]) !== 0 || Number(row[9]) !== 0);
        const result = consumedEarlier ? { handled: false, number } : dispatchTellTip(event, number, hooks);
        assert.equal(result.number, Number(row[4]), `번호 ${row[0]}/${row[2]}/${row[3]}`);
        // 원본 trace 의 `number:N;` 은 증감 직후 번호를 알리는 관찰 항목이며 N 은 갱신된 번호와 같다. 나머지 호출 순서가 같아야 한다.
        const expected = row[5] === '-' ? '' : row[5];
        const numberEntry = /^number:(\d+);/.exec(expected);
        if (numberEntry) assert.equal(Number(numberEntry[1]), result.number);
        assert.equal(trace.join(';'), expected.replace(/^number:\d+;/, ''), `호출 순서 ${row[0]}/${row[2]}/${row[3]}`);
        if (!consumedEarlier) dispatched += 1;
    }
    assert.ok(dispatched > 380);
});

// 팁 명령의 처리 대상과 뗌·필요 장치 경계이다. (2026-10-30 WB03-J)
test('팁 명령: OK 는 저장만, 뗌·다른 명령은 소비하지 않음, 필요한 장치가 없으면 오류', () => {
    const calls = [];
    const hooks = { format: String, setConfig: (key, value) => calls.push(`${key}=${value}`), tell: (section) => calls.push(`tell ${section}`) };
    assert.deepEqual(dispatchTellTip({ code: TELL_TIP_COMMAND, y: 3 }, 1, hooks), { handled: true, number: 2 });
    assert.deepEqual(calls, ['tipNumber=2']);
    assert.deepEqual(dispatchTellTip({ code: TELL_TIP_COMMAND, y: 1 }, 0, hooks), { handled: true, number: 0 });
    assert.deepEqual(dispatchTellTip({ code: TELL_TIP_COMMAND | INPUT_RELEASE, y: 2 }, 5, {}), { handled: true, number: 5 });
    assert.deepEqual(dispatchTellTip({ code: 0x020000ec, y: 2 }, 5, {}), { handled: false, number: 5 });
    assert.throws(() => dispatchTellTip({ code: TELL_TIP_COMMAND, y: 2 }, 5, {}), /장치/);
});

// Tell 창 공장: BLANK 조기 분기, param 객체의 수명과 값, 이전 이름 기록, 인라인 명령 수집. 느슨한 `d/tell.english` 로 시험용 섹션을 덮어쓴다. (2026-10-30 WB03-J)
test('Tell 창 공장: BLANK·지역 인자·param 수명·이전 이름·인라인 명령', () => {
    const script = [
        '[ParamTest]',
        '<$config,seen=1>',
        '<h2>{H2}Args</h2>',
        '<p>{Text}',
        'First={param.1} Second={param.2} Third={param.3}',
        '$Button=Go,Tell,Next({param.3})',
        '',
        '[Empty]',
        ' ',
        '',
    ].join('\r\n');
    const data = newData({ extra: [['d/tell.english', new TextEncoder().encode(script)]] });
    // 바깥의 같은 이름 param 객체가 있으면 본문 처리 동안만 가려진다.
    const outer = new Config(data.registry, 'param');
    outer.append('1="outer"');
    const before = data.registry.objects.length;
    const result = createTellWindow({ gameData: data, resources, name: 'ParamTest(A, "b c" ,{currentLanguage})', screen: SCREEN });
    assert.ok(result.window);
    assert.equal(data.registry.objects.length, before, 'param 객체는 본문 처리 뒤 내려간다');
    assert.equal(data.substitute('{param.1}'), 'outer');
    // 본문의 {param.N} 이 인자로 풀리고 버튼 명령에도 들어간다(버튼의 명령 인자는 공백에서 끝나므로 공백 없는 세 번째 인자를 쓴다). 세 번째 인자는 설정 치환을 거친다.
    assert.match(result.window.content.styleText, /First=A Second=b c Third=english/);
    assert.deepEqual(result.window.content.directives.buttons.map((button) => [button.label, button.command.name, button.command.argument]), [['Go', 'Tell', 'Next(english)']]);
    assert.deepEqual(result.inlineCommands, [{ name: 'config', argument: 'seen=1', number: null }]);
    // BLANK 은 대소문자를 가리지 않고 창도 이름 기록도 만들지 않는다.
    const current = data.currentTell;
    const blank = createTellWindow({ gameData: data, resources, name: 'bLaNk', screen: SCREEN });
    assert.deepEqual([blank.blank, blank.window, blank.missing], [true, null, false]);
    assert.equal(data.currentTell, current);
    // 이름이 바뀔 때마다 이전 이름이 global.priorTell 에 기록된다(대소문자만 다르면 바뀐 것이 아니다). 조회에 실패한 이름도 현재 이름이 된다.
    assert.equal(data.currentTell, 'ParamTest');
    createTellWindow({ gameData: data, resources, name: 'Credits', screen: SCREEN });
    assert.equal(data.substitute('{global.priorTell}'), 'ParamTest');
    createTellWindow({ gameData: data, resources, name: 'CREDITS', screen: SCREEN });
    assert.equal(data.substitute('{global.priorTell}'), 'ParamTest');
    const missing = createTellWindow({ gameData: data, resources, name: 'NoSuchSection(1)', screen: SCREEN });
    assert.equal(missing.missing, true);
    assert.equal(data.currentTell, 'NoSuchSection');
    assert.equal(data.substitute('{global.priorTell}'), 'CREDITS');
    // 빈 섹션은 창을 만들지 않고 empty 로 알린다. 원본 지역 입력 범위를 넘는 이름은 없는 섹션처럼 거부한다.
    assert.equal(createTellWindow({ gameData: data, resources, name: 'Empty', screen: SCREEN }).empty, true);
    assert.equal(createTellWindow({ gameData: data, resources, name: 'A'.repeat(TELL_SOURCE_BYTES), screen: SCREEN }).missing, true);
    assert.throws(() => data.findTellSection('A'.repeat(TELL_NAME_BYTES)), RangeError);
});

// `{@미션.키}` 조회: tell 스크립트의 팁 문구를 읽는다. 없는 번호는 기본 문구가 된다. (2026-10-30 WB03-J)
test('팁 문구 조회: {@tell.tipN} 이 tell 스크립트의 키를 읽는다', () => {
    const data = newData();
    assert.match(data.substitute('{@tell.tip3}'), /^If at first you don't succeed/);
    assert.match(data.substitute('{@tell.tip1}'), /^You can use ~i?F2|F2/);
    assert.equal(data.substitute("{@tell.tip999|You're out of Tips!}"), "You're out of Tips!");
    data.set('tipNumber', 2);
    assert.match(data.substitute('{@tell.tip{tipNumber}}'), /right-click/);
});

// 실제 Tips 본문: 번호가 0 이면 Prior Tip 이 없고, 0 이 아니면 있으며, 범위를 벗어나면 기본 문구이다. OK 는 TellTip,3 이다. (2026-10-30 WB03-J)
test('Tips 창: 번호별 문구와 Prior Tip 단추 조건', () => {
    // 게임 데이터로 Tips 창을 만들어 단추를 [라벨, 명령 이름, 인자] 목록으로 돌려주는 보조 함수이다.
    const labels = (data) => createTellWindow({ gameData: data, resources, name: 'Tips', screen: SCREEN }).window.content.directives.buttons.map((button) => [button.label, button.command.name, button.command.argument]);
    const data = newData();
    // 새로 설치한 상태(setup.cfg tipNumber=0): 첫 팁이고 Prior Tip 이 없다.
    assert.equal(data.get('tipNumber'), '0');
    assert.deepEqual(labels(data), [['Next Tip', 'TellTip', '2'], ['No More Tips', 'Config', 'tellTips=0'], ['OK', 'TellTip', '3']]);
    assert.match(createTellWindow({ gameData: data, resources, name: 'Tips', screen: SCREEN }).window.content.styleText, /Did You Know\?/);
    data.set('tipNumber', 3);
    assert.deepEqual(labels(data).map((button) => button[0]), ['Prior Tip', 'Next Tip', 'No More Tips', 'OK']);
    assert.match(createTellWindow({ gameData: data, resources, name: 'Tips', screen: SCREEN }).window.content.styleText, /If at first you don't succeed/);
    data.set('tipNumber', 4000);
    assert.match(createTellWindow({ gameData: data, resources, name: 'Tips', screen: SCREEN }).window.content.styleText, /You're out of Tips!/);
    // 음수·한계 값도 본문은 만들어진다(조건은 0 이 아니면 참).
    data.set('tipNumber', -1);
    assert.equal(labels(data).length, 4);
});

// 첫 실행 환영 창: Welcome 의 OK 는 Welcome2 를 열고, Welcome2 는 열리자마자 welcomeGiven=1 을 쓰는 인라인 명령을 가진다. (2026-10-30 WB03-J)
test('환영 창: Welcome → Welcome2 의 인라인 Config 와 버튼', () => {
    const data = newData();
    assert.equal(configParseLong(data.get('WelcomeGiven')), 0);
    const first = createTellWindow({ gameData: data, resources, name: 'Welcome', screen: SCREEN });
    assert.deepEqual(first.inlineCommands, []);
    assert.deepEqual(first.window.content.directives.buttons.map((button) => [button.label, button.command.argument]), [['OK', 'Welcome2']]);
    const second = createTellWindow({ gameData: data, resources, name: 'Welcome2', screen: SCREEN });
    assert.deepEqual(second.inlineCommands, [{ name: 'config', argument: 'welcomeGiven=1', number: null }]);
    assert.deepEqual(second.window.content.directives.buttons.map((button) => [button.label, button.command.name, button.command.argument]), [['Play Early Missions', 'MissionBegin', 'Tutorial1'], ['Main Menu', 'Tell', 'Blank']]);
    // 명령을 실행하면 게임 데이터의 값이 바뀌고 쓰기 통지가 나간다(실제 앱은 이를 설정 저장소에 옮긴다).
    const written = [];
    const watched = GameData.build({ files: new Map(files), language: 'english', options: {}, onChange: (key, value) => written.push([key, value]) });
    applyScriptConfig(second.inlineCommands[0].argument, (key, value) => watched.set(key, value));
    assert.equal(configParseLong(watched.get('WelcomeGiven')), 1);
    assert.deepEqual(written, [['welcomeGiven', '1']]);
});
