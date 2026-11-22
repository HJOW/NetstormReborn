// cpppj `client/AssetDeletionVtables.inc`(PE 생성자 83개의 VT+14/+18/+34)를 웹 가상 표 83칸과 삭제 자격 함수(kinds)에 대조한다. (2026-11-22 WB04-E1209)
// 원문 표는 행마다 (가상 표, 전처리 진입점, 후처리 진입점, 부모 삭제 통지 진입점)이다. 웹은 진입점 주소 대신 같은 가상 표 DWORD 로 자격을 고르므로,
// ① 가상 표와 통지 진입점이 `SQUID_ASSET_VTABLES` 83칸과 같은지, ② 같은 (전처리, 후처리) 묶음이 웹의 어느 자격 함수에 해당하는지가 일대일인지 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { SQUID_ASSET_VTABLES } from '../www/core/squid-vtables.js';
import { REPO_ROOT, readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put } from './raw-support.mjs';

// 원문 표 파일을 읽어 `[표, 전처리, 후처리, 통지]` 배열로 만든다(없는 환경에서는 null 이라 시험을 건너뛴다).
const incPath = path.join(REPO_ROOT, 'cpppj', 'src', 'client', 'AssetDeletionVtables.inc');
const rows = fs.existsSync(incPath)
    ? [...fs.readFileSync(incPath, 'utf8').matchAll(/\{0x([0-9a-f]{8})U,0x([0-9a-f]{8})U,0x([0-9a-f]{8})U,0x([0-9a-f]{8})U\}/g)].map((match) => match.slice(1).map((hex) => parseInt(hex, 16)))
    : null;
// 진단 문구용 16진 표기이다.
const hex = (value) => '0x' + value.toString(16);
// 미선택 외부 서비스는 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자료와 실제 팔레트를 공유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 외부 경계만 명시적으로 공급한다.
function owner() { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } }); }
// 원문 생성 순서만 진행한다(raw-campaign-session-deletion.test.mjs 와 같은 순서).
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); o.connectContentLifetime(); o.connectProduction(); o.connectFactories(); o.connectFlare(); o.connectConstruction(); o.connectDamageable(); o.connectIceTower(); o.connectCarrierSteal(); o.connectBombPostPop(); o.connectAcidZap(); o.connectAcidFence(); o.connectCarrierRegular(); o.connectSunAviary(); o.connectWindAviary(); o.connectRainAviary(); o.connectArcSpire(); o.connectFlyer(); o.connectFence(); o.connectGemReward(); o.connectWorldDeletion(); }
// 정적 가상 선택 입력을 같은 실제 풀에 쓴다.
function image(o, sid, type, vt) { const raw = o.core.pool.physical(sid); raw[10] = type; put(raw, 0, vt, 4); return raw; }

test('AssetDeletionVtables: 83행의 가상 표가 웹 SQUID_ASSET_VTABLES 와 같고 부모 삭제 통지(VT+34) 열의 구성이 원문과 같다', { skip: !rows && '원문 표 없음' }, () => {
    assert.equal(rows.length, 83);
    // 표는 원문 주소 오름차순이고 중복이 없으며, 웹 표(SQUID_ASSET_VTABLES)와 같은 주소·같은 순서이다.
    const tables = rows.map((row) => row[0]);
    assert.deepEqual(tables, [...tables].sort((a, b) => a - b)); assert.equal(new Set(tables).size, 83);
    assert.deepEqual(tables, SQUID_ASSET_VTABLES.map((row) => row[0]));
    // 네 번째 열은 부모 삭제 통지(VT+34)이다. 웹 표의 세 번째 열(내용물 부모 통지 VT+24)과는 다른 슬롯이라 직접 비교하지 않는다.
    // 77행은 빈 반환(0x41c100), 세 행(0x509148·0x509210·0x50eb98)은 현재 자산을 flags 0 으로 삭제(0x48f0a0), 세 행은 비행장 홈 통지(훨리베이스·맨 오 워·데빌메이커)이다.
    const notice = new Map(); for (const row of rows) notice.set(row[3], [...(notice.get(row[3]) ?? []), row[0]]);
    assert.equal(notice.get(0x41c100).length, 77);
    assert.deepEqual(notice.get(0x48f0a0), [0x509148, 0x509210, 0x50eb98]);
    assert.deepEqual([notice.get(0x45b4a0), notice.get(0x45b5b0), notice.get(0x45b680)], [[0x509930], [0x5099e8], [0x509aa0]]);
    assert.equal(notice.size, 5);
});

test('AssetDeletionVtables: 전처리/후처리 묶음이 웹 삭제 자격 함수와 일대일이다', { skip: !rows && '원문 표 없음' }, () => {
    const o = owner(); try {
        before(o); const sid = o.core.pool.allocate(2); const k = o.deletionModules.kinds;
        // VT 만으로 고르는 자격: 원문 묶음에서 기대하는 자격 이름(없으면 공통 삭제만 쓰는 VT)을 정한다.
        const vtOnly = ['plainDamageable', 'blocker', 'growingIce', 'vortex', 'rune', 'sunFence', 'nugget'];
        const expectedKind = (vt, pre, post) => {
            if (post === 0x48edf0) return 'blocker'; // 세 Blocker 의 완성형 Ice 후처리
            if (post === 0x48eb80) return 'growingIce'; // 성장형 Ice 의 후처리
            if (pre === 0x4dca50) return 'vortex'; // 네 신전의 공통 전처리
            if (pre === 0x44b4b0 && post === 0x44b9e0) return 'plainDamageable'; // 공통 Damageable 14개(통지 진입점만 다른 4개 포함)
            if (pre === 0x457990) return 'sunFence'; // Fence 수명 전처리
            if (pre === 0x484f80) return 'nugget'; // 너겟의 단일 점프
            if (vt === 0x5075d8) return 'rune'; // Rune 은 공통 삭제를 쓴다
            return null;
        };
        for (const [vt, pre, post] of rows) {
            image(o, sid, 255, vt);
            const hits = vtOnly.filter((name) => k[name](sid)); const want = expectedKind(vt, pre, post);
            assert.deepEqual(hits, want ? [want] : [], hex(vt));
        }
        // TYPE 까지 보는 자격은 문서화된 (VT, TYPE) 쌍이 원문 묶음에 있고 같은 전처리/후처리인지 확인한다.
        const find = (vt) => rows.find((row) => row[0] === vt);
        const typed = [
            ['bridge', 0x5034c8, o.core.bridgeState.bridgeType, 0x4221b0, 0x422300], ['connector', 0x503360, o.core.bridgeState.connectorType, 0x4b0950, 0x4b0840],
            ['priest', 0x50f210, 158, 0x4919b0, 0x44b9e0], ['priestForcefield', 0x507770, 167, 0x4b0950, 0x4b0840], ['buried', 0x50a748, o.gemRewardState.buriedType, 0x41f910, 0x4b0840],
            ['fenceCommon', 0x5092d0, 90, 0x4b0950, 0x4b0840], ['fenceCommon', 0x509388, 170, 0x4b0950, 0x4b0840], ['visualDummy', 0x50eb98, 110, 0x4b0950, 0x4b0840],
            ['flyer', 0x50a388, 144, 0x45e7e0, 0x44b9e0], ['flyer', 0x50a1e8, 73, 0x45e7e0, 0x44b9e0], ['flyer', 0x50a050, 138, 0x45e7e0, 0x44b9e0],
        ];
        for (const [name, vt, type, pre, post] of typed) {
            const row = find(vt); assert.ok(row, hex(vt)); assert.deepEqual([row[1], row[2]], [pre, post], `${name} ${hex(vt)}`);
            image(o, sid, type, vt); assert.equal(k[name](sid), true, `${name} ${hex(vt)}`);
            // 같은 VT 라도 다른 TYPE 이면 자격이 아니다(표시 TYPE 가 아닌 VT+TYPE 쌍).
            image(o, sid, (type + 1) & 0xff, vt); assert.equal(k[name](sid), false, `${name} 다른 TYPE ${hex(vt)}`);
        }
        // 폭탄: 후보 genus 비트가 있는 TYPE 에서 자격이 되는 VT 는 정확히 열 개(+ 163 전용 그림)이며 전처리는 Point Blast 만 0x41e960, 나머지는 공통이다.
        const bombType = 174; const flags = o.core.types[bombType].flags2; o.core.types[bombType].flags2 |= 0x100;
        const bombs = rows.filter((row) => { image(o, sid, bombType, row[0]); return k.bomb(sid); });
        o.core.types[bombType].flags2 = flags;
        assert.deepEqual(bombs.map((row) => row[0]), [0x501dd0, 0x502370, 0x502448, 0x502510, 0x5025d8, 0x502e68, 0x502f30, 0x502ff8, 0x5030c0, 0x503188]);
        for (const row of bombs) assert.deepEqual([row[1], row[2]], row[0] === 0x502370 ? [0x41e960, 0x4b0840] : [0x4b0950, 0x4b0840], hex(row[0]));
        image(o, sid, 163, 0x509b98); assert.equal(k.bomb(sid), true); assert.deepEqual([find(0x509b98)[1], find(0x509b98)[2]], [0x4b0950, 0x4b0840]);
    } finally { o.dispose(); }
});

test('AssetDeletionVtables: 통지 열(VT+34)의 83행이 웹 의존 통지 분류(삭제·홈 통지·빈 반환)와 일대일이다(E1221)', { skip: !rows && '원문 표 없음' }, () => {
    const o = owner(); try {
        before(o); const sid = o.core.pool.allocate(2); const calls = [];
        const destroy = o.world.destroy; const notify = o.core.processes.notifyDependency; const flyers = o.flyerModules;
        o.world.destroy = (...args) => { calls.push('destroy'); }; o.core.processes.notifyDependency = () => { calls.push('notify'); }; o.flyerModules = { frameHooks: { homeNotice: () => { calls.push('home'); } } };
        const expected = new Map([[0x41c100, []], [0x48f0a0, ['destroy']], [0x45b4a0, ['home']], [0x45b5b0, ['home']], [0x45b680, ['home']]]);
        for (const [vt, , , notice] of rows) {
            image(o, sid, 83, vt); calls.length = 0; o.worldHooks.dependencyNotice(sid, 8, 9);
            assert.deepEqual(calls, expected.get(notice), `${hex(vt)} 통지 ${hex(notice)}`);
        }
        o.world.destroy = destroy; o.core.processes.notifyDependency = notify; o.flyerModules = flyers;
    } finally { o.dispose(); }
});
