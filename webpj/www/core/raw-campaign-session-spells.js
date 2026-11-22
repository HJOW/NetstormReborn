// 생성자1656..1658의 BombPostPop/AcidZap/AcidFence 결선이다(225553df, WB04-E607).
// 사용: connectCarrierSteal 뒤 bindBombPostPop → bindAcidZap → bindAcidFence. 현재 World/Host/입자/피해를 빌린다.
import { RawBombPostPop } from './raw-bomb-post-pop.js';
import { RawAcidZap } from './raw-acid-zap.js';
import { RawAcidFence } from './raw-acid-fence.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { RegularProcess } from './squid-process.js';
import { typeNumber } from '../assets/type-table.js';

// 현재 선택된 실제 부모/장치가 없으면 그 위치에서 거부한다.
function need(value, message) { if (!value) throw new Error(message); return value; }
// 애시드 송신 뒤 네 진단의 줄 번호와 원문 식이다(Periodicharmer.cpp). (2026-11-22 WB04-E1211)
const ACID_SEND_ASSERTIONS = new Map([[0x2fa, 'zd.victimSid == s.getSid()'], [0x2fb, 'zd.aSid == getSid()'], [0x2fc, 'zd.bSid == otherPole.getSid()'], [0x2fd, 'zd.xOriented == xOriented']]);

// 세 원문 생성 구간의 값형 훅과 독립 typed 역할을 함께 보관한다.
export class RawCampaignSessionSpells {
    // 순서/부분 실패·반복 설치와 반납 뒤 호출 관문이다.
    #owner; #bombStarted = false; #bombReady = false; #zapStarted = false; #zapReady = false; #fenceStarted = false; #ready = false; #disposed = false; #bombRegistered = false;
    // 기존 몸체가 빌리는 실제 효과 표와 수신 제외 TYPE/현재11BYTE 헤더다.
    bombHooks = null; zapHooks = null; fenceHooks = null; fenceState = null; friendlyFire = null;
    zapState = { altar: 164, growingIceTower: 150 }; zapHeader = { message: 0x61, protocol: 1, length: 11 };
    // 현재 여섯 역할 TYPE다. 공중3종은 정적 역할 결선만 하며 새 전용 실행 검사를 만들지 않는다.
    fenceTypes = { rainFlyer: 144, sunFlyer: 73, windFlyer: 138, rainBalloon: 146, windBalloon: 141, sunBalloon: 111 };

    // 생성 자체는 효과 자산/예약/난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // ConnectBombPostPop2441..2451의 같은 Host/게임 난수와 공통 접두 등록이다.
    bindBombPostPop() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.carrierStealModules?.ready || this.#bombStarted || o.bombPostPop) throw new Error('Session BombPostPop의 초기화 전/반복 결선');
        this.#bombStarted = true;
        const hooks = {
            hasRegular: (sid, event) => { this.#live(); return c.processes.findEvent(sid, event) !== null; }, // 실제 종속 Host에서 중복 예약을 조회한다.
            reserveRegular: () => { this.#live(); if (o.pendingBombPopRegular) throw new Error('주문 효과 Regular 중복 예약'); o.pendingBombPopRegular = new RegularProcess(0, 0); return true; }, // 난수 계산 전에 같은 미부착 객체를 확보한다.
            addRegular: (sid, event, delay) => { // 앞선 난수 효과를 보존한 채 같은 확보 객체를 생성/부착하고 현재 시각으로 재설정한다.
                this.#live(); const reserved = need(o.pendingBombPopRegular, '주문 효과 Regular 확보 누락'); o.pendingBombPopRegular = null; reserved.initializeDetached(event, delay);
                const process = c.processes.attachRegular(reserved, sid); if (!process) throw new Error('주문 효과 Regular 실제 부착 실패'); process.resetTask(event, c.processState.now);
            },
        };
        this.bombHooks = hooks; o.bombPostPop = new RawBombPostPop(c.pool, o.random, hooks); c.postPop.setBody('bomb', o.bombPostPop); this.#bombRegistered = true; this.#bombReady = true;
    }

    // ConnectAcidZap2453..2460의 현재 위치 음향→분해18→가상 삭제를 연결한다.
    bindAcidZap() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !this.#bombReady || this.#zapStarted || o.acidZap) throw new Error('Session AcidZap의 초기화 전/반복 결선');
        this.#zapStarted = true;
        const hooks = {
            sound: (x, y, name, loop) => { this.#live(); if (loop || !o.services.sound.playOnceAt) throw new Error('애시드 실제 단발 위치 음향 미연결'); o.services.sound.playOnceAt(x, y, o.soundNames.lookup(name), 0, 0); }, // Source의 반복0 관문 뒤 현재5인자 위치 소리를 재생한다.
            dissolve: (victim, first, second, orientation) => { this.#live(); o.particles.addDissolve(victim, first, second, orientation); }, // 현재 같은 픽셀 캡처/분해 시스템으로 인자를 전달한다.
            destroy: (victim, flags) => { this.#live(); o.world.destroy(victim, flags); }, // 원소유자 비트를 포함한 현재 가상 삭제를 실행한다.
        };
        this.zapHooks = hooks; o.acidZap = new RawAcidZap(c.pool, c.types, this.zapState, this.zapHeader, hooks); this.#zapReady = true;
    }

    // ConnectAcidFence2175..2194의 같은 공간 순회/현재 역할·친군사격과 실제 주기를 구성한다.
    bindAcidFence() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !this.#zapReady || this.#fenceStarted || o.acidFence) throw new Error('Session AcidFence의 초기화 전/반복 결선');
        this.#fenceStarted = true; const damage = o.hits.damageHooks({}).damage;
        const hooks = {
            range: (sid) => { // 현재 TYPE에 대응하는 실제 자산 정의의 필수 range를 float로 저장해 사용한다.
                this.#live(); const type = c.pool.slot(sid)[10]; const asset = o.assets.game.types()[type - 70]; if (!asset) throw new RangeError('애시드 range의 현재 자산 TYPE 범위');
                const value = typeNumber(asset.definition, 'range'); if (value === null) throw new Error('애시드 자산의 필수 range 없음'); return Math.fround(value);
            },
            find: (x, y, type) => { this.#live(); return o.world.find(x, y, type); }, // 이웃을 같은 현재 World의 좌표/TYPE 검색으로 찾는다.
            iterate: (area, consume) => { // 같은 flags1 공간 체인의 순서/중복과 효과 이전에 저장한 다음 후보를 유지한다.
                this.#live(); const finder = new RawSquidFinder(c.pool, c.hash, c.types);
                // Source의 Begin/Next 순서로 각 현재 후보를 소비한다.
                for (let sid = finder.begin(area, 1); sid; sid = finder.next()) consume(sid);
            },
            reserveRegular: () => { this.#live(); o.pendingAcidRegular = new RegularProcess(0, 0); return true; }, // 애니메이션용 같은 미부착 예약 한 개를 확보한다.
            regular: (sid, event, delay) => { // 같은 예약의 생성/부착 뒤 원문의 event/count0과 현재 시각을 설정한다.
                this.#live(); const reserved = need(o.pendingAcidRegular, '애시드 Regular 예약 누락'); o.pendingAcidRegular = null; reserved.initializeDetached(event, delay);
                const task = c.processes.attachRegular(reserved, sid); if (!task) throw new Error('애시드 Regular 부착 실패'); task.resetTask(event, c.processState.now);
            },
            damage: (victim, amount, owner, attacker) => { this.#live(); need(damage, '애시드 현재 피해 몸체 미연결')(victim, amount | 0, owner & 0xff, attacker); }, // 생성 시 고른 같은 DamageHooks의 signed 피해/소유자 BYTE를 사용한다.
            broadcast: (packet) => { this.#live(); o.tables.broadcast.send(packet); }, // 같은 wire를 현재 기존 방송 관문으로 보낸다.
            // 원문(RawSurfaceAttack Native 결선)처럼 송신 뒤 네 진단(Periodicharmer.cpp 0x2fa..0x2fd)을 표시하고 계속한다. 다른 줄은 원문도 선택하지 않아 오류다. (2026-11-22 WB04-E1211: 이전에는 오류를 던졌다)
            assertion: (line) => {
                this.#live(); const expression = ACID_SEND_ASSERTIONS.get(line); if (!expression) throw new Error(`애시드 다른 송신 후 진단 ${line}`);
                o.world.storage.assertDisplay(expression, '\\Ns\\O\\Periodicharmer.cpp', line);
            },
            setFrame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame, flags); }, // 전용 애니메이션의 현재 프레임을 실제 표시 몸체에 보낸다.
        };
        this.fenceState = { get boss() { return o.state.assetState.readBoss(); }, get editor() { return o.globals.editor; }, get localOwner() { return o.playerState.battleOwner; } };
        this.friendlyFire = () => o.gameplayGlobals.gameAux().missionSettings()[3]; // RawCheckingFlag 사본도 같은55a490 owner를 빌리며 GunTarget 지역 대입에 재연결되지 않는다.
        this.fenceHooks = hooks; o.acidFence = new RawAcidFence(c.pool, c.types, c.frames, c.spots, o.alliances, this.fenceState, this.friendlyFire, this.fenceTypes, o.acidZap, hooks); this.#ready = true;
    }

    // 이전 Session의 효과 closure가 반납한 World에 재진입하지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Spells'); }
    // 상위가 실제 Host를 비운 뒤 접두 등록과 두 종류의 미부착 예약만 반납한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; const o = this.#owner; o.pendingBombPopRegular = null; o.pendingAcidRegular = null; if (this.#bombRegistered) { o.core.postPop.setBody('bomb', null); this.#bombRegistered = false; } }
    // 생성 순서의 각 정상 복귀를 나타내며 World/미션 게시의 완료는 아니다.
    get bombReady() { return this.#bombReady; }
    // 61h 본문 생성의 정상 복귀다. 실제 패킷 등록은 Session 외곽에서 한다.
    get zapReady() { return this.#zapReady; }
    // 세 생성 구간을 모두 구성했는지 확인한다.
    get ready() { return this.#ready; }
}
