// 같은 실제 owner의 Template 선택/WorldRuntime·Fort 준비/섬 population 부모 결선 계약이다(WB04-E622).
// 전체 Load·자연 플레이/포획/희생/승리·공중3종 전용 생성/생산/행동은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SECTION_NAMES } from '../www/assets/fort.js';
import { formatFortPriestCount } from '../www/core/raw-campaign-session-install.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { get, put } from './raw-support.mjs';

// 최초 설치에 전달하는 논리 Fort다. 원시 stream/WorldRuntime은 실제 구현을 쓴다.
function fort() { return { sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }; }
// 실제 FE/35섹션 파일 이미지다. 읽기 전 공개 여부와 별도 파일명 인계만 검사한다.
function image() {
    const bytes = [0x46, 0xfe];
    // 원문35개 섹션마다 자기 길이2BYTE만 있는 빈 레코드를 붙인다.
    for (let index = 0; index < SECTION_NAMES.length; index += 1) bytes.push(2, 0);
    return Uint8Array.from(bytes);
}
// 번역/서식의 원문 바이트와 NUL이다. UTF-8 본문도 같은 방식으로 공급한다.
function text(value) { return new TextEncoder().encode(value + '\0'); }
// 검사 외곽 효과가 선택되면 실패한다.
function forbidden() { throw new Error('미선택 외곽 효과'); }

test('Session 최초 설치: 선행/반복/반납과 같은 Template·Runtime·Pool, Load/외곽 미공개', () => {
    const o = campaignOwners(); const terrain = o.terrainModules;
    try {
        o.terrainModules = null; assert.throws(() => o.connectWorldInstall(fort()), /초기화 전/); o.terrainModules = terrain;
        const bytes = o.core.pool.bytes(); const seed = o.random.state; const input = fort(); o.connectWorldInstall(input);
        assert.equal(o.installModules.ready, true); assert.equal(o.templateStorage.template(1), input); assert.equal(o.templateStorage.name(1), 'thewarbegins'); assert.equal(o.installModules.data.headers, o.headers);
        assert.equal(o.runtime.pool, o.core.pool); assert.equal(o.runtime.objects.pool, o.core.pool); assert.equal(o.loaded, false); assert.equal(o.installModules.hooks.decoderDiagnostics, o.metadata.geometry.decoderDiagnostics);
        assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); assert.throws(() => o.connectWorldInstall(input), /반복/);
    } finally { o.terrainModules = terrain; o.dispose(); }
    assert.throws(() => o.connectWorldInstall(fort()), /반납/); assert.throws(() => o.installModules.data.templateAt(1), /반납/);
});

test('Session 지연 파일: localLogin+파일 선택은 최초 Template를 읽거나 공개하지 않는다', () => {
    const o = campaignOwners({ localLogin: true, missionFortFileSelect: forbidden });
    try { o.connectWorldInstall(undefined); assert.equal(o.installModules.ready, true); assert.ok(o.runtime); assert.equal(get(o.headers, 20), 0); assert.equal(o.services.missionFortImage, undefined); }
    finally { o.dispose(); }
});

test('Session 지연 파일 중복: 빈 선행 이미지도 존재하는 optional이며 manager/Runtime 전에 거부', () => {
    const o = campaignOwners({ localLogin: true, missionFortFileSelect: forbidden, missionFortImage: new Uint8Array(0) });
    try { assert.throws(() => o.connectWorldInstall(fort()), /선행 이미지/); assert.equal(get(o.headers, 20), 0); assert.equal(o.runtime, null); assert.throws(() => o.connectWorldInstall(fort()), /반복/); }
    finally { o.dispose(); }
});

test('Session 읽은 파일: 실제 FE/섹션 reader로 설치한 뒤에만 선행 이미지를 반납', () => {
    const bytes = image(); const o = campaignOwners({ missionFortImage: bytes, missionFortFileName: 'd/loaded.fort' });
    try { o.connectWorldInstall(fort()); assert.ok(get(o.headers, 20)); assert.equal(o.templateStorage.name(1), 'd/loaded.fort'); assert.equal(o.services.missionFortImage, null); assert.equal(bytes[0], 0x46); }
    finally { o.dispose(); }
    const broken = campaignOwners({ missionFortImage: Uint8Array.of(1, 2) }); // source reader의 실제 파일 거부 입력이다.
    try { assert.throws(() => broken.connectWorldInstall(fort())); assert.ok(broken.services.missionFortImage); assert.equal(get(broken.headers, 20), 0); assert.equal(broken.runtime, null); }
    finally { broken.dispose(); }
});

test('Session install 훅: 값형 함수 복사·현재 월드 부모·미선택 장치의 최초 실패', () => {
    const calls = []; const install = { setConfig: (...args) => calls.push(args) }; const o = campaignOwners({ install }); install.setConfig = forbidden;
    try {
        o.connectWorldInstall(fort()); const hooks = o.installModules.hooks; o.services.install.setConfig = forbidden;
        hooks.setConfig('name', Uint8Array.of(3)); assert.deepEqual(calls, [['name', Uint8Array.of(3)]]);
        assert.throws(() => hooks.translateYou(), /cpp:3476/); assert.throws(() => hooks.builderAssertion(), /cpp:3478/);
        o.globals.missionPost = true; const pool = o.core.pool.bytes(); o.runtime.finishMissionPriests(); assert.deepEqual(o.core.pool.bytes(), pool);
    } finally { o.dispose(); }
});

test('Session 사제 준비: 실제 현재 Player 기술 부모의 Form6·기본/테마 TYPE와 중복 방지', () => {
    const o = campaignOwners();
    try {
        const input = fort(); input.sections[1] = new Uint8Array(28); input.sections[1][17] = 1; o.connectWorldInstall(input);
        const parent = o.world.create(152); o.core.pool.allocated(parent)[34] = 1; put(o.players, 180 + 0x9c, parent); // 실제 기술 부모를 가리키는 검사 입력이며 Player Pop/미션 시작이 아니다.
        o.globals.missionPost = false; o.globals.mySubscriber = 4; o.playerState.templateOwner = 1; o.playerState.battleOwner = 1;
        const before = o.core.pool.freeCount; o.runtime.finishMissionPriests(); assert.equal(o.core.pool.freeCount, before - 4);
        // 원문 기본3개와 현재 subscriber%3의 테마1개가 같은 부모에 실제 부착됐다.
        for (const type of [o.templeProduct, o.tutorialArcherType, 128, 76]) { const sid = o.priests.carrier.findContained(parent, 4, type, 0); assert.ok(sid); assert.equal(get(o.core.pool.slot(sid), 18, 2), type); }
        o.runtime.finishMissionPriests(); assert.equal(o.core.pool.freeCount, before - 4);
        o.tutorialTypes.set(0x541258, 129); o.runtime.finishMissionPriests(); assert.equal(o.core.pool.freeCount, before - 5);
    } finally { o.dispose(); }
});

test('Session population: 같은 실제 Pool·누락된 섬/진단 관문은 생성/난수 효과 전에 거부', () => {
    const o = campaignOwners();
    try {
        o.connectWorldInstall(fort()); const pool = o.core.pool.bytes(); const seed = o.random.state;
        o.installModules.withPopulation('선택 장면', (body) => { assert.equal(body.pool, o.core.pool); assert.throws(() => body.populate(99), /population 진단/); });
        assert.throws(() => o.installModules.populateAlternate({ bounds: { left: -1, top: 1, right: 15, bottom: 15 } }), /실제 진단 미연결/);
        assert.deepEqual(o.core.pool.bytes(), pool); assert.equal(o.random.state, seed);
    } finally { o.dispose(); }
});

test('Fort 사제 수 서식: 독립 Windows CRT 계산6장면·UTF-8 본문·NUL/256BYTE 경계', () => {
    // msvcrt._snprintf를 직접 호출한 독립 계산값이다. 게임/원본 실행 결과로 세지 않는다.
    for (const [format, count, expected] of [['%d Priest%s', 0, '0 Priests'], ['%d Priest%s', 1, '1 Priest'], ['%d Priest%s', 2, '2 Priests'], ['%+05d Priest%-3s!', 2, '+0002 Priests  !'], ['%.0d Priest%.999s', 0, ' Priests'], ['%% %04.3d %s', 7, '%  007 s']]) assert.deepEqual(formatFortPriestCount(text(format), count), text(expected));
    assert.deepEqual(formatFortPriestCount(text('사제 %d명'), 2), text('사제 2명'));
    assert.throws(() => formatFortPriestCount(Uint8Array.of(65), 1), /NUL/); assert.throws(() => formatFortPriestCount(text('%256d%s'), 1), /256BYTE/);
    assert.throws(() => formatFortPriestCount(text('%f'), 1), /미복원/);
});
