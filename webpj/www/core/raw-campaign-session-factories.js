// InstallContents3157..3176와 ConnectFactoryUpgrade/Input3178..3192를 같은 세션에 연결한다(225553df, WB04-E599).
// 사용: connectProduction 뒤 bind. Flare/건설/AI 등 뒤 설치 포인터는 호출 때 고르며 미설치를 성공으로 대신하지 않는다.
import { RawFactoryResearch } from './raw-factory-research.js';
import { RawFactoryUpgradeCommand } from './raw-factory-upgrade-command.js';
import { RawFactoryUpgrade } from './raw-factory-upgrade.js';
import { RawFactoryInput, createFactoryInputState } from './raw-factory-input.js';
import { RegularProcess } from './squid-process.js';

// 같은 원문 물리의 현재 비정렬 DWORD를 읽는다.
function read(raw, offset = 0) { return new DataView(raw.buffer, raw.byteOffset + offset, 4).getUint32(0, true); }
// 같은 SP Get DWORD의 비트를 float로 해석한다.
function spFloat(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits, true); return view.getFloat32(0, true); }
// 실제 선택된 함수/후속 모듈을 요구한다. 앞선 효과를 되돌리거나 빈 성공을 반환하지 않는다.
function need(value, message) { if (!value) throw new Error(message); return value; }

// 같은 연구/업그레이드 명령·진행·입력을 구성하는 실제 생성자 마지막 내용물 구간이다.
export class RawCampaignSessionFactories {
    // 기반 owner와 한 번 설치/부분 실패·반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 각 기존 몸체가 빌리는 원문 값형 훅과 현재 전역 조회 표다.
    researchHooks = null; commandHooks = null; upgradeHooks = null; inputHooks = null;
    researchState = null; commandState = null; upgradeState = null; inputState = null;

    // 생성은 연구/업그레이드 요청이나 게임 효과를 실행하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 원문 연구→업그레이드 명령→진행→입력 순서이며 기존 생산/HP/Host를 그대로 사용한다.
    bind() {
        const o = this.#owner; const c = o.core; const g = o.gameplayGlobals;
        if (this.#disposed || !o.productionModules?.ready || this.#started) throw new Error('Session Factory의 초기화 전/반복 결선');
        this.#started = true;
        const destroy = (sid, flags) => { this.#live(); o.world.destroy(sid, flags); };
        const create = (parent, kind, quantity, flags, attach, quality) => { this.#live(); return o.contained.create(parent, kind, quantity, flags, attach, quality); };
        this.researchHooks = {
            help: (section, mode) => { this.#live(); need(o.services.factoryHelp, 'Factory 실제 연구 도움말 미연결')(section, mode); },
            inputLog: (parent, slots) => { this.#live(); need(o.services.player.log, 'Factory 실제 입력 슬롯 로그 미연결')(5, `${c.pool.slot(parent)[34]}: slotsRemaining/A=${slots | 0}\n`); },
            find: (parent, mask, kind, index) => { this.#live(); return o.priests.carrier.findContained(parent, mask, kind, index); },
            slots: (parent) => { this.#live(); return o.production.appendFactoryProducts(parent, 0); }, destroy, create,
            send: (target, packet) => { this.#live(); need(o.services.carrierRequest, '실제 연구 송신의 현재 전송 미설치')(target | 0, packet); },
            flare: (parent, count, event) => { this.#live(); need(o.flare, '실제 연구 플레어 미설치').spawn(parent, count, event); },
            refresh: (owner) => { this.#live(); o.production.refresh(owner); },
            assertion: (message) => { this.#live(); console.error(`Factory 연구 원본 진단: ${message}`); },
            upgradeReceive: (packet) => { this.#live(); need(o.factoryUpgradeCommand, 'Factory 업그레이드 실제 수신 미설치').receive(packet); },
        };
        this.researchState = {
            loadingDepth: () => o.objectState.loadingDepth,
            productionDirty: { get value() { return c.bookkeeping.productionDirty; }, set value(value) { c.bookkeeping.productionDirty = value; } },
            boss: () => o.state.assetState.readBoss(), now: () => c.processState.now, outpostType: () => g.outpostType().value,
        };
        o.factoryResearch = new RawFactoryResearch(c.pool, c.types, this.researchState, this.researchHooks);
        this.commandHooks = {
            cost: (type) => { this.#live(); return o.hp.typeCostUnits(type); }, addSp: (owner, delta) => { this.#live(); o.hp.addSp(owner, delta); },
            reserveShared: () => { this.#live(); o.factoryUpgradePending = new RegularProcess(0, 0); return true; },
            addShared: (parent, event, delay) => {
                this.#live(); const process = need(o.factoryUpgradePending, 'Factory 업그레이드 실제 SharedRegular 예약 누락'); o.factoryUpgradePending = null; process.initializeDetached(event, delay);
                const attached = c.processes.attachRegular(process, parent, true); if (!attached) throw new Error('Factory 업그레이드 실제 SharedRegular 부착 실패'); attached.resetTask(event, c.processState.now);
            },
            send: (target, packet) => { this.#live(); need(o.services.carrierRequest, 'Factory 업그레이드 실제 요청 송신 미연결')(target | 0, packet); },
            broadcast: (packet) => { this.#live(); o.tables.broadcast.send(packet); },
            log: (kind, text) => { this.#live(); need(o.services.player.log, 'Factory 업그레이드 실제 로그 미연결')(kind, text); },
            assertion: (text) => { this.#live(); console.error(`Factory 업그레이드 원본 진단: ${text}`); },
        };
        this.commandState = { boss: () => o.state.assetState.readBoss(), checking: () => o.state.gunState.checking, now: () => c.processState.now };
        o.factoryUpgradeCommand = new RawFactoryUpgradeCommand(c.pool, c.types, this.commandState, this.commandHooks);
        this.upgradeHooks = {
            level: (parent) => { this.#live(); const table = read(c.pool.slot(parent)); if (table !== 0x509148 && table !== 0x509210) throw new Error('업그레이드 현재Factory level 가상표 미복원'); return o.assetModules.virtualCount(parent); },
            setLevel: (parent, value) => { this.#live(); o.assetBindings.setState(parent, value); }, maxHp: (parent) => { this.#live(); return o.hp.maxHitPoints(parent); },
            adjustFrame: (parent) => { this.#live(); o.assetBindings.adjustFactoryFrame(parent); }, display: (parent) => { this.#live(); c.display.update(c.pool.physical(parent)); },
            soundAt: (x, y, name) => { this.#live(); need(o.services.sound.playOnceAt, '업그레이드 실제 위치 소리 미연결')(x, y, o.soundNames.lookup(name), 0, 0); },
            sound: (name) => { this.#live(); need(o.services.sound.play, '업그레이드 실제 로컬 소리 미연결')(o.soundNames.lookup(name), 0, 0, 0, 1, 0); },
            flare: (parent, count, event) => { this.#live(); need(o.flare, '업그레이드 실제 플레어 미설치').spawn(parent, count, event); },
            create: (type, flags) => { this.#live(); return c.factory.create(type, flags); }, owner: (parent, owner) => { this.#live(); o.ownership.setOwner(parent, owner); },
            pop: (parent, x, y, flags) => { this.#live(); o.world.pop(parent, x, y, flags); },
            construction: (parent, duration) => { this.#live(); o.assetModules.prepareBuilding(c.processState.now); need(o.building, '업그레이드 실제 건설 미설치').start(parent, duration); },
            depend: (watched, watcher) => { this.#live(); c.dependencies.create(watched, watcher); }, watcher: (form) => { this.#live(); return c.dependencies.watcher(form); }, destroy,
            aiPresent: (parent) => { this.#live(); const owner = c.pool.slot(parent)[34]; if (!owner) return false; if (owner >= 9) throw new RangeError('업그레이드현재AI Player 범위'); return read(o.players, owner * 0xb4 + 0xb0) !== 0; },
            notifyAi: (owner, parent) => { this.#live(); if (c.ownerMode.challenge || (owner | 0) <= 0) return; if (owner >= 9) throw new RangeError('업그레이드완료AI Player 범위'); const token = read(o.players, owner * 0xb4 + 0xb0); if (token) need(o.aiFactoryProduction, '업그레이드 완료 실제 AI 생산 미설치').upgraded(token, parent); },
            log: (kind, text) => { this.#live(); need(o.services.player.log, '업그레이드진행 실제로그 미연결')(kind, text); },
            assertion: (line) => { this.#live(); console.error(`Factory 업그레이드 원본 진단 줄:${(line >>> 0).toString(16)}`); },
        };
        this.upgradeState = { outpost: () => g.outpostType().value, dependType: () => o.flyerDependType, localPlayer: () => o.playerState.battleOwner, checking: () => o.state.gunState.checking, challenge: () => c.ownerMode.challenge, boss: () => o.state.assetState.readBoss() };
        o.factoryUpgrade = new RawFactoryUpgrade(c.pool, c.processes, this.upgradeState, this.upgradeHooks);
        this.inputHooks = {
            level: (parent) => { this.#live(); return o.assetModules.virtualCount(parent); }, setLevel: this.upgradeHooks.setLevel, adjustFrame: this.upgradeHooks.adjustFrame, cost: this.commandHooks.cost,
            money: (owner) => { this.#live(); return spFloat(o.sp.get(owner)); }, upgrading: (parent) => { this.#live(); return o.factoryUpgrade.isUpgrading(parent); }, request: (parent) => { this.#live(); o.factoryUpgradeCommand.request(parent); },
            tell: (section) => { this.#live(); need(o.services.factoryTell, 'Factory 실제 업그레이드 안내 미연결')(section); },
            research: (parent, code, parameter) => { this.#live(); return o.factoryResearch.input(parent, code, parameter); },
            parent: (parent, code, parameter) => { this.#live(); return need(o.objectCommand, 'Factory 실제 공통 입력 미설치').command(parent, code); },
        };
        this.inputState = createFactoryInputState({ editor: () => o.globals.editor, localOwner: () => o.playerState.battleOwner });
        o.factoryInput = new RawFactoryInput(c.pool, this.inputState, this.inputHooks); this.#ready = true;
    }

    // 반납한 세션의 현재 포인터/슬롯으로 다시 접근하지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Factory'); }
    // 부모가 World 프로세스를 먼저 정리한다. 미부착 실제 Regular 예약만 이 구간에서 반납한다.
    dispose() { this.#disposed = true; this.#owner.factoryUpgradePending = null; }
    // InstallContents의 마지막 Factory 구간 정상 복귀다. 전체 Session/World 게시는 아니다.
    get ready() { return this.#ready; }
}
