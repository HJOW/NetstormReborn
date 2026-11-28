// 12차 스커미시 순수 규칙(설정 저장, Tell 대상 해석, 화면 본문, 시작 대본·섬 선택, 빈 월드 fort)을 게임 실행 없이 검사한다. cpppj `SkirmishSetupTests.cpp` 의 시험을 옮기고 mt19937·문구 치환 경계를 더했다. (2026-11-20 WB04-E1101)
import test from 'node:test';
import assert from 'node:assert/strict';
import {
    Mt19937, SKIRMISH_BANNABLE, SKIRMISH_MAX_AI, SKIRMISH_MAX_BANS, SKIRMISH_START_ISLANDS, applySkirmishAction, buildSkirmishLaunch, buildSkirmishScreen,
    createSkirmishFileSource, createSkirmishSettings, isSkirmishMissionName, loadSkirmishSettings, makeSkirmishEmptyWorldFort, makeSkirmishWorldPlan, parseSkirmishTarget, saveSkirmishSettings,
} from '../www/core/skirmish-setup.js';
import { AiDifficulty } from '../www/core/ai-difficulty.js';
import { TERRITORY_PATTERNS } from '../www/assets/territory-patterns.js';

// 설정 저장소를 흉내 내는 맵(키 → 값 문자열)이다. (2026-11-20 WB04-E1101)
function fakeConfig(values = {}) {
    const map = new Map(Object.entries(values));
    return { map, get: (key) => map.get(key) ?? '', set: (key, value) => { map.set(key, value); } };
}
const has = (text, part) => text.includes(part);

test('Tell 대상 해석: 종류와 인자를 소문자 기준으로 읽고 스커미시가 아니면 none', () => {
    assert.equal(parseSkirmishTarget('Patch1101.Skirmish').kind, 'main');
    assert.equal(parseSkirmishTarget('Patch1101.SkirmishCount').kind, 'count');
    assert.deepEqual(parseSkirmishTarget('Patch1101.SkirmishAi(3)'), { kind: 'ai', args: ['3'] });
    assert.deepEqual(parseSkirmishTarget('Patch1101.SkirmishSet(ban,4,1)'), { kind: 'set', args: ['ban', '4', '1'] });
    assert.deepEqual(parseSkirmishTarget('Patch1101.SkirmishSet(ban;4;1)'), { kind: 'set', args: ['ban', '4', '1'] });
    assert.equal(parseSkirmishTarget('Patch1101.SkirmishStart').kind, 'start');
    assert.equal(parseSkirmishTarget('patch1101.skirmishbans(2)').kind, 'bans');
    assert.equal(parseSkirmishTarget('Patch1101.Credits').kind, 'none'); assert.equal(parseSkirmishTarget('UCampaign').kind, 'none');
    // 닫는 괄호가 없으면 인자 없이 종류만 읽고, 모르는 이름은 인자가 있어도 none 이다.
    assert.deepEqual(parseSkirmishTarget('Patch1101.SkirmishAi(3'), { kind: 'ai', args: [] });
    assert.equal(parseSkirmishTarget('Patch1101.SkirmishZzz(1)').kind, 'none');
    assert.deepEqual(parseSkirmishTarget('Patch1101.SkirmishSet(a,,b)').args, ['a', 'b']);
    assert.ok(isSkirmishMissionName('skirmish') && isSkirmishMissionName('Skirmish') && !isSkirmishMissionName('Skirmish2'));
});

test('동작 적용: AI 수, AI별 난이도/동맹, 금지 유닛 토글(최대 5개), 모두 해제와 돌아갈 화면', () => {
    const settings = createSkirmishSettings();
    assert.equal(applySkirmishAction(settings, ['count', '3']), 'Patch1101.Skirmish'); assert.equal(settings.aiCount, 3);
    applySkirmishAction(settings, ['count', '99']); assert.equal(settings.aiCount, SKIRMISH_MAX_AI); // 범위 밖은 최대로 줄인다.
    applySkirmishAction(settings, ['count', 'x']); assert.equal(settings.aiCount, SKIRMISH_MAX_AI); // 해석 못 하면 그대로다.
    applySkirmishAction(settings, ['count', '0']); assert.equal(settings.aiCount, 1); applySkirmishAction(settings, ['count', '-4']); assert.equal(settings.aiCount, 1); // 0 이하는 최소 1 이다.
    applySkirmishAction(settings, ['count', '3']); applySkirmishAction(settings, ['count', '99999999999']); assert.equal(settings.aiCount, 3); // 32비트 밖 정수는 해석 실패라 그대로다.
    applySkirmishAction(settings, ['count', '7']);
    assert.equal(applySkirmishAction(settings, ['diff', '3', '1']), 'Patch1101.SkirmishAi(3)'); assert.equal(settings.ai[1].difficulty, AiDifficulty.Easy);
    assert.equal(applySkirmishAction(settings, ['ally', '3', '1']), 'Patch1101.SkirmishAi(3)'); assert.equal(settings.ai[1].ally, true);
    applySkirmishAction(settings, ['ally', '3', '0']); assert.equal(settings.ai[1].ally, false);
    applySkirmishAction(settings, ['ally', '3', '2']); assert.equal(settings.ai[1].ally, true); // 0 이 아닌 값은 모두 같은 편이다.
    applySkirmishAction(settings, ['ally', '3', '0']);
    // 범위 밖 소유자는 2..8 로 줄인다.
    applySkirmishAction(settings, ['diff', '99', '3']); assert.equal(settings.ai[6].difficulty, AiDifficulty.Hard);
    applySkirmishAction(settings, ['diff', '0', '3']); assert.equal(settings.ai[0].difficulty, AiDifficulty.Hard);
    // 금지 유닛은 5개까지만 받고 같은 항목을 다시 고르면 해제한다.
    for (let index = 0; index < 7; index += 1) applySkirmishAction(settings, ['ban', String(index), '0']);
    assert.equal(settings.bans.length, SKIRMISH_MAX_BANS);
    assert.equal(applySkirmishAction(settings, ['ban', '0', '0']), 'Patch1101.SkirmishBans(0)'); assert.equal(settings.bans.length, SKIRMISH_MAX_BANS - 1);
    applySkirmishAction(settings, ['ban', '26', '2']); // 마지막 후보(outpost)도 고를 수 있다.
    assert.equal(settings.bans.at(-1), 'outpost');
    assert.equal(applySkirmishAction(settings, ['ban', '27', '-5']), 'Patch1101.SkirmishBans(0)'); // 후보 밖 번호는 무시하고 음수 쪽은 0 이다.
    assert.equal(applySkirmishAction(settings, ['clearbans']), 'Patch1101.SkirmishBans(0)'); assert.equal(settings.bans.length, 0);
    assert.equal(applySkirmishAction(settings, ['unknown']), 'Patch1101.Skirmish'); assert.equal(applySkirmishAction(settings, []), 'Patch1101.Skirmish');
    // 인자가 모자란 동작은 아무것도 바꾸지 않는다.
    const before = JSON.stringify(settings);
    assert.equal(applySkirmishAction(settings, ['diff', '3']), 'Patch1101.Skirmish'); assert.equal(applySkirmishAction(settings, ['ban', '1']), 'Patch1101.Skirmish'); assert.equal(JSON.stringify(settings), before);
});

test('설정 저장/읽기 왕복과 잘못된 저장 값의 정리', () => {
    const config = fakeConfig(), settings = createSkirmishSettings();
    settings.aiCount = 4; settings.ai[0] = { difficulty: AiDifficulty.Hard, ally: true }; settings.ai[2] = { difficulty: AiDifficulty.Easy, ally: false }; settings.bans = ['outpost', 'sunCannon'];
    saveSkirmishSettings(config.set, settings);
    assert.equal(config.map.get('skirmishAiCount'), '4'); assert.equal(config.map.get('skirmishAi2Diff'), '3'); assert.equal(config.map.get('skirmishAi2Ally'), '1'); assert.equal(config.map.get('skirmishBans'), 'outpost,sunCannon');
    const loaded = loadSkirmishSettings(config.get);
    assert.equal(loaded.aiCount, 4); assert.deepEqual(loaded.ai[0], { difficulty: AiDifficulty.Hard, ally: true }); assert.deepEqual(loaded.ai[2], { difficulty: AiDifficulty.Easy, ally: false });
    assert.deepEqual(loaded.bans, settings.bans);
    const bad = fakeConfig({ skirmishAiCount: '99', skirmishBans: 'outpost,notAType,sunCannon,sunCannon,windBattery,rainBattery,thunderBattery,bulf' });
    const cleaned = loadSkirmishSettings(bad.get);
    assert.equal(cleaned.aiCount, SKIRMISH_MAX_AI); assert.deepEqual(cleaned.bans, ['outpost', 'sunCannon', 'windBattery', 'rainBattery', 'thunderBattery']); // 모르는 이름/중복은 버리고 5개까지만 받는다.
    assert.equal(loadSkirmishSettings(fakeConfig().get).aiCount, 1); // 값이 없으면 기본값이다.
    assert.equal(loadSkirmishSettings(() => undefined).aiCount, 1); // 저장소가 undefined 를 돌려줘도 기본값이다.
    assert.equal(loadSkirmishSettings(fakeConfig({ skirmishAiCount: '0' }).get).aiCount, 1); // 0 은 최소 1 로 올린다.
    assert.equal(loadSkirmishSettings(fakeConfig({ skirmishAi3Ally: '2' }).get).ai[1].ally, true); // 저장된 동맹 값은 0 이 아니면 같은 편이다.
    assert.equal(loadSkirmishSettings(fakeConfig({ skirmishAi5Diff: '9' }).get).ai[3].difficulty, AiDifficulty.Normal); // 범위 밖 난이도는 보통이다.
});

test('시작 대본: AI 수만큼 소유자 2..N+1 이 있고 동맹/금지/AI 이름이 설정을 따른다', () => {
    assert.ok(SKIRMISH_START_ISLANDS.length >= SKIRMISH_MAX_AI + 1); // 후보는 최대 참가자(사람 1 + AI 7)보다 많아야 한다.
    const settings = createSkirmishSettings();
    settings.aiCount = 3; settings.ai[0].ally = true; settings.ai[1].ally = false; settings.bans = ['outpost', 'sunCannon'];
    const launch = buildSkirmishLaunch(settings, 7);
    assert.equal(launch.islands.length, 4); // 사람 1 + AI 3 = 시작 섬 4개이며 서로 다른 후보다.
    assert.equal(new Set(launch.islands.map((island) => `${island.fort}:${island.territory}`)).size, 4);
    assert.equal(launch.mapSize, 2); // 4명은 링 간격 5칸(크기 2)이다.
    assert.equal(launch.central.length, 6); assert.equal(launch.central[2], 0); assert.ok((launch.central[0] & 63) < 68); // 중앙섬 기록은 원점 배치(위치 0)이고 패턴 번호가 영역 패턴 표 안이다.
    assert.equal(launch.central[1], 0x09);
    const { script } = launch;
    assert.ok(has(script, 'ai2Name=') && has(script, 'ai3Name=') && has(script, 'ai4Name=') && !has(script, 'ai5Name='));
    assert.ok(has(script, 'myAllyList="2"')); // 같은 편은 AI 2뿐이다.
    assert.ok(has(script, 'ai2AllyList="1;2"') && has(script, 'ai3AllyList="3;4"') && has(script, 'ai4AllyList="3;4"')); // 적끼리도 같은 편이다.
    assert.ok(has(script, 'techAllowed="deny;outpost;sunCannon"') && has(script, 'randGeysers=1') && has(script, '[Succeeded][BadTeamDead]') && has(script, '[Failed][GoodTeamDead]'));
    assert.equal(launch.script, buildSkirmishLaunch(settings, 7).script); // 같은 난수는 같은 시작 자료다.
    assert.deepEqual(buildSkirmishLaunch(settings, 7).islands, launch.islands); assert.deepEqual([...buildSkirmishLaunch(settings, 7).central], [...launch.central]);
    assert.ok(!has(buildSkirmishLaunch(createSkirmishSettings(), 0).script, 'techAllowed')); // 금지가 없으면 줄 자체가 없다.
    // 난수에 따라 후보가 달라질 수 있다.
    let different = false;
    for (let value = 1; value < 10; value += 1) {
        const other = buildSkirmishLaunch(settings, value);
        different = different || other.islands[0].fort !== launch.islands[0].fort || other.islands[0].territory !== launch.islands[0].territory || other.central.join() !== launch.central.join();
    }
    assert.ok(different);
    // 참가자 수에 따라 맵 크기가 커진다(2명=1, 3~4명=2, 5명 이상=3).
    const size = createSkirmishSettings();
    size.aiCount = 1; assert.equal(buildSkirmishLaunch(size, 1).mapSize, 1); assert.equal(buildSkirmishLaunch(size, 1).islands.length, 2);
    size.aiCount = 2; assert.equal(buildSkirmishLaunch(size, 1).mapSize, 2); // 참가자 3명
    size.aiCount = 3; assert.equal(buildSkirmishLaunch(size, 1).mapSize, 2); // 참가자 4명
    size.aiCount = 4; assert.equal(buildSkirmishLaunch(size, 1).mapSize, 3); // 참가자 5명
    size.aiCount = 7; assert.equal(buildSkirmishLaunch(size, 1).mapSize, 3); assert.equal(buildSkirmishLaunch(size, 1).islands.length, 8);
});

test('시작 자료: 중앙섬 패턴은 3x3 이고 모든 후보가 골고루 뽑히며 AI 수와 무관하게 서로 다른 섬이다', () => {
    const settings = createSkirmishSettings(); settings.aiCount = 7;
    const seen = new Set(), patterns = new Set();
    // 여러 씨앗으로 뽑아 모든 시작 섬이 8개씩 서로 다른지, 어느 후보든 언젠가 뽑히는지, 중앙 패턴이 3x3 뿐인지 본다.
    for (let seed = 0; seed < 200; seed += 1) {
        const launch = buildSkirmishLaunch(settings, seed);
        assert.equal(new Set(launch.islands.map((island) => `${island.fort}:${island.territory}`)).size, 8, `씨앗 ${seed}`);
        for (const island of launch.islands) seen.add(`${island.fort}:${island.territory}`);
        patterns.add(launch.central[0] & 63); assert.ok((launch.central[0] >> 6) <= 3);
        const shape = TERRITORY_PATTERNS[launch.central[0] & 63]; assert.ok(shape && shape.width === 3 && shape.height === 3, `씨앗 ${seed} 중앙섬 패턴`);
    }
    assert.equal(seen.size, SKIRMISH_START_ISLANDS.length);
    assert.ok(patterns.size > 1);
});

test('시작 자료의 난수 소비 순서: 번호표 Fisher-Yates(N-1회) 다음 패턴 고르기, 회전, 씨앗 바이트', () => {
    const settings = createSkirmishSettings(); settings.aiCount = 3;
    const big = TERRITORY_PATTERNS.flatMap((shape, index) => (shape.width === 3 && shape.height === 3 ? [index] : []));
    // 여러 씨앗으로 같은 순서를 시험 쪽에서 독립적으로 다시 계산해 대조한다.
    for (let seed = 0; seed < 60; seed += 1) {
        const random = new Mt19937(seed), order = SKIRMISH_START_ISLANDS.map((_, index) => index);
        for (let at = order.length - 1; at > 0; at -= 1) { const pick = random.next() % (at + 1); [order[at], order[pick]] = [order[pick], order[at]]; }
        const pattern = big[random.next() % big.length], rotation = random.next() & 3, byte = random.next() & 0xff;
        const launch = buildSkirmishLaunch(settings, seed);
        assert.deepEqual(launch.islands, order.slice(0, 4).map((index) => SKIRMISH_START_ISLANDS[index]), `씨앗 ${seed} 섬`);
        assert.deepEqual([...launch.central], [pattern | (rotation << 6), 0x09, 0, byte, 0, 0], `씨앗 ${seed} 중앙섬`);
    }
});

test('mt19937: 표준 기준값(씨앗 5489 의 첫 값과 10000번째 값)과 씨앗별 재현', () => {
    const random = new Mt19937(5489);
    assert.equal(random.next(), 3499211612);
    for (let count = 2; count < 10000; count += 1) random.next();
    assert.equal(random.next(), 4123659995);
    const first = new Mt19937(1), second = new Mt19937(1), other = new Mt19937(2);
    const a = [first.next(), first.next(), first.next()];
    assert.deepEqual([second.next(), second.next(), second.next()], a); assert.notDeepEqual([other.next(), other.next(), other.next()], a);
});

// 서로 다른 길이의 섹션 35개로 이루어진 시험용 fort 를 만든다(Chaff=4, Territory=6, Terr=15..34). (2026-11-20 WB04-E1101)
function sampleFort() {
    const bytes = [0x46, 0];
    // 길이(자기 2바이트 포함)가 서로 다른 섹션 35개를 만든다.
    for (let index = 0; index < 35; index += 1) {
        const body = new Array(index === 6 ? 120 : (index % 5) + 3).fill(index + 1), length = body.length + 2;
        bytes.push(length & 0xff, length >> 8, ...body);
    }
    return Uint8Array.from(bytes);
}

test('빈 월드 fort: 길이 표 35개 섹션을 유지하고 Chaff/Territory/Terr 만 비우며 입력이 틀리면 빈 결과', () => {
    const base = sampleFort(), world = makeSkirmishEmptyWorldFort(base);
    assert.ok(world.length > 0 && world[0] === 0x46);
    // 결과를 다시 섹션으로 읽는다.
    const sections = [];
    let position = 2;
    while (sections.length < 35 && position + 2 <= world.length) {
        const length = world[position] | (world[position + 1] << 8);
        sections.push(Array.from(world.subarray(position + 2, position + length)));
        position += length;
    }
    assert.equal(sections.length, 35); assert.equal(position, world.length);
    assert.deepEqual(sections[0], [1, 1, 1]); // 나머지 섹션(예: Subscriber)은 그대로다.
    assert.equal(sections[5].length, 3 + 5 % 5 + 0); assert.deepEqual(sections[14], new Array(3 + (14 % 5)).fill(15)); // 15 번 앞 섹션도 그대로다.
    assert.equal(sections[4].length, 1 + 256 * 3); assert.deepEqual(sections[4].slice(0, 4), [2, 99, 0, 0]); assert.deepEqual(sections[4].slice(-3), [99, 0, 0]); // Chaff 는 빈 청크 256개다.
    assert.equal(sections[6].length, 120); assert.ok(sections[6].every((byte) => byte === 0)); // 영역 표는 모두 0이다.
    for (let index = 15; index < 35; index += 1) assert.deepEqual(sections[index], [2]); // Terr 섹션은 버전만 남는다.
    assert.equal(makeSkirmishEmptyWorldFort(new Uint8Array(0)).length, 0); assert.equal(makeSkirmishEmptyWorldFort(null).length, 0);
    assert.equal(makeSkirmishEmptyWorldFort(base.subarray(0, base.length - 1)).length, 0); // 마지막 섹션이 잘렸다.
    const wrong = Uint8Array.from(base); wrong[0] = 0x58; assert.equal(makeSkirmishEmptyWorldFort(wrong).length, 0); // 서명이 틀렸다.
    for (const bad of [0, 1]) { const tiny = Uint8Array.from(base); tiny[2] = bad; tiny[3] = 0; assert.equal(makeSkirmishEmptyWorldFort(tiny).length, 0, `섹션 길이 ${bad}`); } // 섹션 길이가 2 미만이다.
});

test('화면 본문: 시작/취소와 현재 값, 금지 표시와 쪽 이동, 최대 도달 안내, 언어별 제목', () => {
    const settings = createSkirmishSettings();
    settings.aiCount = 2; settings.ai[0] = { difficulty: AiDifficulty.Hard, ally: true }; settings.bans = ['windBattery'];
    const target = (text) => parseSkirmishTarget(text);
    const main = buildSkirmishScreen(target('Patch1101.Skirmish'), settings, 0, null);
    assert.ok(has(main, 'AI players: 2') && has(main, 'AI 1: Hard / Ally') && has(main, 'AI 2: Normal / Enemy') && !has(main, 'AI 3:'));
    assert.ok(has(main, 'Banned units: 1/5') && has(main, '$Button=Start,Tell,Patch1101.SkirmishStart') && has(main, '$Button=Cancel,Tell,UCampaign'));
    assert.ok(main.startsWith('<background="nsstart">\n<h2>{H2}Skirmish</h2>\n<p>{Text}\n'));
    const korean = buildSkirmishScreen(target('Patch1101.Skirmish'), settings, 7, null);
    assert.ok(has(korean, '스커미시') && has(korean, '인공지능 수: 2') && has(korean, '어려움 / 동맹'));
    assert.ok(has(buildSkirmishScreen(target('Patch1101.Skirmish'), settings, 5, null), 'スカーミッシュ'));
    assert.ok(has(buildSkirmishScreen(target('Patch1101.Skirmish'), settings, 1, null), '$Button=Start,')); // 언어 1 도 영어다.
    const count = buildSkirmishScreen(target('Patch1101.SkirmishCount'), settings, 0, null);
    assert.ok(has(count, '$Checked=2,Tell,Patch1101.SkirmishSet(count;2),1,1') && has(count, 'SkirmishSet(count;7),0,1') && has(count, '$Checked=1,Tell,Patch1101.SkirmishSet(count;1),0,1') && has(count, '$Button=Back,Tell,Patch1101.Skirmish\n'));
    const ai = buildSkirmishScreen(target('Patch1101.SkirmishAi(2)'), settings, 0, null);
    assert.ok(has(ai, '$Checked=Hard,Tell,Patch1101.SkirmishSet(diff;2;3),1,1') && has(ai, '$Checked=Ally,Tell,Patch1101.SkirmishSet(ally;2;1),1,1') && has(ai, '$Checked=Easy,Tell,Patch1101.SkirmishSet(diff;2;1),0,1') && has(ai, '<h2>{H2}AI 1</h2>'));
    assert.ok(has(buildSkirmishScreen(target('Patch1101.SkirmishAi'), settings, 0, null), 'SkirmishSet(diff;2;')); // 소유자를 안 주면 2 이다.
    const translate = (text) => `${text}!`;
    const bans = buildSkirmishScreen(target('Patch1101.SkirmishBans(0)'), settings, 0, translate);
    assert.ok(has(bans, '$Checked=Wind Generator! - windBattery,Tell,Patch1101.SkirmishSet(ban;0;0),1,1') && has(bans, '$Checked=Rain Generator! - rainBattery,Tell,Patch1101.SkirmishSet(ban;1;0),0,1') && has(bans, 'Patch1101.SkirmishBans(1)') && !has(bans, 'Previous'));
    assert.ok(has(bans, '$Checked=Clear all,Tell,Patch1101.SkirmishSet(clearbans),0,1') && has(bans, 'Select up to 5 units'));
    // 쪽당 12개이고 마지막 쪽(2)에 Outpost 와 이전 쪽 이동이 있으며 쪽 번호가 넘치면 마지막 쪽으로 줄인다.
    assert.equal((bans.match(/\$Checked=[^\n]+SkirmishSet\(ban;/gu) ?? []).length, 12);
    const lastPage = buildSkirmishScreen(target('Patch1101.SkirmishBans(2)'), settings, 0, translate);
    assert.ok(has(lastPage, 'Outpost!') && has(lastPage, '$Checked=Previous page,Tell,Patch1101.SkirmishBans(1),0,1') && !has(lastPage, 'Next page'));
    const middle = buildSkirmishScreen(target('Patch1101.SkirmishBans(1)'), settings, 0, translate); // 가운데 쪽은 양쪽 이동이 모두 있고 0 쪽으로 돌아갈 수 있다.
    assert.ok(has(middle, '$Checked=Next page,Tell,Patch1101.SkirmishBans(2),0,1') && has(middle, '$Checked=Previous page,Tell,Patch1101.SkirmishBans(0),0,1'));
    assert.equal(buildSkirmishScreen(target('Patch1101.SkirmishBans(9)'), settings, 0, translate), lastPage);
    assert.ok(has(buildSkirmishScreen(target('Patch1101.SkirmishBans(0)'), settings, 0, null), '$Checked=Wind Generator - windBattery,Tell,Patch1101.SkirmishSet(ban;0;0),1,1')); // 번역 함수가 없으면 영어 이름 그대로다.
    settings.bans = ['windBattery', 'rainBattery', 'thunderBattery', 'sunArcher', 'windArcher'];
    assert.ok(has(buildSkirmishScreen(target('Patch1101.SkirmishBans(0)'), settings, 0, translate), 'Five units are banned'));
    // 후보 표는 27개이고 내부 타입 이름이 서로 다르다.
    assert.equal(SKIRMISH_BANNABLE.length, 27); assert.equal(new Set(SKIRMISH_BANNABLE.map((item) => item.type)).size, 27);
});

test('월드 계획: 시작 섬마다 fort 바이트를 묶고 하나라도 없으면 거부한다', () => {
    const settings = createSkirmishSettings(); settings.aiCount = 3;
    const launch = buildSkirmishLaunch(settings, 11);
    const read = (fort) => Uint8Array.of(0x46, fort.length);
    const plan = makeSkirmishWorldPlan(launch, read);
    assert.equal(plan.islands.length, 4); assert.equal(plan.mapSize, launch.mapSize); assert.deepEqual([...plan.central], [...launch.central]);
    plan.islands.forEach((island, index) => { assert.equal(island.fort, launch.islands[index].fort); assert.equal(island.territory, launch.islands[index].territory); assert.deepEqual([...island.bytes], [0x46, island.fort.length]); });
    // 중앙섬 기록은 사본이라 계획을 고쳐도 시작 자료가 바뀌지 않는다.
    plan.central[0] ^= 0xff; assert.notEqual(plan.central[0], launch.central[0]);
    assert.throws(() => makeSkirmishWorldPlan(launch, (fort) => (fort === launch.islands[2].fort ? null : read(fort))), /읽을 수 없음/u);
    assert.throws(() => makeSkirmishWorldPlan(launch, () => new Uint8Array(0)), /읽을 수 없음/u);
    assert.throws(() => makeSkirmishWorldPlan({ ...launch, islands: [] }, read), /후보 부족/u);
});

test('가상 파일 source: 대본·빈 월드 fort 경로만 구분자·대소문자 무시로 서비스한다', () => {
    const text = '[Header]\nA=1\n', bytes = [...Buffer.from(text, 'latin1')];
    const empty = Uint8Array.of(1, 2, 3), source = createSkirmishFileSource(text, empty, '\\D\\Skirmish.english', '\\D\\Skirmish.fort');
    assert.deepEqual([...source('\\D\\Skirmish.english')], bytes);
    assert.deepEqual([...source('/d/SKIRMISH.English')], bytes);
    assert.equal(source('\\D\\Skirmish.fort'), empty); assert.equal(source('/d/skirmish.FORT'), empty); assert.equal(source('d/skirmish.fort'), null);
    for (const other of ['\\D\\tell.english', '\\D\\Skirmish2.fort', '', 'Skirmish.fort']) assert.equal(source(other), null, other);
    // 비ASCII 문자는 하위 바이트만 남는다(대본은 ASCII 만 쓰므로 방어 동작이다).
    assert.deepEqual([...createSkirmishFileSource('é', empty, 'a', 'b')('a')], [0xe9]);
});
