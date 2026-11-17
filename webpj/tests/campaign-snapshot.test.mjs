// Session 전체 snapshot의 hook 결선·stream 수명과 실제 설치 관문 검사다(WB04-E636).
// 최소 owner 계약 검사는 전체 미션 Load/재로드 성공의 증거가 아니며 실제 Session의 loaded를 바꾸지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { snapshotCampaignTemplate } from '../www/core/raw-campaign-snapshot.js';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { GameRandom } from '../www/core/random.js';
import { SECTION_NAMES, fortSection } from '../www/assets/fort.js';
import { campaignOwners } from './campaign-session-support.mjs';

// 초기화한 실제 manager/stream과 원문 외부 child 관측 장치를 공급하는 최소 owner 계약이다.
function snapshotContract({ mission = true, badges = true, battle = false } = {}) {
    const storage = new CampaignTemplateStorage(), calls = [], records = new Uint8Array(6160);
    storage.install(1, { sections: SECTION_NAMES.map((name) => name === 'Deck' ? Uint8Array.of(0) : new Uint8Array(0)) }, 'snapshot-binding-contract');
    new DataView(records.buffer).setUint32(216, 0x1234, true);
    const owner = { // 전체 게임 대신 결선 경계만 검사한다. 하위 world child의 성공을 주장하지 않는다.
        loaded: true, templateStorage: storage, random: new GameRandom(123), services: {},
        core: { ownerMode: { fort: !battle, battle } }, playerState: { templateOwner: 1 },
        // technologyParent 캡처 뒤 실제 current가 바뀌어도 처음 반환된 stream만 사용해야 한다.
        player: { currentTechnology: () => { calls.push('parent'); return 77; } },
        writeFortTechnology: (parent, stream) => {
            assert.equal(parent, 77); calls.push('technology'); storage.selectSection(1, 'Deck', 0); storage.writeSection(stream, Uint8Array.of(9));
        },
        // 별도 실제 child의 입력만 관측한다. 자식 규칙은 기존 원본 fixture 시험에서 검증한다.
        snapshotFortBasicSection: (slot, names) => { assert.equal(slot, 1); calls.push(names ? 'names' : 'deck'); },
        writeFortObjects: (slot) => { assert.equal(slot, 1); calls.push('objects'); },
        writeFortTerritory: (slot) => { assert.equal(slot, 1); calls.push('territory'); },
        writeFortCoreDataBody: (slot) => { assert.equal(slot, 1); calls.push('core'); },
        templateState: { changed: (slot) => { assert.equal(slot, 1); calls.push('notify'); } },
    };
    if (mission) owner.services.fortMission = {
        // Mission은 이름을 쓴 뒤 다시 조회한다. 다른 current 노드를 선택해 명시 stream 고정을 확인한다.
        current: () => { calls.push('mission'); storage.selectSection(1, 'Deck', 0); return 1; },
        name: () => Uint8Array.of(65, 0), field: (token, offset) => { assert.equal(token, 1); return offset === 0x58 ? 7 : 0x89abcdef; },
    };
    if (badges) owner.services.fortBadges = { records: () => records };
    return { owner, calls, storage, records };
}

// 원문 부모의Fort/Battle 순서와 실제 하위 writer의 고정 stream을 함께 검사한다.
test('CampaignSnapshot 결선: Fort/Battle 분기·동일 stream·현재 Mission 재조회와 장부', () => {
    // 두 모드의 미선택 child가 서로 섞이지 않아야 한다.
    for (const battle of [false, true]) {
        const { owner, calls, storage, records } = snapshotContract({ battle }); const before = records.slice(), random = new GameRandom(123);
        const money = random.next(1010) + 333; snapshotCampaignTemplate(owner, 1, battle);
        assert.deepEqual(calls, battle ? ['parent', 'technology', 'deck', 'objects', 'territory', 'mission', 'mission', 'core', 'notify'] : ['names', 'parent', 'technology', 'objects', 'territory', 'deck', 'mission', 'mission', 'notify']);
        assert.deepEqual([...fortSection(storage.template(1), 'Technology')], [9]);
        assert.deepEqual([...fortSection(storage.template(1), 'Mission')], [1, 0, 65, 7, 0, 0, 0, 239, 205, 171, 137]);
        const savedMoney = fortSection(storage.template(1), 'Money'); assert.equal(savedMoney.length, 4);
        assert.equal(new DataView(savedMoney.buffer, savedMoney.byteOffset, 4).getFloat32(0, true), money);
        assert.deepEqual([...fortSection(storage.template(1), 'Badges')], battle ? [] : [1, 0, 0x34]);
        assert.deepEqual(records, before); assert.equal(owner.random.state, random.state);
    }
});

// 장치 누락을 미리 검사하지 않고 Mission/Badges의 실제 선택 위치 앞 부분 효과를 보존한다.
test('CampaignSnapshot 결선: 늦은 Mission/Badges 누락은 생성/난수/최초 count BYTE를 보존한다', () => {
    const absentMission = snapshotContract({ mission: false });
    assert.throws(() => snapshotCampaignTemplate(absentMission.owner, 1, false), /현재 Mission 읽기 장치 미연결/);
    assert.equal(fortSection(absentMission.storage.template(1), 'Money').length, 4);
    assert.equal(absentMission.storage.currentSection(1), 'Mission'); assert.equal(absentMission.calls.includes('notify'), false);
    const absentBadges = snapshotContract({ badges: false });
    assert.throws(() => snapshotCampaignTemplate(absentBadges.owner, 1, false), /Client Badges 장부 미연결/);
    assert.deepEqual([...fortSection(absentBadges.storage.template(1), 'Badges')], [0]); assert.equal(absentBadges.calls.includes('notify'), false);
    const battle = snapshotContract({ badges: false, battle: true }); snapshotCampaignTemplate(battle.owner, 1, true); assert.equal(battle.calls.at(-1), 'notify');
});

// 실제 기반 owner는 설치 전/반납 뒤 전체 저장을 거부한다. loaded=true로 미구현 Load를 우회하지 않는다.
test('CampaignSnapshot 실제 owner: Load 전·반납 뒤 전체 snapshot의 부분 효과가 없다', () => {
    const owner = campaignOwners();
    try {
        const headers = owner.templateStorage.headers().slice(), pool = owner.core.pool.bytes(), seed = owner.random.state;
        assert.throws(() => owner.snapshotFort(1), /설치 전 전체 Fort/); assert.throws(() => owner.snapshotBattle(1), /설치 전 전체 Battle/);
        assert.throws(() => owner.snapshotFortBody(1), /설치\/슬롯 계약/); assert.throws(() => owner.snapshotTemplateBody(9, true), /설치\/슬롯 계약/);
        assert.deepEqual(owner.templateStorage.headers(), headers); assert.deepEqual(owner.core.pool.bytes(), pool); assert.equal(owner.random.state, seed); assert.equal(owner.loaded, false);
    } finally { owner.dispose(); }
    assert.throws(() => owner.snapshotFort(1), /반납한 Session/); assert.throws(() => owner.snapshotBattle(1), /반납한 Session/);
    assert.throws(() => owner.snapshotFortBody(1), /반납한 Session/); assert.throws(() => owner.snapshotTemplateBody(1, false), /반납한 Session/);
});
