// Session 생성자1188..1235의 실제 섬·소리·사제 모듈을 같은 기반 owner에 연결한다(225553df, WB04-E577).
// 사용: Player 테이블 결선 뒤 owner.connectIslandsAndPriests(). 전체 생성자/Load/Runtime 공개는 아직 별도다.
import { RawIslandRuntime } from './raw-island-runtime.js';
import { RawNuggetReplenish } from './raw-nugget-replenish.js';
import { RawPriestParticipation } from './raw-priest-participation.js';
import { bindPriestRegenSpStore } from './raw-priest-regen.js';
import { RawPriestWorldBindings } from './raw-priest-world-bindings.js';
import { SoundProcessSystem } from './sound-process.js';

// 원문 참조 멤버와 같은 현재 속성 슬롯을 상자로 빌린다.
function reference(owner, key) { return { get value() { return owner[key]; }, set value(value) { owner[key] = value; } }; }
// 원문 SessionMissing/FillMissing의 실제 선택 지점에서 원래 생성자 연결 위치를 알린다.
function missing(line) { return () => { throw new Error(`실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:${line}`); }; }
// 후속 생성자가 아직 설치하지 않은 실제 모듈은 임시 대체 몸체로 실행하지 않는다.
function requireModule(owner, key, label) { if (!owner[key]) throw new Error(`${label} 실제 설치 전 호출`); return owner[key]; }

// 같은 Impl의 섬·사제 생성 구간과 그 수명이다. 후속 소유권/이동/건설/피해 모듈은 실제 슬롯에서 조회한다.
export class RawCampaignSessionPriests {
    // 실제 기반 owner와 단일 생성/반납 상태다. 부분 실패의 앞선 효과는 보존한다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문 지역 효과 묶음의 생성 중 소유다. 하위 Bindings는 함수/참조를 값형 인수로 복사한다.
    effects = null;

    // 같은 기반 owner만 빌리며 실제 모듈 생성은 원문 위치의 bind에서 수행한다.
    constructor(owner) { this.#owner = owner; }

    // 원문1188..1235 typed 캠페인 경로다. 첫 객체·등록/전체 Session 공개를 앞당기지 않는다.
    bind() {
        const o = this.#owner; const c = o.core; const services = o.services; const s = o.state;
        if (this.#disposed || !o.tables.ready || this.#started) throw new Error('Session 섬/사제의 초기화 전/반복 결선');
        this.#started = true;
        o.islands = new RawIslandRuntime(c.pool, c.hash, o.islandBytes, o.finishState, s.assetState.currentResidences(), o.worldRedraw, reference(o, 'revision'), {
            setResidenceOwner: (sid, owner) => requireModule(o, 'ownership', '섬 소유권').setOwner(sid, owner), assertion: missing(1189),
        });
        const e = { ...services.priest, shield: { ...services.priest.shield }, regen: { ...services.priest.regen } }; this.effects = e;
        e.forcefieldAssertion = (expression, file, line) => {
            const report = services.ai?.assertion; if (!report) throw new Error(`보호막 Regular 실제 진단 장치 미연결 ${line}`); report(expression);
        };
        e.findTypeAt = (x, y, type) => o.world.find(x, y, type);
        e.pop = (sid, x, y, flags) => o.world.pop(sid, x, y, flags);
        e.unpop = (sid, flags) => o.world.unpop(sid, flags);
        e.repop = (sid, flags) => o.world.repop(sid, flags);
        e.destroy = (sid, flags) => o.world.destroy(sid, flags);
        e.claimGround = (x, y, owner) => o.world.setSupportOwner(x, y, owner);
        e.nonPriestOwner = (sid, owner) => o.world.setAssetOwner(sid, owner);
        const storage = o.world.storage;
        e.ownerAssertion = (expression, file, line) => storage.assertDisplay(expression, file, line);
        if (!e.ownerNotice) e.ownerNotice = (sid) => c.building.notifySurface(sid);
        e.notifySurface = (sid) => c.building.notifySurface(sid);
        e.dependentFound = () => this.replenishNuggets();
        e.reserveRegular = () => true; e.shield.reserveSound = () => true;
        o.sounds = new SoundProcessSystem(c.processes, s.soundState, services.sound);
        e.shield.loadSound = (name) => o.soundNames.lookup(name);
        e.shield.attachSound = (parent, sound, alternate, flags) => { if (!o.sounds.add(parent, sound, alternate, flags)) throw new Error('실제 보호막 Sound47 부착 실패'); };
        // 세 원문 FillMissing은 장치를 요구하는 시점을 뒤로 유지한다.
        for (const name of ['noticeSound', 'translate', 'tell']) if (!e.shield[name]) e.shield[name] = missing(1213);
        if (!e.fallSound) e.fallSound = (sid, x, y) => {
            if (!services.sound.playOnceAt) throw new Error('사제 낙하 실제 위치 음향 미연결'); services.sound.playOnceAt(x, y, o.soundNames.lookup('priestFall.wav'), 0, 0);
        };
        const otherWalkerFall = e.otherWalkerFall;
        e.otherWalkerFall = (sid) => {
            if ((o.carrierRegular && o.carrierRegular.handles(sid)) || (o.balloonBody && o.balloonBody.handles(sid))) {
                if (!o.carrierFallStart) throw new Error('캐리어 낙하 시작 설치 전 호출'); return o.carrierFallStart.begin(sid);
            }
            if (!otherWalkerFall) throw new Error('다른 실제 Walker 낙하 미연결'); return otherWalkerFall(sid);
        };
        // 원문 const span도 같은 물리를 빌린다. 표시 효과가 갱신한 현재 슬롯을 읽도록 사본을 만들지 않는다.
        e.regen.refresh = (sid) => c.display.update(c.pool.physical(sid));
        e.regen.neutralAction = (sid) => requireModule(o, 'priestPanic', '사제 panic').run(sid);
        e.regen.patchAudit = () => o.sp.rotate();
        e.regen.spReads = bindPriestRegenSpStore(o.sp, services.spGlobals ? null : o.priestState.regen.patch);
        if (!e.regen.measurement) e.regen.measurement = missing(1228);
        e.regen.damageable = (sid) => requireModule(o, 'hits', '사제 피해').burning.refresh(sid);
        o.priests = new RawPriestWorldBindings({ pool: c.pool, hash: c.hash, types: c.types, frames: c.frames, spots: c.spots, surfaces: c.hash.view(0), reward: o.hp, random: o.random, factory: c.factory, owner: c.owner, ownerMode: c.ownerMode, postPop: c.postPop, frame: c.frame }, o.priestState, e);
        o.priests.bindProcesses(c.processes); o.priests.bindAuthority(o.gameplayGlobals.authority()); o.priests.bindPlayerRecords(o.players); o.priests.setLocalPlayer(1);
        o.priestParticipation = new RawPriestParticipation(c.pool, c.hash, c.types, o.priests.priests, o.priests.carrier, {
            get priestType() { return o.priestState.movement.priest; }, get altarType() { return s.runeState.readAltarType(); }, get daisType() { return s.runeState.readDaisType(); },
        });
        this.#ready = true;
    }

    // 원문2723..2729의 보충 부모다. 실제 지도 미설치는 모드 관문보다 먼저 거부하며 장기 전역은 현재 읽는다.
    replenishNuggets() {
        const o = this.#owner;
        if (!o.construction) throw new Error('보충 부모의 실제 지도 범위 미설치');
        const body = new RawNuggetReplenish(o.construction.playerBounds, {
            get boss() { return o.state.assetState.readBoss(); }, get single() { return o.globals.missionPost; }, get fort() { return o.core.ownerMode.fort; }, get editor() { return o.globals.editor; }, get archipelagoSpan() { return o.state.farm.currentSpan(); },
        }, {
            assertion: (line) => o.world.storage.assertDisplay('boss', '\\Ns\\O\\Nugget.cpp', line),
            place: (rectangle, quantity) => { if (!o.placeBattleNuggets) throw new Error('너겟 보충의 같은 Session 배치 미연결'); o.placeBattleNuggets(rectangle, quantity); },
        });
        body.run();
    }

    // 현재 World 프로세스가 먼저 정리된 뒤 사제의 postPop 등록/Player 대여를 반납한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; this.#owner.priests?.dispose(); }
    // 이 구간의 정상 복귀다. 나머지 순환 연결과 전체 Load 완료를 뜻하지 않는다.
    get ready() { return this.#ready; }
}
