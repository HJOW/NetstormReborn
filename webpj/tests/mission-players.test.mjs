// 미션 플레이어 목록(cpppj `o/Player` 의 `MissionPlayers`)의 웹 대응 `core/mission-players.js` 를 검사한다(4-C, WB04-E321).
//  - 원본 관측 fixture 가 없는 모듈이라 모든 기대값을 손으로 계산한다: 머리 값 조회 순서와 구식 `ai…` 대체, C `strtod` 숫자 판독, 기술 허용표 토큰, 시작 지식(all/이름/중복/정렬), 색 이름, 동맹 목록과 대칭화, 저장 기술/덱 복사, 입력 검사.
//  - 실제 타입 표(`netstorm.tarc` 의 `.type` 116개)로 `all` 지식을 독립 계산(정의의 group 속성 이름 표)과 대조한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E321)
import assert from 'node:assert/strict';
import test from 'node:test';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { buildTypeRecords } from '../www/assets/type-flags.js';
import { FIRST_ASSET_TYPE_NUMBER, TYPE_LOAD_ORDER, TypeTable, typeString } from '../www/assets/type-table.js';
import { MissionPlayers, PLAYER_COUNT, Player, cStringToDouble } from '../www/core/mission-players.js';
import { readData } from './helpers.mjs';

// 타입 레코드 188개를 만든다: 이름 없는 기본 레코드에 번호별 필드를 덮어쓴다(group 기본 10 = NO_GROUP). (2026-11-11 WB04-E321)
function makeTypes(overrides = {}) {
    const records = Array.from({ length: 188 }, (_, number) => ({ number, name: '', fromAsset: false, group: 10 }));
    for (const [number, fields] of Object.entries(overrides)) Object.assign(records[Number(number)], fields);
    return { records };
}

// 시험용 타입 표이다: 그룹 0~8 의 자산(70·72·75·76·마지막 칸 187), NO_GROUP(71·73), misc(74), 알 수 없는 그룹 -1(77), 이름이 같은 뒤쪽 자산(90), 자산이 아닌 이름 있는 타입(0·1·5·20·100: 100 은 그룹 0 이어도 자산이 아님), 70 앞의 자산 표시 레코드(69: 전체 지식은 70 부터만 훑음). (2026-11-11 WB04-E321)
const TYPES = makeTypes({
    0: { name: 'zero' },
    1: { name: 'one' },
    5: { name: 'builtin' },
    20: { name: 'Vortex Process', group: 0 },
    69: { name: 'pre', fromAsset: true, group: 0 },
    70: { name: 'sunArcher', fromAsset: true, group: 0 },
    71: { name: 'sunFactory', fromAsset: true },
    72: { name: 'windBattery', fromAsset: true, group: 5 },
    73: { name: 'bridge', fromAsset: true },
    74: { name: 'misc1', fromAsset: true, group: 9 },
    75: { name: 'walkerX', fromAsset: true, group: 7 },
    76: { name: 'balloonX', fromAsset: true, group: 8 },
    77: { name: 'unknownGroup', fromAsset: true, group: -1 },
    90: { name: 'SUNARCHER', fromAsset: true, group: 0 },
    100: { name: 'ghost', group: 0 },
    187: { name: 'tail', fromAsset: true, group: 2 },
});

// 머리 값 표에서 get 함수를 만든다(조회한 키를 기록한다). (2026-11-11 WB04-E321)
function scriptOf(values, calls = []) {
    return (key) => {
        calls.push(key);
        return values[key];
    };
}

// 빈 Fort 이다. (2026-11-11 WB04-E321)
const EMPTY_FORT = { technology: { items: [], complete: true }, deck: [] };

// 머리 값이 하나도 없으면 사람 1 과 AI 2~8 의 기본 상태이다: 번호=색, 사람만 활성, 시작 SP 는 대체값, 자기 동맹 비트, 기술 허용표는 전부 허용. (2026-11-11 WB04-E321)
test('미션 플레이어: 머리 값이 없을 때의 기본 상태와 조회 순서', () => {
    const calls = [];
    const players = MissionPlayers.load(scriptOf({}, calls), EMPTY_FORT, TYPES, 6500);
    assert.equal(PLAYER_COUNT, 8);
    assert.equal(players.players.length, 9);
    // 0 번 자리는 중립이며 기본 Player 그대로이다(번호·색·동맹 0, 이름 없음, 비활성, 저장 기술은 완료 상태의 빈 목록).
    assert.deepEqual({ ...players.players[0] }, { ...new Player() });
    assert.deepEqual({ ...players.players[0] }, {
        number: 0, color: 0, active: false, computer: false, stormPower: 0, name: '', startingTech: '', ability: '', allies: 0, knowledge: [], storedTechnology: { items: [], complete: true }, storedDeck: [],
    });
    for (let owner = 1; owner <= 8; owner += 1) {
        const player = players.players[owner];
        assert.deepEqual([player.number, player.color, player.computer, player.allies], [owner, owner, owner !== 1, 1 << owner], `플레이어 ${owner}`);
        assert.deepEqual([player.name, player.active, player.startingTech, player.ability, player.knowledge], [owner === 1 ? 'Player' : '', owner === 1, '', '', []]);
        assert.equal(player.stormPower, owner === 1 ? 6500 : 0);
    }
    assert.deepEqual([players.denySalvage, players.denyAscend, players.aiOff], [false, false, false]);
    assert.equal(players.techAllowed.length, 188);
    assert.ok(players.techAllowed.every((allowed) => allowed === true));
    // 조회 순서: 전역 값 4개 → 사람(이름 없음) → 2번은 번호 있는 키 뒤에 번호 없는 구식 키 → 3~8번.
    const expected = ['denySalvage', 'denyAscend', 'aiOff', 'techAllowed', 'myStartMoney', 'myTech', 'ai1Ability', 'ai1Color', 'myAllyList'];
    expected.push('ai2Name', 'aiName', 'ai2StartMoney', 'aiStartMoney', 'ai2Tech', 'aiTech', 'ai2Ability', 'aiAbility', 'ai2Color', 'aiColor', 'ai2AllyList', 'aiAllyList');
    for (let owner = 3; owner <= 8; owner += 1) expected.push(...['Name', 'StartMoney', 'Tech', 'Ability', 'Color', 'AllyList'].map((suffix) => `ai${owner}${suffix}`));
    assert.deepEqual(calls, expected);
});

// 숫자 머리 값은 C strtod 처럼 앞의 숫자를 읽고, 0 이 아니면 참이다(NaN·inf 포함). (2026-11-11 WB04-E321)
test('미션 플레이어: 숫자 머리 값의 strtod 판독', () => {
    const table = [
        ['12.5', 12.5], [' \t\n\v\f\r7', 7], ['-3', -3], ['+.5e1', 5], ['1e3', 1000], ['1e', 1], ['1E+2x', 100], ['5.', 5], ['.5', 0.5], ['1.e2', 100], ['1_000', 1], ['.', 0], ['-', 0], ['+', 0],
        ['', 0], ['abc', 0], ['0x10', 16], ['0X1.8p1', 3], ['0xp1', 0], ['0x1p', 1], ['0x.8', 0.5], ['0x1.p4', 16], ['0xg', 0], ['-0x10', -16], ['0x1P-2', 0.25], ['0xFF', 255],
        ['inf', Infinity], ['-INF', -Infinity], ['Infinity', Infinity], ['infinit', Infinity], ['Infx', Infinity], ['1e999', Infinity], ['-1e999', -Infinity], ['1e-400', 0],
        ['5\u0000 6', 5], ['\u0000 5', 0], ['0.1', 0.1], ['123456789012345678901234567890', 123456789012345678901234567890],
    ];
    for (const [text, expected] of table) assert.equal(cStringToDouble(text), expected, JSON.stringify(text));
    // NaN 은 nan 과 nan(...) 둘 다 읽고 부호는 상관없다. 음의 0 도 보존한다.
    for (const text of ['nan', 'NaN', 'nan(abc_1)', '-nan', ' +NAN']) assert.ok(Number.isNaN(cStringToDouble(text)), text);
    assert.ok(Object.is(cStringToDouble('-0'), -0));
    assert.ok(Object.is(cStringToDouble('nanx'), Number.NaN));
    // 머리 값의 거짓/참: -0·0 과 0.0·0x0·1e-400 은 거짓, 음수·소수·NaN·inf 는 참.
    for (const [text, expected] of [['0', false], ['-0', false], ['0.0', false], ['0x0', false], ['1e-400', false], ['abc', false], ['', false], ['-17', true], ['.5', true], ['nan', true], ['inf', true], ['1', true]]) {
        const players = MissionPlayers.load(scriptOf({ denySalvage: text, denyAscend: text, aiOff: text }), EMPTY_FORT, TYPES, 0);
        assert.deepEqual([players.denySalvage, players.denyAscend, players.aiOff], [expected, expected, expected], JSON.stringify(text));
    }
});

// 시작 SP: 사람은 myStartMoney(없으면 대체값), AI 는 ai<번호>StartMoney(없으면 0)를 같은 규칙으로 읽는다. (2026-11-11 WB04-E321)
test('미션 플레이어: 사람·AI 의 이름·시작 SP·기술·능력과 구식 ai 대체', () => {
    const players = MissionPlayers.load(scriptOf({
        myStartMoney: '12345', myTech: 'sunArcher', ai1Ability: 'ignored-for-human-name', ai3Name: 'Third', ai3StartMoney: ' 250.75xyz', ai3Tech: 'windBattery', ai3Ability: '!BUILD_DAIS',
        aiName: 'Old', aiStartMoney: '300', aiTech: 'walkerX', aiAbility: 'old-ability', aiColor: 'Red', aiAllyList: '1;3',
    }), EMPTY_FORT, TYPES, 6500);
    const [, one, two, three, four] = players.players;
    // 사람: 이름은 항상 Player, 능력은 ai1Ability 를 읽는다(원문 그대로).
    assert.deepEqual([one.name, one.active, one.stormPower, one.startingTech, one.knowledge, one.ability, one.computer], ['Player', true, 12345, 'sunArcher', [70], 'ignored-for-human-name', false]);
    // 2번은 번호 있는 값이 없어 번호 없는 구식 값을 대신 읽는다.
    assert.deepEqual([two.name, two.active, two.stormPower, two.startingTech, two.knowledge, two.ability, two.color], ['Old', true, 300, 'walkerX', [75], 'old-ability', 2]);
    assert.deepEqual([three.name, three.stormPower, three.startingTech, three.ability, three.active], ['Third', 250.75, 'windBattery', '!BUILD_DAIS', true]);
    // 3번 이후는 구식 값을 읽지 않는다.
    assert.deepEqual([four.name, four.active, four.stormPower, four.ability, four.color], ['', false, 0, '', 4]);
    // 번호 있는 값이 있으면(빈 문자열도 있는 값) 구식 값으로 돌아가지 않는다.
    const both = MissionPlayers.load(scriptOf({ ai2Name: '', aiName: 'Old', ai2StartMoney: '0', aiStartMoney: '300', ai2Tech: '', aiTech: 'sunArcher' }), EMPTY_FORT, TYPES, 1);
    assert.deepEqual([both.players[2].name, both.players[2].active, both.players[2].stormPower, both.players[2].startingTech, both.players[2].knowledge], ['', false, 0, '', []]);
    // 시작 SP 가 숫자가 아니거나 빈 문자열이어도 머리 값이 있으면 0 이다(대체값은 값이 아예 없을 때만).
    for (const value of ['many', '']) assert.equal(MissionPlayers.load(scriptOf({ myStartMoney: value }), EMPTY_FORT, TYPES, 6500).players[1].stormPower, 0, JSON.stringify(value));
});

// 색 이름은 대소문자를 무시하고 none 과 알 수 없는 이름은 번호 색을 유지한다. (2026-11-11 WB04-E321)
test('미션 플레이어: 색 이름', () => {
    const names = { ai3Color: 'Blue', ai4Color: 'NONE', ai5Color: 'unknown', ai6Color: 'ORANGE', ai7Color: 'lightBlue', ai8Color: 'yellow', ai1Color: 'Purple', aiColor: 'white' };
    const players = MissionPlayers.load(scriptOf(names), EMPTY_FORT, TYPES, 0);
    assert.deepEqual(players.players.slice(1).map((player) => player.color), [5, 3, 1, 4, 5, 8, 7, 6]);
    // 모든 색 이름의 번호이다.
    for (const [name, number] of [['blue', 1], ['red', 2], ['white', 3], ['green', 4], ['purple', 5], ['yellow', 6], ['lightblue', 7], ['orange', 8]]) {
        assert.equal(MissionPlayers.load(scriptOf({ ai8Color: name }), EMPTY_FORT, TYPES, 0).players[8].color, number, name);
    }
    // 색 이름 앞뒤 공백은 다듬지 않는다.
    assert.equal(MissionPlayers.load(scriptOf({ ai8Color: ' red' }), EMPTY_FORT, TYPES, 0).players[8].color, 8);
});

// 동맹 목록: 유효 번호(1~8)만 더하고 빈 조각에서 끝나며, 한쪽만 지정해도 양쪽 비트를 켜 대칭으로 만든다(전이는 없음). (2026-11-11 WB04-E321)
test('미션 플레이어: 동맹 목록과 대칭화', () => {
    const players = MissionPlayers.load(scriptOf({ myAllyList: '2;9;0;-1;x; 4;3x;1', ai5AllyList: '6', ai7AllyList: '8;;1' }), EMPTY_FORT, TYPES, 0);
    // 사람: 자기 1, 목록의 2·4(공백 앞)·3(숫자 앞부분)·1; 9·0·-1·x 는 무시.
    assert.equal(players.players[1].allies, (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4));
    // 한쪽만 지정해도 상대 비트가 켜진다: 2·3·4 는 사람 비트(1)를 얻는다.
    for (const owner of [2, 3, 4]) assert.equal(players.players[owner].allies, (1 << owner) | (1 << 1), `플레이어 ${owner}`);
    assert.deepEqual([players.players[5].allies, players.players[6].allies], [(1 << 5) | (1 << 6), (1 << 6) | (1 << 5)]);
    // 빈 조각 뒤의 항목은 읽지 않는다: 7 은 8 만, 8 은 7 만 얻고 1 과는 동맹이 아니다.
    assert.deepEqual([players.players[7].allies, players.players[8].allies], [(1 << 7) | (1 << 8), (1 << 8) | (1 << 7)]);
    assert.equal(players.allied(1, 7), false);
    // 전이는 없다: 1-2, 1-3 이 동맹이어도 2-3 은 아니다.
    assert.equal(players.allied(2, 3), false);
    assert.equal(players.allied(1, 2) && players.allied(2, 1) && players.allied(1, 3) && players.allied(1, 4) && players.allied(5, 6) && players.allied(6, 5), true);
    // 자기 자신은 동맹이고 중립·범위 밖·정수가 아닌 번호는 아니다.
    assert.equal(players.allied(3, 3), true);
    for (const [first, second] of [[0, 1], [1, 0], [0, 0], [9, 1], [1, 9], [-1, 2], [1.5, 2], [2, Number.NaN], [1, '2']]) assert.equal(players.allied(first, second), false, `${first},${second}`);
    // 9 번 이상의 비트는 (8 비트 범위의) 동맹 마스크에 들어가지 않는다.
    assert.ok(players.players.slice(1).every((player) => player.allies < 1 << 9));
    // 번호가 더 큰 쪽만 지정해도 번호가 작은 쪽이 상대 비트를 얻는다(6 이 5 를 지정 → 5 가 6 의 비트를 얻음).
    const higher = MissionPlayers.load(scriptOf({ ai6AllyList: '5' }), EMPTY_FORT, TYPES, 0);
    assert.deepEqual([higher.players[5].allies, higher.players[6].allies], [(1 << 5) | (1 << 6), (1 << 6) | (1 << 5)]);
    // 목록 번호는 C 의 long 규칙(부호 없는 32비트로 감김)으로 읽는다: 2³²+2 는 2, -4294967294 는 2.
    assert.equal(MissionPlayers.load(scriptOf({ myAllyList: '4294967298' }), EMPTY_FORT, TYPES, 0).allied(1, 2), true);
    assert.equal(MissionPlayers.load(scriptOf({ myAllyList: '-4294967294' }), EMPTY_FORT, TYPES, 0).allied(1, 2), true);
    assert.equal(MissionPlayers.load(scriptOf({ myAllyList: '0x2' }), EMPTY_FORT, TYPES, 0).allied(1, 2), false);
    // 조회는 두 방향 중 한쪽 비트만 있어도 참이다(수동 상태: 4 만 3 의 비트를 가짐). 중립 자리의 비트는 무시한다.
    const manual = new MissionPlayers();
    manual.players[3].allies = 1 << 3;
    manual.players[4].allies = (1 << 4) | (1 << 3);
    assert.deepEqual([manual.allied(3, 4), manual.allied(4, 3)], [true, true]);
    manual.players[0].allies = 0xffff;
    manual.players[1].allies = 0xffff;
    assert.deepEqual([manual.allied(0, 1), manual.allied(1, 0), manual.allied(0, 0)], [false, false, false]);
});

// 기술 허용표: allow/deny/all 토큰은 앞에서부터 상태를 바꾸고 타입 이름 토큰은 현재 상태를 그 타입에 쓴다(모든 레코드의 이름, 첫 일치, 대소문자 무시). (2026-11-11 WB04-E321)
test('미션 플레이어: 기술 허용표', () => {
    const load = (text) => MissionPlayers.load(scriptOf({ techAllowed: text }), EMPTY_FORT, TYPES, 0).techAllowed;
    const denied = (table) => table.flatMap((allowed, type) => (allowed ? [] : [type]));
    // 전부 거부 뒤 이름만 허용.
    const only = load('deny;all;allow;sunArcher;DENY;windBattery;bogus;sunFactory');
    assert.deepEqual(only.flatMap((allowed, type) => (allowed ? [type] : [])), [70]);
    assert.deepEqual(denied(load('deny;sunArcher')), [70]);
    assert.deepEqual(denied(load('DENY;SunArcher;ALL;Allow;windBattery')), Array.from({ length: 188 }, (_, type) => type).filter((type) => type !== 72));
    // 처음 상태는 허용이라 이름만 쓰면 아무것도 바뀌지 않는다.
    assert.deepEqual(denied(load('sunArcher;windBattery')), []);
    // 번호 0 과 1 의 이름: 0 은 허용표를 바꾸지 않고(번호는 0 보다 커야 함) 1 은 바꾼다.
    assert.deepEqual(denied(load('deny;zero')), []);
    assert.deepEqual(denied(load('deny;one')), [1]);
    // 자산이 아닌 이름 있는 타입도 찾고(5·20), 이름이 같으면 앞의 레코드가 이긴다(70 이 허용표를 바꾸고 90 은 그대로).
    assert.deepEqual(denied(load('deny;builtin;vortex process;sunarcher')), [5, 20, 70]);
    assert.equal(load('deny;SUNARCHER')[90], true);
    // 공백이 붙은 토큰은 다른 이름이라 무시하고 빈 조각에서 끝난다.
    assert.deepEqual(denied(load('deny; all')), []);
    assert.deepEqual(denied(load('deny;;sunArcher')), []);
    assert.deepEqual(denied(load('deny;all;')), Array.from({ length: 188 }, (_, type) => type));
    // 번호 0 의 이름 없는 레코드와 알 수 없는 이름은 허용표를 바꾸지 않는다.
    assert.deepEqual(denied(load('deny;none;?')), []);
    // 표 길이는 타입 레코드 수이다.
    assert.equal(load('').length, 188);
    assert.equal(MissionPlayers.load(scriptOf({}), EMPTY_FORT, makeTypes(), 0).techAllowed.length, 188);
    assert.equal(MissionPlayers.load(scriptOf({}), EMPTY_FORT, { records: makeTypes().records.slice(0, 100) }, 0).techAllowed.length, 100);
});

// 시작 지식: all 은 생산 그룹(0~8, 알 수 없는 그룹 -1 포함)의 자산 타입만, 이름은 0 보다 큰 번호만 모아 중복 없이 오름차순이다. (2026-11-11 WB04-E321)
test('미션 플레이어: 시작 지식', () => {
    const load = (tech) => MissionPlayers.load(scriptOf({ myTech: tech, ai2Tech: tech }), EMPTY_FORT, TYPES, 0);
    // all: 70 부터만 훑으므로 69(자산 표시)와 20(group 0)은 제외, 자산이 아닌 100(group 0)·misc 74·NO_GROUP 71·73 도 제외, 알 수 없는 그룹 77 은 포함, 이름이 같은 90 과 마지막 칸 187 도 자산이라 포함.
    assert.deepEqual(load('all').players[1].knowledge, [70, 72, 75, 76, 77, 90, 187]);
    assert.deepEqual(load('ALL').players[2].knowledge, [70, 72, 75, 76, 77, 90, 187]);
    // 이름: 중복 제거와 오름차순 정렬, 대소문자 무시, 첫 일치 번호.
    assert.deepEqual(load('windBattery;SUNARCHER;windbattery').players[1].knowledge, [70, 72]);
    assert.deepEqual(load('all;sunFactory').players[1].knowledge, [70, 71, 72, 75, 76, 77, 90, 187]);
    // 알 수 없는 이름은 무시하고 빈 조각에서 읽기를 끝낸다.
    assert.deepEqual(load('bogus;sunArcher;;windBattery').players[1].knowledge, [70]);
    assert.deepEqual(load('bogus').players[1].knowledge, []);
    // 이름으로는 자산이 아닌 타입(5·20·100)과 70 앞의 타입(69)도 번호가 0 보다 크면 포함하고, 번호 0 의 이름은 포함하지 않는다.
    assert.deepEqual(load('builtin;Vortex Process').players[1].knowledge, [5, 20]);
    assert.deepEqual(load('ghost;pre;one;zero').players[1].knowledge, [1, 69, 100]);
    assert.deepEqual(load('zero').players[1].knowledge, []);
    // 시작 기술 문자열은 그대로 보존한다.
    assert.equal(load(' all').players[1].startingTech, ' all');
    assert.deepEqual(load(' all').players[1].knowledge, []);
});

// 저장 기술/덱은 사람 1 번에만 있고 원본 Fort 와 독립된 복사본이다. (2026-11-11 WB04-E321)
test('미션 플레이어: 저장 기술/덱의 복사', () => {
    const fort = {
        technology: { items: [{ type: 70, count: 2 }, { type: 72, count: 1 }], complete: false },
        deck: [{ storedType: 3, type: 70, chance: 1, power: -2, remaining: 5 }],
    };
    const players = MissionPlayers.load(scriptOf({}), fort, TYPES, 0);
    assert.deepEqual(players.players[1].storedTechnology, fort.technology);
    assert.deepEqual(players.players[1].storedDeck, fort.deck);
    assert.notEqual(players.players[1].storedTechnology, fort.technology);
    assert.notEqual(players.players[1].storedDeck, fort.deck);
    // 복사본을 바꿔도 원본 Fort 는 그대로이다.
    players.players[1].storedTechnology.items[0].count = 99;
    players.players[1].storedDeck.push({});
    assert.equal(fort.technology.items[0].count, 2);
    assert.equal(fort.deck.length, 1);
    // 다른 플레이어는 비어 있다.
    for (let owner = 2; owner <= 8; owner += 1) assert.deepEqual([players.players[owner].storedTechnology, players.players[owner].storedDeck], [{ items: [], complete: true }, []]);
    // 기술/덱이 없는 Fort 는 빈 기본값이다.
    const bare = MissionPlayers.load(scriptOf({}), {}, TYPES, 0);
    assert.deepEqual([bare.players[1].storedTechnology, bare.players[1].storedDeck], [{ items: [], complete: true }, []]);
});

// 입력 검사와 기본 생성: 잘못된 입력은 거부하고 기본 MissionPlayers 는 빈 허용표와 기본 Player 9개이다. (2026-11-11 WB04-E321)
test('미션 플레이어: 입력 검사와 기본 생성', () => {
    const invalid = /미션 플레이어 입력 오류/;
    assert.throws(() => MissionPlayers.load(null, EMPTY_FORT, TYPES, 0), invalid);
    assert.throws(() => MissionPlayers.load(scriptOf({}), null, TYPES, 0), invalid);
    assert.throws(() => MissionPlayers.load(scriptOf({}), undefined, TYPES, 0), invalid);
    assert.throws(() => MissionPlayers.load(scriptOf({}), 5, TYPES, 0), invalid);
    assert.throws(() => MissionPlayers.load(scriptOf({}), EMPTY_FORT, null, 0), invalid);
    assert.throws(() => MissionPlayers.load(scriptOf({}), EMPTY_FORT, {}, 0), invalid);
    assert.throws(() => MissionPlayers.load(scriptOf({}), EMPTY_FORT, { records: {} }, 0), invalid);
    // 머리 값은 문자열이어야 하며 null 은 없는 값이다.
    assert.throws(() => MissionPlayers.load(scriptOf({ myTech: 5 }), EMPTY_FORT, TYPES, 0), /문자열/);
    assert.doesNotThrow(() => MissionPlayers.load(scriptOf({ myTech: null, ai2Name: undefined }), EMPTY_FORT, TYPES, 0));
    const empty = new MissionPlayers();
    assert.deepEqual([empty.players.length, empty.techAllowed, empty.denySalvage, empty.denyAscend, empty.aiOff], [9, [], false, false, false]);
    assert.equal(empty.allied(1, 1), false);
    // 서로 다른 인스턴스는 Player 객체를 공유하지 않는다.
    assert.notEqual(new MissionPlayers().players[1], new MissionPlayers().players[1]);
});

// 실제 타입 표: all 지식은 정의의 group 속성 이름 표로 독립 계산한 생산 그룹 자산 타입과 같다. (2026-11-11 WB04-E321)
test('미션 플레이어: 실제 타입 표의 all 지식과 이름 조회', () => {
    const table = TypeTable.fromArchive(new TaffArchive(readData('netstorm.tarc')), decodeOriginalText);
    const types = { records: buildTypeRecords(table.definitions) };
    const groups = ['archer', 'cannon', 'blocker', 'aviary', 'flyer', 'battery', 'fence', 'walker', 'balloon', 'misc'];
    // 독립 계산: group 속성이 없으면 NO_GROUP(10), 이름이 표에 없으면 -1, 있으면 그 위치이다. 9 미만이면 생산 그룹이다.
    const expected = [];
    TYPE_LOAD_ORDER.forEach((name, index) => {
        const text = table.definitions[index] ? typeString(table.definitions[index], 'group') : null;
        const group = text === null ? 10 : groups.indexOf(text.toLowerCase());
        if (group < 9) expected.push(FIRST_ASSET_TYPE_NUMBER + index);
    });
    assert.ok(expected.length > 20 && expected.length < 116);
    const players = MissionPlayers.load(scriptOf({ myTech: 'all' }), EMPTY_FORT, types, 0);
    assert.deepEqual(players.players[1].knowledge, expected);
    // 알려진 타입의 이름 조회(대소문자 무시): bridge 는 82, sunArcher 는 71.
    assert.deepEqual(MissionPlayers.load(scriptOf({ myTech: 'BRIDGE;sunarcher' }), EMPTY_FORT, types, 0).players[1].knowledge, [table.find('sunArcher'), table.find('bridge')].sort((a, b) => a - b));
    assert.equal(table.find('bridge'), 82);
    // 허용표: 전부 거부 뒤 sunArcher 만 허용하면 그 번호 하나만 참이다.
    const allowed = MissionPlayers.load(scriptOf({ techAllowed: 'deny;all;allow;sunArcher' }), EMPTY_FORT, types, 0).techAllowed;
    assert.deepEqual(allowed.flatMap((value, type) => (value ? [type] : [])), [table.find('sunArcher')]);
});
