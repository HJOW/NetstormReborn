// 생성자1653/ConnectDamageable2626..2705의 독립 typed 수명을 같은 World에 구성한다(225553df, WB04-E604).
// 사용: connectConstruction 뒤 bind. 첫 저장 객체 이전의 Dude/해방/피해·신전·간헐천 삭제 부모이며 World 게시는 뒤 구간이다.
import { RawDudeSpawn, RawDudePostPop, createDudeSpawnState, makeDudePostPopHooks } from './raw-dude-spawn.js';
import { RawDudeTask, makeDudeTaskHostHooks } from './raw-dude-task.js';
import { RawDudeBehavior } from './raw-dude-behavior.js';
import { RawDudeRegular } from './raw-dude-regular.js';
import { RawDudeDrop } from './raw-dude-drop.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RawDamageableRelease } from './raw-damageable-release.js';
import { DamageablePreDestroyState, RawDamageablePreDestroy, DAMAGEABLE_SOUND } from './raw-damageable-pre-destroy.js';
import { RawDamageablePostDestroy } from './raw-damageable-post-destroy.js';
import { RawBombPreDestroy } from './raw-bomb-pre-destroy.js';
import { RawFlyerPreDestroy } from './raw-flyer-pre-destroy.js';
import { RawVortexPreDestroy } from './raw-vortex-pre-destroy.js';
import { RawGeyserLifetime } from './raw-geyser-lifetime.js';
import { RawGeyserAiRegistration } from './raw-geyser-ai-registration.js';
import { makePriestSpawnHooks } from './raw-priest-spawn.js';
import { hasScheduledBridgeFall } from './squid-process.js';
import { ShrapnelKind } from './raw-shrapnel-frame.js';

// 도달한 실제 하위만 요구하고 빈 성공을 만들지 않는다.
function need(value, message) { if (!value) throw new Error(message); return value; }
// Dude.cpp 진단을 원문(RawSurfaceStorage::DudeAssert)과 같은 식·파일·행으로 표시하고 계속한다: 0xde `!s.isDead() && !s.isFree()`, 0x127 `walkSid`, 0x9c `task == taskFALL`, 그 밖은 `0`. (2026-11-22 WB04-E1211)
function dudeAssertion(owner, line) {
    const expression = line === 0xde ? '!s.isDead() && !s.isFree()' : line === 0x127 ? 'walkSid' : line === 0x9c ? 'task == taskFALL' : '0';
    owner.world.storage.assertDisplay(expression, '.\\Dude.cpp', line);
}
// 현재 비정렬 좌표를 단정도 그대로 읽는다.
function coordinate(raw, offset) { return new DataView(raw.buffer, raw.byteOffset + offset, 4).getFloat32(0, true); }
// 원문 상수float 비트를 수로 해석한다.
function float(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits, true); return view.getFloat32(0, true); }

// 기존 실제 모듈의 생성 순서와 값형 훅을 소유하는 Session 구간이다.
export class RawCampaignSessionDamageable {
    // 부분 실패/한 번 설치와 반납 뒤 closure의 접근을 구별한다.
    #owner; #started = false; #ready = false; #disposed = false; #dudeRegistered = false;
    // 실제 하위 몸체에 전달한 상태/효과 표다. 시험과 후속 결선이 같은 참조를 재사용한다.
    spawnState = null; preState = null; behaviorState = null; regularState = null;
    behaviorHooks = null; regularHooks = null; releaseHooks = null; preHooks = null; postHooks = null; vortexHooks = null; geyserHooks = null;

    // 선언 상태를 같은 기반 owner로 확보하고 population/사제 타입을 새 월드로 복사하지 않는다.
    constructor(owner) { this.#owner = owner; this.spawnState = createDudeSpawnState(owner.dudePopulation); this.preState = new DamageablePreDestroyState(); }

    // Dude→해방→공통 전/후처리→신전/간헐천 순서로 같은 World를 연결한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.constructionModules?.ready || this.#started || o.dudeDrop) throw new Error('Session Damageable의 초기화 전/반복 결선');
        this.#started = true;
        const destroy = (sid, flags) => { this.#live(); return o.world.destroy(sid, flags); };
        const pop = (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); };
        const notifySurface = (sid) => { this.#live(); c.building.notifySurface(sid); };
        const find = (parent, type, index, remove) => { this.#live(); return new RawContainedLookup(c.pool, destroy).find(parent, type, index, remove); };
        o.dudeSpawn = new RawDudeSpawn(c.pool, c.factory, c.owner, this.spawnState, { pop });
        o.dudePostPop = new RawDudePostPop(c.pool, c.frames, this.spawnState, { ...makeDudePostPopHooks(c.pool, c.frame, c.postPop), notifySurface }, () => c.bridgeState.bridgeType);
        c.postPop.setBody('dude', o.dudePostPop); this.#dudeRegistered = true;
        o.dudeTask = new RawDudeTask(c.pool, { clock: c.processState, bridgeType: c.bridgeState.bridgeType }, makeDudeTaskHostHooks(c.pool, c.processes, { notifySurface, assertion: () => { this.#live(); dudeAssertion(o, 0x9c); } })); // 낙하 교체 진단은 원문(RawSurfaceStorage::DudeTask)처럼 표시 후 계속한다. (2026-11-22 WB04-E1211)
        const task = (sid, event, delay) => { this.#live(); o.dudeTask.set(sid, event, delay); };
        const behavior = {
            task, movementType: (type) => { this.#live(); return o.priests.movementProcessType(type); }, find, destroy,
            basePostDestroy: (sid, flags) => { this.#live(); o.world.commonDeletion.postDestroy(sid, flags); },
            assertion: (line) => { this.#live(); dudeAssertion(o, line); }, // 원문 DudeAssert 와 같이 표시한 뒤 계속한다. (2026-11-22 WB04-E1211: 이전에는 오류를 던졌다)
        };
        this.behaviorHooks = behavior; this.behaviorState = { checking: () => o.gameplayGlobals.aiDebug().checking().value, population: o.dudePopulation };
        o.dudeBehavior = new RawDudeBehavior(c.pool, c.spots, this.behaviorState, behavior);
        const regular = {
            checkSpace: (sid) => { this.#live(); return o.dudeBehavior.checkSpace(sid); }, findPath: (parent, type) => find(parent, type, 0, false), task,
            nearestResidence: (sid) => { this.#live(); return o.hits.dudes.nearestResidence(sid); }, entrance: (sid) => { this.#live(); const point = o.hits.dudes.residencePoint(sid); return { x: point[0], y: point[1] }; },
            move: (sid, point, quickly) => { this.#live(); return o.hits.dudes.move(sid, [point.x, point.y], quickly); },
            setFrame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame, flags); },
            unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); }, pop: (sid, point, flags) => pop(sid, point.x, point.y, flags), destroy,
            assertion: (line) => { this.#live(); dudeAssertion(o, line); }, // 부모 진단과 같은 DudeAssert 이다. (2026-11-22 WB04-E1211)
        };
        this.regularHooks = regular; this.regularState = { checking: () => o.gameplayGlobals.aiDebug().checking().value, providers: c.bookkeeping.currentProviders(), get providerIndex() { return o.dudeProviderIndex; }, set providerIndex(value) { o.dudeProviderIndex = value | 0; }, pollEvent: 0x1a2b, pathType: 24 };
        o.dudeRegular = new RawDudeRegular(c.pool, c.frames, o.random, this.regularState, regular);
        o.dudeDrop = new RawDudeDrop(c.pool, c.types, o.dudeSpawn, o.dudeTask, o.hits.dudes, o.random);
        this.releaseHooks = makePriestSpawnHooks(c.pool, o.priests.spawn, { create: (type, flags) => { this.#live(); return o.world.create(type, flags); }, setOwner: (sid, owner) => { this.#live(); o.ownership.setOwner(sid, owner); }, pop, notifySurface });
        o.damageableRelease = new RawDamageableRelease(c.pool, c.types, c.spots, o.contentFinder, o.state.damageableReleaseState, this.releaseHooks);
        const pre = { ...o.services.damageablePre }; this.preHooks = pre;
        if (!pre.collapse) pre.collapse = (sid) => { this.#live(); o.particles.addShrapnel(sid, ShrapnelKind.Collapse); };
        if (!pre.explosion) pre.explosion = (sid) => {
            this.#live(); const state = o.state.impactState;
            if (state.currentSuppressedTime === 0 && !state.suppressed && c.kernel.size < 30001) {
                const raw = c.pool.slot(sid); const x = coordinate(raw, 14); const y = coordinate(raw, 18); const foot = Math.fround(c.types[raw[10]].footX);
                o.hits.impact.visual(Math.fround(x - foot * 0.5), y, 1, float(0x3d23d70a));
            }
            o.particles.addShrapnel(sid, ShrapnelKind.Explosion);
        };
        if (!pre.sound) pre.sound = (event) => {
            this.#live(); if (event.sound === DAMAGEABLE_SOUND.priestFree) { need(o.services.sound.play, '실제 priestFree 전역 소리 미연결')(o.soundNames.lookup('priestFree.wav'), 0, 0, 0, 1, 0); return; }
            need(o.services.sound.playOnceAt, '실제 붕괴/폭발 위치 소리 미연결')(event.x, event.y, o.soundNames.lookup(event.sound === DAMAGEABLE_SOUND.collapse ? 'collapse.wav' : 'explosion.wav'), 0, 0);
        };
        pre.validateBase = (sid, flags) => { this.#live(); o.world.commonDeletion.validatePre(sid, flags); };
        pre.base = (sid, flags) => { this.#live(); o.world.commonDeletion.preDestroy(sid, flags); };
        pre.releaseContained = (sid) => { this.#live(); o.state.damageableReleaseState.battle = c.ownerMode.battle; o.damageableRelease.release(sid); };
        o.bombPre = new RawBombPreDestroy(c.pool, { explosion: pre.explosion, sound: (x, y) => { this.#live(); need(o.services.sound.playOnceAt, 'Bomb 실제 폭발 음향 미연결')(x, y, o.soundNames.lookup('explosion.wav'), 0, 0); }, base: pre.base });
        o.collapseEffect = pre.collapse; o.ritualExplosion = pre.explosion;
        o.damageablePre = new RawDamageablePreDestroy(c.pool, c.spots, o.contentFinder, this.preState, pre); o.priests.bindDamageable(o.damageablePre);
        // 공중3종은 원문 생성자/기존 두 하위만 정적으로 연결하며 전용 관측/실행 검사를 추가하지 않는다.
        o.flyerPre = new RawFlyerPreDestroy(c.pool, { destroy, damageable: (sid, flags) => { this.#live(); o.damageablePre.preDestroy(sid, flags); } });
        const post = { ...o.services.damageablePost }; this.postHooks = post;
        if (!post.dropDude) post.dropDude = (source, index, randomPoint) => { this.#live(); o.dudeDrop.drop(source, index, randomPoint); };
        if (!post.dropIsland) post.dropIsland = (x, y, flags, owner) => { this.#live(); const body = need(o.buildingEffects, '파괴 후 실제 받침 정리 설치 전 호출'); o.assetModules.prepareBuilding(c.processState.now); body.dropIsland(x, y, flags, owner); };
        post.regular = (sid, event, payload, count) => { this.#live(); const process = c.processes.addRegular(sid, event, payload); if (!process) throw new Error('실제 피해 Regular 확보 실패'); process.setInitialCount(count); };
        post.hasBridgeFall = (sid) => { this.#live(); return hasScheduledBridgeFall(c.processes, sid); }; post.notifySurface = notifySurface;
        post.displayUpdate = (sid) => { this.#live(); c.display.update(c.pool.physical(sid)); }; post.destroy = destroy;
        post.base = (sid, flags) => { this.#live(); o.world.commonDeletion.postDestroy(sid, flags); };
        // 다리 손상은 FrameCodeTable 객체가 아닌 같은 실제 표면의 프레임 코드 배열을 사용한다.
        o.damageablePost = new RawDamageablePostDestroy(c.pool, c.hash, c.types, o.world.storage.codes, o.hp, c.neighbors, o.state.damageablePostState, post);
        const refresh = o.services.ownership.refreshProduction;
        const vortex = {
            islandNumber: (x, y) => { this.#live(); return o.ownership.islandNumber(x, y); }, islandExists: (island) => { this.#live(); return Boolean(o.islands.exists(island)); }, islandTemple: (island) => { this.#live(); return o.islands.temple(island); },
            // 원문(ClientConstructionGlobals::VortexPre)처럼 섬이 없으면 Vortex.cpp 0x16f 진단을 표시하고 계속한다. (2026-11-22 WB04-E1207: 이전에는 오류를 던졌다)
            clearIslandTemple: (island) => { this.#live(); if (!o.islands.exists(island)) o.world.storage.assertDisplay('islandList.exists( islandId )', '\\Ns\\O\\Vortex.cpp', 0x16f); o.islands.setTemple(island, 0); need(o.services.ownership.refreshProduction, '신전 삭제의 실제 생산 갱신 미연결')(); },
            islandOwner: (island, owner) => { this.#live(); o.islands.setOwner(island, owner); }, regionOwnership: (x, y) => { this.#live(); o.ownership.updateRegion(x, y); },
            damageablePre: (sid, flags) => { this.#live(); o.damageablePre.preDestroy(sid, flags); }, refreshProduction: () => { this.#live(); need(refresh, '신전 삭제 후 실제 생산 갱신 미연결')(); },
            // 0x2ab 는 섬 존재, 그 밖(0x2ac)은 섬의 신전 SID 일치 검사식이며 표시 후 계속한다. (2026-11-22 WB04-E1207)
            assertion: (line) => { this.#live(); o.world.storage.assertDisplay(line === 0x2ab ? 'islandList.exists( islandId )' : 'islandList.getVortexSid( islandId ) == getSid()', '\\Ns\\O\\Vortex.cpp', line); },
        };
        this.vortexHooks = vortex;
        o.vortexPre = new RawVortexPreDestroy(c.pool, o.players, o.state.assetState.currentTemples(), o.state.assetState.currentResidences(), o.state.assetState.currentOutposts(), () => o.objectState.loadingDepth, vortex);
        const geyser = {
            defaultFrame: (sid) => { this.#live(); const type = c.pool.slot(sid)[10]; if (type < 70 || type - 70 >= o.assets.game.types().length) throw new RangeError('간헐천 기본 프레임 TYPE 오류'); return o.assets.game.types()[type - 70].definition.defaultFrame >>> 0; },
            update: (sid) => { this.#live(); c.display.update(c.pool.physical(sid)); },
            registerAi: (sid, state) => { this.#live(); const current = new RawGeyserAiRegistration(o.players); current.bindPatch((token, number, operation) => need(o.aiWorld, '간헐천 삭제의 실제 AI World 미설치').geyser(token, number, operation)); current.register(sid, state); },
            damageablePre: (sid, flags) => { this.#live(); o.damageablePre.preDestroy(sid, flags); }, damageablePost: (sid, flags) => { this.preparePost(); o.damageablePost.postDestroy(sid, flags); },
            dropIsland: (x, y, flags, owner) => { this.#live(); o.assetModules.prepareBuilding(c.processState.now); need(o.buildingEffects, '간헐천 삭제의 실제 받침 정리 미설치').dropIsland(x, y, flags, owner); },
        };
        this.geyserHooks = geyser; o.geyserLifetime = new RawGeyserLifetime(c.pool, o.state.assetState.currentGeysers(), () => o.globals.editor, geyser);
        this.#ready = true;
    }

    // 각 typed PostDestroy 직전 같은 현재 전투 모드와 signed population을 공급한다.
    preparePost() { this.#live(); const o = this.#owner; o.state.damageablePostState.battle = o.core.ownerMode.battle; o.state.damageablePostState.dudeCount = o.dudePopulation.count | 0; }
    // 반납한 Session의 closure가 이전 물리에 재진입하지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Damageable'); }
    // 실제 프로세스를 먼저 비운 상위가 Dude PostPop 등록을 반납하며 장기 목록은 보존한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; if (this.#dudeRegistered) { this.#owner.core.postPop.setBody('dude', null); this.#dudeRegistered = false; } }
    // ConnectDamageable의 정상 복귀이며 전체 World/Session 게시는 아니다.
    get ready() { return this.#ready; }
}
