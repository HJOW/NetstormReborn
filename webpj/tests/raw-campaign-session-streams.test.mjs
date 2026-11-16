// 같은 실제 Template·255BYTE 변환표·World 로더의 원시 reader 결선 계약이다(WB04-E621).
// 공중3종 새 전용 생성/생산/행동·실제 미션 플레이를 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { RawCampaignWorldRuntime } from '../www/core/raw-campaign-world-runtime.js';
import { makeCampaignFortContentsReader } from '../www/core/raw-campaign-session-streams.js';
import { loadFortContents } from '../www/core/raw-fort-contents-load.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { get, put } from './raw-support.mjs';

// 독립 원본 계약의 Alpha/Beta hash와 실제 내부 TYPE 두 번호다. 제품 hash 함수를 기대값으로 쓰지 않는다.
const ALPHA = 0x68706ca2, BETA = 0x61746542, FIRST = 75, SECOND = 76;
// 실제 칸을 빌리는 참조다.
function reference(owner, key) { return { get value() { return owner[key]; }, set value(value) { owner[key] = value; } }; }
// 미선택 UI/부모 효과의 명시 실패다.
function forbidden() { throw new Error('선택하지 않은 외곽 효과'); }
// 현재 원문 이름 표와 버전만 있는20 Terr/Chaff를 실제 Template manager에 설치한다.
function install(o, storage = o.templateStorage, hashes = [ALPHA, BETA]) {
    storage.install(1, { sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }, 'streams.fort');
    const names = new Uint8Array(1 + hashes.length * 4); names[0] = hashes.length;
    // 각 저장 이름 hash DWORD를 원문 순서에 쓴다.
    for (let index = 0; index < hashes.length; index += 1) put(names, 1 + index * 4, hashes[index]);
    storage.writeSection(storage.createSection(1, 'TypeNames'), names);
    // 읽기 범위를 명시해 누락 파일을 빈 성공으로 대체하지 않는다.
    for (const name of [...Array.from({ length: 20 }, (_, index) => `Terr${String(index).padStart(2, '0')}`), 'Chaff']) storage.writeSection(storage.createSection(1, name), Uint8Array.of(1));
    o.core.types[FIRST].name = 'Alpha'; o.core.types[SECOND].name = 'Beta';
}
// 아직 외곽 Session에 게시하지 않은 실제 기존 WorldRuntime의 같은 객체 로더다.
function objects(o) {
    return new RawCampaignWorldRuntime(o.world, o.chunks, o.islands, o.player, o.normal, o.random, o.hp, o.ownership, o.finish, o.contained, o.objectState, o.globals, reference(o, 'piece'), reference(o, 'noRegion'),
        { headers: o.headers, handle: o.handle, templateAt: (slot) => o.templateStorage.template(slot), territoryAt: (slot) => o.templateStorage.selectTerritory(slot) },
        { translateYou: forbidden, setConfig: forbidden, refreshWindowTitle: forbidden, resetCamera: forbidden, builderAssertion: forbidden, territoryAssertion: forbidden,
            notifySurface: (sid) => o.core.building.notifySurface(sid), setState: (sid, value) => o.assetBindings.setState(sid, value), virtualCount: (sid) => o.rewardCount(sid) | 0 }).objects;
}

test('Session TypeNames: 실제 현재 이름/manager·동일255BYTE 변환표와 World 깊이', () => {
    const o = campaignOwners();
    try {
        install(o); const before = o.objectState.loadingDepth; const seed = o.random.state; o.prepareFortTypeNames(1);
        const token = o.fortTypeConversion.current().value; const bytes = o.fortTypeConversion.bytes(token); // 실제 같은 할당이다.
        assert.equal(bytes.length, 255); assert.deepEqual([...bytes.subarray(0, 3)], [FIRST, SECOND, 0]); assert.equal(o.objectState.loadingDepth, before); assert.equal(o.state.carrierState.loadingDepth, before); assert.equal(o.random.state, seed);
        const selected = o.templateStorage.currentWriteStream(1); o.templateStorage.accessSection(selected, (fields) => assert.deepEqual([get(fields, 4), get(fields, 8)], [1, 9]));
        o.fortTypeConversion.release(token); o.fortTypeConversion.current().value = 0;
    } finally { o.dispose(); }
    assert.throws(() => o.prepareFortTypeNames(1), /반납/);
});

test('Session TypeNames: 지역 Template를 읽어도 전역 manager는 그대로이고 같은 변환표만 갱신', () => {
    const o = campaignOwners(); const local = new CampaignTemplateStorage();
    try {
        install(o); install(o, local, [BETA, ALPHA]); const headers = o.headers.slice(); const globalSection = o.templateStorage.currentSection(1);
        o.prepareFortTypeNames(1, local); assert.deepEqual(o.headers, headers); assert.equal(o.templateStorage.currentSection(1), globalSection);
        assert.deepEqual([...o.fortTypeConversion.bytes(o.fortTypeConversion.current().value).subarray(0, 2)], [SECOND, FIRST]);
        assert.throws(() => o.prepareFortTypeNames(9), /소유자 미연결/);
    } finally { local.clearAll(); o.dispose(); }
});

test('Session Objects: 실제 WorldRuntime 로더의20 Terr/Chaff 커서·변환표 정상 반납·깊이', () => {
    const o = campaignOwners();
    try {
        install(o); const loader = objects(o); const before = o.objectState.loadingDepth; const bytes = o.core.pool.bytes(); const seed = o.random.state;
        o.readFortObjectStreams(loader, 1); assert.equal(o.fortTypeConversion.current().value, 0); assert.equal(o.objectState.loadingDepth, before); assert.equal(o.objectState.version, 1);
        assert.equal(o.templateStorage.currentSection(1), 'Chaff'); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed);
        const current = o.templateStorage.currentWriteStream(1); o.templateStorage.accessSection(current, (fields) => assert.equal(get(fields, 4), 1));
    } finally { o.dispose(); }
    assert.throws(() => o.readFortObjectStreams(null, 1), /반납/);
});

test('Session Objects: 부분 Terr 범위와 지역 manager도 같은 raw reader를 사용', () => {
    const o = campaignOwners(); const local = new CampaignTemplateStorage();
    try {
        install(o); install(o, local); const globalSection = o.templateStorage.currentSection(1); const loader = objects(o);
        o.objectState.chaffMode = false; o.readFortObjectStreams(loader, 1, 3, 5, local);
        assert.equal(local.currentSection(1), 'Terr04'); assert.equal(o.templateStorage.currentSection(1), globalSection); assert.equal(o.fortTypeConversion.current().value, 0);
        assert.throws(() => o.readFortObjectStreams(loader, 1, 2, 2), /1017/);
    } finally { local.clearAll(); o.dispose(); }
});

test('Session reader 실패: 기존 변환 진단·중첩 unwind의 현재 할당/stream 접두를 보존', () => {
    const o = campaignOwners();
    try {
        install(o); const before = o.objectState.loadingDepth; o.prepareFortTypeNames(1); // 현재 변환표가 이미 설치된 원문 진단 입력이다.
        const token = o.fortTypeConversion.current().value; assert.ok(token); assert.equal(o.fortTypeConversion.bytes(token).length, 255); assert.equal(o.objectState.loadingDepth, before); assert.deepEqual([...o.fortTypeConversion.bytes(token).subarray(0, 2)], [FIRST, SECOND]);
        assert.throws(() => o.prepareFortTypeNames(1), /TypeNames 진단 미연결/); assert.equal(o.objectState.loadingDepth, before);
        assert.throws(() => o.readFortObjectStreams(objects(o), 1), /TypeNames 진단 미연결/); assert.equal(o.objectState.loadingDepth, before); assert.equal(o.fortTypeConversion.current().value, token);
    } finally { o.dispose(); }
});

test('Session Contents: 실제 현재 TYPE/부모·변환표와 ContainedCreate/Form6 부착', () => {
    const o = campaignOwners();
    try {
        install(o); o.prepareFortTypeNames(1); const parent = o.world.create(FIRST); const raw = o.core.pool.allocated(parent); put(raw, 0, 0x50ee08); // 기존 Player 통지의 실제 입력 VT이며 세계에는 Pop하지 않는다.
        const record = o.core.types[FIRST]; const input = [1, 0]; let quantity = 0; // 한 항목의 저장 종류0은 현재 TypeNames로 FIRST에 매핑된다.
        if (record.flags1 & 0x1000) { input.push(0x23); quantity = 0x23; }
        if (record.flags1 & 0x2000) { input.push(0x34, 0x12); quantity = 0x1234; }
        const listFlags = record.contentListFlags || record.containerListFlags || 0x20;
        if (!record.contentListFlags && !record.containerListFlags) input.push(listFlags);
        if (record.flags1 & 0x20000) input.push(0); // 실제 container 형식이면 자식 목록0을 덧붙인다.
        const stream = o.templateStorage.createSection(1, 'Contents'); o.templateStorage.writeSection(stream, Uint8Array.from(input)); const selected = o.templateStorage.selectSection(1, 'Contents', 1);
        const reader = makeCampaignFortContentsReader(o, (token, operation) => { assert.equal(token, 1); o.templateStorage.accessSection(selected, operation); });
        assert.equal(loadFortContents(parent, 1, 9, 0, { get conversion() { return o.fortTypeConversion.current().value; }, checking: () => o.state.gunState.checking }, reader), true);
        const child = get(o.core.pool.slot(parent), 6, 2); assert.ok(child); const content = o.core.pool.slot(child);
        assert.deepEqual([content[10], get(content, 18, 2), get(content, 12, 2), get(content, 20, 2), content[30]], [6, FIRST, quantity, (listFlags & ~0x100) & 0xffff, 0]);
        const original = o.contained; o.contained = null; assert.throws(() => reader.create(parent, FIRST, 0, 0, 0, 0), /미연결/); o.contained = original;
        assert.throws(() => reader.parentListFlags(65536), /Pool 범위/);
    } finally { o.dispose(); }
});
