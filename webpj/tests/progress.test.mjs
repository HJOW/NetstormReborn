// 캠페인 진행 값(`Done*`) 쓰기·저장 층을 검사한다: 진행 저장소(platform/progress.js)의 검증·저장 형식·손상 처리·다른 탭 값 합치기,
// 게임 데이터의 값 쓰기(GameData.set, 원본 Config::Set 규칙)와 통지, 쓴 진행 값이 캠페인 목록의 체크 표시·잠금 해제로 이어지는 흐름,
// 앱(Netstorm)이 설정·진행 저장소·게임 데이터를 서로 맞추는 연결이다. 원본 Config::Set·RemoveKey 자체는 config.test.mjs 가 원본 x86 기대값으로 대조한다.
// (2026-10-30 WB03-H: 진행 값 쓰기·저장 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, readDataMap } from './helpers.mjs';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { Netstorm } from '../www/netstorm.js';
import { PROGRESS_KEY_MAX, PROGRESS_STORAGE_KEY, PROGRESS_VERSION, Progress, isProgressKey, normalizeProgressValue } from '../www/platform/progress.js';
import { SETTINGS_STORAGE_KEY, Settings, findSettingKey } from '../www/platform/settings.js';
import { createTellWindow } from '../www/ui/tell-window.js';

// 메뉴 묶음 파일 전부를 읽어 자원을 만든다(검사마다 공유). (2026-10-30 WB03-H)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const resources = buildMenuResources(files);

// getItem·setItem·removeItem 을 가진 가짜 저장소이다. writes 는 setItem 호출 횟수이고 map 은 저장된 문자열이다. (2026-10-30 WB03-H)
function fakeStorage(initial = {}) {
    const map = new Map(Object.entries(initial));
    return {
        map,
        writes: 0,
        getItem(key) {
            return map.has(key) ? map.get(key) : null;
        },
        setItem(key, value) {
            this.writes += 1;
            map.set(key, String(value));
        },
        removeItem(key) {
            map.delete(key);
        },
    };
}

// 저장된 진행 값 JSON 을 해독해 { 키: 값 } 객체로 돌려준다. (2026-10-30 WB03-H)
function storedValues(storage) {
    return JSON.parse(storage.map.get(PROGRESS_STORAGE_KEY)).values;
}

// 진행 키·값의 모양 규칙이다: `Done` + 영숫자·밑줄 한 글자 이상, 값은 부호 있는 10자리 이하 십진수를 원본 ConfigParseLong 으로 정리한다.
test('진행 키·값 검증', () => {
    // 원본 미션 파일 이름에서 나오는 키(밑줄·숫자 포함)와 대소문자 변형은 받는다.
    for (const key of ['DoneTutorial1', 'DoneTheWarBegins', 'Donebc3acid_bar76', 'DONETHENOOSE', 'Done1']) assert.equal(isProgressKey(key), true, key);
    // 설정 키·`Done` 만 있는 키·점이 들어간 객체 키·공백·너무 긴 키·문자열이 아닌 값은 받지 않는다.
    for (const key of ['sound', 'Done', 'global.DoneX', 'Done X', 'Done-X', `Done${'a'.repeat(PROGRESS_KEY_MAX)}`, '', null, undefined, 7]) assert.equal(isProgressKey(key), false, String(key));
    assert.equal(isProgressKey(`Done${'a'.repeat(PROGRESS_KEY_MAX - 4)}`), true);
    // 값: 정수와 숫자 문자열을 십진 문자열로 맞춘다.
    assert.equal(normalizeProgressValue(1), '1');
    assert.equal(normalizeProgressValue('1'), '1');
    assert.equal(normalizeProgressValue('+7'), '7');
    assert.equal(normalizeProgressValue('-0'), '0');
    assert.equal(normalizeProgressValue(-3), '-3');
    // 32비트 감김은 원본 ConfigParseLong 과 같다: 2^32 는 0.
    assert.equal(normalizeProgressValue('4294967296'), '0');
    // 소수·큰 정수·글자 섞인 값·빈 값·줄바꿈이나 따옴표가 든 값은 받지 않는다(저장 파일을 깨지 않게).
    for (const value of [1.5, 2 ** 31, '1.5', '12345678901', 'x', '', ' 1', '1\n', '1"', null, undefined, true, {}]) assert.equal(normalizeProgressValue(value), null, String(value));
});

// 저장소 없음·정상 저장·같은 값 무시·대소문자 무시·적은 대로의 키 보존·다시 읽기를 확인한다.
test('Progress: 대소문자 무시·저장 형식·같은 값은 쓰지 않음·다시 읽기', () => {
    const storage = fakeStorage();
    const changes = [];
    const progress = new Progress({ storage, onChange: (key, value, previous) => changes.push([key, value, previous]) });
    assert.equal(progress.loadProblem, null);
    assert.equal(progress.persisted, true);
    assert.deepEqual(progress.snapshot(), {});
    assert.equal(storage.writes, 0);

    assert.equal(progress.set('DoneTheWarBegins', 1), true);
    assert.equal(progress.get('donethewarbegins'), '1');
    assert.equal(progress.get('DONETHEWARBEGINS'), '1');
    assert.equal(progress.get('DoneMasterOfWhirligigs'), undefined);
    // 저장 형식: 버전이 있는 JSON, 키는 적은 대로이다.
    assert.deepEqual(JSON.parse(storage.map.get(PROGRESS_STORAGE_KEY)), { version: PROGRESS_VERSION, values: { DoneTheWarBegins: '1' } });
    // 같은 값(키의 대소문자만 다름)은 쓰지 않고 통지하지 않는다.
    assert.equal(progress.set('DONETHEWARBEGINS', '1'), false);
    assert.equal(storage.writes, 1);
    // 값을 바꾸면 적은 대로의 새 키가 한 항목으로 남는다.
    assert.equal(progress.set('DONETHEWARBEGINS', 2), true);
    assert.deepEqual(progress.snapshot(), { DONETHEWARBEGINS: '2' });
    assert.deepEqual(storedValues(storage), { DONETHEWARBEGINS: '2' });
    assert.deepEqual(changes, [['DoneTheWarBegins', '1', undefined], ['DONETHEWARBEGINS', '2', '1']]);
    // 잘못된 키·값은 거부하며 저장하지 않는다.
    assert.equal(progress.set('sound', 1), false);
    assert.equal(progress.set('DoneX', 'abc'), false);
    assert.equal(storage.writes, 2);
    // persist:false 는 메모리에서만 바꾼다.
    assert.equal(progress.set('DoneTemp', 1, { persist: false }), true);
    assert.equal(storage.writes, 2);
    assert.equal(progress.get('DoneTemp'), '1');

    // 새 인스턴스(새로고침)는 저장된 값을 그대로 읽는다. 메모리 전용 값은 없다.
    const reloaded = new Progress({ storage });
    assert.deepEqual(reloaded.snapshot(), { DONETHEWARBEGINS: '2' });
    assert.equal(reloaded.loadProblem, null);
});

// 통지 훅 예외·저장소 없음·저장소 오류·손상·다른 버전·잘못된 항목에서 게임이 계속 동작하는지 확인한다.
test('Progress: 저장소 없음·읽기/쓰기 오류·손상·버전 불일치·잘못된 항목', () => {
    const originalError = console.error;
    console.error = () => {};
    try {
        // 통지 훅이 예외를 던져도 값은 바뀌고 저장된다.
        const storage = fakeStorage();
        const throwing = new Progress({ storage, onChange: () => { throw new Error('훅 오류'); } });
        assert.equal(throwing.set('DoneA', 1), true);
        assert.deepEqual(storedValues(storage), { DoneA: '1' });
    } finally {
        console.error = originalError;
    }
    // 저장소 없음: 값은 메모리에서 동작하고 persisted 는 false 이다.
    const none = new Progress();
    assert.equal(none.persisted, false);
    assert.equal(none.set('DoneA', 1), true);
    assert.equal(none.get('DoneA'), '1');
    assert.equal(none.persisted, false);
    // 읽기 오류: 진행 값 없이 시작하되 사유를 남긴다.
    const unreadable = new Progress({ storage: { getItem() { throw new Error('차단'); }, setItem() {}, removeItem() {} } });
    assert.match(unreadable.loadProblem, /읽을 수 없/);
    assert.equal(unreadable.persisted, false);
    // 쓰기 오류(용량 초과 등): 값은 바뀌고 persisted 가 false 가 된다.
    const full = new Progress({ storage: { getItem: () => null, setItem() { throw new Error('용량'); }, removeItem() {} } });
    assert.equal(full.set('DoneA', 1), true);
    assert.equal(full.persisted, false);
    assert.equal(full.get('DoneA'), '1');
    // 손상된 JSON·다른 버전·모양이 다른 내용은 진행 값 없이 시작한다.
    assert.match(new Progress({ storage: fakeStorage({ [PROGRESS_STORAGE_KEY]: '{깨짐' }) }).loadProblem, /손상/);
    for (const content of [JSON.stringify({ version: 99, values: { DoneA: '1' } }), JSON.stringify({ values: {} }), JSON.stringify([1]), JSON.stringify({ version: PROGRESS_VERSION, values: [1] }), 'null']) {
        const progress = new Progress({ storage: fakeStorage({ [PROGRESS_STORAGE_KEY]: content }) });
        assert.match(progress.loadProblem, /버전이 맞지 않/, content);
        assert.deepEqual(progress.snapshot(), {}, content);
    }
    // 잘못된 항목만 버리고 나머지는 받는다(설정 키·잘못된 값·문자열이 아닌 값).
    const mixed = new Progress({ storage: fakeStorage({ [PROGRESS_STORAGE_KEY]: JSON.stringify({ version: PROGRESS_VERSION, values: { DoneA: '1', sound: '1', DoneB: 'abc', DoneC: 1, DoneD: null } }) }) });
    assert.deepEqual(mixed.snapshot(), { DoneA: '1', DoneC: '1' });
    assert.match(mixed.loadProblem, /올바르지 않/);
    // 손상된 저장 내용 위에 새 값을 쓰면 정상 형식으로 덮어쓴다.
    const damaged = fakeStorage({ [PROGRESS_STORAGE_KEY]: '{깨짐' });
    new Progress({ storage: damaged }).set('DoneA', 1);
    assert.deepEqual(storedValues(damaged), { DoneA: '1' });
});

// 다른 탭이 먼저 저장한 완료 값을 이 탭의 저장이 지우지 않아야 한다(진행 값은 늘기만 하므로 합쳐 쓴다). 같은 키는 이 탭의 값이 이긴다.
test('Progress: 다른 탭이 저장한 값과 합쳐 저장', () => {
    const storage = fakeStorage();
    const tabA = new Progress({ storage });
    const tabB = new Progress({ storage });
    tabA.set('DoneTutorial1', 1);
    // B 는 A 의 값을 모르는 채로 다른 값을 저장한다.
    tabB.set('DoneTutorial2', 1);
    assert.deepEqual(storedValues(storage), { DoneTutorial1: '1', DoneTutorial2: '1' });
    // 같은 키는 나중에 저장한 탭의 값이 이긴다. 키의 대소문자만 다르면 한 항목이다.
    tabA.set('DoneTutorial2', 5);
    assert.deepEqual(storedValues(storage), { DoneTutorial1: '1', DoneTutorial2: '5' });
    tabB.set('DONETUTORIAL2', 7);
    assert.deepEqual(storedValues(storage), { DoneTutorial1: '1', DONETUTORIAL2: '7' });
    // 새로 읽으면 합쳐진 값이다.
    assert.deepEqual(new Progress({ storage }).snapshot(), { DoneTutorial1: '1', DONETUTORIAL2: '7' });
});

// 진행 값 지우기: 메모리와 저장소 항목을 모두 비운다.
test('Progress.clear: 저장된 진행 값을 모두 지운다', () => {
    const storage = fakeStorage();
    const progress = new Progress({ storage });
    assert.equal(progress.clear(), false);
    progress.set('DoneA', 1);
    assert.equal(storage.map.has(PROGRESS_STORAGE_KEY), true);
    assert.equal(progress.clear(), true);
    assert.deepEqual(progress.snapshot(), {});
    assert.equal(storage.map.has(PROGRESS_STORAGE_KEY), false);
    assert.deepEqual(new Progress({ storage }).snapshot(), {});
});

// 설정 키 이름 되돌리기: 대소문자를 무시하고 설정 키만 찾는다.
test('findSettingKey: 대소문자 무시로 설정 키만 찾는다', () => {
    assert.equal(findSettingKey('SOUNDVOLUME'), 'soundVolume');
    assert.equal(findSettingKey('swapleftrightspeakers'), 'swapLeftRightSpeakers');
    assert.equal(findSettingKey('DoneTutorial1'), null);
    assert.equal(findSettingKey(''), null);
});

// 새 게임 데이터를 만든다(Tell 목록이 설정 객체를 등록하므로 검사마다 따로 만든다). (2026-10-30 WB03-H)
function newData(options = {}, onChange = null) {
    return GameData.build({ files, language: 'english', options: { soundVolume: 3, musicVolume: 2, ...options }, onChange });
}

// 전역 설정 버퍼에서 키가 있는 줄들을 찾는다(대소문자 무시). 같은 키가 한 줄만 남는지 확인하는 데 쓴다. (2026-10-30 WB03-H)
function linesOf(data, key) {
    return data.configuration.text.split('\r\n').filter((line) => line.trim().toLowerCase().startsWith(`${key.toLowerCase()} =`));
}

// 게임 데이터 쓰기: 원본 Config::Set 규칙(같은 키의 줄을 모두 지우고 끝에 덧붙임)과 통지 훅·silent·훅 예외·정수 검사를 확인한다.
test('GameData.set: 같은 키의 줄을 지우고 끝에 덧붙이며 값을 바로 조회한다', () => {
    const writes = [];
    const data = newData({ DoneTheWarBegins: 0 }, (key, value) => writes.push([key, value]));
    assert.equal(data.get('DoneTheWarBegins'), '0');
    assert.equal(linesOf(data, 'DoneTheWarBegins').length, 1);
    // 이미 있던 키를 덮어쓰면 true, 줄은 하나만 남고 버퍼 끝으로 간다.
    assert.equal(data.set('DoneTheWarBegins', 1), true);
    assert.equal(data.get('DoneTheWarBegins'), '1');
    assert.deepEqual(linesOf(data, 'DoneTheWarBegins'), ['DoneTheWarBegins = "1"']);
    assert.ok(data.configuration.text.endsWith('DoneTheWarBegins = "1"\r\n'));
    // 없던 키는 false 이다. 키는 대소문자 무시로 조회된다.
    assert.equal(data.set('DoneNew', '1'), false);
    assert.equal(data.get('donenew'), '1');
    assert.equal(data.substitute('{DoneNew}|{DONENEW}|{nothing|none}'), '1|1|none');
    // 같은 키를 대소문자만 달리 다시 쓰면 줄은 하나이고 새로 적은 대로이다.
    assert.equal(data.set('DONENEW', 2), true);
    assert.deepEqual(linesOf(data, 'DoneNew'), ['DONENEW = "2"']);
    // 정수 설정과 비정수 거부.
    assert.equal(data.setInt('DoneInt', 7), false);
    assert.equal(data.get('DoneInt'), '7');
    assert.throws(() => data.setInt('DoneInt', 1.5), RangeError);
    // 통지 훅: 쓴 키와 문자열 값을 받는다. silent 이면 부르지 않는다.
    assert.deepEqual(writes, [['DoneTheWarBegins', '1'], ['DoneNew', '1'], ['DONENEW', '2'], ['DoneInt', '7']]);
    data.set('DoneQuiet', 1, { silent: true });
    assert.equal(data.get('DoneQuiet'), '1');
    assert.equal(writes.length, 4);
    // 훅이 예외를 던져도 값은 쓰인다.
    const originalError = console.error;
    console.error = () => {};
    try {
        const failing = newData({}, () => { throw new Error('훅 오류'); });
        failing.set('DoneA', 1);
        assert.equal(failing.get('DoneA'), '1');
    } finally {
        console.error = originalError;
    }
});

// 객체 접두어가 있는 키는 그 객체에 쓰인다(원본 Config::Set 의 '.' 규칙): `global.` 실행 중 값은 전역 설정이 아니라 global 객체에 들어간다.
test('GameData.set: 접두어가 맞는 객체에 쓴다', () => {
    const data = newData();
    assert.equal(data.set('global.priorTell', 'Credits'), false);
    assert.equal(data.get('global.priorTell'), 'Credits');
    assert.ok(data.global.text.includes('priorTell = "Credits"'));
    assert.equal(data.global.changed, true);
    assert.equal(data.configuration.text.includes('priorTell'), false);
    assert.equal(data.configuration.changed, false);
    // 같은 키를 다시 쓰면 global 안에서 한 줄만 남는다.
    assert.equal(data.set('global.priorTell', 'Demo'), true);
    assert.equal(data.global.text.split('\r\n').filter((line) => line.startsWith('priorTell')).length, 1);
});

// 캠페인 목록의 행 상태를 읽는다(c = 활성, k = 체크 표시, x = 잠김). 앱도 Campaign 대화상자(UCampaign)를 거쳐야 하위 목록 섹션이 등록되므로 같은 순서로 연다. (2026-10-30 WB03-H)
function campaignStates(data, name) {
    createTellWindow({ gameData: data, resources, name: 'UCampaign', screen: { width: 1024, height: 768 } });
    const result = createTellWindow({ gameData: data, resources, name, screen: { width: 1024, height: 768 } });
    assert.ok(result.window, `${name} 대화상자를 만들지 못했습니다`);
    return result.window.menu.items.map((item) => (item.disabled ? 'x' : item.checked ? 'k' : 'c'));
}

// 진행 값 쓰기 → 캠페인 목록의 체크 표시·다음 미션 해제 → 저장 → 새로 읽은 게임 데이터에서도 같은 상태: 미션 하나를 끝내는 전체 흐름이다.
test('진행 값을 쓰면 캠페인 목록의 체크 표시와 잠금 해제가 바뀌고, 저장한 값으로 다시 시작해도 같다', () => {
    const storage = fakeStorage();
    const progress = new Progress({ storage });
    // 앱과 같은 연결: 게임 데이터에 쓴 진행 값은 진행 저장소로 간다.
    const data = newData(progress.snapshot(), (key, value) => progress.set(key, value));
    assert.deepEqual(campaignStates(data, 'offical1.overview'), ['c', 'x', 'x', 'x', 'x', 'x']);
    // 튜토리얼 1 완료: 미션 스크립트의 `<$Config,Done{mission.fileName}=1>` 와 같은 쓰기이다.
    data.set('DoneTutorial1', 1);
    assert.deepEqual(campaignStates(data, 'offical1.overview'), ['k', 'c', 'x', 'x', 'x', 'x']);
    assert.deepEqual(storedValues(storage), { DoneTutorial1: '1' });
    // 이어서 2번과 3번 완료.
    data.set('DoneTutorial2', 1);
    data.set('DoneTutorial3', 1);
    assert.deepEqual(campaignStates(data, 'offical1.overview'), ['k', 'k', 'k', 'c', 'x', 'x']);
    // 다른 묶음은 그대로이다(Struggle For Freedom 은 1-1 완료 전까지 첫 행만 활성).
    assert.deepEqual(campaignStates(data, 'offical3.overview'), ['c', 'x', 'x', 'x', 'x', 'x']);
    // 새로고침: 저장소에서 다시 읽은 값으로 새 게임 데이터를 만들면 같은 상태이다.
    const reloaded = new Progress({ storage });
    const fresh = newData(reloaded.snapshot());
    assert.deepEqual(campaignStates(fresh, 'offical1.overview'), ['k', 'k', 'k', 'c', 'x', 'x']);
    // 1-1(DoneTheWarBegins) 완료는 Struggle For Freedom 의 다음 행을 연다.
    fresh.set('DoneTheWarBegins', 1);
    assert.deepEqual(campaignStates(fresh, 'offical3.overview'), ['k', 'c', 'x', 'x', 'x', 'x']);
});

// 한국어 브라우저(언어 korean)에서도 Campaign 목록이 나온다: `.korean` 캠페인 파일이 아직 없어도 영어 파일로 대체한다.
// 이 대체가 없으면 목록이 비어 Back 버튼만 있는 대화상자가 열렸다(실제 한국어 브라우저에서 발견, 2026-10-30 WB03-H).
test('한국어 설정에서도 Campaign 목록과 진행 상태가 나온다', () => {
    const korean = GameData.build({ files, language: 'korean', options: { DoneTutorial1: 1 } });
    const window = createTellWindow({ gameData: korean, resources, name: 'UCampaign', screen: { width: 1024, height: 768 } }).window;
    assert.ok(window.menu, '한국어에서 캠페인 목록이 비어 있음');
    // 웹 판본은 User Made Campaigns 행과 그 앞 구분선을 뺀 6행이다(2026-10-30 WB03-I, 이전에는 8행).
    assert.equal(window.menu.items.length, 6);
    assert.deepEqual(campaignStates(korean, 'offical1.overview'), ['k', 'c', 'x', 'x', 'x', 'x']);
});

// 앱 연결용 가짜 환경이다: DOM 없이 Netstorm 의 설정·진행·게임 데이터 연결 메서드만 쓰도록 필요한 필드를 직접 채운다. (2026-10-30 WB03-H)
function wiredApp(storage) {
    const app = new Netstorm();
    app._settings = new Settings({ storage, onChange: (key) => app._onSettingChange(key) });
    app._progress = new Progress({ storage });
    app._gameData = newData({ ...app._settings.snapshot(), ...app._progress.snapshot() }, (key, value) => app._onConfigWrite(key, value));
    return app;
}

// 앱 연결: 게임 데이터에 쓴 진행 값은 진행 저장소로, 설정 키는 설정 저장소로 가고, 설정 변경은 게임 데이터에 반영되며, 그 밖의 값은 저장되지 않는다.
test('Netstorm 연결: 게임 데이터 쓰기 → 진행/설정 저장소, 설정 변경 → 게임 데이터', () => {
    const storage = fakeStorage();
    const app = wiredApp(storage);
    // 진행 값 쓰기는 진행 저장소에 저장되고 설정 저장소는 건드리지 않는다.
    app._gameData.set('DoneTheWarBegins', 1);
    assert.deepEqual(storedValues(storage), { DoneTheWarBegins: '1' });
    assert.equal(storage.map.has(SETTINGS_STORAGE_KEY), false);
    // 게임 데이터에 설정 키를 쓰면(대소문자 무시) 설정 저장소에 정리된 값으로 저장된다.
    app._gameData.set('SOUNDVOLUME', 5);
    assert.equal(app._settings.get('soundVolume'), 5);
    assert.equal(JSON.parse(storage.map.get(SETTINGS_STORAGE_KEY)).values.soundVolume, 5);
    // 잘못된 설정 값은 설정 저장소가 거부한다(저장된 값은 그대로).
    app._gameData.set('soundVolume', 99);
    assert.equal(app._settings.get('soundVolume'), 5);
    // 설정을 옵션 메뉴 경로(settings.set)로 바꾸면 게임 데이터에도 반영되며 되돌아오는 통지로 진행 값을 만들지 않는다.
    app._settings.set('musicVolume', 4);
    assert.equal(app._gameData.get('musicVolume'), '4');
    app._settings.toggle('sound');
    assert.equal(app._gameData.get('sound'), '0');
    assert.deepEqual(storedValues(storage), { DoneTheWarBegins: '1' });
    // 진행 키도 설정 키도 아닌 값(실행 중 값)은 어느 저장소에도 저장되지 않는다.
    const before = storage.writes;
    app._gameData.set('global.priorTell', 'Credits');
    app._gameData.set('SomethingElse', 1);
    assert.equal(storage.writes, before);
    // 새로고침: 같은 저장소로 만든 앱은 진행 값과 바뀐 설정을 모두 이어받는다.
    const reloaded = wiredApp(storage);
    assert.equal(reloaded._gameData.get('DoneTheWarBegins'), '1');
    assert.equal(reloaded._gameData.get('soundVolume'), '5');
    assert.equal(reloaded._gameData.get('musicVolume'), '4');
    assert.equal(reloaded._gameData.get('sound'), '0');
});

// 재시작(init 이 저장소를 새로 읽음)에서는 이미 만든 게임 데이터에 새로 읽은 값을 맞춘다. 다른 탭이 저장한 값이 다음 init 부터 목록에 나타나야 한다.
test('Netstorm 연결: 재시작 때 새로 읽은 저장 값을 기존 게임 데이터에 맞춘다', () => {
    const storage = fakeStorage();
    const app = wiredApp(storage);
    assert.equal(app._gameData.get('DoneTutorial1'), null);
    // 다른 탭이 튜토리얼 1 완료와 음량 변경을 저장했다.
    new Progress({ storage }).set('DoneTutorial1', 1);
    new Settings({ storage }).set('soundVolume', 1);
    // 재시작: 저장소를 다시 읽어 게임 데이터에 옮긴다. 옮기는 동안 되돌아오는 통지(저장)는 없어야 한다.
    const writesBefore = storage.writes;
    app._settings = new Settings({ storage, onChange: (key) => app._onSettingChange(key) });
    app._progress = new Progress({ storage });
    app._syncGameData();
    assert.equal(app._gameData.get('DoneTutorial1'), '1');
    assert.equal(app._gameData.get('soundVolume'), '1');
    assert.equal(storage.writes, writesBefore);
    assert.deepEqual(campaignStates(app._gameData, 'offical1.overview'), ['k', 'c', 'x', 'x', 'x', 'x']);
    // 게임 데이터나 저장소가 아직 없으면 아무 일도 하지 않는다.
    const empty = new Netstorm();
    assert.doesNotThrow(() => empty._syncGameData());
    assert.equal(empty.progress, null);
});
