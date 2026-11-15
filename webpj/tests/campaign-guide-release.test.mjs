// Guide 소멸 campaign-guide-release.js(CampaignGuideRuntime·releaseCampaignGuideMission)를 cpppj `CampaignGuideReleaseTests`의 계약으로 대조한다(4-D B 구획, WB04-E524).
//  - `CampaignGuideRelease_EndKeepsActualWorldInputAndOrdinaryAiLifetime` 대응: 실제 Tutorial 미션의 End 가 지연 Fort 요청·허용 표(첫 255칸만)·회수/승급 금지·미션 번호·세대 토큰·AI 반납·Config 를 한 번에 정리하고
//    World/입력/기록 소유자는 건드리지 않는다(World 를 빌린 Session 경로, 실제 Session 은 덕타입).
//  - `CampaignGuideRelease_NoWorldUsesActualPlayersAndUnwindRetiresBase` 대응: World 없이 초기화된 Player 9 를 읽어 NULL brain 이면 정상, 비NULL brain 이면 명시 오류 + unwind 로 기본 Mission 만 한 번 반납하고 Player 기록은 불변.
//  - 웹 계약: 기본(비Tutorial) Mission 은 허용 표/번호/AI 소멸을 추가하지 않음, 실행기 소멸 순서(Tutorial→Start), 미초기화 Player 물리/다른 Session Player 소유자 거부, 지연 요청이 없어도 LoadMissionFort/LoadMissionWorld 취소.
// 실제 미션 `TheWarBegins` 와 실제 `GameData`/`MissionScript`/`Guide 교체`를 쓰고 Client/Session 은 계약만 가진 덕타입이다. 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E524)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { CampaignGuideRuntime, releaseCampaignGuideMission } from '../www/core/campaign-guide-release.js';
import { closeGuideMission, endGuideMission } from '../www/core/guide-mission-transitions.js';
import { createGuideMissionScript } from '../www/core/mission-script.js';
import { createMissionFortLoadRequest } from '../www/core/mission-fort-request.js';

// 큰 아카이브는 한 번만 읽고 GameData 는 시험마다 새로 만든다. 경로 설정은 원본 영어 미션/요새만 연결한다. (2026-11-15 WB04-E524)
let cachedFiles = null;
function makeData() {
    cachedFiles ??= new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]);
    const data = GameData.build({ files: cachedFiles, language: 'english' });
    data.configuration.assign('DataDir="d"\ncurrentLanguage="english"\nmissionSpec="{DataDir}\\{local.1}.{currentLanguage}"\nfortSpec="{DataDir}\\{local.1}.fort"\n[END]\n');
    return data;
}

// Player 9 기록(1620바이트)에서 owner 의 brain DWORD(+b0)를 쓴다. (2026-11-15 WB04-E524)
function putBrain(records, owner, value) {
    for (let index = 0; index < 4; index += 1) records[owner * 180 + 0xb0 + index] = (value >>> (index * 8)) & 0xff;
}

// 현재 Client 대역이다: 전역(금지 DWORD 두 개)·Player 기록·Config, 선택적으로 World 를 빌린 Session 이다. (2026-11-15 WB04-E524)
function makeScene({ borrowed = false, sessionRecords = null, sizeBytes = 1620 } = {}) {
    const data = makeData();
    const records = new Uint8Array(sizeBytes);
    const events = [];
    const session = {
        playerRecords: () => sessionRecords ?? records,
        participation: () => ({ removeAllBrains: () => { events.push('removeAllBrains'); } }),
    };
    const game = { denySalvage: -7, denyAscend: 23, borrowed: () => borrowed, requireSession: () => session };
    const client = { gameplayGlobals: () => game, playerRecords: () => ({ records: () => records }), configuration: () => data };
    const nativeMission = { value: false };
    const mission = { value: createGuideMissionScript(data, 'TheWarBegins', nativeMission) };
    assert.ok(mission.value.loaded);
    const runtime = { value: new CampaignGuideRuntime(mission.value, 0, 0, 671) };
    const restrictions = new Uint32Array(256).fill(0xabcdef01);
    const cancelled = [];
    const pending = { cancel: (command) => { cancelled.push(command); return true; } };
    const request = { value: createMissionFortLoadRequest() };
    const owned = { mission, runtime, request, restrictions, missionNumber: { value: 11 }, worldMissionToken: { value: 671 }, nativeMission };
    return { data, records, events, game, client, owned, pending, cancelled };
}

test('CampaignGuideRuntime: 이름·데모·Tutorial 판정·저장 토큰을 초기 입력으로만 둔다', () => {
    const scene = makeScene();
    const runtime = scene.owned.runtime.value;
    assert.equal(runtime.saveToken, 671);
    assert.equal(runtime.guided, true);
    assert.equal(runtime.gate.demo, 0);
    assert.equal(runtime.mode64, 0);
    assert.equal(runtime.init.name, 'TheWarBegins');
    assert.equal(runtime.start, null);
    assert.equal(runtime.tutorial, null);
    // 번호 정규화는 이후 단계의 몫이라 생성자는 Tutorial 번호/Start 효과를 만들지 않는다.
    assert.equal(runtime.gate.tutorialNumber, 0);
    assert.equal(runtime.gate.started, 0);
    const demo = new CampaignGuideRuntime(scene.owned.mission.value, 1, 1, 5);
    assert.equal(demo.gate.demo, 1);
    assert.equal(demo.mode64, 1);
});

test('releaseCampaignGuideMission: World 를 빌린 Tutorial End 는 요청·허용 표·전역·AI·Config 를 정리하고 소유자는 건드리지 않는다', () => {
    const scene = makeScene({ borrowed: true });
    const { owned, game, data } = scene;
    owned.runtime.value.gate.tutorialNumber = 3;
    owned.runtime.value.gate.stage = 6;
    owned.runtime.value.gate.stageDirty = 9;
    const gate = owned.runtime.value.gate;
    const guideRuntime = owned.runtime.value;
    let finished = 0;
    // 실제 End: finish(가상 종료) 와 release 를 한 번씩 수행한다.
    const hooks = {
        current: () => owned.runtime.value !== null,
        finish: () => { finished += 1; },
        release: () => { releaseCampaignGuideMission(scene.client, scene.pending, owned); },
    };
    endGuideMission(hooks);
    assert.equal(finished, 1);
    // 지연 Fort 로드 요청과 호스트 큐의 두 명령 취소, 실행기·미션·세대 토큰·미션 번호.
    assert.deepEqual(scene.cancelled, ['LoadMissionFort', 'LoadMissionWorld']);
    assert.equal(owned.request.value, null);
    assert.equal(owned.runtime.value, null);
    assert.equal(owned.mission.value, null);
    assert.equal(owned.nativeMission.value, false);
    assert.equal(owned.worldMissionToken.value, 0);
    assert.equal(owned.missionNumber.value, 0);
    assert.equal(data.getInt('global.inMission'), 0);
    // 허용 표는 첫 255칸만 허용(0)이고 마지막 물리 칸은 보존된다.
    assert.ok(Array.from(owned.restrictions.subarray(0, 255)).every((value) => value === 0));
    assert.equal(owned.restrictions[255], 0xabcdef01);
    // 회수/승급 금지와 Tutorial 번호·단계·표식은 0 이다.
    assert.equal(game.denySalvage, 0);
    assert.equal(game.denyAscend, 0);
    assert.equal(gate.tutorialNumber, 0);
    assert.equal(gate.stage, 0);
    assert.equal(gate.stageDirty, 0);
    // AI 반납은 같은 Session 에서 한 번이며 실행기는 소멸했다.
    assert.deepEqual(scene.events, ['removeAllBrains']);
    assert.equal(guideRuntime.tutorial, null);
    assert.equal(guideRuntime.start, null);
});

test('releaseCampaignGuideMission: World 없이 초기화된 Player 의 NULL brain 은 정상이고 비NULL brain 은 명시 오류이며 unwind 가 기본 Mission 을 한 번만 반납한다', () => {
    for (const invalid of [false, true]) {
        const scene = makeScene();
        const { owned, game, data, records } = scene;
        if (invalid) putBrain(records, 1, 0x76543210);
        const before = Uint8Array.from(records);
        let finished = false;
        let diagnosed = false;
        const hooks = {
            current: () => owned.runtime.value !== null,
            finish: () => { finished = true; },
            release: () => { releaseCampaignGuideMission(scene.client, scene.pending, owned); },
        };
        try {
            // Close 는 finish 없이 같은 현재만 반납한다.
            closeGuideMission(hooks);
        } catch (error) {
            diagnosed = /비NULL brain/.test(error.message);
        }
        assert.equal(diagnosed, invalid);
        assert.equal(finished, false);
        // 오류여도 unwind 로 기본 Mission 은 정확히 한 번 반납되어 실행기/미션/Config 가 정리된다.
        assert.equal(owned.runtime.value, null);
        assert.equal(owned.mission.value, null);
        assert.equal(owned.request.value, null);
        assert.equal(data.getInt('global.inMission'), 0);
        assert.equal(owned.worldMissionToken.value, 0);
        // 실제 Tutorial 전역 쓰기는 AI 검사 앞에서 끝난다(허용 표·금지 DWORD·번호는 0).
        assert.equal(game.denySalvage, 0);
        assert.equal(game.denyAscend, 0);
        assert.equal(owned.missionNumber.value, 0);
        assert.ok(Array.from(owned.restrictions.subarray(0, 255)).every((value) => value === 0));
        assert.equal(owned.restrictions[255], 0xabcdef01);
        // Player 기록은 변경하지 않는다.
        assert.deepEqual(records, before);
        assert.deepEqual(scene.events, []);
    }
});

test('releaseCampaignGuideMission: 기본(비Tutorial) Mission 은 허용 표·번호·AI 소멸 없이 기본 해제만 하고 미초기화/불일치 Player 소유자는 거부한다', () => {
    // 비Tutorial 미션: 같은 Guide 해제에서 기본 Mission 만 반납한다.
    const plain = makeScene();
    plain.owned.runtime.value.guided = false;
    releaseCampaignGuideMission(plain.client, plain.pending, plain.owned);
    assert.equal(plain.owned.runtime.value, null);
    assert.equal(plain.owned.mission.value, null);
    assert.equal(plain.game.denySalvage, -7);
    assert.equal(plain.game.denyAscend, 23);
    assert.equal(plain.owned.missionNumber.value, 11);
    assert.equal(plain.owned.restrictions[0], 0xabcdef01);
    assert.deepEqual(plain.events, []);
    assert.deepEqual(plain.cancelled, ['LoadMissionFort', 'LoadMissionWorld']);
    // runtime 가 아예 없어도 기본 해제만 수행하며 오류가 없다.
    const none = makeScene();
    none.owned.runtime.value = null;
    releaseCampaignGuideMission(none.client, none.pending, none.owned);
    assert.equal(none.owned.mission.value, null);
    // 미초기화/손실된 Player 물리는 가짜 빈 Player 로 대체하지 않고 거부하며 기본 Mission 은 그래도 반납한다.
    const small = makeScene({ sizeBytes: 100 });
    assert.throws(() => releaseCampaignGuideMission(small.client, small.pending, small.owned), /Player9 물리 미초기화/);
    assert.equal(small.owned.mission.value, null);
    // 빌린 Session 의 Player 기록이 Client 의 것과 다르면 AI 를 반납하지 않는다.
    const foreign = makeScene({ borrowed: true, sessionRecords: new Uint8Array(1620) });
    assert.throws(() => releaseCampaignGuideMission(foreign.client, foreign.pending, foreign.owned), /Player 소유자 불일치/);
    assert.deepEqual(foreign.events, []);
    assert.equal(foreign.owned.mission.value, null);
});
