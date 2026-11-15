// 게임 메뉴 명령 표 game-menu-commands.js(gameMenuCommandValue·gameMenuScriptCode·isGameMenuCommand·isEscapeKeyPress·queuePlayerListKey)를 원본 PE 이름표와 cpppj 시험 계약으로 대조한다(WB04-E538).
//  - html-solo-command-names-pe.tsv 43개(원본 PE 의 이름·전역 주소·명령): 주소 조회는 null 이거나 같은 명령이고, 이름(4가지 대소문자)은 같은 명령이거나 미등록(0xffffffff)이어야 한다.
//    (이 이름들을 묶는 UberGump DialogScriptCode 의 나머지 해석기 — Tell·Config·SetOwner 등 — 는 다른 단위 몫이라 이 표가 모르는 이름은 미등록이 맞다.)
//  - knowledge-world-names-x86.tsv 21행: 같은 이름 조회가 문자열 고리·40칸 큐를 거쳐 만드는 사건과 고리 위치.
//  - 계약: 특수 주소 4개·주소 표 값·입력 모듈 우선 순서·NUL 처리·허용표·ESC/F9 판정과 예약 순서.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E538)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { InputCode, InputQueue } from '../www/core/input-queue.js';
import { ScriptStringRing, queueScriptCommand } from '../www/ui/script-command.js';
import { knowledgeScriptCode } from '../www/core/raw-knowledge-menu.js';
import { CAPTURE_COMMAND, CREATE_HELP_LIST_COMMAND, RECHECK_CD_COMMAND, gameMenuCommandValue, gameMenuScriptCode, isEscapeKeyPress, isGameMenuCommand, queuePlayerListKey } from '../www/core/game-menu-commands.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E538)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const peRows = fixture('html-solo-command-names-pe.tsv');
const nameRows = fixture('knowledge-world-names-x86.tsv');
// 이름이 없을 때 원본이 돌려주는 값이다. (2026-11-15 WB04-E538)
const NOT_FOUND = 0xffffffff;
// 원본 PE 43개 중 이 표가 전역 주소로 직접 아는 수(23)와 이름으로 아는 수(29)이다. 나머지 이름 14개(아래 UNOWNED)는 UberGump DialogScriptCode 의 다른 해석기가 소유한다. (2026-11-15 WB04-E538)
const BY_ADDRESS = 23;
const BY_NAME = 29;
// 이 표가 모르는(UberGump 의 Tell·Config·MissionBegin·SetOwner 등 다른 해석기가 소유하는) 원본 이름 14개이다. (2026-11-15 WB04-E538)
const UNOWNED = ['Config', 'DoNothing', 'MissionAbort', 'MissionBegin', 'MissionRestart', 'MoveDialog', 'Salvage', 'SendAllBucks', 'SendBucks', 'SetOwner', 'StartDemo', 'Tell', 'ViewSpot', 'Warning'];

// UberGump DialogScriptCode 의 끝부분과 같은 순서이다: 기술 창 이름을 먼저 보고 없으면 게임 메뉴 표를 본다(나머지 해석기는 이 시험의 이름과 무관). (2026-11-15 WB04-E538)
const resolve = (name) => { const knowledge = knowledgeScriptCode(name); return knowledge !== NOT_FOUND ? knowledge : gameMenuScriptCode(name); };

test('게임 메뉴 표: 원본 PE 43개 이름표의 전역 주소와 이름이 같은 명령이거나 미등록이다(다른 값을 내지 않는다)', { skip: !peRows && '원본 fixture 없음' }, () => {
    const data = peRows.filter((row) => row[0] !== 'name');
    assert.equal(data.length, 43);
    let byAddress = 0;
    let byName = 0;
    const unowned = [];
    // 첫 원문의 주소/이름/코드를 기준으로 현재 조회 결과만 읽는다.
    for (const row of data) {
        assert.equal(row.length, 4);
        const expected = parseInt(row[3], 16) >>> 0;
        const address = gameMenuCommandValue(parseInt(row[2], 16));
        if (address !== null) {
            byAddress += 1;
            assert.equal(address, expected, `${row[0]} 주소 ${row[2]}`);
            assert.equal(isGameMenuCommand(expected), true, `${row[0]} 허용표`);
        }
        // 원본 ASCII 이름의 양쪽 대소문자와 교대 표기를 만든다.
        const variants = [row[0], row[0].toLowerCase(), row[0].toUpperCase(), Array.from(row[0], (ch, index) => (index & 1 ? ch.toLowerCase() : ch.toUpperCase())).join('')];
        let resolved = 0;
        for (const name of variants) {
            const code = resolve(name);
            if (code !== NOT_FOUND) { assert.equal(code, expected, `${name}`); resolved += 1; }
        }
        // 대소문자 변형은 모두 같은 결과여야 한다(네 가지가 모두 등록이거나 모두 미등록).
        assert.ok(resolved === 0 || resolved === 4, `${row[0]} 대소문자 일관성`);
        if (resolved === 4) byName += 1;
        else unowned.push(row[0]);
    }
    assert.deepEqual(unowned.sort(), UNOWNED);
    // 이 표가 직접 소유하는 이름과 주소의 수(원본 43개 중 나머지는 Tell/Config/SetOwner 등 UberGump 의 다른 해석기 소유).
    assert.equal(byAddress, BY_ADDRESS);
    assert.equal(byName, BY_NAME);
});

test('원문 이름→명령→큐 21관측: 같은 이름 조회가 문자열 고리와 40칸 큐로 만드는 사건·고리 위치가 같다', { skip: !nameRows && '원본 fixture 없음' }, () => {
    assert.equal(gameMenuCommandValue(0x00542b84), 0x02000132);
    assert.equal(isGameMenuCommand(0x02000132), true);
    assert.equal(gameMenuCommandValue(0x00542b88), 0x02000133);
    assert.equal(isGameMenuCommand(0x02000133), true);
    assert.equal(nameRows.length, 21);
    let count = 0;
    // 원문 코드와 인자 기대값은 동결 관측에서만 읽는다.
    for (const row of nameRows) {
        assert.equal(row.length, 9);
        const source = Buffer.from(row[1], 'hex').toString('latin1');
        const queue = new InputQueue();
        const strings = new ScriptStringRing();
        let unknown = 0;
        queueScriptCommand(source, queue, strings, { lookup: resolve, unknown: () => { unknown += 1; } });
        assert.equal(strings.index, Number(row[7]), source);
        if (row[6] === '-') {
            assert.ok(queue.empty && unknown > 0, source);
            count += 1;
            continue;
        }
        const expected = row[6].split(',');
        assert.equal(expected.length, 2);
        const event = queue.pop(false);
        assert.ok(event.code === Number(expected[0]) && event.x === event.y && queue.empty && unknown === 0, source);
        const token = event.y >>> 0;
        if (expected[1].startsWith('n')) assert.equal(token, Number(expected[1].slice(1)) >>> 0, source);
        else {
            assert.ok(expected[1].startsWith('s'), source);
            assert.equal(Buffer.from(strings.resolve(token), 'latin1').toString('hex'), expected[1].slice(1), source);
        }
        count += 1;
    }
    assert.equal(count, 21);
});

// cpppj 소스의 명령 표(kCommands 64쌍)와 이름 if-사슬(51개)을 소스 텍스트에서 직접 읽어 헤더의 기호 상수까지 풀어 대조한다(소스 교차 확인). (2026-11-15 WB04-E538)
const CPP_FILE = path.join(REPO_ROOT, 'cpppj', 'src', 'client', 'GameMenuCommands.cpp');
// 기호 상수를 정의하는 같은 폴더의 client 헤더들이다(등록된 소스 파일의 이웃이라 별도 항목 없이 읽는다).
const CPP_DIR = path.dirname(CPP_FILE);
test('게임 메뉴 표 소스 교차 확인: cpppj kCommands 64쌍(첫 쌍 규칙)과 이름 if-사슬 51개의 값이 모두 같다', { skip: !fs.existsSync(CPP_FILE) && 'cpppj 소스 없음' }, () => {
    const cpp = fs.readFileSync(CPP_FILE, 'utf8');
    // 모든 client 헤더의 `kXxxCommand=0x...U` 기호 상수를 모은다(한 줄에 여러 개 선언 포함).
    const symbols = new Map();
    for (const file of fs.readdirSync(CPP_DIR).filter((name) => name.endsWith('.h'))) {
        for (const match of fs.readFileSync(path.join(CPP_DIR, file), 'utf8').matchAll(/\b(k\w+Command)\s*=\s*(0x[0-9a-fA-F]+)U/g)) symbols.set(match[1], parseInt(match[2], 16) >>> 0);
    }
    const value = (token) => (token.startsWith('0x') ? parseInt(token, 16) >>> 0 : symbols.get(token));
    // 표 본문 64쌍이다. 같은 주소가 여럿이면 첫 쌍만 조회된다.
    const body = cpp.slice(cpp.indexOf('kCommands{{'), cpp.indexOf('}};'));
    const pairs = [...body.matchAll(/\{\s*(0x[0-9a-fA-F]+)U\s*,\s*(0x[0-9a-fA-F]+|k\w+)U?\s*\}/g)].map((match) => [parseInt(match[1], 16) >>> 0, value(match[2]), match[2]]);
    assert.equal(pairs.length, 64);
    const first = new Map();
    for (const [address, command, token] of pairs) {
        assert.ok(command !== undefined, `기호 상수 해석 실패 ${token}`);
        if (!first.has(address)) first.set(address, command);
        assert.equal(isGameMenuCommand(command), true, `${address.toString(16)} → ${command.toString(16)}`);
    }
    // 특수 주소 4개(함수 앞부분)는 먼저 처리되므로 표의 첫 쌍 규칙과 별개이다.
    const special = new Map([[0x00542a40, 0x020000d3], [0x0054279c, symbols.get('kChangePalCommand')], [0x0054291c, symbols.get('kChangePriorityCommand')], [0x00542a10, symbols.get('kSetFortTypeCommand')]]);
    for (const [address, command] of special) assert.equal(gameMenuCommandValue(address), command, address.toString(16));
    for (const [address, command] of first) if (!special.has(address)) assert.equal(gameMenuCommandValue(address), command, address.toString(16));
    // 이름 if-사슬(`key=="x") return V;`)이다. 앞선 입력 모듈이 가로채는 이름이 있어도 값은 같아야 한다(같은 순서의 조회이므로).
    const names = [...cpp.matchAll(/if \(key=="([a-z0-9]+)"\) return (0x[0-9a-fA-F]+|k\w+)U?;/g)];
    assert.equal(names.length, 51);
    for (const [, name, token] of names) {
        const expected = value(token);
        assert.ok(expected !== undefined, `기호 상수 해석 실패 ${token}`);
        assert.equal(gameMenuScriptCode(name), expected, name);
        // 키 표 이름은 대소문자 변형도 같은 값이다.
        assert.equal(gameMenuScriptCode(name.toUpperCase()), expected, name.toUpperCase());
    }
});

test('게임 메뉴 표 계약: 특수 주소 네 개·주소 표의 기호 상수·미등록 주소와 같은 값 첫 쌍', () => {
    // 개발자 NextStage 와 세 설정 명령(원문 전역 주소)이다.
    assert.equal(gameMenuCommandValue(0x00542a40), 0x020000d3);
    assert.equal(gameMenuCommandValue(0x0054279c), 0x02000029);
    assert.equal(gameMenuCommandValue(0x0054291c), 0x0200008a);
    assert.equal(gameMenuCommandValue(0x00542a10), 0x020000c7);
    // 헤더 상수에서 온 기호 항목(명령 ID 는 각 입력 모듈 헤더의 원문 초기 DWORD)이다.
    for (const [address, value] of [[0x005429a8, 0x020000ad], [0x005429f8, 0x020000c1], [0x005428d8, 0x02000078], [0x005429d0, 0x020000b7], [0x00542760, 0x0200001a],
        [0x00542a34, 0x020000d0], [0x00542a38, 0x020000d1], [0x00542964, 0x0200009c], [0x00542764, 0x0200001b], [0x00542a68, 0x020000dd], [0x00542988, 0x020000a5],
        [0x0054298c, 0x020000a6], [0x00542a54, 0x020000d8], [0x00542ae8, 0x020000fd], [0x00542968, 0x0200009d], [0x00542864, 0x0200005b], [0x00542a74, 0x020000e0],
        [0x00542a78, 0x020000e1], [0x00542954, 0x02000098], [0x00542a44, 0x020000d4], [0x00542a48, 0x020000d5], [0x00542ad8, 0x020000f9], [0x00542974, 0x020000a0],
        [0x0054287c, 0x02000061], [0x00542700, 0x02000001], [0x005429ec, 0x020000be], [0x00542b54, 0x02000123], [0x005427a4, 0x0200002b]]) {
        assert.equal(gameMenuCommandValue(address), value, address.toString(16));
        assert.equal(isGameMenuCommand(value), true, value.toString(16));
    }
    // exclude-host 단위의 명령 ID 도 표에 남아 있다.
    assert.equal(CREATE_HELP_LIST_COMMAND, 0x0200009c);
    assert.equal(CAPTURE_COMMAND, 0x0200009d);
    assert.equal(RECHECK_CD_COMMAND, 0x020000fd);
    // 표에 없는 주소는 null 이고 음수/큰 입력은 하위 32비트 주소로 읽는다.
    assert.equal(gameMenuCommandValue(0), null);
    assert.equal(gameMenuCommandValue(0x00542701), null);
    assert.equal(gameMenuCommandValue(-0xffabd900), gameMenuCommandValue(0x00542700));
});

test('게임 메뉴 이름 조회 계약: 입력 모듈 이름표가 먼저이고 이름 표는 대소문자만 무시하며 NUL 은 CreateHelpList 만 자른다', () => {
    // 입력 모듈 이름표(MakeAi·AiUtility·CreateAt·DrawWater·AiDebug·Presentation·Utility·Configuration·ScenarioEffect·ScenarioObject·Vortex)가 이름 표보다 먼저이다.
    for (const [name, code] of [['MakeAi', 0x020000b7], ['createat', 0x020000e0], ['DRAWWATER', 0x0200001a], ['ChangeFontFace', 0x0200001b], ['WipeOut', 0x0300006f], ['ViewMeta', 0x02000013]]) {
        assert.equal(gameMenuScriptCode(name), code, name);
    }
    // 이름 표(원본 PE 이름표에서 확인한 이름과 보호 PE 의 실제 이름)이다.
    for (const [name, code] of [['QuitApp', 0x02000001], ['RequestQuitApp', 0x02000094], ['MenuJustClosed', 0x020000be], ['ReloadTheams', 0x02000123], ['pause', 0x020000cf],
        ['GETFULLDECK', 0x020000cd], ['PlaySound', 0x0200009a], ['ChangeMoney', 0x02000061], ['TechHelp', 0x02000049], ['ScreenModeAbsChange', 0x02000030],
        ['ScreenModeChange', 0x02000002], ['AcceptScreenModeChange', 0x020000e4], ['JoinTestBattle', 0x0200002b], ['RequestMenu', 0x0200011c], ['RefreshPlayers', 0x0200011e],
        ['capture', CAPTURE_COMMAND], ['RecheckCD', RECHECK_CD_COMMAND], ['PlayIntro', 0x020000d8], ['URL', 0x0200005b], ['SetLanguage', 0x020000a5]]) {
        assert.equal(gameMenuScriptCode(name), code, name);
        assert.equal(isGameMenuCommand(code), true, name);
    }
    // 원본에 없는 오자·공백 이름은 별칭을 만들지 않는다. NUL 이 붙은 이름은 CreateHelpList 만 첫 NUL 앞으로 비교한다.
    for (const name of ['Movedialogbox', 'Show Technology', 'QuitApps', '', 'quitapp\0junk']) assert.equal(gameMenuScriptCode(name), NOT_FOUND, JSON.stringify(name));
    assert.equal(gameMenuScriptCode('CreateHelpList\0junk'), CREATE_HELP_LIST_COMMAND);
    assert.equal(gameMenuScriptCode('createhelplist'), CREATE_HELP_LIST_COMMAND);
});

test('게임 메뉴 허용표 계약: 설정/유틸리티/주소표 값과 세 기본 전역 명령만 true 이다', () => {
    for (const code of [0x02000029, 0x0200008a, 0x020000c7, 0x0200001c, 0x0200005a, 0x020000da, 0x02000006, 0x020000aa, 0x02000049, 0x02000002, 0x020000e4, 0x02000001, 0x020000f1]) {
        assert.equal(isGameMenuCommand(code), true, code.toString(16));
    }
    // 상위 입력 표시가 붙은 값/무관한 값/키의 하위 BYTE 는 허용하지 않는다.
    for (const code of [0, 0x1b, 0x82000001, 0x02000000, 0x020003fe, 0x02000003, 0x03000001]) assert.equal(isGameMenuCommand(code), false, code.toString(16));
});

test('ESC/F9 키 판정: ESC 는 전체 코드 비교(수정키 무시)이고 F9 는 Ctrl·뗌을 거부하며 Shift 는 Pause, 그 밖은 Player List 를 예약한다', () => {
    // 원문 전체 코드 판정에서 수정키는 비교 대상이 아니다.
    for (const code of [0x1b, InputCode.shift | 0x1b, InputCode.control | 0x1b, InputCode.alt | 0x1b]) assert.equal(isEscapeKeyPress({ code: code >>> 0, x: 37, y: -19 }), true);
    // 하위 16비트가 같아도 입력 종류나 뗌이 다르면 메뉴를 열지 않는다.
    for (const code of [InputCode.release | 0x1b, InputCode.command | 0x1b, InputCode.leftButton | 0x1b, 0x1b0000, 0x41]) assert.equal(isEscapeKeyPress({ code: code >>> 0, x: 0, y: 0 }), false);
    const queue = new InputQueue();
    queue.push(0x1234, 5, 6);
    assert.equal(queuePlayerListKey({ code: 0x780000, x: 99, y: 88 }, queue), true);
    assert.equal(queuePlayerListKey({ code: (InputCode.shift | 0x780000) >>> 0, x: 0, y: 0 }, queue), true);
    assert.equal(queuePlayerListKey({ code: (InputCode.alt | 0x780000) >>> 0, x: 0, y: 0 }, queue), true);
    assert.equal(queuePlayerListKey({ code: (InputCode.control | 0x780000) >>> 0, x: 0, y: 0 }, queue), false);
    assert.equal(queuePlayerListKey({ code: (InputCode.control | InputCode.shift | 0x780000) >>> 0, x: 0, y: 0 }, queue), false);
    assert.equal(queuePlayerListKey({ code: (InputCode.release | 0x780000) >>> 0, x: 0, y: 0 }, queue), false);
    assert.equal(queuePlayerListKey({ code: 0x770000, x: 0, y: 0 }, queue), false);
    const old = queue.pop(false);
    assert.ok(old.code === 0x1234 && old.x === 5 && old.y === 6);
    // 원본 예약 순서대로 기존 사건 다음에 목록/정지/목록이 오며 좌표를 인자로 재사용하지 않는다.
    for (const code of [0x020000f0, 0x020000cf, 0x020000f0]) {
        const event = queue.pop(false);
        assert.ok(event.code === code && event.x === 0 && event.y === 0, code.toString(16));
    }
    assert.equal(queue.empty, true);
});
