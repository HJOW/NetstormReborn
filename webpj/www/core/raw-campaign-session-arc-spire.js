// 생성자1668/ConnectArcSpire2128..2142의 같은 현재 공간·피해·빔 결선이다(225553df, WB04-E610).
// 사용: 세 Aviary 결선 뒤 bind. World 게시와 실제 Kernel 분배는 상위 Session이 맡는다.
import { typeNumber } from '../assets/type-table.js';
import { RawArcSpire } from './raw-arc-spire.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 같은 Session의 현재 자산·TYPE와 효과 표를 기존 아크 스파이어 몸체에 대여한다.
export class RawCampaignSessionArcSpire {
    // 초기화 순서/부분 실패·반복과 반납 뒤 호출을 구별한다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 기존 몸체가 빌리는 값형 훅과 현재 편집 모드 참조다.
    hooks = null; state = null;

    // 생성 자체로 슬롯·프로세스·난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 실제 range 정의/현재 피해율·flags1 finder와 기존 효과를 원문 순서로 연결한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.aviaryModules?.ready || this.#started || o.arcSpire) throw new Error('Session ArcSpire의 초기화 전/반복 결선');
        this.#started = true; const damage = o.hits.damageHooks({}).damage;
        const hooks = {
            range: (sid) => { // 현재 TYPE의 필수 자산 값을 float로 저장한다. 가변 TYPE의 range 사본은 읽지 않는다.
                this.#live(); const asset = o.assets.game.types()[c.pool.slot(sid)[10] - 70];
                if (!asset) throw new RangeError('ArcSpire range의 현재 자산 TYPE 범위');
                const value = typeNumber(asset.definition, 'range'); if (value === null) throw new Error('ArcSpire 자산의 필수 range 없음'); return Math.fround(value);
            },
            damageRate: (sid) => { this.#live(); const type = c.pool.slot(sid)[10]; if (!o.hitTypes[type]) throw new RangeError('ArcSpire 현재 피해 TYPE 범위'); return o.hitTypes[type].damagePerSecond; }, // 현재 hitTypes의 같은 메타다.
            find: (x, y, type) => { this.#live(); return o.world.find(x, y, type); }, // 같은 World의 현재 좌표/TYPE 검색이다.
            iterate: (area, consume) => { // 실제 동적 해시 체인의 순서/중복과 다음 링크의 선행 읽기를 유지한다.
                this.#live(); const finder = new RawSquidFinder(c.pool, c.hash, c.types);
                // 원문 Begin(area,1)부터 Next가0을 돌려줄 때까지 소비한다.
                for (let sid = finder.begin(area, 1); sid; sid = finder.next()) consume(sid);
            },
            beam: (source, target) => { this.#live(); o.particles.addBeam(source, target, 'shortZap.wav'); }, // 현재 같은 입자/위치 음향 몸체다.
            damage: (victim, amount, owner, attacker) => { this.#live(); if (typeof damage !== 'function') throw new Error('ArcSpire 실제 피해 미연결'); damage(victim, amount | 0, owner & 0xff, attacker); }, // 원문이 복사한 피해 함수와 signed/BYTE 인수다.
            setFrame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame, flags); }, // 현재 같은 프레임/표시를 갱신한다.
            damageable: (sid, event, count, value) => { this.#live(); return o.assetBindings.regular.damageableHandle(sid, event, count, value); }, // 현재 같은 공통 Regular의 반환을 보존한다.
        };
        this.hooks = hooks; this.state = { get editor() { return o.globals.editor; } };
        o.arcSpire = new RawArcSpire(c.pool, c.types, c.frames, c.spots, o.alliances, this.state, hooks); this.#ready = true;
    }

    // 반납한 이전 Session의 효과 closure로 World에 재진입하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session ArcSpire'); }
    // 실제 Host를 비운 상위가 현재 대여 closure를 닫는다.
    dispose() { this.#disposed = true; }
    // 생성 구간의 정상 복귀이며 실제 World 게시/미션 Load의 완료는 아니다.
    get ready() { return this.#ready; }
}
