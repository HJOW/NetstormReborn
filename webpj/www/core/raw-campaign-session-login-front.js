// Session Login 생성자의 옵션/내부 전송 앞단과 InstallZone을 실제 Session 저장소에 연결한다.
// 전체 패킷·상태 조립과 Begin/Pump는 후속 단계이며 이 객체 생성만으로 접속하지 않는다.
import { bindInternalCampaignTransport } from './campaign-transport-services.js';
import { createCampaignLoginServices, requireLoginService } from './campaign-login-services.js';
import { installLocalMissionZone, LocalMissionTransport } from './local-mission-transport.js';
import { RawMissionBattleOptions, resetMissionBattleOptions } from './raw-mission-battle-options.js';
import { sanitizeMissionPlayerName } from './raw-mission-client-packets.js';
import { RawMissionLoginPackets } from './raw-mission-login-packets.js';
import { RawMissionServerPackets } from './raw-mission-server-packets.js';
import { RawMissionSettingsPacket } from './raw-mission-settings-packet.js';
import { RawClockSynchronization } from './raw-clock-synchronization.js';

// 현재 Session을 빌려 원본 Login의 전송 앞단과 Zone 설치만 소유한다.
export class RawCampaignSessionLoginFront {
    // 실제 부모·장치와 내부 전송 수명, 반납 관문이다.
    #owner; #effects; #internal = null; #disposed = false;
    // 원문의 독립 초기 지역 입력과 현재 수신 종류 기록이다.
    pendingStamp = 0; overrideFlags = 0; wireTypes = [];
    // 현재 연결표와 패킷 분배기 및 같은 옵션/패킷/시계 처리기를 소유자와 함께 공개한다.
    transport; options; metadata; forts; settings; clockSynchronization;

    // Player 초기화 뒤 옵션 기본값과 현재 전송을 순서대로 준비한다.
    constructor(owner, effects = {}) {
        if (!owner?.playersReady) throw new TypeError('Login의 실제 Player 초기화 누락');
        this.#owner = owner; this.#effects = createCampaignLoginServices(effects);
        if (!owner.services.missionOptions) resetMissionBattleOptions(owner.battleOptions);
        if (this.#effects.transport) {
            const transport = { ...this.#effects.transport };
            if (!transport.lifetime || !transport.registry || !transport.dispatch || typeof transport.detachSession !== 'function' || typeof transport.callbacksActive !== 'function') throw new Error('외부 Login 전송의 실제 수명/등록표/반납 관문 누락');
            if (owner.services.connectionRegistry && owner.services.connectionRegistry !== transport.registry) throw new Error('Login과 Client State의 현재 연결표 불일치');
            owner.services.connectionRegistry = transport.registry; this.transport = transport;
        } else {
            this.#internal = new LocalMissionTransport(owner.players, () => owner.core.processState.server, this.#device(), owner.services.connectionRegistry ?? null);
            this.transport = bindInternalCampaignTransport(this.#internal);
        }
        try {
            const aux = owner.gameplayGlobals.gameAux(), ranges = owner.gameplayGlobals.manaRanges(), energy = owner.energyState;
            this.options = new RawMissionBattleOptions({
                options: owner.battleOptions, types: owner.core.types, typeUse: aux.typeUse(),
                rangeTable: new Uint32Array(ranges.buffer, ranges.byteOffset, ranges.length), selectedType: () => owner.geyserType,
                range: owner.gameplayGlobals.manaRadius(),
                rangeSquared: { get bits() { return energy.radiusSquaredBits; }, set bits(value) { energy.radiusSquaredBits = value; } },
                value: { get value() { return aux.refundPercent; }, set value(value) { aux.refundPercent = value; } },
            });
            const local = owner.localStateGlobals;
            this.metadata = new RawMissionServerPackets(owner.players, {
                get server() { return owner.core.processState.server; }, get metamap() { return owner.core.ownerMode.challenge; },
                get localOwner() { return owner.playerState.battleOwner; }, get centralCreated() { return local.centralCreated().value; },
                get archipelago() { return aux.archipelagoPercent; }, zoneAddress: local.zone(),
            }, {
                send: (target, packet) => this.#send(target, packet, true),
                peerAddress: (ownerId) => this.#transport('peerAddress')(ownerId | 0), log: this.#effects.log,
                assertion: (expression) => this.#assertion(expression), creatingIslands: this.#effects.creatingIslands,
                setArchipelago: (value) => { aux.archipelagoPercent = value; }, setCentralCreated: (value) => { local.centralCreated().value = value; },
                layout: () => owner.battleOptions[5], predictableCount: () => owner.core.pool.predictableCursor,
                randomCentral: () => { throw new Error('스커미시 중앙섬 생성은 후속 단계'); },
                surroundedCentral: () => { throw new Error('스커미시 특수 중앙섬 생성은 후속 단계'); },
            });
            this.forts = new RawMissionLoginPackets(owner.players, {
                current: (slot) => {
                    this.#live(); if (slot >= 9) return null;
                    const headers = owner.headers, pointer = new DataView(headers.buffer, headers.byteOffset, headers.byteLength).getUint32(slot * 20, true);
                    return pointer ? owner.templateStorage.template(slot) : null;
                }, initialLength: () => 4,
            }, { typeCount: () => owner.core.types.length }, {
                log: () => this.#need('log', 'Fort 송신')('로컬 Fort 전송\n'),
                send: (target, packet) => this.#send(target, packet, true),
            });
            const values = aux.missionSettings();
            this.settings = new RawMissionSettingsPacket({ current: (field) => values[field], setCurrent: (field, value) => { values[field] = value; }, send: (target, packet) => this.#send(target, packet, false) });
            const registry = this.transport.registry;
            this.clockSynchronization = new RawClockSynchronization(local.clockSynchronization(), local.clockSamples(), {
                gameSeconds: this.#effects.gameSeconds, wallSeconds: this.#effects.wallSeconds,
                send: (token, packet) => registry.sendDirect(token, packet),
                reply: (connection, packet, address) => {
                    if (address && (address.length !== connection.address.length || address.some((byte, index) => byte !== connection.address[index]))) throw new Error('현재 내부 연결과 다른 시간 응답 주소는 웹 범위 밖');
                    registry.sendDirect(registry.tokenFor(connection), packet);
                },
            });
        } catch (error) {
            if (this.#effects.transport) this.transport.detachSession(); else this.#internal.dispose();
            this.#disposed = true; throw error;
        }
    }

    // 선택 위치에 도달한 실제 장치만 요구한다.
    #need(name, where) { return requireLoginService(this.#effects, name, where); }
    // 반납 뒤 낡은 전송과 현재 Session을 다시 쓰지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Login 앞단'); return this.#owner; }
    // 호출 시 선택된 현재 전송 장치만 요구한다.
    #transport(name) { this.#live(); const device = this.transport[name]; if (typeof device !== 'function') throw new Error(`Login 전송 ${name} 미연결`); return device; }
    // 현재 실제 Session 진단을 호출한다.
    #assertion(expression) { const o = this.#live(); if (typeof o.services.ai?.assertion !== 'function') throw new Error('Login 진단 장치 누락'); o.services.ai.assertion(expression); }
    // 현재 6eh 관찰 로그 뒤 원문의 mutable 또는 읽기 전용 송신을 선택한다.
    #send(target, packet, mutable) {
        this.#live();
        if (this.#effects.observePacket && packet[1] === 0x6e) {
            if (packet.length < 8) throw new RangeError('Ready 송신 마스크 물리 부족');
            const mask = new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getUint32(4, true);
            this.#need('log', 'Ready 송신')(`Ready 송신 target${target | 0} mask${mask}\n`);
        }
        this.#transport(mutable ? 'sendMutable' : 'send')(target | 0, packet);
    }
    // 내부 패킷 분배기가 실제 시계·수신 기록·연결 반납을 사용한다.
    #device() {
        return {
            now: () => this.#need('wallSeconds', '패킷 분배')() >>> 0,
            record: (connection, packet) => {
                const o = this.#live(); if (packet.length < 4) throw new RangeError('현재 수신 기록의 실제 헤더 부족');
                this.wireTypes.push(packet[1]);
                if (this.#effects.observePacket) this.#effects.observePacket(connection.sender, o.state.assetState.readBoss(), o.state.assetState.localOwner, packet);
            },
            terminate: (connection) => { connection.flags |= 0x40; this.#need('log', '연결 종료')('로컬 미션 연결 종료\n'); },
            close: () => { this.#need('log', '연결 닫기')('로컬 연결 큐/소켓 종료\n'); },
            release: () => { this.#need('log', '연결 반납')('로컬 연결 반납\n'); },
        };
    }

    // 실제 공유 Player·Core·권한·Zone에 원본 가변 Zone을 설치한다.
    installZone(zone) {
        const o = this.#live(), self = this, local = o.localStateGlobals, active = o.connectionActivityGlobals, aux = o.gameplayGlobals.gameAux();
        const globals = {
            get server() { return o.core.processState.server; }, set server(value) { o.core.processState.server = value; },
            get boss() { return o.state.assetState.readBoss(); }, set boss(value) { o.state.assetState.writeBoss(value); },
            get mySubscriber() { return o.globals.mySubscriber; }, get pendingStamp() { return self.pendingStamp; },
            get activeStamp() { return active.activeStamp().value; }, set activeStamp(value) { active.activeStamp().value = value; },
            get activeZoneWord() { return active.activeZoneWord().value; }, set activeZoneWord(value) { active.activeZoneWord().value = value; },
            get zoneFlags() { return local.zoneWords()[0]; }, set zoneFlags(value) { local.zoneWords()[0] = value; },
            get earlyCaptureEnd() { return local.captureEnd().value; }, set earlyCaptureEnd(value) { local.captureEnd().value = value; },
            get adjustZoneFlags() { return aux.terrainEnabled; }, get missionWorld() { return o.objectState.mission; },
            get zoneFlagOverride() { return self.overrideFlags; }, get metamap() { return o.core.ownerMode.challenge; },
            get currentCoreData() { return o.coreDataGlobals.current; }, set currentCoreData(value) { o.coreDataGlobals.current = value; },
            get cachedCoreData() { return o.coreDataGlobals.cached(); },
        };
        const hooks = {
            loginTimeout: () => this.#need('configInt', 'Zone 로그인 제한')('loginTimeout', 30),
            seconds: () => this.#need('wallSeconds', 'Zone 벽시각')(),
            storeName: (subscriber, name, storage) => sanitizeMissionPlayerName(subscriber, storage.subarray(0, name.length + 1)), // C++ string_view의 길이와 NUL 한 칸만 빌린다.
            serve: (port) => { if (typeof this.transport.serve !== 'function') throw new Error('Zone 서버 전송 장치 누락'); this.transport.serve(port); },
            assertion: (expression) => { if (typeof o.services.ai?.assertion !== 'function') throw new Error('Zone 진단 장치 누락'); o.services.ai.assertion(expression); },
        };
        installLocalMissionZone(zone, o.players, o.playerState, globals, hooks, this.#need('configInt', 'Zone 서버 포트')('gameServerPort', 6799) >>> 0);
    }

    // 외부 등록 대여 또는 내부 두 끝을 실제 소유 방식에 따라 반납한다.
    dispose() {
        if (this.#disposed) return;
        if (this.#effects.transport) this.transport.detachSession(); else this.#internal.dispose();
        this.#disposed = true;
    }
}
