// 실제 Session 기반의 현재 Core/CoreData·Client seed·같은 SP stream 결선을 검사한다(WB04-E635).
// 근거: RawCampaignSession.cpp4447..4482·4488..4509와 ClientFortCoreSeedTests.cpp. 전체 Load/snapshot 성공 검사는 후속이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { get, put } from './raw-support.mjs';
import { SECTION_NAMES, fortSection } from '../www/assets/fort.js';
import { ClientRandomGlobals, GameRandom } from '../www/core/random.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { RawPriestRitualActions } from '../www/core/raw-priest-ritual-actions.js';
import { RawTempleRitual, TempleRitualState } from '../www/core/raw-temple-ritual.js';

// 실제 저장소의 공개 설치 API에 명시적인 빈 섹션 입력만 준다. loaded는 바꾸지 않는다.
function installCoreTemplate(owner, slot = 1) { owner.templateStorage.install(slot, { sections: Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)) }, 'current-core-component'); }
// 실제88BYTE 출력에서 보호bit만 읽는다. 잡음 기대값이나 writer의 난수 선택은 계산하지 않는다.
function decodedSp(bytes, origin) { let value = 0, at = origin; /* 원문8BYTE/8WORD/16DWORD의 보호bit 순서다. */ for (let bit = 0; bit < 32; bit++) { const width = bit < 8 ? 1 : bit < 16 ? 2 : 4; value |= get(bytes, at, width) & (1 << bit); at += width; } return value >>> 0; }
// 가상 count가 선택하지 않는 의식 효과도 빈 성공으로 대신하지 않는다.
function unselectedRitualEffect() { throw new Error('현재 Core count에서 선택하지 않은 의식 효과'); }

// 같은 실제 다이스/가상80을 지나며 복사한 bytes와 실제 물리 뷰의 혼동을 잡는다. 의식/포획/희생은 실행하지 않는다.
test('CampaignCurrentCore: 실제 다이스 가상80은 같은 Pool의 물리 뷰를 사용한다', () => {
    const owner = campaignOwners();
    try {
        const c = owner.core; owner.coreDataGlobals.select(owner.playerState, 0, 1, 1);
        const footprint = new RawSquidNeighbors(c.pool, c.hash, c.spots, c.types, c.frames);
        const actions = new RawPriestRitualActions(c.pool, c.frames, { setFrame: unselectedRitualEffect, reserveShared: unselectedRitualEffect, addShared: unselectedRitualEffect });
        owner.templeRitual = new RawTempleRitual(c.pool, c.hash, c.types, c.frames, c.shapes, c.spots, footprint, owner.hp, actions, new TempleRitualState());
        const sid = owner.world.create(165), raw = c.pool.allocated(sid); raw[34] = 1; put(raw, 36, c.frames[165].run(0x4c).first);
        assert.equal(owner.rewardCount(sid), 3); const before = c.pool.bytes(); owner.refreshCurrentPlayerCore();
        assert.equal(owner.coreDataGlobals.read(owner.playerState, 0, {})[5], 2); assert.deepEqual(c.pool.bytes(), before); assert.equal(owner.loaded, false);
    } finally { owner.dispose(); }
});

// 현재 포인터를 공개 선택으로 준비하고 같은 Player2만 갱신한다. 설치 완료/게임 진행으로 세지 않는다.
test('CampaignCurrentCore: 실제 현재 Player/Core 포인터·기술 finder와 Pool을 공유한다', () => {
    const owner = campaignOwners();
    try {
        owner.coreDataGlobals.select(owner.playerState, 0, 2, 1); const selected = owner.players.subarray(2 * 180 + 108, 2 * 180 + 118); selected.set([9, 8, 7, 6, 5, 4, 255, 255, 255, 255]);
        const before = owner.players.slice(), pool = owner.core.pool.bytes().slice(), seed = owner.random.state;
        owner.refreshCurrentPlayerCore(); assert.equal(owner.coreDataGlobals.read(owner.playerState, 0, {}).byteOffset, selected.byteOffset);
        assert.deepEqual([...selected], [9, 8, 7, 6, 5, 4, 0, 0, 0, 0]); before.set(selected, 2 * 180 + 108); assert.deepEqual(owner.players, before);
        assert.deepEqual(owner.core.pool.bytes(), pool); assert.equal(owner.random.state, seed); assert.equal(owner.loaded, false);
    } finally { owner.dispose(); }
    assert.throws(() => owner.refreshCurrentPlayerCore(), /반납한 Session/);
});

// 같은 Client의 고정 seed와 실제 일반/인라인 writer를 한 새 CoreData 노드에 연결한다.
test('CampaignCoreWrite: 실제 현재 Player10BYTE·같은 seed DWORD·세 SP 기록과1536회 난수 소비', () => {
    const randomGlobals = new ClientRandomGlobals(), owner = campaignOwners({ randomGlobals });
    try {
        installCoreTemplate(owner); owner.coreDataGlobals.select(owner.playerState, 0, 1, 1); owner.globals.missionPost = false;
        const core = owner.players.subarray(180 + 108, 180 + 118); core.set([0x34, 0x12, 0x78, 0x56, 42, 7, 255, 255, 255, 255]); randomGlobals.fortCoreSeed = 0xabcd1234;
        const random = new GameRandom(owner.random.state), pools = owner.sp.pools().slice(), pool = owner.core.pool.bytes().slice();
        // 세 SP child가 소비하는 전체1536회와 같은 공유 스트림의 최종 상태를 비교한다.
        for (let index = 0; index < 1536; index++) random.next(32);
        owner.writeFortCoreDataBody(1); const bytes = fortSection(owner.templateStorage.template(1), 'CoreData');
        assert.equal(bytes.length, 276); assert.deepEqual(bytes.subarray(0, 10), core); assert.deepEqual([...bytes.subarray(10, 12)], [0x34, 0x12]);
        assert.deepEqual([decodedSp(bytes, 12), decodedSp(bytes, 100), decodedSp(bytes, 188)], [42, 0x1234, 0x5678]);
        assert.equal(randomGlobals.fortCoreSeed, 0xabcd1234); assert.equal(owner.random.state, random.state); assert.equal(owner.random, randomGlobals.game);
        assert.deepEqual(owner.sp.pools(), pools); assert.deepEqual(owner.core.pool.bytes(), pool); assert.equal(owner.templateStorage.currentSection(1), 'CoreData'); assert.equal(owner.loaded, false);
    } finally { owner.dispose(); }
    assert.throws(() => owner.writeFortCoreDataBody(1), /반납한 Session/);
});

// Client seed 미주입도 post 자연 반환 뒤 실제 선택 위치에서만 실패한다. fallback seed를 만들지 않는다.
test('CampaignCoreWrite: post 자연 반환과 늦은 Client seed 누락 앞의10BYTE 부분 쓰기', () => {
    const owner = campaignOwners();
    try {
        installCoreTemplate(owner); owner.coreDataGlobals.select(owner.playerState, 0, 1, 1); const order = owner.templateStorage.sectionOrder(1), seed = owner.random.state;
        owner.writeFortCoreDataBody(1); assert.deepEqual(owner.templateStorage.sectionOrder(1), order); assert.equal(fortSection(owner.templateStorage.template(1), 'CoreData').length, 0);
        owner.globals.missionPost = false; assert.throws(() => owner.writeFortCoreDataBody(1), /실제 장치 누락/);
        assert.equal(fortSection(owner.templateStorage.template(1), 'CoreData').length, 10); assert.equal(owner.random.state, seed); assert.equal('fortCoreSeed' in owner.randomGlobals, false); assert.equal(owner.loaded, false);
    } finally { owner.dispose(); }
});

// 원문 manager 진단은 post보다 먼저이고 실제 Core 포인터 누락은 Create 뒤/일반 쓰기 앞이다.
test('CampaignCoreWrite: manager/현재 Core 포인터 진단은 원문 위치의 부분 효과를 유지한다', () => {
    const owner = campaignOwners({ randomGlobals: new ClientRandomGlobals() });
    try {
        assert.throws(() => owner.writeFortCoreDataBody(1), /1119/); installCoreTemplate(owner); owner.globals.missionPost = false;
        assert.throws(() => owner.writeFortCoreDataBody(1), /실제 store 물리 범위/); assert.equal(owner.templateStorage.currentSection(1), 'CoreData'); assert.equal(fortSection(owner.templateStorage.template(1), 'CoreData').length, 0);
        owner.services.randomGlobals.fortCoreSeed = 0xabcd0000; owner.coreDataGlobals.select(owner.playerState, 0, 1, 1); owner.globals.missionPost = true; owner.writeFortCoreDataBody(1); assert.equal(owner.services.randomGlobals.fortCoreSeed, 0xabcd0000);
        assert.throws(() => owner.writeFortCoreDataBody(9), /9슬롯 범위/); assert.equal(owner.loaded, false);
    } finally { owner.dispose(); }
});
