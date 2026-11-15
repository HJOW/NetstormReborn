// CampaignMotionTests의 단일 Pool/Host/BFS/도착/정리 조립을 보호 자산 시험 월드에 연결한다. 저장 미션 전체 설치는 Session 통합 몫이다. (2026-11-16 WB04-E568)
import { CampaignArcherScene } from './campaign-archer-support.mjs';
import { typeProperty } from './campaign-gun-support.mjs';
import { forbidden } from './campaign-asset-support.mjs';
import { PlayerPlacementAnchorState, RawPlayerPlacementAnchor } from '../www/core/raw-player-placement-anchor.js';
import { RawContainedCreate, makeContainedCreateHooks } from '../www/core/contained-create.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawPathProcessState } from '../www/core/raw-path-process.js';
import { RawCampaignMotionState, RawCampaignMotionBindings } from '../www/core/raw-campaign-motion-bindings.js';
import { RawSquidFinder } from '../www/core/raw-squid-finder.js';

// 실제 사제·자산·전투 장면에 Motion/Path와 현재 clock/Graph/음향/전송을 더한다.
export class CampaignMotionScene extends CampaignArcherScene {
    // 외부 큰 효과는 원문 시험처럼 명시 거부하고 공간/BFS/도착은 실제 모듈을 사용한다.
    constructor(options = {}) {
        super(options);
        this.motionState = new RawCampaignMotionState(); this.motionState.halt.battle = true; this.motionState.priest.suppressNeutralReturn = true;
        this.anchorState = new PlayerPlacementAnchorState(); Object.assign(this.anchorState, { graphReady: 1, fenceType: 90, windArcherType: 140 });
        this.additional = new SquidPostPopList();
        this.anchor = new RawPlayerPlacementAnchor(this.core.pool, this.core.types, this.core.hash.view(0), this.core.bookkeeping, this.additional, this.priestGlobals.contained, this.anchorState);
        this.contained = new RawContainedCreate(this.core.pool, { containedType: () => this.world.storage.gameAux.contentFormType, bridgeType: () => this.core.bridgeState.bridgeType }, makeContainedCreateHooks(this.core.pool, this.core.factory, {
            notifySurface: (sid) => this.core.building.notifySurface(sid), attach: (sid, parent, flags) => this.core.forms.attach(sid, parent, flags),
        }));
        // C++ 참조 멤버처럼 같은 이동 타입/간헐천/clock을 현재 getter로 빌린다.
        const globals = this.priestGlobals; const aux = this.world.storage.gameAux;
        this.startState = { clock: this.core.processState, get dudeType() { return globals.movement.dude; }, get teleportType() { return globals.movement.teleport; }, get manaBoltType() { return globals.movement.manaBolt; }, get priestType() { return globals.movement.priest; }, get geyserType() { return aux.geyserType; }, snapPriestDestination: true, retrying: false };
        this.processState = new RawPathProcessState(this.core.processState);
        this.steps = []; this.pathPackets = [];
        this.motionEffects = {
            cleanup: { reserveShared: () => true, ritual: forbidden('이동 의식'), refund: forbidden('이동 환불') },
            halt: { carrierPickup: forbidden('이동 포획'), nuggetTransfer: forbidden('이동 너겟 전달') },
            carrier: { genusAt: (x, y, genus) => {
                // 원문 일반 finder의 첫 통과/skipSurface 순서를 유지한다.
                const finder = new RawSquidFinder(this.core.pool, this.core.hash, this.core.types);
                for (let sid = finder.begin({ left: Math.trunc(x), top: Math.trunc(y), right: Math.trunc(x), bottom: Math.trunc(y) }, (genus & 6) === 0 ? 1 : 0); sid; sid = finder.next()) if (this.core.types[this.core.pool.slot(sid)[10]].flags2 & genus) return sid;
                return 0;
            }, pickup: forbidden('운반 pickup'), escape: forbidden('운반 회피'), chooseReturn: forbidden('운반 귀환 선택'), chooseGeyser: forbidden('운반 간헐천 선택'), goReturn: forbidden('운반 귀환'), goGeyser: forbidden('운반 간헐천'), minedPacket: forbidden('채굴 패킷'), robust: forbidden('운반 robust') },
            priest: { neutralReturn: forbidden('중립 사제 귀환') },
            path: {
                metadata: (sid) => { const definition = this.table.definitions[this.core.pool.slot(sid)[10] - 70]; const speed = typeProperty(definition, 'speed', null); const turning = typeProperty(definition, 'turningSpeed', null); if (typeof speed !== 'number' || typeof turning !== 'number') throw new Error('실제 Path TYPE 속도 정의 누락'); return { speed: Math.fround(speed), turningSpeed: Math.fround(turning), defaultFrame: definition.defaultFrame }; },
                soundId: (sid) => typeProperty(this.table.definitions[this.core.pool.slot(sid)[10] - 70], 'stepSound', '') ? this.core.pool.slot(sid)[10] : 0,
                stepSound: (sid, x, y, token) => this.steps.push([sid, x, y, token]), startBlocked: forbidden('시작 겹침'), arrivalBlocked: forbidden('도착 겹침'), retarget: forbidden('이동 재탐색'),
            },
        };
        this.motion = this.makeMotion();
        this.pathHeader = { packetId: 0x30, packetFlags: 1, packetLength: 0 }; this.pathBuffer = new Uint8Array(16384);
        this.pathTransmission = this.motion.paths.transmission(this.pathHeader, this.pathBuffer, (length) => this.pathPackets.push(Uint8Array.from(this.pathBuffer.subarray(0, length))));
        this.core.processes.setFormTransmit(this.combat.transmission((packet) => this.events.push(`Animation 송신:${Buffer.from(packet).toString('hex')}`), this.bindings.transmission(this.pathTransmission)));
    }

    // 같은 실제 구성에서 한 입력만 바꾸어 계약을 대조한다.
    makeMotion(overrides = {}) {
        // 실제 하위 모듈 참조와 함수 인수를 묶은 시험 구성이다.
        const input = { world: this.world, priests: this.priests, islands: this.islands, contained: this.contained, anchor: this.anchor, hp: this.hp, frames: this.frameTypes, state: this.motionState, start: this.startState, process: this.processState, effects: this.motionEffects, ...overrides };
        return new RawCampaignMotionBindings(input.world, input.priests, input.islands, input.contained, input.anchor, input.hp, input.frames, input.state, input.start, input.process, input.effects);
    }

    // 실제 소유자 설정과 Pop을 loading 깊이 안에서 실행해 사제 목록/HP/Owner를 같은 몸체로 초기화한다.
    priest(x = 23.5, y = 23.5, owner = 1) {
        this.priests.setLoadingDepth(1); const sid = this.create(158, owner); this.world.pop(sid, x, y, 3); this.priests.setLoadingDepth(0); return sid;
    }
}
