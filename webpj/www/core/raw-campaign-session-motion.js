// ConnectMotion3806..3844의 기준점·경로 효과·일반 운반체 줍기 조립이다(225553df, WB04-E641).
// 사용: 같은 owner의 connectBuilding/connectMotionArrivals 뒤 bindInputs. 전체 경로 설치/Load는 후속이다.
import { RawPlayerPlacementAnchor } from './raw-player-placement-anchor.js';
import { RawCarrierPickup } from './raw-carrier-pickup.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// C++ 값형 함수 표의 중첩 묶음만 복사하고 실제 장기 owner/함수 참조는 빌린다.
export function copyCampaignMotionEffects(input = {}) {
    return { ...input, cleanup: { ...input.cleanup }, halt: { ...input.halt }, carrier: { ...input.carrier }, priest: { ...input.priest }, path: { ...input.path } };
}

// 원문의 FillMissing은 호출 시 실패하는 함수를 넣는다. 이미 지정한 외부 효과는 보존한다.
function fillMissing(table, key) {
    if (!table[key]) table[key] = () => { throw new Error(`Session 이동의 실제 ${key} 미연결`); };
}

// 기준점과 경로/줍기 함수 표를 같은 실제 World에 연결하며 후속 순환 참조는 호출 때 조회한다.
export class RawCampaignSessionMotion {
    // 설치 시작/정상 복귀/반납 관문과 현재 Impl 참조다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 다음 경로 조립이 그대로 이어받는 값형 함수 표와 줍기 하위 효과다.
    effects = null; pickupHooks = null;

    // 생성 자체는 장기 상태/난수/Pool을 바꾸지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 낡은 함수 표가 반납된 World나 다음 Session의 전역에 진입하지 못하게 한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 이동 입력'); }
    // 의식 등 후속 설치 항목은 선택한 순간에만 요구한다.
    #body(name, message) { this.#live(); const body = this.#owner[name]; if (!body) throw new Error(message); return body; }

    // 현재 halt·Graph 기준점과 원문3808..3844의 함수 표를 같은 실제 owner에 한 번 설치한다.
    bindInputs() {
        const o = this.#owner, c = o.core, s = o.state;
        if (this.#disposed || this.#started || o.motion || o.anchor || !o.arrivalModules?.ready) throw new Error('Session 이동 입력의 초기화 전/반복 결선');
        this.#started = true;
        s.motionState.halt.battle = true; Object.assign(s.anchorState, { graphReady: 1, fenceType: 90, windArcherType: 140 });
        o.anchor = new RawPlayerPlacementAnchor(c.pool, c.types, c.hash.view(0), c.bookkeeping, o.additional, o.contentFinder, s.anchorState);
        const e = this.effects = copyCampaignMotionEffects(o.services.motion);
        e.carrier.currentAlliance = (first, second) => {
            this.#live(); if (first >= 9 || second >= 9 || first < 0 || second < 0) throw new RangeError('운반 도착 현재 동맹 번호 범위');
            return o.alliances[first * 9 + second];
        };
        e.path.assertion = (expression, file, line) => {
            this.#live(); if (typeof o.services.ai.assertion !== 'function') throw new Error(`경로 현재 원문 진단 장치 미연결: ${file}:${line}`);
            o.services.ai.assertion(expression);
        };
        e.path.stagePoint = (temple, stage) => { const body = this.#body('templeRitual', 'Path 수신의 실제 의식 목표점 미설치'); o.runtimeModules.prepareRitual(); return body.stagePoint(temple, stage); };
        e.path.balloonFlame = (x, y) => { this.#live(); this.#sound(x, y, 'balloonFlame.wav'); };
        e.path.bulfRandom = (low, high) => { this.#live(); if (high <= low) throw new RangeError('Bulf 공용 난수 범위 오류'); return (o.random.next(high - low) | 0) + low; };
        e.path.bulfSound = (x, y, name) => { this.#live(); this.#sound(x, y, name); };
        e.path.startBlocked = (point, type, owner) => { this.#live(); o.assetModules.prepareBuilding(c.processState.now); o.buildingEffects.displace(point.x, point.y, type, owner); };
        e.path.arrivalBlocked = (_sid, point, type, owner) => { this.#live(); o.assetModules.prepareBuilding(c.processState.now); o.buildingEffects.displace(point.x, point.y, type, owner); };
        e.path.constructionArrival = (sid, _first, _second) => {
            this.#live(); const type = c.pool.slot(sid)[10]; if (type === 134) return o.buildArrival.arrive(sid); if (type === 105) return o.manaArrival.arrive(sid);
            throw new Error('실제 건설 도착 타입 오류');
        };
        if (!e.cleanup.reserveShared) e.cleanup.reserveShared = () => { this.#live(); return true; };
        e.cleanup.ritual = (dais, priest, mode) => { const body = this.#body('templeRitual', '의식 설치 전 Path 정리'); o.runtimeModules.prepareRitual(); body.ritual(dais, priest, mode); };
        fillMissing(e.cleanup, 'refund');
        e.carrier.genusAt = (x, y, genus) => {
            this.#live(); const finder = new RawSquidFinder(c.pool, c.hash, c.types), px = Math.trunc(x), py = Math.trunc(y);
            // 원문의 정수 절삭·표면 생략과 첫 현재 후보/flags2 순서를 유지한다.
            for (let sid = finder.begin({ left: px, top: py, right: px, bottom: py }, (genus & 6) === 0 ? 1 : 0); sid; sid = finder.next()) if (c.types[c.pool.slot(sid)[10]].flags2 & genus) return sid;
            return 0;
        };
        fillMissing(e.carrier, 'cdPriestPickupPriest');
        const pickup = this.pickupHooks = {
            // 웹 도착 getter의 객체 반환을 기존 줍기 모듈의 pair 배열 계약으로 옮긴다.
            candidate: (sid, x, y) => { const found = this.#body('motion', '이동 설치 전 줍기 후보 조회').carrierArrival.pickupCandidate(sid, x, y); return [found.sid, found.flag]; },
            send: o.services.carrierRequest,
            assertion: (line) => { this.#live(); throw new Error(`원본 Carrier.cpp 줍기 권한 진단 ${line}`); },
            reveal: (sid) => { this.#live(); return o.gemReward.reveal(sid); },
            queue: (command, target, parent) => { this.#live(); if (typeof o.services.assets.queueCommand !== 'function') throw new Error('줍기 실제 입력 큐 미연결'); o.services.assets.queueCommand(command, target, parent); },
            find: (sid, mask, kind, index) => { this.#live(); return o.priests.carrier.findContained(sid, mask, kind, index); },
            afterDiscard: () => { this.#live(); o.priestModules.replenishNuggets(); },
            create: (parent, kind, argument, flags, attachFlags, quality) => { this.#live(); return o.contained.create(parent, kind, argument, flags, attachFlags, quality); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            detach: (form, flags) => { this.#live(); if (flags) throw new Error('줍기 종속 Unpop flags 오류'); o.contentForms.unpop(form); },
            attach: (form, parent, flags) => { this.#live(); o.contentForms.attach(form, parent, flags); },
        };
        s.carrierPickupState.refuseHeldPriest = true;
        o.carrierPickup = new RawCarrierPickup(c.pool, c.types, o.priestState.contained, () => s.assetState.readBoss(), () => o.globals.missionPost, s.carrierPickupState, pickup);
        e.carrier.pickup = (sid) => { this.#live(); o.carrierPickup.request(sid); };
        this.#ready = true;
    }

    // 동일 SoundList의 현재 토큰을 위치 소리 서비스로 보낸다. 미연결은 선택 시 오류다.
    #sound(x, y, name) {
        const o = this.#owner; if (typeof o.services.sound.playOnceAt !== 'function') throw new Error('이동 위치 음향 미연결');
        o.services.sound.playOnceAt(x, y, o.soundNames.lookup(name), 0, 0);
    }
    // 부모가 실제 Host를 비운 다음 지역 함수 표의 대여를 닫는다. 장기 전역은 초기화하지 않는다.
    dispose() { this.#disposed = true; }
    // 앞부분 입력 결선의 정상 복귀이며 전체 ConnectMotion/Load 완료가 아니다.
    get inputsReady() { return this.#ready; }
}
