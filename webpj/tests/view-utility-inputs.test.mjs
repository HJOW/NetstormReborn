// 일반 입력 분배 네 개(표시 키 F2/F3/F7/F8·일반 유틸리티 명령·글꼴/음질 명령·시나리오 표시 명령)를 원본 10.78 관측으로 대조한다(WB04-E25, cpppj ViewKeyInputTests·UtilityCommandInputTests·PresentationCommandInputTests·ScenarioEffectInputTests 이식).
//  - view-key-input-x86.tsv 310행: 수정키·뗌·정지·테마 설정 조회·Fort/메타맵/편집기 분기와 F3/F8 큐 인자를 실제 40칸 FIFO 위에서 대조한다.
//  - utility-commands-x86.tsv 355행(MP 항해 명령 72행은 cpppj 와 같이 제외 → 283행), utility-command-names-x86.tsv 48행·presentation-command-names-x86.tsv 21행(이름→명령 번호 표만), sound-quality-command-x86.tsv 32행,
//    font-face-rebuild-x86.tsv 중 일반 입력 부모 15행, scenario-effect-input-x86.tsv 576행, dump-squids-input-x86.tsv 48행.
// 대본 문자열 고리·큐 예약(QueueScriptCommand)·실제 FontStore/ClientAudio/카메라/스크린샷 저장은 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E25: 입력 분배 네 개 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, readRows } from './raw-support.mjs';
import { InputCode, InputQueue } from '../www/core/input-queue.js';
import { dispatchViewKeyInput } from '../www/core/view-key-input.js';
import {
    dispatchUtilityCommandInput, isUtilityCommand, utilityCommandScriptCode,
    TAKE_SCREENSHOT_COMMAND, URL_ALIAS_COMMAND, RUN_COMMAND, MOVE_ISLAND_COMMAND, LOOK_AT_SID_COMMAND,
} from '../www/core/utility-command-input.js';
import {
    dispatchPresentationCommandInput, isPresentationCommand, presentationCommandScriptCode,
    CHANGE_FONT_FACE_COMMAND, CHANGE_SOUND_QUALITY_COMMAND,
} from '../www/core/presentation-command-input.js';
import {
    dispatchScenarioEffectInput, isScenarioEffectCommand, scenarioEffectScriptCode,
    SMOKE_COMMAND, LIGHTNING_COMMAND, RANDOM_GUNS_COMMAND, DUMP_SQUIDS_COMMAND,
} from '../www/core/scenario-effect-input.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E25)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const viewRows = fixture('view-key-input-x86.tsv');
const utilityRows = fixture('utility-commands-x86.tsv');
const utilityNameRows = fixture('utility-command-names-x86.tsv');
const presentationNameRows = fixture('presentation-command-names-x86.tsv');
const soundRows = fixture('sound-quality-command-x86.tsv');
const fontRows = fixture('font-face-rebuild-x86.tsv');
const effectRows = fixture('scenario-effect-input-x86.tsv');
const dumpRows = fixture('dump-squids-input-x86.tsv');

// 원문 DWORD 를 읽고(감김), signed 로 되돌리거나 비트 열로 돌려준다. (2026-11-08 WB04-E25)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const signed = (value) => value | 0;
const bits = (value) => value >>> 0;
// 원문 바이트 문자열(글자 하나당 바이트 하나)과 16진 문자열 사이 변환이다. '-' 는 빈 문자열이다. (2026-11-08 WB04-E25)
const hexToText = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));
const textToHex = (text) => Buffer.from(text, 'latin1').toString('hex');
// 도달한 하위 실패를 잡는다. (2026-11-08 WB04-E25)
const failed = (action) => { try { action(); } catch { return true; } return false; };

// 독립 PE 가 결정한 F2/F3/F7/F8 의 분기·장치 순서·큐 인자이다.
test('표시 키: 원본10.78 310행(수정키·뗌·정지·테마 조회·Fort/메타맵/편집기·F3/F8 큐)', { skip: !viewRows && '원본 fixture 없음' }, () => {
    const rows = viewRows.filter((row) => row[0] !== 'name');
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 23);
        count += 1;
        const queue = new InputQueue();
        const globals = { hideBuildings: number(row[9]), redraw: number(row[10]) };
        const trace = [];
        const read = number(row[7]);
        const filled = number(row[8]);
        // 공개 API 만으로 빈 큐의 두 커서를 전진한 뒤 현재 활성 슬롯 표식을 채운다.
        for (let index = 0; index < read; index += 1) { queue.push(1, 0, 0); queue.pop(false); }
        for (let index = 0; index < filled; index += 1) {
            const physical = (read + index) % 40;
            queue.push(0x02000300 + physical, 100 + physical, 200 + physical);
        }
        const hooks = {
            paused: () => { trace.push(`paused:${row[4]}`); return signed(number(row[4])) > 0; },
            readInt: (key, holder) => { trace.push(`readInt:${key}:${bits(holder.value)}`); if (number(row[11])) holder.value = signed(number(row[12])); return number(row[11]) !== 0; },
            tell: (section) => trace.push(`tell:${section}`),
            queue: (command, first, second) => { trace.push(`queue:${command}:${bits(first)}:${bits(second)}`); queue.pushCommand(command, first, second); },
        };
        for (const text of row[1].split(';')) {
            const fields = text.split(':');
            assert.equal(fields.length, 3);
            // 선행 Guide/Gump/Console 소비는 원문 별도 관문이며 이 함수에 전달하지 않는다.
            if (number(row[2]) || number(row[3]) || number(row[5])) continue;
            dispatchViewKeyInput({ code: number(fields[0]), x: signed(number(fields[1])), y: signed(number(fields[2])) }, globals, hooks);
        }
        const active = [];
        while (!queue.empty) { const event = queue.pop(false); active.push(`${event.code}:${bits(event.x)}:${bits(event.y)}`); }
        const label = `행 ${count} ${row[0]}`;
        // 외부 Guide/Gump/공통 tail 은 이 부분 본체의 계약과 구별해 읽은 장치 호출만 비교한다.
        const expectedTrace = row[18].split(';').filter((item) => /^(readInt:|tell:|paused:|queue:)/.test(item)).join(';');
        assert.equal(globals.hideBuildings, number(row[21]), `${label} 건물 숨김`);
        assert.equal(globals.redraw, number(row[22]), `${label} 다시 그리기`);
        assert.equal(trace.join(';'), expectedTrace, `${label} 장치 순서`);
        assert.equal(active.length === 0 ? '-' : active.join(';'), row[16], `${label} 큐`);
        assert.ok(row[13] === '1' && number(row[20]) > 0, `${label} 원문 종료`);
    }
    assert.equal(count, 310);
});

// 미선택 장치는 요구하지 않고 Tell 뒤에 현재 정지를 읽는다.
test('표시 키: 장치 지연 요구와 Tell 뒤 정지 조회', () => {
    const globals = { hideBuildings: 0, redraw: 9 };
    assert.equal(dispatchViewKeyInput({ code: 0x80710000, x: 0, y: 0 }, globals, {}), false);
    assert.equal(dispatchViewKeyInput({ code: 0x51, x: 0, y: 0 }, globals, {}), false);
    let order = '';
    let paused = false;
    const hooks = { paused: () => { order += 'P'; return paused; } };
    assert.equal(dispatchViewKeyInput({ code: 0x710000, x: 0, y: 0 }, globals, hooks), true);
    assert.deepEqual([globals.hideBuildings, globals.redraw, order], [1, 1, 'P']);
    order = '';
    hooks.readInt = () => { order += 'R'; return false; };
    hooks.tell = (name) => { assert.equal(name, 'SetIslands(1)'); order += 'T'; paused = true; };
    assert.equal(dispatchViewKeyInput({ code: 0x20720000, x: 0, y: 0 }, globals, hooks), true);
    assert.deepEqual([order, paused], ['RTP', true]);
    // Ctrl+Shift+F7 은 별도 진단 범위여서 소비하지 않고 장치도 요구하지 않는다.
    assert.equal(dispatchViewKeyInput({ code: 0x30760000, x: 0, y: 0 }, globals, {}), false);
    // 선택된 장치의 누락은 거부한다.
    assert.throws(() => dispatchViewKeyInput({ code: 0x770000, x: 0, y: 0 }, globals, {}), /미연결/);
});

// 명령 이름표와 번호는 서로 겹치지 않는다.
test('이름표: 네 입력의 명령 번호와 ASCII 대소문자 무시 조회', () => {
    const names = [
        ['TakeScreenShot', utilityCommandScriptCode, TAKE_SCREENSHOT_COMMAND],
        ['URLAlias', utilityCommandScriptCode, URL_ALIAS_COMMAND],
        ['rUn', utilityCommandScriptCode, RUN_COMMAND],
        ['MOVEISLAND', utilityCommandScriptCode, MOVE_ISLAND_COMMAND],
        ['lookatsid', utilityCommandScriptCode, LOOK_AT_SID_COMMAND],
        ['ChangeFontFace', presentationCommandScriptCode, CHANGE_FONT_FACE_COMMAND],
        ['changesoundquality', presentationCommandScriptCode, CHANGE_SOUND_QUALITY_COMMAND],
        ['StartSmoke', scenarioEffectScriptCode, SMOKE_COMMAND],
        ['Lightning', scenarioEffectScriptCode, LIGHTNING_COMMAND],
        ['cReAtErAnDoMgUnS', scenarioEffectScriptCode, RANDOM_GUNS_COMMAND],
        ['dUmPsQuIdS', scenarioEffectScriptCode, DUMP_SQUIDS_COMMAND],
    ];
    const seen = new Set();
    for (const [name, lookup, expected] of names) {
        assert.equal(lookup(name), expected, name);
        assert.ok(!seen.has(expected), `${name} 번호 중복`);
        seen.add(expected);
    }
    assert.equal(utilityCommandScriptCode('Screenshot'), 0xffffffff);
    assert.equal(presentationCommandScriptCode('ChangeFont'), 0xffffffff);
    assert.equal(scenarioEffectScriptCode('Smoke'), 0xffffffff);
    assert.equal(scenarioEffectScriptCode('CreateRandomGun'), 0xffffffff);
    assert.equal(scenarioEffectScriptCode('DumpSquid'), 0xffffffff);
    // 하위 BYTE 만 같은 문자/마우스는 명령으로 취급하지 않는다.
    assert.ok(isUtilityCommand(RUN_COMMAND | InputCode.shift));
    assert.ok(!isUtilityCommand('D'.charCodeAt(0)));
    assert.ok(!isPresentationCommand(0xdd));
    assert.ok(isScenarioEffectCommand(DUMP_SQUIDS_COMMAND | InputCode.outside));
});

// 동결 이름표 행에서 이름→명령 번호 대응만 읽는다(인자·문자열 고리·큐 예약은 대본 큐 단위에서).
test('이름표: 원본10.78 utility 48행·presentation 21행의 이름→번호', { skip: (!utilityNameRows || !presentationNameRows) && '원본 fixture 없음' }, () => {
    const check = (rows, lookup, expectedCount) => {
        let count = 0;
        for (const row of rows) {
            assert.equal(row.length, 9);
            count += 1;
            const name = hexToText(row[1]).split(',')[0];
            const expected = row[6] === '-' ? 0xffffffff : number(row[6].split(',')[0]);
            assert.equal(lookup(name), expected, `${name} 행 ${count}`);
        }
        assert.equal(count, expectedCount);
    };
    check(utilityNameRows, utilityCommandScriptCode, 48);
    check(presentationNameRows, presentationCommandScriptCode, 21);
});

// 독립 PE 가 결정한 화면 저장/URL 별칭/Run/강조/보기의 호출 순서와 인자이다. MP 항해 명령 72행은 cpppj 와 같이 제외한다.
test('일반 유틸리티: 원본10.78 355행 중 비멀티 283행', { skip: !utilityRows && '원본 fixture 없음' }, () => {
    let count = 0;
    let skipped = 0;
    for (const row of utilityRows) {
        if (row[0] === 'event') continue;
        assert.equal(row.length, 15);
        count += 1;
        if (((number(row[0]) & 0x03ffffff) >>> 0) === MOVE_ISLAND_COMMAND) { skipped += 1; continue; }
        const trace = [];
        const source = hexToText(row[7]);
        const hooks = {
            text: () => source,
            screenshot: () => trace.push('save'),
            tell: (text) => trace.push(`tell:${text}`),
            readString: (key, holder) => { trace.push(`read:${textToHex(key)}:${row[8]}`); if (number(row[8])) holder.value = hexToText(row[9]); return number(row[8]) !== 0; },
            url: (text) => trace.push(`url:${textToHex(text)}`),
            run: (text, mode) => trace.push(`run:${textToHex(text)}:${mode}`),
            highlight: (mode) => trace.push(`highlight:${mode}`),
            center: (sid) => trace.push(`center:${sid}`),
        };
        // 선행 Guide/Gump 소비만 분배 전에 적용한다. 현재 Console/정지는 이 명령의 추가 거부 조건이 아니다.
        if (!number(row[5]) && !number(row[6])) {
            assert.equal(dispatchUtilityCommandInput({ code: number(row[0]), x: signed(number(row[1])), y: signed(number(row[2])) }, hooks), true, `행 ${count}`);
        }
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[11], `행 ${count} ${row[0]}`);
        assert.ok(row[12] === '1' && row[13] === '0', `행 ${count} 종료`);
    }
    assert.deepEqual([count, skipped], [355, 72]);
});

// 키는 설정 조회 전에 완성되고 조회 실패는 링크 정책을 실행하지 않는다.
test('일반 유틸리티: URL 별칭 실패·소유 문자열·Run 모드·길이 거부', () => {
    assert.equal(dispatchUtilityCommandInput({ code: 'D'.charCodeAt(0), x: 0, y: 0 }, {}), false);
    assert.equal(dispatchUtilityCommandInput({ code: URL_ALIAS_COMMAND | InputCode.release, x: 0, y: 0 }, {}), true);
    let scratch = 'Help';
    let urls = 0;
    let runs = 0;
    let seen = '';
    const hooks = {
        text: (token) => { assert.equal(token, 7); return scratch; },
        readString: (key) => { assert.equal(key, 'Help URL'); scratch = 'X'.repeat(300); return false; },
    };
    assert.equal(dispatchUtilityCommandInput({ code: URL_ALIAS_COMMAND, x: 0, y: 7 }, hooks), true);
    assert.equal(urls, 0);
    // 설정값의 첫 NUL 뒤는 버리고, 앞선 키를 만든 뒤 바뀐 공유 문자열에 영향받지 않는다.
    scratch = 'Before\0After';
    hooks.readString = (key, holder) => { assert.equal(key, 'Before URL'); scratch = 'Changed'; holder.value = 'help:temple\0tail'; return true; };
    hooks.url = (text) => { urls += 1; seen = text; scratch = 'Y'.repeat(400); };
    assert.equal(dispatchUtilityCommandInput({ code: URL_ALIAS_COMMAND, x: 0, y: 7 }, hooks), true);
    assert.deepEqual([urls, seen], [1, 'help:temple']);
    scratch = 'Init(3)';
    hooks.run = (text, mode) => { runs += 1; scratch = 'Z'.repeat(400); assert.equal(text, 'Init(3)'); assert.equal(mode, 1); };
    assert.equal(dispatchUtilityCommandInput({ code: RUN_COMMAND, x: 0, y: 7 }, hooks), true);
    assert.equal(runs, 1);
    // 키가 256바이트 이상이면 원문은 버퍼 밖에 쓰므로 명시 거부하며 링크 정책은 실행하지 않는다.
    scratch = 'A'.repeat(252);
    assert.throws(() => dispatchUtilityCommandInput({ code: URL_ALIAS_COMMAND, x: 0, y: 7 }, hooks), RangeError);
    assert.equal(urls, 1);
    // 설정값도 255바이트까지만 허용한다.
    scratch = 'k';
    hooks.readString = (key, holder) => { holder.value = 'v'.repeat(256); return true; };
    assert.throws(() => dispatchUtilityCommandInput({ code: URL_ALIAS_COMMAND, x: 0, y: 7 }, hooks), RangeError);
    assert.equal(urls, 1);
});

// 화면 저장 부모가 실패하면 Tell 을 실행하지 않고 다음 정상 호출은 둘 다 실행한다.
test('일반 유틸리티: 화면 저장 실패 순서와 보기 대상 전체 DWORD', () => {
    let fail = true;
    const trace = [];
    const hooks = {
        screenshot: () => { trace.push('S'); if (fail) throw new Error('명시 저장 실패'); },
        tell: (text) => { assert.equal(text, 'TellGifWritten'); trace.push('T'); },
        center: (sid) => trace.push(`C${sid}`),
    };
    assert.ok(failed(() => dispatchUtilityCommandInput({ code: TAKE_SCREENSHOT_COMMAND, x: 0, y: 0 }, hooks)));
    assert.equal(trace.join(''), 'S');
    fail = false;
    assert.equal(dispatchUtilityCommandInput({ code: TAKE_SCREENSHOT_COMMAND, x: 0, y: 0 }, hooks), true);
    assert.equal(trace.join(''), 'SST');
    // 현재 SID 전체 DWORD 를 소유자나 낮은 WORD 로 치환하지 않고 그대로 넘긴다.
    assert.equal(dispatchUtilityCommandInput({ code: LOOK_AT_SID_COMMAND, x: 0, y: -1 }, hooks), true);
    assert.equal(trace.join(''), 'SSTC4294967295');
    // 선택된 장치만 요구한다.
    assert.throws(() => dispatchUtilityCommandInput({ code: LOOK_AT_SID_COMMAND, x: 0, y: 1 }, {}), /미연결/);
    assert.equal(dispatchUtilityCommandInput({ code: MOVE_ISLAND_COMMAND, x: 0, y: 123 }, { highlight: (mode) => trace.push(`H${mode}`) }), true);
    assert.equal(trace.at(-1), 'H6');
});

// 독립 PE 가 결정한 음질 변경의 선행 종료·signed Config·priestmove.wav 이다.
test('음질 명령: 원본10.78 32행(선행 종료·signed 설정·소리 재생·뗌)', { skip: !soundRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of soundRows) {
        assert.equal(row.length, 4);
        count += 1;
        const trace = [];
        const hooks = {
            closeSound: () => trace.push('Close'),
            config: (text) => trace.push(`Config:${textToHex(text)}`),
            sound: (name, a, b, c, d, e) => { assert.equal(a | b | c | d | e, 0); trace.push(`Sound:${textToHex(name)}`); },
        };
        assert.equal(dispatchPresentationCommandInput({ code: number(row[0]), x: 37, y: signed(number(row[1])) }, hooks), true);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[2], `행 ${count} ${row[0]}`);
    }
    assert.equal(count, 32);
});

// 종료 뒤 Config 가 실패하면 되돌림이나 소리 합성을 하지 않는다.
test('음질 명령: 부분 실패와 뗌·다른 키', () => {
    assert.equal(dispatchPresentationCommandInput({ code: 'D'.charCodeAt(0), x: 0, y: 0 }, {}), false);
    assert.equal(dispatchPresentationCommandInput({ code: CHANGE_FONT_FACE_COMMAND | InputCode.release, x: 0, y: 0 }, {}), true);
    assert.equal(dispatchPresentationCommandInput({ code: CHANGE_SOUND_QUALITY_COMMAND | InputCode.release, x: 0, y: 0 }, {}), true);
    let trace = '';
    const hooks = {
        closeSound: () => { trace += 'C'; },
        config: (text) => { trace += 'S'; assert.equal(text, 'soundQuality=-1'); throw new Error('명시 Config 실패'); },
        sound: () => { trace += 'P'; },
    };
    assert.ok(failed(() => dispatchPresentationCommandInput({ code: CHANGE_SOUND_QUALITY_COMMAND, x: 0, y: -1 }, hooks)));
    assert.equal(trace, 'CS');
});

// 현재 H/W·dirty 뒤 이름 재조회와 각 효과의 실패 꼬리를 원문 관측에서 읽는다(일반 입력 부모 행만).
test('글꼴 명령: 원본10.78 부모 15행(높이→폭 재조회·dirty·이름 재조회·중단 꼬리)', { skip: !fontRows && '원본 fixture 없음' }, () => {
    // 최대 31바이트·첫 NUL 까지의 현재 이름 저장이다.
    const current31 = (source) => { const end = source.indexOf('\0'); return source.slice(0, Math.min(31, end < 0 ? Infinity : end)); };
    // 부모가 직접 호출한 Dirty/Set 만 남긴다.
    const parentTrace = (trace) => trace.split(';').filter((item) => item.startsWith('Dirty:') || item.startsWith('Set:')).join(';');
    let count = 0;
    for (const row of fontRows) {
        assert.equal(row.length, 9);
        if (row[1] !== '1') continue;
        count += 1;
        const source = row[0] === 'NULL' ? '' : hexToText(row[0]);
        let current = 'PreviousFace';
        let width = 640;
        let height = 480;
        const trace = [];
        const hooks = {
            fontFace: (index) => { assert.equal(index, 0); return row[0] === 'NULL' ? null : source; },
            rebuildFonts: (face) => {
                if (['Clear', 'Create', 'Cache'].includes(row[2])) throw new Error('명시 child 중단');
                current = current31(face === null ? 'Arial' : face);
                if (row[3] === '1') { width = 1280; height = 720; }
            },
            width: () => width,
            height: () => height,
            invalidate: (x, y, w, h, flags) => {
                trace.push(`Dirty:${x}:${y}:${w}:${h}:${flags}`);
                if (row[2] === 'Dirty') throw new Error('명시 dirty 중단');
                if (row[4] === '1') current = 'ChangedAtDirty';
            },
            currentFontFace: () => current,
            set: (key, value) => { trace.push(`Set:${textToHex(key)}:${textToHex(value)}`); if (row[2] === 'Set') throw new Error('명시 Config 중단'); },
        };
        let completed = true;
        try { assert.equal(dispatchPresentationCommandInput({ code: CHANGE_FONT_FACE_COMMAND, x: 37, y: 0 }, hooks), true); } catch (error) { if (error.code === 'ERR_ASSERTION') throw error; completed = false; }
        assert.equal(completed, row[5] === '1', `행 ${count} 완료`);
        assert.equal(trace.join(';'), parentTrace(row[8]), `행 ${count} 부모 순서`);
        const expected = hexToText(row[6]);
        assert.equal(current, expected.slice(0, expected.indexOf('\0') < 0 ? undefined : expected.indexOf('\0')), `행 ${count} 현재 이름`);
    }
    assert.equal(count, 15);
});

// 높이를 먼저 읽고 폭을 읽으며 dirty 뒤 현재 이름을 다시 읽는다.
test('글꼴 명령: 읽기 순서와 선택 장치 누락', () => {
    const order = [];
    const hooks = {
        fontFace: () => 'Face',
        rebuildFonts: (face) => order.push(`R:${face}`),
        height: () => { order.push('H'); return 7; },
        width: () => { order.push('W'); return 9; },
        invalidate: (...args) => order.push(`D:${args.join(',')}`),
        currentFontFace: () => { order.push('N'); return 'Face2'; },
        set: (key, value) => order.push(`S:${key}=${value}`),
    };
    assert.equal(dispatchPresentationCommandInput({ code: CHANGE_FONT_FACE_COMMAND, x: 0, y: 1 }, hooks), true);
    assert.equal(order.join(' '), 'R:Face H W D:0,0,9,7,0 N S:fontFaceName=Face2');
    delete hooks.currentFontFace;
    assert.throws(() => dispatchPresentationCommandInput({ code: CHANGE_FONT_FACE_COMMAND, x: 0, y: 1 }, hooks), /미연결/);
});

// 독립 PE 가 결정한 연기/번개의 생성 인자와 무동작·폭 보관·기술 조회 순서이다.
test('시나리오 표시: 원본10.78 576행(연기·번개·무동작·폭 선행 조회)', { skip: !effectRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    for (const fields of effectRows) {
        if (fields[0] === 'name') continue;
        assert.equal(fields.length, 9);
        let width = signed(number(fields[3]));
        const technology = number(fields[4]);
        const trace = [];
        const hooks = {
            width: () => width,
            // 기술 조회 효과가 폭을 바꿔도 먼저 읽은 폭을 사용해야 한다.
            technology: () => { trace.push('technology'); width = 1920; return technology; },
            smoke: (parent, x, y) => trace.push(`smoke:${parent}:${x}:${y}`),
            lightning: (parent, from, to, life) => trace.push(`lightning:${parent}:${from[0]}:${from[1]}:${to[0]}:${to[1]}:${floatToBits(life)}`),
        };
        assert.equal(dispatchScenarioEffectInput({ code: number(fields[1]), x: 305419896, y: signed(number(fields[2])) }, hooks), true);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), fields[6], `행 ${rows + 1} ${fields[0]}`);
        assert.ok(fields[7] === '1' && fields[8] === '0');
        rows += 1;
    }
    assert.equal(rows, 576);
});

// 일반 입력의 DumpSquids 48개는 장치 없이 효과 없이 소비된다.
test('시나리오 표시: DumpSquids 48행은 장치 없이 무동작', { skip: !dumpRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const fields of dumpRows) {
        if (fields[0] === 'event') continue;
        assert.equal(fields.length, 6);
        assert.ok(isScenarioEffectCommand(number(fields[0])));
        assert.equal(dispatchScenarioEffectInput({ code: number(fields[0]), x: 305419896, y: signed(number(fields[1])) }, {}), true);
        assert.ok(fields[3] === '-' && fields[4] === '1' && fields[5] === '0');
        count += 1;
    }
    assert.equal(count, 48);
});

// 도달하지 않는 장치는 요구하지 않고, 선택된 부모의 누락은 그대로 실패한다.
test('시나리오 표시: 뗌·무동작은 장치 불필요, 누름은 선택 장치 요구', () => {
    assert.equal(dispatchScenarioEffectInput({ code: 0x02000017, x: 0, y: 0 }, {}), false);
    assert.equal(dispatchScenarioEffectInput({ code: RANDOM_GUNS_COMMAND, x: 0, y: 0 }, {}), true);
    for (const code of [SMOKE_COMMAND, LIGHTNING_COMMAND, RANDOM_GUNS_COMMAND]) {
        assert.equal(dispatchScenarioEffectInput({ code: (code | InputCode.release) >>> 0, x: 0, y: 6008 }, {}), true);
    }
    for (const code of [SMOKE_COMMAND, LIGHTNING_COMMAND]) {
        assert.throws(() => dispatchScenarioEffectInput({ code, x: 0, y: 0 }, {}), /미연결/);
    }
    // 번개는 폭 조회가 먼저이고 그 뒤 기술 조회 누락은 거부한다.
    let widthReads = 0;
    assert.throws(() => dispatchScenarioEffectInput({ code: LIGHTNING_COMMAND, x: 0, y: 0 }, { width: () => { widthReads += 1; return 640; } }), /미연결/);
    assert.equal(widthReads, 1);
});
