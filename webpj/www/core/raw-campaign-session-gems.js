// 생성자1671/ConnectGemReward2020..2032와1672..1679 Buried 후처리 결선이다(225553df, WB04-E613).
// 사용: Fence 뒤 bind. 같은 현재 보상 상태·건설 제외 목록과 게임 난수를 빌린다.
import { RawGemReward } from './raw-gem-reward.js';
import { RawBuriedPostPop } from './raw-buried-post-pop.js';

// 독립 typed의 보상 추첨/공개와 최초 Buried 공통 장부/예약을 같은 현재 World에 연결한다.
export class RawCampaignSessionGems {
    // 부분 실패/반복/반납과 실제 Buried 등록 여부다.
    #owner; #started = false; #ready = false; #disposed = false; #registered = false;
    // 원문 함수 값 사본을 보관하는 두 훅 표와 현재 boss/로딩/Bridge getter다.
    rewardHooks = null; buriedHooks = null; boss = null; loading = null; bridgeType = null;

    // 생성 자체는 슬롯·Deck·게임 난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 실제 GemReward를 먼저 만든 뒤 같은 몸체를 선택하는 Buried를 공통 후처리에 등록한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.fenceModules?.ready || this.#started || o.gemReward || o.buriedPostPop) throw new Error('Session GemReward의 초기화 전/반복 결선'); this.#started = true;
        // Source의 두 std::function 대입은 생성 시의 실제 장치를 복사한다.
        const fill = o.services.production.allocationBytes; const wallSeconds = o.services.sound.wallSeconds;
        const diagnostic = (line) => { this.#live(); throw new Error(`원본 Gem/Deck.cpp 보상 진단 ${line}`); };
        const hooks = {
            fill: typeof fill === 'function' ? (raw) => { this.#live(); fill(raw); } : null,
            assertion: diagnostic, deckAssertion: (kind) => { this.#live(); throw new Error(`Gem 보상 Deck 실제 진단 미연결 ${kind}`); }, // 원문 SessionMissing이다.
            deck: {
                wallSeconds: typeof wallSeconds === 'function' ? () => { this.#live(); return wallSeconds(); } : null, assertion: diagnostic,
                mismatch: (stored, current) => { this.#live(); if (typeof o.services.player.log !== 'function') throw new Error('Deck 실제1d 로그 미연결'); o.services.player.log(0x1d, `Deck chance mismatch. ${stored | 0}!=${current | 0}\n`); }, // 현재 장치에 signed 두 값과 원문 줄바꿈을 보낸다.
            },
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); }, pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); }, destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); }, // 실제 가상 생성/공간/삭제다.
        };
        this.rewardHooks = hooks; o.gemReward = new RawGemReward(c.pool, c.types, o.random, o.constructionExcludedTypes, o.gemRewardState, hooks);
        const buried = {
            chooseType: (value) => { this.#live(); return o.gemReward.choose(value); }, reserveRegular: () => { this.#live(); return true; }, // 원문처럼 실제 할당은 addRegular에서 하며 가짜 확보를 만들지 않는다.
            addRegular: (sid, event, delay) => { this.#live(); if (!c.processes.addRegular(sid, event, delay)) throw new Error('Buried 실제 Regular 부착 실패'); }, // 실제 Host가 초기 event/count·현재 시각을 소유한다.
            validateBase: (sid, flags, projected) => { this.#live(); c.postPop.validateBase(sid, flags, projected); }, base: (sid, flags) => { this.#live(); c.postPop.postPopBase(sid, flags); }, notifySurface: (sid) => { this.#live(); c.building.notifySurface(sid); }, // 같은 공통 장부와 Bridge 통지다.
        };
        this.buriedHooks = buried; this.boss = () => o.state.assetState.readBoss(); this.loading = () => o.objectState.loadingDepth; this.bridgeType = () => c.bridgeState.bridgeType;
        o.buriedPostPop = new RawBuriedPostPop(c.pool, this.boss, this.loading, buried, this.bridgeType); c.postPop.setBody('buried', o.buriedPostPop); this.#registered = true; this.#ready = true;
    }

    // 반납한 Session의 현재 World에 재진입하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session GemReward'); }
    // 실제 Host를 먼저 비운 상위가 공통 후처리 등록과 대여 수명을 닫는다.
    dispose() { if (this.#disposed) return; this.#disposed = true; if (this.#registered) { this.#owner.core.postPop.setBody('buried', null); this.#registered = false; } }
    // 두 생성 구간의 정상 복귀이며 실제 World 게시/Load의 완료는 아니다.
    get ready() { return this.#ready; }
}
