// cpppj RawPriestWorldBindingsTests.cpp의 PATCH PriestWorld를 같은 raw 모듈로 조립한다. 실제 게임 실행이 아니다. (2026-11-16 WB04-E563)
import assert from 'node:assert/strict';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { makeSquidFrameHooks } from '../www/core/squid-frame-binding.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';
import { SquidFormDispatch } from '../www/core/squid-form-dispatch.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { Kernel } from '../www/core/kernel.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { RawDamageableRelease, DamageableReleaseState } from '../www/core/raw-damageable-release.js';
import { RawDamageablePreDestroy, DamageablePreDestroyState } from '../www/core/raw-damageable-pre-destroy.js';
import { RawPriestPlacement } from '../www/core/raw-priest-placement.js';
import { createPlacementState } from '../www/core/raw-canon-placement.js';
import { RawPriestWorldBindings, RawPriestWorldState } from '../www/core/raw-priest-world-bindings.js';
import { bitsToFloat, get, floatToBits } from './raw-support.mjs';

// 이 장면에서 미도달인 외부 장치는 실행을 즉시 거부한다. (2026-11-16 WB04-E563)
export const unreachable = () => { throw new Error('사제 조립 검사에서 미도달 장치'); };
// 사제·보호막·받침 TYPE와 첫 사제 위치다. (2026-11-16 WB04-E563)
export const PRIEST = 158;
export const FIELD = 167;
export const SUPPORT = 100;

// 같은 슬롯/공간/장부/Kernel을 소유하는 합성 통합 장면이다. (2026-11-16 WB04-E563)
export class PriestWorld {
    // cpppj와 같은 초기 TYPE/프레임/공간과 실제 장치를 구성한다. (2026-11-16 WB04-E563)
    constructor({ bind = true } = {}) {
        // 물리 입력과 하위 소유자들은 검사 종료까지 유지한다.
        this.pool = new SidPool(32768, { server: true });
        this.types = Array.from({ length: 188 }, (_, type) => ({ flags1: 0, flags2: 0, cost: 0, damageEffect: 0, maxHitPoints: 0, footX: 0, footY: 0,
            constructorAddress: [1, 2, 3, 6, PRIEST, FIELD].includes(type) ? typeConstructorAddress(type) : 0 }));
        Object.assign(this.types[PRIEST], { maxHitPoints: 200, flags1: 0x69012, flags2: 0x210000, footX: 1, footY: 1 });
        Object.assign(this.types[FIELD], { footX: 1, footY: 1 });
        Object.assign(this.types[SUPPORT], { flags1: 0x800, flags2: 2, footX: 1, footY: 1 });
        // cpppj의 AP/EP/JP 프레임 배열이며 단계 기대값은 원문의 고정 숫자다.
        const codes = (text) => new FrameCodeTable([...text].map((side) => ({ side: side.charCodeAt(0), variant: 80, flags: 0 })));
        this.frames = this.types.map(() => codes(''));
        this.frames[PRIEST] = codes('AEJJ'); this.frames[FIELD] = codes('AAA'); this.frames[SUPPORT] = codes('A');
        this.shapes = this.types.map(() => ({ loaded: false, frameCount: 0, frames: [] }));
        // 실제 공간 프레임 메타는 cpppj와 같은 16×11 픽셀·1×1칸이다.
        for (const type of [PRIEST, FIELD, SUPPORT]) {
            const count = this.frames[type].codes.length;
            this.shapes[type] = { loaded: true, frameCount: count, frames: Array.from({ length: count * ((this.types[type].flags1 & 0x40000) ? 2 : 1) }, () => ({ width: 16, height: 11, hotspotX: 8, hotspotY: 11, cellWidth: 1, cellHeight: 1 })) };
        }
        this.hash = new SquidHash(); this.spots = new Uint8Array(65536); this.book = new SquidPostPopState(); this.book.localOwner = 1;
        this.random = new GameRandom(); this.store = new ScrambledSpStore(this.random, () => 1); this.store.initialize(); this.store.set(1, floatToBits(1000.5));
        this.hp = new SquidReward(this.pool, this.types, this.book, new SquidRewardState(), this.store);
        this.state = new RawPriestWorldState(); this.state.priests.priests.entries.length = 100;
        this.state.regen.patch.sequence = this.store.offset | 0; this.state.regen.patch.observedSequence = this.store.offset | 0;
        this.ownerMode = new SquidOwnerMode(); this.ownerMode.battle = true;
        this.display = new SquidDisplay(this.types, this.shapes, { suppressed: () => false, invalidate: () => {} }, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } });
        this.base = new SquidPostPop(this.pool, this.types, this.book);
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots, this.display);
        this.popBody = new SquidPop(this.pool, this.hash, this.spots, { display: this.display, postPop: this.base });
        // C++ 독립 장면의 외부 전송 요청을 관찰한다. 패킷 수신/네트워크 조립은 이 장면의 범위 밖이다.
        this.transmissions = [];
        this.popBody.bindTransmission((sid, flags) => { this.transmissions.push([sid, flags]); });
        this.factory = new SquidFactory(this.pool, this.types, { unpop: this.unpop });
        this.owner = new SquidOwner(this.pool, this.types, this.book, this.ownerMode);
        this.frame = new SquidFrame(this.pool, this.types, makeSquidFrameHooks(this.pool, this.types, this.display, this.unpop, this.popBody));
        // 받침은 실제 hash0/spot에 Pop으로 넣고 별도 표면 사본을 만들지 않는다.
        for (const [x, y] of [[30, 31], [40, 41]]) { const sid = this.factory.create(SUPPORT); this.owner.set(sid, 0); this.pop(sid, x, y); }
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.deletion = new SquidDeletionState(); this.damageRequests = 0;
        this.common = new SquidDestroyLifecycle(this.pool, this.types, this.book, this.deletion, this.destroy, {
            emit: () => { this.damageRequests += 1; }, forward: (event) => { assert.equal(event.effect, SQUID_DESTROY_EFFECT.transmit); },
        }, null, this.hp);
        this.kernel = new Kernel(); this.clock = new SquidProcessState();
        // 외부 경계의 관찰 횟수이며 게임 계산을 대신하지 않는다.
        this.claims = 0; this.ownerNotices = 0; this.soundLoads = 0; this.soundAttachments = 0; this.notices = 0; this.falls = 0; this.healthRequests = 0; this.audits = 0; this.fallbackEvents = 0;
        this.effects = {
            pop: (sid, x, y, flags) => { this.pop(sid, x, y, flags); },
            unpop: (sid, flags) => { this.unpop.unpop(sid, this.types[this.pool.slot(sid)[10]], flags); },
            repop: (sid, flags) => { const raw = this.pool.slot(sid); this.pop(sid, bitsToFloat(get(raw, 14)), bitsToFloat(get(raw, 18)), flags); },
            destroy: (sid, flags) => { assert.equal(this.dispatch.destroy(sid, flags), true); },
            claimGround: () => { this.claims += 1; }, ownerNotice: () => { this.ownerNotices += 1; }, notifySurface: unreachable, dependentFound: unreachable,
            reserveRegular: () => true, fallSound: () => { this.falls += 1; },
            shield: { reserveSound: () => true, loadSound: (file) => { assert.equal(file, 'priestForceField.wav'); this.soundLoads += 1; return 17; },
                attachSound: (sid, sound, flags, mode) => { assert.equal(this.pool.slot(sid)[10], FIELD); assert.deepEqual([sound, flags, mode], [17, 0, 1]); this.soundAttachments += 1; },
                noticeSound: () => { this.notices += 1; }, translate: (key) => { assert.equal(key, 'PriestImmobile'); return key; }, tell: (text) => { assert.equal(text, 'PriestImmobile'); } },
            regen: { refresh: (sid) => { this.display.update(this.pool.slot(sid)); }, damageable: () => { this.healthRequests += 1; }, neutralAction: unreachable,
                patchAudit: () => { this.store.rotate(); this.state.regen.patch.observedSequence = this.store.offset | 0; this.audits += 1; }, measurement: unreachable },
        };
        this.core = { pool: this.pool, hash: this.hash, types: this.types, frames: this.frames, spots: this.spots, surfaces: this.hash.view(0), reward: this.hp,
            random: this.random, factory: this.factory, owner: this.owner, ownerMode: this.ownerMode, postPop: this.base, frame: this.frame };
        if (bind) this.bind();
    }
    // 사제 순환 연결을 실제 Form/Host/삭제/배치 몸체로 해결한다. (2026-11-16 WB04-E563)
    bind() {
        this.bindings = new RawPriestWorldBindings(this.core, this.state, this.effects);
        this.bindings.setLoadingDepth(1); this.bindings.setLocalPlayer(1); this.bindings.setAuthority(true);
        this.dispatch = new SquidFormDispatch(this.pool, this.destroy, this.bindings.destroyHooks(this.common.hooks()));
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.clock, {
            regular: this.bindings.regularHandler((sid, event, count, payload) => { this.fallbackEvents += 1; return payload; }), asset: this.dispatch.hooks(),
        });
        this.dispatch.bind(this.host); this.bindings.bindProcesses(this.host);
        const releaseState = new DamageableReleaseState(); releaseState.battle = true;
        this.release = new RawDamageableRelease(this.pool, this.types, this.spots, this.state.contained, releaseState, {
            spawnPriest: (x, y, type, word) => { this.bindings.spawn.spawn(x, y, type, word); }, create: (type, flags) => this.factory.create(type, flags),
            setOwner: (sid, player) => { this.bindings.setOwner(sid, player); }, pop: this.effects.pop, notifySurface: unreachable,
        });
        this.damageable = new RawDamageablePreDestroy(this.pool, this.spots, this.state.contained, new DamageablePreDestroyState(), {
            collapse: () => { this.damageRequests += 1; }, explosion: () => { this.damageRequests += 1; }, sound: () => { this.damageRequests += 1; },
            releaseContained: (sid) => { this.release.release(sid); }, validateBase: (sid, flags) => { this.common.validatePre(sid, flags); }, base: (sid, flags) => { this.common.preDestroy(sid, flags); },
        });
        this.bindings.bindDamageable(this.damageable);
        this.placementState = createPlacementState(); this.placementState.forcePlacement = 1;
        this.placement = new RawPriestPlacement(this.pool, this.types, this.placementState, { shape: unreachable, inspectGeometry: unreachable });
        this.bindings.bindPlacement(this.placement); this.bindings.validateReady();
    }
    // 현재 프레임의 실제 SHP 칸 크기로 Pop한다. (2026-11-16 WB04-E563)
    pop(sid, x, y, flags = 0) {
        const raw = this.pool.slot(sid); const current = get(raw, 36) | 0;
        const size = this.display.frameSize(raw, current);
        assert.equal(this.popBody.pop(sid, this.types[raw[10]], size[0], size[1], x, y, flags), RAW_POP_RESULT.registered);
    }
    // 최초 사제는 실제 생성자·가상 소유자·Pop만으로 만든다. (2026-11-16 WB04-E563)
    add(x = 30, y = 31) { const sid = this.factory.create(PRIEST); this.bindings.setOwner(sid, 1); this.pop(sid, x, y); return sid; }
}
