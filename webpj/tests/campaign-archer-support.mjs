// CampaignInstallSupport.h의 보호 TYPE/SHP 자료와 실제 E563~E567 World/Host/전투 몸체를 묶는다. Session/실제 플레이 검사가 아니다. (2026-11-16 WB04-E567)
import { CampaignGunScene, typeProperty } from './campaign-gun-support.mjs';
import { readRawArcherTypeData } from '../www/core/raw-archer-type-data.js';
import { RawProjectileGeometry } from '../www/core/raw-projectile-geometry.js';
import { buildProjectileGeometryTypes } from '../www/core/raw-projectile-geometry-types.js';
import { RawCampaignArcherBindings } from '../www/core/raw-campaign-archer-bindings.js';
import { ArcherAnimationState } from '../www/core/raw-archer-animation.js';
import { MissileState } from '../www/core/raw-missile.js';
import { forbidden } from './campaign-asset-support.mjs';
import { patchGunGraphFromFrame } from '../www/core/raw-gun-angle.js';

// 실제 자산으로 각 보충 표를 만들고 같은 난수/피격/포구 기하/Host/전송을 빌리는 검사 장면이다. (2026-11-16 WB04-E567)
export class CampaignArcherScene extends CampaignGunScene {
    // 외부 소리·송신은 관찰 목록에 남기며 미연결 장치를 빈 성공으로 대신하지 않는다.
    constructor(options = {}) {
        super({ ...options, assets: false });
        // 보호 정의의 실제 방향/물리 프레임과 Missile 사운드 토큰/사거리다.
        this.animationTypes = this.core.types.map(() => ({ directions: 0, frameCount: 0 }));
        this.missileTypes = this.core.types.map(() => ({ range: 0, frameCount: 0, firingSound: 0, impactSound: 0 }));
        this.frameTypes = this.core.frames.map((frames) => ({ frames }));
        // cpppj와 동일한 TYPE70 이후 정의 순서로 보충 표를 채운다.
        for (let block = 0; block < this.table.definitions.length; block += 1) {
            // 현재 raw 부모 TYPE이다.
            const type = block + 70; const definition = this.table.definitions[block];
            this.animationTypes[type] = readRawArcherTypeData(definition);
            Object.assign(this.missileTypes[type], { range: this.targetTypes[type].range, frameCount: this.animationTypes[type].directions,
                firingSound: typeProperty(definition, 'firingSound', '') ? type : 0, impactSound: typeProperty(definition, 'impactSound', '') ? type : 0 });
        }
        // 실제 SHP 포구/프레임 코드와 같은 Pool을 사용하는 기하 처리기다.
        this.geometryTypes = buildProjectileGeometryTypes(this.table.definitions, this.core.shapes);
        this.geometry = new RawProjectileGeometry(this.core.pool, this.core.types, this.geometryTypes);
        this.archerState = new ArcherAnimationState(); this.missileState = new MissileState();
        this.combatEffects = {
            animation: { common: {
                sound: (sid, sound) => this.events.push(`Archer 소리:${sid}:${sound}`),
                // cpppj 초기 장면처럼 실제 fenceMark가 없을 때만 invalid corner를 공급한다.
                targetPoint: () => { for (let sid = 5; sid < this.core.pool.capacity; sid += 1) { const raw = this.core.pool.slot(sid); if (!(raw[11] & 3) && raw[10] === 90) throw new Error('현재 fenceMark 교차 corner helper 미연결'); } return [-10000, -10000]; },
            } },
            missile: this.hits.missileHooks(this.gunHit, this.missileState, {
                muzzle: (sid) => this.geometry.muzzle(sid), hotspot: (sid) => this.geometry.hotspot(sid),
                directionFrame: (from, to, type) => this.geometry.directionFrame(from, to, type), targetDimensions: (sid) => this.geometry.dimensions(sid), targetBounds: (sid, point) => this.geometry.targetBounds(sid, point),
                firingSound: (x, y, token) => this.events.push(`Missile 발사음:${x}:${y}:${token}`), impactSound: (x, y, token) => this.events.push(`Missile 충돌음:${x}:${y}:${token}`),
            }),
            iceCooling: (sid) => this.events.push(`아이스 냉각:${sid}`),
        };
        this.combat = this.makeCombat();
        if (options.combat !== false) this.installCombat();
    }

    // 입력 Pool/장치 차이를 검사할 때 지정한 인수만 바꿔 실제 바인딩을 만든다. (2026-11-16 WB04-E567)
    makeCombat(overrides = {}) {
        // 호출자가 제공한 참조/함수를 값 사본으로 묶은 인수이다.
        const input = { world: this.world, target: this.gunTarget, random: this.random, animationTypes: this.animationTypes, frameTypes: this.frameTypes, missileTypes: this.missileTypes, animation: this.archerState, missile: this.missileState, effects: this.combatEffects, ...overrides };
        return new RawCampaignArcherBindings(input.world, input.target, input.random, input.animationTypes, input.frameTypes, input.missileTypes, input.animation, input.missile, input.effects);
    }

    // 실제 자산/사제/피격 체인에 발사 공장·Disc Regular·공유 전송을 합성한다. (2026-11-16 WB04-E567)
    installCombat() {
        // 실제 Session이 공급하는 Wind Archer 각도→Graph와 현재 Graph BYTE 쓰기다.
        this.effects.postPop.gunFrame = (frame) => patchGunGraphFromFrame(frame, () => this.gunState.checking, (expression, file, line) => this.world.storage.assertDisplay(expression, file, line));
        this.effects.postPop.setGraph = (sid, number) => this.core.graph.setNumber(sid, number & 255);
        this.gunHooks = this.combat.gunHooks(this.queries);
        this.installGunAssets(this.gunHooks);
        this.transmission = this.combat.transmission((packet) => this.events.push(`Animation 송신:${Buffer.from(packet).toString('hex')}`), this.bindings.transmission());
        this.core.processes.setFormTransmit(this.transmission);
        this.core.processes.setRegularHandler(this.combat.handler(this.hits.handler(this.bindings.handler(this.priests.regularHandler(forbidden('Regular'))))));
    }

    // 실제 부모의 Form 체인에서 이미 생성된 Gun을 찾는다. (2026-11-16 WB04-E567)
    gunFor(parent) {
        let form = this.core.pool.slot(parent)[6] | (this.core.pool.slot(parent)[7] << 8);
        // 유한 풀 범위의 종속 체인을 읽고 실제 현재 Gun 프로세스를 반환한다.
        for (let count = 0; form && count < this.core.pool.capacity; count += 1) { const raw = this.core.pool.slot(form); if (raw[10] === 34) return this.core.processes.fromForm(form); form = raw[4] | (raw[5] << 8); }
        throw new Error('검사 부모의 실제 Gun Form 없음');
    }
}
