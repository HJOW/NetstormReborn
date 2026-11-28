// 설정/진행 백업의 전체 왕복과 실패 시 기존 저장소 보존을 실제 저장 형식으로 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Settings, SETTINGS_STORAGE_KEY } from '../www/platform/settings.js';
import { Progress, PROGRESS_STORAGE_KEY } from '../www/platform/progress.js';
import { parseSettingsTransfer, serializeSettingsTransfer, storeSettingsTransfer, SETTINGS_TRANSFER_FORMAT } from '../www/platform/settings-transfer.js';
import { buildOptionsItems } from '../www/ui/option-menus.js';
import { Netstorm } from '../www/netstorm.js';

// 같은 저장소의 원문을 관찰하고 지정한 한 번의 쓰기만 거부할 수 있는 장치다.
function memoryStorage() {
    const values = new Map(); let failure = null; // 원문 값과 다음에 거부할 항목 이름이다.
    return { values, fail: (key) => { failure = key; }, getItem: (key) => values.get(key) ?? null, removeItem: (key) => values.delete(key), setItem: (key, text) => { if (failure === key) { failure = null; throw new Error('저장 실패 입력'); } values.set(key, text); } };
}

test('설정 JSON: 모든 현재 설정과 진행을 복사하고 과거 백업으로 정확히 교체한다', () => {
    const storage = memoryStorage(), settings = new Settings({ storage }), progress = new Progress({ storage }); // 실제 두 저장 소유자다.
    settings.set('language', 2); settings.set('difficulty', 3); settings.set('soundVolume', 2); settings.set('skirmishBans', 'outpost,windArcher'); progress.set('DoneTheWarBegins', 1);
    const text = serializeSettingsTransfer(settings, progress), value = parseSettingsTransfer(text); // 사용자가 복사/붙여넣을 실제 JSON이다.
    assert.equal(value.format, SETTINGS_TRANSFER_FORMAT); assert.deepEqual(value.settings, settings.snapshot()); assert.deepEqual(value.progress, progress.snapshot());
    progress.set('DoneTrainingDay', 1); settings.set('language', 1); storage.setItem('unrelated', 'keep'); storage.setItem('netstorm.mods', 'installed');
    storeSettingsTransfer(storage, value);
    assert.deepEqual(new Settings({ storage }).snapshot(), value.settings); assert.deepEqual(new Progress({ storage }).snapshot(), value.progress);
    assert.equal(new Progress({ storage }).get('DoneTrainingDay'), undefined); assert.equal(storage.getItem('unrelated'), 'keep'); assert.equal(storage.getItem('netstorm.mods'), 'installed');
});

test('설정 JSON: 빈 입력은 취소하고 손상/알 수 없는 키/잘못된 값은 적용 전에 거부한다', () => {
    assert.equal(parseSettingsTransfer(' \n\t'), null);
    const value = JSON.parse(serializeSettingsTransfer(new Settings(), new Progress())); // 유효한 내보내기에서 한 입력만 바꾼다.
    assert.throws(() => parseSettingsTransfer('{oops'), (error) => error.code === 'json');
    assert.throws(() => parseSettingsTransfer(JSON.stringify({ ...value, version: 99 })), /format/u);
    assert.throws(() => parseSettingsTransfer(JSON.stringify({ ...value, settings: { ...value.settings, soundVolume: 9 } })), /settings/u);
    assert.throws(() => parseSettingsTransfer(JSON.stringify({ ...value, settings: JSON.parse('{"__proto__":1}') })), /settings/u);
    assert.throws(() => parseSettingsTransfer(JSON.stringify({ ...value, progress: { DoneTest: 1, donetest: 1 } })), /progress/u);
});

test('설정 JSON: 두 번째 저장 실패 시 첫 항목 원문도 복구하고 현재 객체는 바꾸지 않는다', () => {
    const storage = memoryStorage(), settings = new Settings({ storage }), progress = new Progress({ storage }); // 실제 실패/복구 대상이다.
    settings.set('music', 0); progress.set('DoneTheWarBegins', 1);
    const previous = new Map(storage.values), before = settings.snapshot(), value = parseSettingsTransfer(serializeSettingsTransfer(new Settings(), new Progress())); // 복원할 원문과 새로운 입력이다.
    storage.fail(PROGRESS_STORAGE_KEY); assert.throws(() => storeSettingsTransfer(storage, value), (error) => error.code === 'storage');
    assert.deepEqual(storage.values, previous); assert.deepEqual(settings.snapshot(), before);
    assert.ok(storage.getItem(SETTINGS_STORAGE_KEY));
});

test('설정 전송 메뉴: 세 언어의 마지막 두 행이며 원본 비교 메뉴에는 추가하지 않는다', () => {
    for (const [language, labels] of [['english', ['Load Settings', 'Copy Settings']], ['korean', ['설정 불러오기', '설정 복사']], ['japanese', ['設定を読み込む', '設定をコピー']]]) { // 지원 언어를 같은 메뉴 생성기로 확인한다.
        assert.deepEqual(buildOptionsItems(new Settings(), { language }).slice(-2).map((item) => item.label), labels);
    }
    assert.equal(buildOptionsItems(new Settings(), { includeAll: true }).some((item) => item.command?.argument === 'Patch1101.LoadSettings'), false);
});

test('미션 난이도: 메인 메뉴에서만 변경하고 진행 중/로드 중 직접 Tell은 차단한다', () => {
    const app = Object.create(Netstorm.prototype), settings = new Settings(); // 실제 Tell 처리기를 UI/브라우저 없는 현재 문맥에 연결한다.
    Object.assign(app, { _settings: settings, _mainMenu: {}, _gameData: {}, _resources: {}, _campaignWorld: null, _clientGuide: { current: () => null } });
    assert.equal(app._openTell('Patch1101.SetDifficulty(1)'), true); assert.equal(settings.get('difficulty'), 1);
    app._campaignWorld = {}; assert.equal(app._openTell('Patch1101.SetDifficulty(3)'), true); assert.equal(settings.get('difficulty'), 1);
    app._campaignWorld = null; app._clientGuide = { current: () => ({}) };
    assert.equal(app._openTell('Patch1101.SetDifficulty(2)'), true); assert.equal(settings.get('difficulty'), 1);
});
