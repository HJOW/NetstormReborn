// CampaignInstallSupport.h의 실제 Gun 자료/조회기를 E564의 단일 World에 연결한다. 전체 Session이나 실제 교전 검사가 아니다. (2026-11-16 WB04-E565)
import { CampaignHitScene } from './campaign-hit-support.mjs';
import { RawGunMode } from '../www/core/raw-gun-mode.js';
import { RawGunTarget, makeGunTargetHooks } from '../www/core/raw-gun-target.js';
import { RawGunHit } from '../www/core/raw-gun-hit.js';
import { makeCampaignGunQueryHooks } from '../www/core/raw-campaign-gun-queries.js';

// 보호 타입 정의의 마지막 지정 속성 또는 원문 기본값을 읽는다. (2026-11-16 WB04-E565)
export function typeProperty(definition, name, fallback) {
    return definition?.properties.filter((property) => property.name.toLowerCase() === name.toLowerCase()).at(-1)?.value ?? fallback;
}

// 실제 TYPE/SHP/World/피격과 같은 모드·표적·시각을 공유하는 검사 장면이다. (2026-11-16 WB04-E565)
export class CampaignGunScene extends CampaignHitScene {
    // options.bind=false면 기존 Hit 장면처럼 runtime만 설치하지 않는다. assets=false면 순수 조회기 구성만 확인한다. (2026-11-16 WB04-E565)
    constructor(options = {}) {
        super(options);
        // .type 속성과 현재 프레임에서 만든 원문 보충 표들이다.
        this.targetTypes = this.core.types.map(() => ({ range: 0, theme: 3, directionFrames: [-1, -1, -1, -1], heightExtension: 0 }));
        this.hitTypes = this.core.types.map(() => ({ damagePerSecond: 0 }));
        this.firingIntervals = new Float32Array(this.core.types.length);
        // 실제 자산 로딩 번호 순서로 같은 range/height/theme/hpPerSec/delayBetweenShots를 읽는다.
        for (let block = 0; block < this.table.definitions.length; block += 1) {
            const definition = this.table.definitions[block]; const type = block + 70;
            const theme = typeProperty(definition, 'theme', 'sun');
            Object.assign(this.targetTypes[type], {
                range: Math.fround(typeProperty(definition, 'range', 0)), heightExtension: Math.trunc(typeProperty(definition, 'height', 0)),
                theme: theme === 'wind' ? 0 : theme === 'rain' ? 1 : theme === 'thunder' ? 2 : 3,
                directionFrames: ['L', 'M', 'N', 'O'].map((letter) => this.core.frames[type].run(letter).first),
            });
            this.hitTypes[type].damagePerSecond = Math.fround(typeProperty(definition, 'hpPerSec', 0));
            this.firingIntervals[type] = typeProperty(definition, 'delayBetweenShots', 0);
        }
        // 원문 Player의 자기 동맹과 표적 검색의 독립 상태다. GUI/원격 전송을 합성하지 않는다.
        this.targetState = {
            friendlyFire: false, checking: false, showRanges: false, sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148,
            windHalfAngle: 30, scaleX: 16, scaleY: 11, allyMasks: Array.from({ length: 9 }, (_, owner) => 1 << owner), playerFlags: new Array(9).fill(0), angles: {},
        };
        this.gunMode = new RawGunMode(this.core.pool, this.core.frames, () => this.gunState.readWindArcherType());
        this.gunTarget = new RawGunTarget(this.core.pool, this.core.hash, this.core.types, this.targetTypes, this.core.shapes, this.targetState);
        this.gunHit = new RawGunHit(this.core.pool, this.core.types, this.hitTypes, this.hits.damageHooks({ notifySurface: (sid) => { this.core.building.notifySurface(sid); } }));
        this.queries = this.makeQueries();
        if (options.assets !== false) this.installGunAssets();
    }

    // 틀린 Pool/배열 구성을 검사할 때 지정한 인수만 바꾸어 같은 연결 함수를 호출한다. (2026-11-16 WB04-E565)
    makeQueries(overrides = {}) {
        const input = { core: this.core, mode: this.gunMode, target: this.gunTarget, hit: this.gunHit, state: this.gunState, intervals: this.firingIntervals, hooks: {}, ...overrides };
        return makeCampaignGunQueryHooks(input.core, input.mode, input.target, input.hit, input.state, input.intervals, input.hooks);
    }

    // 첫 객체 전에 E561 자산 후처리와 같은 실제 Gun 조회를 연결한다. Archer 애니메이션은 아직 미연결이며 선택 시 거부한다. (2026-11-16 WB04-E565)
    installGunAssets(hooks = this.queries) {
        this.effects.gun = makeGunTargetHooks(this.core.pool, this.gunTarget, hooks);
        this.bindings = this.make(); this.bindings.activatePostPop(); this.core.postPop.setBody('carrier', this.carrier);
        this.core.processes.setFormTransmit(this.bindings.transmission());
        this.core.processes.setRegularHandler(this.hits.handler(this.bindings.handler(this.priests.regularHandler())));
    }
}
