// 실제 캠페인 자산 연결 raw-campaign-asset-bindings.js(RawCampaignAssetBindings)를 cpppj 소스 계약으로 대조한다(4-D C 구획, WB04-E561).
// cpppj 에는 이 단위만의 원본 관측 fixture 나 전용 시험이 없고 `tests/CampaignInstallSupport.h` 의 InstallScene 이 실제 저장 건물 후처리·Gun/Regular 를 이 연결로 조립한다.
// 여기서는 같은 방식으로 보호 10.78 TYPE·프레임·SHP 로 만든 실제 RawSurfaceWorld 에 연결을 설치하고(`campaign-asset-support.mjs`) 실제 Pop/Regular/Gun 상태 기계를 실행해 연결이 맡은 몸체를 확인한다:
//  - 구성: 다른 풀·TYPE/원소/Player 크기 불일치, 다른 권한 소유자, 다른 Bridge TYPE 소유자는 공유 상태를 쓰기 전에 거부하고, 성공하면 같은 World 의 권한·현재 Bridge TYPE·게임 시각·Fort 모드를 연결한다.
//  - 활성/해제: activatePostPop 이 자산·Factory 후처리를 공통 분배기에 등록하고 dispose 는 활성 등록만 해제한다.
//  - Gun: 최초 Pop 이 현재 005410f8 TYPE 으로 실제 GunProcess 를 붙이고 flags 0 서버 전송이 지역 64BYTE 버퍼와 sendGunPacket 으로 나간다. 표적 감시·Animation Form 중지·차단 비트·지연 난수·누락 훅 거부.
//  - 거주 구역·간헐천·신전·발사기·Factory/아웃포스트의 최초 Pop 과 Regular 사건이 같은 Core(소유자·목록·섬 기록·예약·프레임·표시)와 외부 효과 경계에 연결된다.
// 기대값은 cpppj 소스의 분기와 독립 계산(프레임 구간·난수·패킷 길이)이며 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E561)
import assert from 'node:assert/strict';
import test from 'node:test';
import { AssetScene, PLAYER_STRIDE } from './campaign-asset-support.mjs';
import { get, put } from './raw-support.mjs';
import { GeyserAiRegistrationBoundary } from '../www/core/raw-geyser-ai-registration.js';
import { GunProcess, GunProcessFields, GunProcessState, RawGunProcess } from '../www/core/raw-gun-process.js';
import { AssetPostPopState } from '../www/core/raw-asset-post-pop-state.js';
import { AssetRegularState } from '../www/core/raw-asset-regular.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { GameRandom } from '../www/core/random.js';
import { RegularProcess, SquidProcess } from '../www/core/squid-process.js';

// 자산 TYPE 번호이다. (2026-11-15 WB04-E561)
const SUN_ARCHER = 71;
const GENERATOR = 75;
const SUN_FACTORY = 85;
const RAIN_FACTORY = 87;
const RESIDENCE = 89;
const GEYSER = 122;
const WIND_VORTEX = 123;
const RAIN_VORTEX = 124;
const OUTPOST = 126;
const SUN_CANNON = 128;
const NO_ISLAND = 157;
const BRIDGE = 82;
// Player 기록의 AI 포인터 위치와 슬롯 필드 위치이다. (2026-11-15 WB04-E561)
const PLAYER_AI = 0xb0;
const HEAD = 6;
const NEXT = 4;
const WORD = 12;
const OWNER = 34;
const FRAME = 36;
const EXTRA = 40;
// 총 훅 가운데 외부 효과로 받는 이름이다(연결이 직접 만드는 여섯 훅은 제외). (2026-11-15 WB04-E561)
const GUN_EFFECTS = ['refreshRange', 'targetingMode', 'findTarget', 'startAnimation', 'validateIslandOwners', 'checkForms', 'shotContext', 'shoot', 'firingInterval'];

// 객체를 놓을 자리들과 지면 표본 (21, 21) 주변에만 지면 조각(owner)을 깐 장면을 만든다. 건물 발자국이 최대 8칸이라 객체는 15칸 이상 띄운다. (2026-11-15 WB04-E561)
const SPOTS = Object.freeze([[21, 21], [25, 25], [40, 40], [55, 55], [45, 25], [25, 45], [45, 45]]);
function grounded(options = {}, owner = 2) {
    const scene = new AssetScene(options);
    scene.patches(SPOTS, owner);
    return scene;
}

// 부모의 Form 체인을 `SID:TYPE` 목록으로 읽는다. (2026-11-15 WB04-E561)
function chain(scene, parent) {
    const result = [];
    // 머리 WORD 에서 다음 WORD 를 따라간다.
    for (let form = get(scene.core.pool.slot(parent), HEAD, 2); form; form = get(scene.core.pool.slot(form), NEXT, 2)) result.push(`${form}:${scene.core.pool.slot(form)[10]}`);
    return result;
}

// 부모에 붙은 실제 GunProcess 를 찾는다(체인의 TYPE 이 현재 총 프로세스 TYPE 인 Form). (2026-11-15 WB04-E561)
function gunOf(scene, parent, type = 34) {
    // 체인에서 총 프로세스 Form 을 찾는다.
    for (let form = get(scene.core.pool.slot(parent), HEAD, 2); form; form = get(scene.core.pool.slot(form), NEXT, 2)) {
        if (scene.core.pool.slot(form)[10] === type) return scene.core.processes.fromForm(form);
    }
    return null;
}

// 총 외부 효과를 모두 기록 장치로 채운 효과 묶음을 만든다. results 로 훅별 반환값을 정한다. (2026-11-15 WB04-E561)
function gunEffects(holder, results = {}) {
    const gun = {};
    // 이름마다 호출 인자를 기록하고 정한 값을 돌려준다.
    for (const name of GUN_EFFECTS) {
        gun[name] = (...args) => {
            holder.scene.events.push(`${name}:${args.map((item) => (item instanceof GunProcessFields ? 'fields' : Array.isArray(item) ? 'context' : item)).join(',')}`);
            return typeof results[name] === 'function' ? results[name](...args) : results[name] ?? 0;
        };
    }
    return gun;
}

test('RawCampaignAssetBindings: 구성 불일치는 권한·Bridge TYPE·시각 어느 공유 상태도 쓰기 전에 거부한다', () => {
    const scene = new AssetScene({ bind: false });
    const other = new AssetScene({ bind: false });
    const untouched = () => {
        assert.equal(scene.assets.sharedAuthority, null);
        assert.equal(scene.gunState.sharedAuthority, null);
        assert.equal(scene.regularState.sharedAuthority, null);
        assert.equal(scene.assets.currentBridgeType, null);
        assert.equal(scene.gunState.now, 0);
    };
    scene.core.processState.now = 4.5;
    // 다른 월드의 Damageable/섬 목록은 같은 Core 가 아니다.
    assert.throws(() => scene.make({ damageable: other.carrier }), /저장소 불일치/u);
    assert.throws(() => scene.make({ islands: other.islands }), /저장소 불일치/u);
    // 원소 표 188칸과 9 Player 기록 크기/형식이 어긋난다.
    assert.throws(() => scene.make({ globals: { ...scene.globals, elements: scene.elements.slice(1) } }), /저장소 불일치/u);
    assert.throws(() => scene.make({ globals: { ...scene.globals, elements: [...scene.elements, 0] } }), /저장소 불일치/u);
    assert.throws(() => scene.make({ players: scene.players.subarray(1) }), /저장소 불일치/u);
    assert.throws(() => scene.make({ players: new Uint8Array(9 * PLAYER_STRIDE + 1) }), /저장소 불일치/u);
    assert.throws(() => scene.make({ players: Array.from(scene.players) }), /저장소 불일치/u);
    // TYPE/프레임/셰이프 표 가운데 하나만 188칸이 아니어도 거부한다(같은 저장소를 빌린 Core 뷰의 한 칸만 바꾼 월드).
    for (const name of ['types', 'frames', 'shapes']) {
        const world = { storage: scene.world.storage, core: () => ({ ...scene.core, [name]: scene.core[name].slice(1) }) };
        assert.throws(() => scene.make({ world }), /저장소 불일치/u, name);
    }
    untouched();
    // 세 전역 가운데 하나라도 다른 실행기의 권한을 이미 빌렸으면 몰래 바꾸지 않는다.
    for (const pick of ['assets', 'gun', 'regular']) {
        const foreign = new RawAuthorityState();
        const assets = Object.assign(new AssetPostPopState(), { sharedAuthority: pick === 'assets' ? foreign : null });
        const gun = Object.assign(new GunProcessState(), { sharedAuthority: pick === 'gun' ? foreign : null });
        const regular = Object.assign(new AssetRegularState(), { sharedAuthority: pick === 'regular' ? foreign : null });
        assert.throws(() => scene.make({ assets, globals: { ...scene.globals, gun, regular } }), /권한 소유자 불일치/u, pick);
        assert.equal(assets.sharedAuthority, pick === 'assets' ? foreign : null);
        assert.equal(gun.sharedAuthority, pick === 'gun' ? foreign : null);
        assert.equal(regular.sharedAuthority, pick === 'regular' ? foreign : null);
        assert.equal(assets.currentBridgeType, null);
    }
    // 다른 World 의 현재 Bridge TYPE getter 를 이미 빌린 자산 상태도 거부한다.
    const borrowed = Object.assign(new AssetPostPopState(), { currentBridgeType: () => 82 });
    assert.throws(() => scene.make({ assets: borrowed }), /Bridge TYPE 소유자 불일치/u);
    assert.equal(borrowed.sharedAuthority, null);
    untouched();
    // 현재 battleOwner 가 8 을 넘으면 권한 연결 뒤 시각 준비에서 거부한다(원문 순서: 모듈 구성 → Prepare).
    scene.playerState.battleOwner = 9;
    assert.throws(() => scene.make(), /clock\/Player owner/u);
    assert.equal(scene.gunState.now, 0);
});

test('RawCampaignAssetBindings: 구성이 같은 World 의 권한·현재 Bridge TYPE·게임 시각·Fort 모드를 연결한다', () => {
    const scene = new AssetScene({ bind: false });
    const authority = scene.world.storage.authority;
    scene.core.processState.now = 4.5;
    scene.playerState.fortMode = true;
    scene.playerState.battleOwner = 8;
    const first = scene.make();
    assert.equal(scene.assets.sharedAuthority, authority);
    assert.equal(scene.gunState.sharedAuthority, authority);
    assert.equal(scene.regularState.sharedAuthority, authority);
    // 권한은 사본이 아니라 같은 소유자의 현재 값이다.
    authority.boss = false;
    assert.equal(scene.assets.readBoss(), false);
    assert.equal(scene.gunState.readBoss(), false);
    assert.equal(scene.regularState.readBoss(), false);
    authority.boss = true;
    // 현재 Bridge TYPE 은 같은 World BridgeState 의 값을 매번 읽는다.
    assert.equal(scene.assets.readBridgeType(), 82);
    scene.core.bridgeState.bridgeType = 83;
    assert.equal(scene.assets.readBridgeType(), 83);
    scene.core.bridgeState.bridgeType = 82;
    // 구성 때 현재 시각과 Fort 모드를 세 전역에 전달한다.
    assert.equal(scene.gunState.now, 4.5);
    assert.deepEqual([scene.gunState.fortMode, scene.regularState.fortMode, scene.assets.fortMode], [true, true, true]);
    // 같은 World/상태의 두 번째 연결은 같은 소유자이므로 허용한다.
    const second = scene.make();
    assert.notEqual(second, first);
    assert.equal(scene.assets.sharedAuthority, authority);
    // 조회하는 원본 모듈은 연결마다 고정이다.
    assert.equal(first.postPop, first.postPop);
    assert.equal(first.regular, first.regular);
    assert.ok(first.gun instanceof RawGunProcess);
    assert.equal(first.gun.pool, scene.core.pool);
    assert.equal(first.postPop.pool, scene.core.pool);
    assert.equal(first.regular.pool, scene.core.pool);
});

test('RawCampaignAssetBindings: prepareFrame 은 시각·Fort 모드를 다시 읽고 setLoadingDepth 는 자산 로딩 깊이를 전달한다', () => {
    const scene = new AssetScene({ bind: false });
    const bindings = scene.make();
    assert.deepEqual([scene.gunState.fortMode, scene.regularState.fortMode, scene.assets.fortMode], [false, false, false]);
    scene.playerState.fortMode = true;
    bindings.prepareFrame(12.5);
    assert.equal(scene.gunState.now, 12.5);
    assert.deepEqual([scene.gunState.fortMode, scene.regularState.fortMode, scene.assets.fortMode], [true, true, true]);
    scene.playerState.fortMode = false;
    bindings.prepareFrame(-3);
    assert.equal(scene.gunState.now, -3);
    assert.deepEqual([scene.gunState.fortMode, scene.regularState.fortMode, scene.assets.fortMode], [false, false, false]);
    // 비유한 시각과 8 을 넘는 현재 owner 는 전역을 바꾸기 전에 거부한다. owner 8 은 허용한다.
    for (const bad of [Number.NaN, Infinity, -Infinity]) assert.throws(() => bindings.prepareFrame(bad), RangeError);
    scene.playerState.battleOwner = 9;
    assert.throws(() => bindings.prepareFrame(1), /clock\/Player owner/u);
    assert.equal(scene.gunState.now, -3);
    scene.playerState.battleOwner = 8;
    bindings.prepareFrame(1);
    assert.equal(scene.gunState.now, 1);
    bindings.setLoadingDepth(3);
    assert.equal(scene.assets.loadingDepth, 3);
    bindings.setLoadingDepth(0);
    assert.equal(scene.assets.loadingDepth, 0);
});

test('RawCampaignAssetBindings: activatePostPop 이 자산·Factory 후처리를 등록하고 dispose 는 활성 등록만 해제한다', () => {
    const scene = new AssetScene({ bind: false });
    const bindings = scene.make();
    const idle = scene.make();
    const geyser = scene.world.create(GEYSER, 0);
    const factory = scene.world.create(SUN_FACTORY, 0);
    assert.equal(scene.core.postPop.handlesDerived(geyser), false);
    assert.equal(scene.core.postPop.handlesDerived(factory), false);
    bindings.activatePostPop();
    assert.equal(scene.core.postPop.handlesDerived(geyser), true);
    assert.equal(scene.core.postPop.handlesDerived(factory), true);
    // 활성화하지 않은 다른 연결의 해제는 현재 등록을 건드리지 않는다.
    idle.dispose();
    assert.equal(scene.core.postPop.handlesDerived(geyser), true);
    assert.equal(scene.core.postPop.handlesDerived(factory), true);
    // 같은 객체의 재활성은 허용하고 해제는 두 몸체를 모두 내린다(두 번째 해제는 무동작).
    bindings.activatePostPop();
    bindings.dispose();
    assert.equal(scene.core.postPop.handlesDerived(geyser), false);
    assert.equal(scene.core.postPop.handlesDerived(factory), false);
    idle.activatePostPop();
    bindings.dispose();
    assert.equal(scene.core.postPop.handlesDerived(geyser), true);
    assert.equal(scene.core.postPop.handlesDerived(factory), true);
});

test('RawCampaignAssetBindings: 총 최초 Pop 이 현재 005410f8 TYPE 으로 실제 GunProcess 를 붙이고 flags 0 전송을 보낸다', () => {
    const scene = grounded();
    const { core } = scene;
    core.processState.now = 7.25;
    const cannon = scene.pop(SUN_CANNON, 25, 25, 3, 1);
    const process = gunOf(scene, cannon);
    assert.ok(process instanceof GunProcess);
    assert.equal(process.parent, cannon);
    // 생성 직전 현재 시각을 Gun 전역에 준비하고 생성자가 그 시각을 필드에 쓴다.
    assert.equal(scene.gunState.now, 7.25);
    assert.equal(process.fields.time, 7.25);
    assert.equal(process.fields.stage, 0);
    // 전송은 같은 프로세스의 전체 패킷(생성 직후 dirty 0xff, 부모 포함)이며 독립 writer 결과와 같다. 전송 뒤 실제 필드의 dirty 는 0 이다.
    assert.equal(process.fields.dirty, 0);
    const twin = new GunProcessFields();
    twin.bytes.set(process.fields.bytes);
    twin.bytes[8] = 0xff;
    const expected = new Uint8Array(64);
    const length = RawGunProcess.writeProcess(twin, expected, true);
    assert.equal(length, 31);
    assert.deepEqual(scene.take(), [`gun:${Buffer.from(expected.subarray(0, length)).toString('hex')}`]);
    assert.deepEqual([expected[0], expected[1], get(expected, 2, 2), get(expected, 4, 2)], [1, 0x3a, 31, process.form]);
    // 다음 생성은 그 시점의 장기 TYPE DWORD 와 현재 시각을 다시 읽는다.
    scene.world.storage.gameAux.gunProcessType = 33;
    core.processState.now = 9.5;
    const second = scene.pop(SUN_CANNON, 40, 40, 3, 1);
    assert.equal(gunOf(scene, second, 34), null);
    const moved = gunOf(scene, second, 33);
    assert.ok(moved instanceof GunProcess);
    assert.equal(moved.fields.time, 9.5);
    assert.equal(scene.take().length, 1);
});

test('RawCampaignAssetBindings: transmission 은 Gun 패킷만 지역 버퍼로 쓰고 나머지는 fallback 에 넘긴다', () => {
    const calls = [];
    const sent = [];
    const scene = grounded({ effects: { sendGunPacket: (packet) => { sent.push(Uint8Array.from(packet)); } } });
    const cannon = scene.pop(SUN_CANNON, 25, 25, 3, 1);
    const gun = gunOf(scene, cannon);
    sent.length = 0;
    const other = new SquidProcess();
    const fallback = {
        writeData: (process, parentChanged) => { calls.push(['writeData', process, parentChanged]); return 17; },
        writeFullData: (process, force) => { calls.push(['writeFullData', process, force]); return 19; },
        packet: (length) => { calls.push(['packet', length]); return Uint8Array.of(9, 8, 7).subarray(0, length); },
        transmit: (length) => { calls.push(['transmit', length]); },
    };
    const hooks = scene.bindings.transmission(fallback);
    // 다른 Process 는 네 경계 모두 fallback 이 맡고 반환값을 그대로 돌려준다.
    assert.equal(hooks.writeData(other, true), 17);
    assert.deepEqual([...hooks.packet(2)], [9, 8]);
    hooks.transmit(2);
    assert.equal(hooks.writeFullData(other, 5), 19);
    hooks.transmit(3);
    assert.deepEqual(calls, [['writeData', other, true], ['packet', 2], ['transmit', 2], ['writeFullData', other, 5], ['transmit', 3]]);
    assert.equal(sent.length, 0);
    calls.length = 0;
    // Gun 의 부분 writer: 부모 변경 여부가 패킷에 반영되고 독립 writer 와 같은 바이트를 같은 버퍼에서 돌려준다.
    put(gun.fields.bytes, 8, 0x0c, 1);
    const twin = new GunProcessFields();
    twin.bytes.set(gun.fields.bytes);
    const expected = new Uint8Array(64);
    const withParent = RawGunProcess.writeProcess(twin, expected, true);
    assert.equal(hooks.writeData(gun, true), withParent);
    assert.deepEqual(hooks.packet(withParent), expected.subarray(0, withParent));
    hooks.transmit(withParent);
    assert.deepEqual(sent.pop(), expected.subarray(0, withParent));
    // 부모 변경이 없으면 더 짧다(dirty 는 앞선 writer 가 0 으로 지웠으므로 다시 쓴다).
    put(gun.fields.bytes, 8, 0x0c, 1);
    twin.bytes.set(gun.fields.bytes);
    const without = RawGunProcess.writeProcess(twin, expected, false);
    assert.equal(hooks.writeData(gun, false), without);
    assert.ok(without < withParent);
    // 전체 writer 는 force 비영일 때만 모든 필드를 쓴다.
    twin.bytes.set(gun.fields.bytes);
    const forced = RawGunProcess.writeProcess(twin, expected, false, true);
    assert.equal(hooks.writeFullData(gun, 1), forced);
    assert.deepEqual(hooks.packet(forced), expected.subarray(0, forced));
    twin.bytes.set(gun.fields.bytes);
    const plain = RawGunProcess.writeProcess(twin, expected, false, false);
    assert.equal(hooks.writeFullData(gun, 0), plain);
    assert.ok(plain < forced);
    // 버퍼 경계: 0 과 64 는 허용하고 음수와 65 는 거부한다(송신도 같다).
    assert.equal(hooks.packet(0).length, 0);
    assert.equal(hooks.packet(64).length, 64);
    assert.throws(() => hooks.packet(-1), RangeError);
    assert.throws(() => hooks.packet(65), RangeError);
    hooks.transmit(64);
    assert.equal(sent.pop().length, 64);
    hooks.transmit(0);
    assert.equal(sent.pop().length, 0);
    assert.throws(() => hooks.transmit(-1), /packet 길이 오류/u);
    assert.throws(() => hooks.transmit(65), /packet 길이 오류/u);
    assert.deepEqual(calls, []);
    // 마지막으로 쓴 Process 가 Gun 이 아니면 버퍼/송신은 다시 fallback 이다.
    hooks.writeFullData(other, 0);
    hooks.packet(1);
    hooks.transmit(1);
    assert.deepEqual(calls, [['writeFullData', other, 0], ['packet', 1], ['transmit', 1]]);
    // fallback 이 없는 다른 Process 요청과 송신기 없는 Gun 송신은 각 경계에서 거부한다.
    const bare = scene.bindings.transmission();
    assert.throws(() => bare.writeData(other, false), /packet writer 미연결/u);
    assert.throws(() => bare.writeFullData(other, 0), /전체 writer 미연결/u);
    assert.throws(() => bare.packet(1), /작성 버퍼 미연결/u);
    assert.throws(() => bare.transmit(1), /packet 송신 미연결/u);
    const silent = new AssetScene({ effects: { sendGunPacket: undefined }, bind: false }).make().transmission();
    silent.writeData(gun, false);
    assert.throws(() => silent.transmit(4), /Gun flags0 실제 packet 송신 미연결/u);
});

test('RawCampaignAssetBindings: 총 훅이 표적 감시·Animation Form 중지·Form 재전송을 같은 Core 에 연결한다', () => {
    const holder = { scene: null };
    const scene = grounded({ effects: { gun: gunEffects(holder) } });
    holder.scene = scene;
    const { core } = scene;
    const cannon = scene.pop(SUN_CANNON, 25, 25, 3, 1);
    const first = scene.pop(RESIDENCE, 40, 40, 3, 2);
    const second = scene.pop(RESIDENCE, 55, 55, 3, 2);
    const process = gunOf(scene, cannon);
    const fields = process.fields;
    scene.take();
    // 표적 지정: 감시 Form 쌍 생성(서버 전송 두 번) → 사거리 통지 → 현재 Form 재전송.
    scene.bindings.gun.setTarget(fields, first);
    assert.equal(fields.target, first);
    assert.equal(fields.stage, 2);
    assert.equal(core.dependencies.has(first, process.form), true);
    const events = scene.take();
    assert.deepEqual(events.slice(0, 3), ['form:1:108', 'form:1:108', `refreshRange:${cannon}`]);
    assert.match(events[3], /^gun:013a/u);
    assert.equal(events.length, 4);
    // 일반 예약(46) 뒤에 붙은 회전(36)·발사(35) Animation Form 은 표적 교체 때 첫 일치부터 중지한다. 다른 Form 은 남는다.
    const regular = core.processes.addRegular(cannon, 9, 1);
    const fire = core.processes.attach(new SquidProcess(), 35, cannon, 0x50);
    const rotate = core.processes.attach(new SquidProcess(), 36, cannon, 0x50);
    assert.deepEqual(chain(scene, cannon).map((item) => item.split(':')[1]), ['36', '35', '46', '34']);
    scene.bindings.gun.setTarget(fields, second);
    assert.equal(core.processes.isRegistered(rotate.pid, rotate), false);
    assert.equal(core.processes.isRegistered(fire.pid, fire), true);
    assert.equal(core.processes.isRegistered(regular.pid, regular), true);
    assert.equal(core.dependencies.has(first, process.form), false);
    assert.equal(core.dependencies.has(second, process.form), true);
    scene.take();
    // 남은 발사(35) Form 도 같은 조회로 찾아 중지한다(현재 장기 TYPE 을 읽는다).
    scene.bindings.gun.setTarget(fields, first);
    assert.equal(core.processes.isRegistered(fire.pid, fire), false);
    assert.equal(core.processes.isRegistered(regular.pid, regular), true);
    scene.take();
    // 표적 해제: 감시 제거(제거 flags 0 → 감시 Form 쌍 삭제가 전송 flags 8 로 두 번) → 단계 1 → Form 재전송.
    scene.bindings.gun.clearTarget(fields, true);
    assert.equal(fields.target, 0);
    assert.equal(fields.stage, 1);
    assert.equal(core.dependencies.has(first, process.form), false);
    const cleared = scene.take();
    assert.match(cleared.at(-1), /^gun:013a/u);
    const removed = cleared.filter((item) => item.startsWith('delete:transmit:'));
    assert.equal(removed.length, 2);
    for (const item of removed) assert.equal(item.split(':')[3], '8');
    // 장기 TYPE 을 바꾸면 그 번호의 Form 을 Animation 으로 본다(35/36 상수 사본이 아니다).
    const custom = core.processes.attach(new SquidProcess(), 37, cannon, 0x50);
    core.processState.rotateAnimationType = 37;
    scene.bindings.gun.setTarget(fields, second);
    assert.equal(core.processes.isRegistered(custom.pid, custom), false);
    core.processState.rotateAnimationType = 36;
    const custom2 = core.processes.attach(new SquidProcess(), 38, cannon, 0x50);
    core.processState.fireAnimationType = 38;
    scene.bindings.gun.setTarget(fields, first);
    assert.equal(core.processes.isRegistered(custom2.pid, custom2), false);
    core.processState.fireAnimationType = 35;
    // Animation Form 조회는 머리 WORD 두 BYTE 만 읽는다(이웃 섬 번호 WORD 가 0 이 아니어도 같다).
    put(core.pool.allocated(cannon), 8, 0x0102, 2);
    const tail = core.processes.attach(new SquidProcess(), 36, cannon, 0x50);
    scene.bindings.gun.setTarget(fields, second);
    assert.equal(core.processes.isRegistered(tail.pid, tail), false);
    put(core.pool.allocated(cannon), 8, 0, 2);
    // Kernel 에 프로세스가 없는 Animation Form 은 현재 Host 오류로 거부한다(중지할 실제 프로세스가 없다).
    const orphan = core.processes.attach(new SquidProcess(), 35, cannon, 0x50);
    core.kernel.remove(orphan.pid);
    assert.throws(() => scene.bindings.gun.setTarget(fields, first), /총 Animation 현재 Host 오류/u);
    // 다른 연결의 raw 필드(같은 Form 번호를 흉내 낸 사본)는 현재 Host 객체가 아니므로 거부한다.
    const forged = new GunProcessFields();
    forged.bytes.set(fields.bytes);
    assert.throws(() => scene.bindings.gun.setTarget(forged, second), /현재 Host\/raw 이미지 불일치/u);
    const stray = new GunProcessFields();
    put(stray.bytes, 44, regular.form);
    put(stray.bytes, 24, first, 2);
    assert.throws(() => scene.bindings.gun.clearTarget(stray, true), /현재 Host\/raw 이미지 불일치/u);
});

test('RawCampaignAssetBindings: 총 프레임이 부모 차단 비트와 현재 게임 난수의 지연을 읽는다', () => {
    const holder = { scene: null };
    const scene = grounded({ effects: { gun: gunEffects(holder) } });
    holder.scene = scene;
    const { core } = scene;
    const cannon = scene.pop(SUN_CANNON, 25, 25, 3, 1);
    const fields = gunOf(scene, cannon).fields;
    scene.take();
    // 부모 extra 의 0x20 이 켜져 있으면 프레임은 아무 일도 하지 않는다. 다른 비트는 차단이 아니다.
    core.pool.allocated(cannon)[EXTRA] |= 0x20;
    scene.bindings.gun.frame(fields);
    assert.equal(fields.stage, 0);
    assert.deepEqual(scene.take(), []);
    core.pool.allocated(cannon)[EXTRA] &= ~0x20;
    core.pool.allocated(cannon)[EXTRA] |= 0x40;
    scene.bindings.gun.frame(fields);
    assert.equal(fields.stage, 1);
    // 검색 단계: 표적이 없으면 (난수 + 0.4f) + 현재 시각을 다음 시각으로 쓴다. 난수는 같은 LCG 의 16비트 × 2^-16 × |0.2f − 0| + 0 이다.
    const twin = new GameRandom(0);
    twin.setState(scene.random.state);
    scene.gunState.now = 100.1;
    put(fields.bytes, 16, 0);
    put(fields.bytes, 20, 0);
    scene.bindings.gun.frame(fields);
    const random = twin.next(65536) * (1 / 65536) * Math.abs(Math.fround(0.2) - 0) + 0;
    assert.equal(fields.time, (random + Math.fround(0.4)) + 100.1);
    assert.equal(scene.random.state, twin.state);
    assert.deepEqual(scene.take(), [`targetingMode:${cannon},1`, `findTarget:25,25,${SUN_CANNON},0,1`]);
    // 발사(0046bd20)는 부모 extra(+40)의 0x20 만 차단으로 본다: 이웃 BYTE(+41)나 다른 비트는 차단이 아니다.
    const target = scene.pop(RESIDENCE, 40, 40, 3, 2);
    scene.bindings.gun.setTarget(fields, target);
    scene.take();
    core.pool.allocated(cannon)[EXTRA] = 0x20;
    scene.bindings.gun.fire(fields);
    assert.deepEqual(scene.take(), []);
    core.pool.allocated(cannon)[EXTRA] = 0x41;
    core.pool.allocated(cannon)[EXTRA + 1] = 0x20;
    scene.bindings.gun.fire(fields);
    assert.equal(scene.take()[0], `shotContext:${cannon}`);
    core.pool.allocated(cannon)[EXTRA + 1] = 0;
    core.pool.allocated(cannon)[EXTRA] = 0;
    // 손상된 Form 체인: 머리 WORD 가 풀 밖(32768)이거나 자기 자신으로 도는 체인은 유한 풀 경계에서 거부한다.
    const head = get(core.pool.slot(cannon), HEAD, 2);
    put(core.pool.allocated(cannon), HEAD, 0x8000, 2);
    assert.throws(() => scene.bindings.gun.setTarget(fields, target), /총 현재 Animation Form 체인 오류/u);
    put(core.pool.allocated(cannon), HEAD, head, 2);
    const regular = core.processes.addRegular(cannon, 9, 1);
    const next = get(core.pool.slot(regular.form), NEXT, 2);
    put(core.pool.allocated(regular.form), NEXT, regular.form, 2);
    assert.throws(() => scene.bindings.gun.setTarget(fields, target), /총 현재 Animation Form 체인 오류/u);
    put(core.pool.allocated(regular.form), NEXT, next, 2);
});

test('RawCampaignAssetBindings: 고르지 않은 총 효과는 그 효과가 호출될 때 거부한다', () => {
    // 훅별 누락 문구이다. 각 장면은 그 훅 하나만 빼고 나머지를 기록 장치로 채운다.
    const messages = {
        targetingMode: /현재 공격방향 getter 미연결/u,
        findTarget: /실제 표적 finder 미연결/u,
        refreshRange: /현재시각 압축\/공간통지 미연결/u,
        shotContext: /현재8DWORD 발사문맥 미연결/u,
        shoot: /실제 MayFire 미연결/u,
        firingInterval: /실제 TYPE 발사간격 미연결/u,
    };
    // 훅 하나를 뺀 장면에서 그 훅에 닿는 동작을 실행한다.
    for (const [name, pattern] of Object.entries(messages)) {
        const holder = { scene: null };
        const gun = gunEffects(holder, { shotContext: () => [1, 2, 3, 4, 5, 6, 7, 8], shoot: () => true });
        delete gun[name];
        const scene = grounded({ effects: { gun } });
        holder.scene = scene;
        const cannon = scene.pop(SUN_CANNON, 25, 25, 3, 1);
        const target = scene.pop(RESIDENCE, 40, 40, 3, 2);
        const fields = gunOf(scene, cannon).fields;
        const act = () => {
            if (name === 'targetingMode' || name === 'findTarget') {
                scene.bindings.gun.frame(fields);
                scene.bindings.gun.frame(fields);
            } else {
                scene.bindings.gun.setTarget(fields, target);
                scene.bindings.gun.fire(fields);
            }
        };
        assert.throws(act, pattern, name);
    }
    // 단독 Construct 는 실제 CreateGunProcess 가 필요하다는 경계로 거부한다.
    const scene = grounded();
    const cannon = scene.pop(SUN_CANNON, 25, 25, 3, 1);
    assert.throws(() => scene.bindings.gun.construct(new GunProcessFields(), cannon), /실제 CreateGunProcess가 필요합니다/u);
});

test('RawCampaignAssetBindings: 거주 구역 최초 Pop 이 지면 owner·거주 목록·예약 0/1/3 을 같은 Core 에 만든다', () => {
    const scene = grounded({}, 2);
    const { core } = scene;
    core.processState.now = 5.5;
    // 신전 genus 는 flags2 의 0x200 뿐이다(이웃 비트 1 이 있어도 일반 owner 경로다).
    core.types[RESIDENCE].flags2 |= 1;
    const residence = scene.pop(RESIDENCE, 25, 25, 3, 0);
    // 지면(owner 2)의 소유자를 일반 Core.owner 로 지정한다. 지정 직전에 현재 시각을 준비한다.
    assert.equal(core.pool.slot(residence)[OWNER], 2);
    assert.equal(scene.gunState.now, 5.5);
    assert.equal(scene.assets.residences.count, 1);
    assert.equal(scene.assets.residences.entries[0], residence);
    assert.deepEqual([0, 1, 2, 3].map((event) => core.processes.findEvent(residence, event) instanceof RegularProcess), [true, true, false, true]);
    // 가상 owner 효과를 주면 Core.owner 대신 그 효과가 맡는다(owner 는 효과가 쓴 값).
    const calls = [];
    const virtual = grounded({ effects: { postPop: { setOwner: (sid, owner) => { calls.push([sid, owner, virtual.gunState.now]); } } } }, 3);
    virtual.core.processState.now = 6.5;
    const sid = virtual.pop(RESIDENCE, 25, 25, 3, 0);
    assert.deepEqual(calls, [[sid, 3, 6.5]]);
    assert.equal(virtual.core.pool.slot(sid)[OWNER], 0);
    // Regular 사건: 0 은 다음 주민 예약, 1 은 장식 조사, 3 은 지면 테마이며 사건 2 는 외부 효과가 없으면 거부한다.
    assert.equal(typeof scene.bindings.regular.handle(residence, 0, 0, 0), 'number');
    assert.equal(scene.bindings.regular.handle(residence, 1, 0, 0), 1);
    assert.equal(scene.bindings.regular.handle(residence, 3, 0, 0), Math.fround(0.1));
    assert.throws(() => scene.bindings.regular.handle(residence, 2, 0, 0), /거주 구역 실제 사건2 미연결/u);
    // 공통 Damageable 사건: 피해(6699)는 지형 피해 효과가 없으면 거부하고, abstract 가 아닌 건설 재시도(6700)는 0 이다.
    assert.throws(() => scene.bindings.regular.damageableHandle(residence, 6699, 0, 0), /지형피해\/효과 사슬 미연결/u);
    assert.equal(scene.bindings.regular.damageableHandle(residence, 6700, 0, 0), 0);
    // abstract 부모의 건설 재시도는 현재 Form 체인에서 건설 프로세스(30)를 찾고 없으면 시작 효과를 요구한다.
    core.pool.allocated(residence)[EXTRA] |= 1;
    assert.throws(() => scene.bindings.regular.damageableHandle(residence, 6700, 0, 0), /건설 프로세스 시작 미연결/u);
    core.processes.attach(new SquidProcess(), 30, residence, 0x50);
    assert.equal(scene.bindings.regular.damageableHandle(residence, 6700, 0, 0), 0);
});

test('RawCampaignAssetBindings: 신전이 아닌 owner 는 Core.owner 가 맡고 신전은 가상 owner 효과를 요구한다', () => {
    const scene = grounded({}, 2);
    scene.islands.add(2, 0);
    scene.setIsland(25, 25, 0);
    // 가상 owner 효과 없이 신전을 Pop 하면 소리 토큰 경계(추가 TYPE DWORD)부터 거부한다.
    assert.throws(() => scene.pop(WIND_VORTEX, 25, 25, 3, 2), /실제 TYPE 추가DWORD 미연결/u);
    const holder = { scene: null };
    const postPop = {
        typeDword: (sid) => holder.scene.core.pool.slot(sid)[10],
        playImpactSound: () => {},
    };
    const second = grounded({ effects: { postPop } }, 2);
    holder.scene = second;
    second.islands.add(2, 0);
    second.setIsland(25, 25, 0);
    assert.throws(() => second.pop(WIND_VORTEX, 25, 25, 3, 2), /실제 Vortex 가상 owner 미연결/u);
});

test('RawCampaignAssetBindings: 간헐천 최초 Pop 이 목록·AI 등록·비용 WORD·예약을 만들고 Regular 가 프레임을 진행한다', () => {
    const scene = grounded();
    const { core } = scene;
    // Player 2 에만 AI 포인터가 있다. 실제 하위(004153c0)를 연결하면 그 포인터·SID·등록 1 을 받는다.
    put(scene.players, 2 * PLAYER_STRIDE + PLAYER_AI, 0x11223344);
    const calls = [];
    scene.bindings.bindPatchGeyserAi((ai, sid, state) => { calls.push([ai, sid, state]); });
    const geyser = scene.pop(GEYSER, 25, 25, 3, 0);
    assert.deepEqual(calls, [[0x11223344, geyser, 1]]);
    assert.equal(scene.assets.geysers.count, 1);
    assert.equal(scene.assets.geysers.entries[0], geyser);
    assert.equal(get(core.pool.slot(geyser), WORD, 2), Math.trunc(core.types[GEYSER].cost));
    assert.ok(core.processes.findEvent(geyser, 0) instanceof RegularProcess);
    // 가득 찬 간헐천의 사건 0 은 프레임을 하나 올리고 0.03f 를 돌려준다. 다른 사건은 payload 그대로다.
    assert.equal(get(core.pool.slot(geyser), FRAME), 0);
    assert.equal(scene.bindings.regular.handle(geyser, 0, 0, 0), Math.fround(0.03));
    assert.equal(get(core.pool.slot(geyser), FRAME), 1);
    assert.equal(scene.bindings.regular.handle(geyser, 1, 0, 0.5), 0.5);
    assert.equal(get(core.pool.slot(geyser), FRAME), 1);
    // 연결한 처리기는 자산 부모를 직접 처리하고 다른 부모는 fallback 에 넘긴다. fallback 이 없으면 거부한다.
    const ground = core.hash.cell(0, 21, 21);
    const seen = [];
    const handler = scene.bindings.handler((sid, event, count, payload) => { seen.push([sid, event, count, payload]); return 42; });
    assert.equal(handler(geyser, 1, 0, 0.25), 0.25);
    assert.equal(handler(ground, 7, 3, 1.5), 42);
    assert.deepEqual(seen, [[ground, 7, 3, 1.5]]);
    assert.throws(() => scene.bindings.handler()(ground, 0, 0, 0), /Regular 부모 Handle 미연결/u);
    // 실제 AI 하위를 연결하지 않은 장면은 비영 AI 포인터에서 경계 오류로 거부한다.
    const unbound = grounded();
    put(unbound.players, 5 * PLAYER_STRIDE + PLAYER_AI, 0x00500000);
    assert.throws(() => unbound.pop(GEYSER, 25, 25, 3, 0), (error) => error instanceof GeyserAiRegistrationBoundary && error.owner === 5 && error.aiPointer === 0x00500000 && error.state === 1);
});

// 신전 후처리의 외부 효과를 기록 장치로 채운다. (2026-11-15 WB04-E561)
function vortexEffects(holder, overrides = {}) {
    const push = (text) => { holder.scene.events.push(text); };
    return {
        typeDword: (sid, offset) => { push(`typeDword:${sid}:${offset.toString(16)}`); return holder.scene.core.pool.slot(sid)[10]; },
        playImpactSound: (token) => { push(`impact:${token}`); },
        regionNumber: (x, y) => { push(`region:${x},${y}`); return 0; },
        hasContent: (sid, type) => { push(`hasContent:${sid}:${type}`); return false; },
        createSlot: (sid, type) => { push(`createSlot:${sid}:${type}`); },
        refreshProduction: () => { push('refreshProduction'); },
        setOwner: (sid, owner) => { push(`setOwner:${sid}:${owner}:${holder.scene.gunState.now}`); holder.scene.core.owner.set(sid, owner); },
        ...overrides,
    };
}

test('RawCampaignAssetBindings: 신전 최초 Pop 이 프레임 구간·섬 기록·소유자·슬롯·생산 갱신을 순서대로 연결한다', () => {
    const holder = { scene: null };
    const scene = grounded({ effects: { postPop: vortexEffects(holder) } }, 2);
    holder.scene = scene;
    const { core } = scene;
    scene.islands.add(2, 0);
    scene.setIsland(25, 25, 0);
    scene.setIsland(45, 45, 0);
    core.processState.now = 3.5;
    const wind = scene.pop(WIND_VORTEX, 25, 25, 3, 2);
    // 소리 토큰(TYPE+0x74)만 외부 효과로 가고 프레임 구간 두 DWORD(+0x130/+0x134)는 현재 프레임 표에서 직접 읽는다.
    assert.deepEqual(scene.take(), [`typeDword:${wind}:74`, 'impact:123', `setOwner:${wind}:2:3.5`, `hasContent:${wind}:133`, `createSlot:${wind}:133`, 'refreshProduction']);
    assert.equal(scene.islands.temple(0), wind);
    assert.equal(scene.assets.temples.count, 1);
    assert.equal(scene.assets.templeCount, 1);
    assert.equal(get(core.pool.slot(wind), FRAME), scene.frames[WIND_VORTEX].run('A').first);
    assert.ok(core.processes.findEvent(wind, 1) instanceof RegularProcess);
    // 레인 신전은 'B' 구간의 첫 프레임을 쓴다(윈드와 달리 +0x134 를 읽는다). 같은 지역의 앞 신전은 boss 가 삭제한다(flags 0x100000).
    assert.notEqual(scene.frames[RAIN_VORTEX].run('B').first, scene.frames[RAIN_VORTEX].run('A').first);
    const rain = scene.pop(RAIN_VORTEX, 45, 45, 3, 2);
    assert.equal(get(core.pool.slot(rain), FRAME), scene.frames[RAIN_VORTEX].run('B').first);
    assert.equal(scene.islands.temple(0), rain);
    const events = scene.take();
    assert.ok(events.indexOf('region:25,25') >= 0 && events.indexOf('region:25,25') < events.indexOf(`destroy:${wind}:100000`));
    assert.equal(scene.islands.temple(0) & 0xffff, rain);
    assert.equal(events.filter((item) => item.startsWith('destroy:')).length, 1);
    // 신전 Regular: 사건 1 은 프레임을 진행하고 payload 를 돌려주며, 사건 2 는 0.1f, 사건 4 의 첫 호출은 1 이다.
    assert.equal(scene.bindings.regular.handle(rain, 1, 0, 0.75), 0.75);
    assert.equal(scene.bindings.regular.handle(rain, 2, 0, 0), Math.fround(0.1));
    assert.equal(scene.bindings.regular.handle(rain, 4, 0, 0), 1);
    // 외부 효과가 없는 사건 0(Ascend)과 사건 3(이탈)은 그 경계에서 거부한다.
    assert.throws(() => scene.bindings.regular.handle(rain, 0, 0, 0), /신전 실제 Ascend 진행 미연결/u);
    assert.throws(() => scene.bindings.regular.handle(rain, 3, 0, 0), /다른소유자 섬 조회 미연결|Player 이탈 UI 통지 미연결/u);
});

test('RawCampaignAssetBindings: 섬 번호 getter 가 표면 genus·다리·지도 밖·비유한 좌표를 원문처럼 가른다', () => {
    // 신전 이탈(사건 3, count 3)이 현재 좌표의 섬 번호를 unsigned 로 조회해 하위 BYTE 를 요청에 싣는다.
    const requests = [];
    const holder = { scene: null };
    const regular = {
        countForeignOwnedIslands: () => 0,
        playerLeavingNotice: () => {},
        requestLeave: (sid, island) => { requests.push([sid, island]); },
    };
    const scene = grounded({ effects: { postPop: vortexEffects(holder), regular } }, 2);
    holder.scene = scene;
    const { core } = scene;
    scene.islands.add(2, 0);
    scene.islands.add(2, 1);
    scene.setIsland(25, 25, 1);
    const temple = scene.pop(WIND_VORTEX, 25, 25, 3, 2);
    assert.equal(scene.islands.temple(1), temple);
    assert.equal(scene.bindings.regular.handle(temple, 3, 3, 0), 0);
    assert.deepEqual(requests, [[temple, 1]]);
    // 좌표를 지도 밖/빈 칸/다리 칸으로 옮기면 127 이다. 소수 좌표는 먼저 절삭한다.
    const move = (x, y) => {
        const raw = core.pool.allocated(temple);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        view.setFloat32(14, x, true);
        view.setFloat32(18, y, true);
    };
    const ask = (x, y) => {
        move(x, y);
        requests.length = 0;
        scene.bindings.regular.handle(temple, 3, 3, 0);
        return requests[0][1];
    };
    assert.equal(ask(25.9, 25.9), 1);
    assert.equal(ask(10, 25), 127);
    assert.equal(ask(-1, 25), 127);
    assert.equal(ask(25, -1), 127);
    assert.equal(ask(256, 25), 127);
    assert.equal(ask(25, 256), 127);
    assert.equal(ask(255.5, 25), 127);
    // 다리 genus(4)는 섬 번호 WORD 가 있어도 127 이고, noIsland·섬 받침 genus 는 자기 WORD 를 그대로 돌려준다(WORD 전체를 읽고 요청에는 하위 BYTE 만 싣는다).
    const bridge = scene.world.create(BRIDGE, 0);
    put(core.pool.allocated(bridge), 8, 3, 2);
    core.hash.setCell(0, 12, 12, bridge);
    assert.equal(ask(12, 12), 127);
    const hole = scene.world.create(NO_ISLAND, 0);
    put(core.pool.allocated(hole), 8, 0x0105, 2);
    core.hash.setCell(0, 12, 14, hole);
    assert.equal(ask(12, 14), 5);
    assert.equal(ask(0, 0), 127);
    core.hash.setCell(0, 0, 0, hole);
    core.hash.setCell(0, 255, 255, hole);
    assert.equal(ask(0, 0), 5);
    assert.equal(ask(255, 255), 5);
    // 다리 genus 는 비트 4 뿐이다: noIsland 에 이웃 비트 1 이 더 있어도 자기 WORD 를 돌려준다.
    core.types[NO_ISLAND].flags2 |= 1;
    assert.equal(ask(12, 14), 5);
    core.types[NO_ISLAND].flags2 &= ~1;
    // 표면/다리/noIsland genus 가 아닌 객체가 0 단계 칸에 있으면 원본 genus assert 이다(비트 1 만 있는 타입도 표면 genus 가 아니다).
    core.hash.setCell(0, 13, 13, temple);
    move(13, 13);
    assert.throws(() => scene.bindings.regular.handle(temple, 3, 3, 0), /island getter 원본 genus assert/u);
    const saved = core.types[WIND_VORTEX].flags2;
    core.types[WIND_VORTEX].flags2 = 1;
    assert.throws(() => scene.bindings.regular.handle(temple, 3, 3, 0), /island getter 원본 genus assert/u);
    core.types[WIND_VORTEX].flags2 = saved;
    // TYPE BYTE 가 표 크기(188)와 같은 객체도 표 밖이므로 같은 assert 이다.
    core.pool.allocated(hole)[10] = 188;
    move(12, 14);
    assert.throws(() => scene.bindings.regular.handle(temple, 3, 3, 0), /island getter 원본 genus assert/u);
    core.pool.allocated(hole)[10] = NO_ISLAND;
    // int32 밖·비유한 좌표는 조회 전에 거부한다. 경계 -2147483648 은 허용 범위다(지도 밖 127).
    move(3e9, 25);
    assert.throws(() => scene.bindings.regular.handle(temple, 3, 3, 0), /좌표 원본 정수 범위 오류/u);
    move(25, Number.NaN);
    assert.throws(() => scene.bindings.regular.handle(temple, 3, 3, 0), /좌표 원본 정수 범위 오류/u);
    move(2147483648, 25);
    assert.throws(() => scene.bindings.regular.handle(temple, 3, 3, 0), /좌표 원본 정수 범위 오류/u);
    assert.equal(ask(-2147483648, 25), 127);
});

test('RawCampaignAssetBindings: 발사기 최초 Pop 은 현재 입력 명령 토큰으로 Animation 제거를 큐에 넣는다', () => {
    const scene = grounded();
    const archer = scene.pop(SUN_ARCHER, 25, 25, 3, 1);
    const events = scene.take();
    assert.equal(events[0], `queue:30000b0:${archer}:0`);
    assert.match(events[1], /^gun:013a/u);
    // 다음 Pop 은 그 시점의 토큰 DWORD 를 다시 읽는다.
    scene.archerCommand.value = 0x030000b1;
    const second = scene.pop(SUN_ARCHER, 40, 40, 3, 1);
    assert.equal(scene.take()[0], `queue:30000b1:${second}:0`);
    // 입력 명령 큐가 없으면 그 경계에서 거부한다.
    const silent = grounded({ effects: { queueCommand: undefined } });
    assert.throws(() => silent.pop(SUN_ARCHER, 25, 25, 3, 1), /실제 입력명령 큐 미연결/u);
    // 제네레이터는 총 프로세스나 큐 없이 공통 Damageable 만 거친다.
    const generator = scene.pop(GENERATOR, 55, 55, 3, 1);
    assert.deepEqual(scene.take(), []);
    assert.equal(gunOf(scene, generator), null);
});

test('RawCampaignAssetBindings: Factory/아웃포스트 후처리가 예약·상태 프레임·Level 진단·지역 소유권·중복 경고를 연결한다', () => {
    const holder = { scene: null };
    const factoryEffects = {
        warnDuplicate: (sid, x, y) => { holder.scene.events.push(`duplicate:${sid}:${x}:${y}`); },
        regionOwnership: (x, y) => { holder.scene.events.push(`region:${x}:${y}`); },
    };
    const scene = grounded({ effects: { factory: factoryEffects, levelAssertion: () => { holder.scene.events.push('level'); } } });
    holder.scene = scene;
    const { core } = scene;
    // 썬 워크샾(첫 타이머)은 사건 2·5, 레인 워크샾(둘째 타이머)은 사건 1·5 를 예약한다.
    const sun = scene.pop(SUN_FACTORY, 25, 25, 3, 1);
    assert.deepEqual([1, 2, 5].map((event) => core.processes.findEvent(sun, event) instanceof RegularProcess), [false, true, true]);
    const rain = scene.pop(RAIN_FACTORY, 45, 25, 3, 1);
    assert.deepEqual([1, 2, 5].map((event) => core.processes.findEvent(rain, event) instanceof RegularProcess), [true, false, true]);
    assert.deepEqual(scene.take(), []);
    // 가상 +84: Level 저장 → 기본 표시 갱신. 프레임 보정(00453780)은 'A'+Level 구간 밖일 때 그 구간의 첫 프레임을 쓴다.
    scene.bindings.setState(sun, 2);
    assert.equal(core.pool.slot(sun)[11] >> 5, 2);
    assert.equal(get(core.pool.slot(sun), FRAME), 0);
    scene.bindings.adjustFactoryFrame(sun);
    assert.equal(get(core.pool.slot(sun), FRAME), scene.frames[SUN_FACTORY].run('C').first);
    // 범위 밖 Level 은 진단 뒤 하위 3비트를 저장한다.
    scene.bindings.setState(sun, 9);
    assert.deepEqual(scene.take(), ['level']);
    assert.equal(core.pool.slot(sun)[11] >> 5, 1);
    // Factory 가 아닌 부모의 가상 +84 는 거부한다.
    assert.throws(() => scene.bindings.setState(core.hash.cell(0, 21, 21), 1), TypeError);
    // 아웃포스트 최초 Pop 은 아웃포스트·거주 목록에 추가하고 현재 좌표로 지역 소유권을 다시 계산한다.
    const outpost = scene.pop(OUTPOST, 25, 45, 3, 1);
    assert.deepEqual(scene.take(), ['region:25:45']);
    assert.equal(scene.assets.outposts.entries[0], outpost);
    assert.equal(scene.assets.residences.entries[0], outpost);
    // 같은 칸의 같은 TYPE 워크샾은 boss 가 경고 뒤 삭제를 요청한다. 일반 Pop 은 점유된 자리를 거부하므로 다른 자리에 Pop 한 쌍둥이의 좌표를 옮겨 후처리만 다시 실행한다.
    const twin = scene.pop(SUN_FACTORY, 45, 45, 3, 1);
    const raw = core.pool.allocated(twin);
    raw.set(core.pool.slot(sun).subarray(14, 22), 14);
    core.postPop.postPopVirtual(twin, 0);
    assert.deepEqual(scene.take(), [`duplicate:${sun}:25:25`, `destroy:${sun}:0`]);
    // boss 가 아니면 중복을 조사하지 않는다.
    scene.world.storage.authority.boss = false;
    core.postPop.postPopVirtual(twin, 0);
    assert.deepEqual(scene.take(), []);
    scene.world.storage.authority.boss = true;
});

test('RawCampaignAssetBindings: 고르지 않은 Factory·자산 효과는 도달한 위치에서 거부한다', () => {
    // Level 진단이 없는 연결은 범위 밖 Level 에서 거부한다(범위 안 저장은 진단을 부르지 않는다).
    const plain = grounded();
    const factory = plain.pop(SUN_FACTORY, 25, 25, 3, 1);
    plain.bindings.setState(factory, 7);
    assert.equal(plain.core.pool.slot(factory)[11] >> 5, 7);
    assert.throws(() => plain.bindings.setState(factory, 8), /Factory 실제 Level 진단 미연결/u);
    // 지역 소유권 효과가 없는 아웃포스트 Pop, 중복 경고가 없는 중복 워크샾 Pop 은 각 경계에서 거부한다.
    assert.throws(() => grounded().pop(OUTPOST, 25, 25, 3, 1), /아웃포스트 실제 지역 소유권 미연결/u);
    const duplicate = grounded();
    const original = duplicate.pop(SUN_FACTORY, 25, 25, 3, 1);
    const copy = duplicate.pop(SUN_FACTORY, 45, 45, 3, 1);
    duplicate.core.pool.allocated(copy).set(duplicate.core.pool.slot(original).subarray(14, 22), 14);
    assert.throws(() => duplicate.core.postPop.postPopVirtual(copy, 0), /Factory 중복 진단 미연결/u);
    // 예약 확보 경계가 없으면 자산·Factory 모두 첫 예약에서 거부한다.
    const noReserve = { postPop: { reserveRegular: undefined } };
    assert.throws(() => grounded({ effects: noReserve }).pop(RESIDENCE, 25, 25, 3, 0), /자산 실제 Regular 확보 경계 미연결/u);
    assert.throws(() => grounded({ effects: noReserve }).pop(SUN_FACTORY, 25, 25, 3, 1), /Factory 실제 Regular 확보 미연결/u);
    // 예약 확보가 거짓이면 예약을 만들지 않고 계속한다(실패한 예약만 생략).
    const refused = grounded({ effects: { postPop: { reserveRegular: () => false } } });
    const sid = refused.pop(SUN_FACTORY, 25, 25, 3, 1);
    assert.deepEqual([2, 5].map((event) => refused.core.processes.findEvent(sid, event)), [null, null]);
});
