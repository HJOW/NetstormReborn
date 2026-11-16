// cpppj CampaignInstallSupport.h 중 실제 World·사제·피격 설치만 조립한다. 실제 게임 실행이나 전체 InstallScene은 아니다. (2026-11-16 WB04-E564)
import { AssetScene, forbidden } from './campaign-asset-support.mjs';
import { RawPriestWorldState, RawPriestWorldBindings } from '../www/core/raw-priest-world-bindings.js';
import { RawCampaignHitBindings } from '../www/core/raw-campaign-hit-bindings.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { PriestDamageState } from '../www/core/raw-priest-damage.js';
import { DamageableHitState } from '../www/core/raw-damageable-hit.js';
import { DamageableBurningState } from '../www/core/raw-damageable-burning.js';
import { GunImpactState } from '../www/core/raw-gun-impact.js';
import { SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';

// 같은 실제 자산/풀/Host 위에 필요한 사제·피격 모듈만 설치하는 장면이다. (2026-11-16 WB04-E564)
export class CampaignHitScene extends AssetScene {
    // options.effects는 선택 외부 효과를 덮어쓰며 bind=false는 잘못된 구성 검사에 쓴다. (2026-11-16 WB04-E564)
    constructor(options = {}) {
        super({ bind: false });
        // 장기 HP/SP/사제 상태와 역할별 피격 전역이다.
        this.sp = new ScrambledSpStore(this.random, () => 1); this.sp.initialize();
        this.hp = new SquidReward(this.core.pool, this.core.types, this.core.bookkeeping, new SquidRewardState(), this.sp);
        this.priestGlobals = new RawPriestWorldState(); this.priestGlobals.priests.priests.entries.length = 100;
        this.priestGlobals.regen.patch.sequence = this.sp.offset | 0; this.priestGlobals.regen.patch.observedSequence = this.sp.offset | 0;
        this.priests = new RawPriestWorldBindings({ ...this.core, reward: this.hp, random: this.random, surfaces: this.core.hash.view(0) }, this.priestGlobals, {
            pop: (sid, x, y, flags) => { this.world.pop(sid, x, y, flags); },
            unpop: (sid, flags) => { this.world.unpop(sid, flags); }, repop: (sid, flags) => { this.world.repop(sid, flags); }, destroy: (sid, flags) => { this.world.destroy(sid, flags); },
            claimGround: (x, y, owner) => { this.world.setSupportOwner(x, y, owner); }, ownerNotice: (sid) => { this.events.push(`owner:${sid}`); },
            notifySurface: (sid) => { this.core.building.notifySurface(sid); }, dependentFound: forbidden('사제 종속 해방'), reserveRegular: () => true,
            fallSound: (sid) => { this.events.push(`fall:${sid}`); },
            shield: { reserveSound: () => false, loadSound: forbidden('보호막 소리'), attachSound: forbidden('보호막 소리 부착'),
                noticeSound: () => { this.events.push('shield-notice'); }, translate: (key) => key, tell: (text) => { this.events.push(text); } },
            regen: { refresh: (sid) => { this.core.display.update(this.core.pool.slot(sid)); }, damageable: (sid) => { this.hits.burning.refresh(sid); },
                neutralAction: forbidden('중립 회복'), patchAudit: () => { this.sp.rotate(); this.priestGlobals.regen.patch.observedSequence = this.sp.offset | 0; }, measurement: forbidden('시각 측정') },
        });
        this.priests.bindProcesses(this.core.processes); this.priests.setLocalPlayer(1); this.priests.setAuthority(true);
        this.levels = new PriestDamageState(); this.levels.levelCorrectionDisabled = true;
        this.generalState = new DamageableHitState(this.levels); this.burningState = new DamageableBurningState(); this.impactState = new GunImpactState();
        this.speeds = this.types.map((record) => ({ speed: record.speed ?? 0 }));
        // 외부 소유자 통지/경고/미복원 생성 장치를 원문 설치 장면과 같이 관찰 또는 명시 거부한다.
        this.hitEffects = {
            priest: { defaultFrame: (sid) => this.table.definitions[this.core.pool.slot(sid)[10] - 70].defaultFrame,
                ownerDamageNotice: (owner, victim, attacker) => { this.events.push(`priest:${owner}:${victim}:${attacker}`); }, ...options.effects?.priest },
            general: { ownerDamageNotice: (owner, victim, attacker) => { this.events.push(`general:${owner}:${victim}:${attacker}`); },
                realTime: () => this.core.processState.now, warningEvent: () => { this.events.push('warn-event'); }, warningSound: () => { this.events.push('warn-sound'); }, warningText: () => { this.events.push('warn-text'); }, ...options.effects?.general },
            burning: { create: forbidden('burning 생성'), bulfSound: forbidden('burning 소리'), ...options.effects?.burning },
            dudes: { createBurning: forbidden('Dude burning'), pathEndpoint: forbidden('Dude 경로 조회'), startPath: forbidden('Dude 경로 생성'), ...options.effects?.dudes },
            vortex: { ...options.effects?.vortex },
        };
        this.hits = this.makeHits();
        if (options.bind !== false) this.bindHitRuntime();
    }
    // 한 입력만 바꾸어 구성의 같은 풀/상태 계약을 확인할 수 있다. (2026-11-16 WB04-E564)
    makeHits(overrides = {}) {
        const input = { world: this.world, priests: this.priests, hp: this.hp, random: this.random, speeds: this.speeds, residences: this.assets.residences,
            levels: this.levels, general: this.generalState, burning: this.burningState, impact: this.impactState, effects: this.hitEffects, ...overrides };
        return new RawCampaignHitBindings(input.world, input.priests, input.hp, input.random, input.speeds, input.residences, input.levels, input.general, input.burning, input.impact, input.effects);
    }
    // 실제 공통 충돌 자산의 삭제·정기와 사제 낙하를 World 수명에 연결한다. 나머지 파생 삭제는 아직 허용하지 않는다. (2026-11-16 WB04-E564)
    bindHitRuntime() {
        // 실제 공용 Squid writer의 패킷을 별도로 관측하며 기존 Form/효과 로그는 그 계약대로 유지한다.
        this.squidPackets = [];
        this.world.bindRuntime({
            squidPacket: (packet) => { this.squidPackets.push(Uint8Array.from(packet)); },
            assets: this.hits.destroyHooks({ emit: (event) => {
                if (event.effect !== SQUID_DESTROY_EFFECT.transmit) throw new Error('다른 실제 자산 삭제 수명 미연결');
                this.events.push(`delete:${event.sid}:${event.flags}`);
            }, selected: () => 0 }),
            destroy: this.hits.destroy(forbidden('다른 자산 가상 삭제')),
            regular: this.hits.handler(this.priests.regularHandler(forbidden('다른 정기 부모'))),
            transmit: { writeData: forbidden('비피격 프로세스 자료 전송'), transmit: forbidden('비피격 프로세스 전송') },
            walkerFall: (sid) => this.priests.fallWalker(sid), bridgeEffect: forbidden('다리 효과'),
            loadingDepth: (depth) => { this.priests.setLoadingDepth(depth); },
            formTransmit: (form, flags) => { this.events.push(`form:${form}:${flags}`); },
        });
    }
    // 현재 실제 생성자/사제 소유자 경로로 만들되 Pop은 각 검사가 필요한 시점에 수행한다. (2026-11-16 WB04-E564)
    create(type, owner = 2) { const sid = this.world.create(type, 0); this.priests.setOwner(sid, owner); return sid; }
}
