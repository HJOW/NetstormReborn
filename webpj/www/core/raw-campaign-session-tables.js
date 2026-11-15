// Session 생성자1152..1186의 캠페인 Player·전송 테이블을 같은 기반 owner에 연결한다(225553df, WB04-E576).
// 사용: owner.initializePlayersAndSp() 뒤 owner.connectPlayerTablesAndPackets(). 전체 Session 공개/Load는 별도다.
import { RawBattleBroadcast } from './raw-battle-broadcast.js';
import { RawFormPacket } from './raw-form-packet.js';

// 현재 DWORD를 복제하지 않고 실제 소유자의 속성 슬롯을 상자로 빌린다.
function reference(owner, key) { return { get value() { return owner[key]; }, set value(value) { owner[key] = value; } }; }
// C++ uint32 번호의 Player9 물리 경계다. 현재 읽기 위치에서만 검사한다.
function playerIndex(owner) { if (!Number.isInteger(owner) || owner < 0 || owner >= 9) throw new RangeError('실제 세션 현재 동맹/Player 번호 오류'); return owner; }

// 같은 Impl 수명의 공용 writer·방송과 실제 Player/SID 목록 결선이다. 설치나 패킷 수신기를 대신하지 않는다.
export class RawCampaignSessionTables {
    // 실제 기반 owner와 한 번 실행의 시작/정상 복귀 상태다. 실패한 접두를 재실행하지 않는다.
    #owner; #started = false; #ready = false;
    // 장기 GameAux의 동일 헤더·삭제 표식·2048BYTE 저장소와 이 단계에서 생성하는 실제 모듈이다.
    formHeaders; deletionStamp; formBuffer; broadcast = null; formWriter = null;

    // C++ 참조 멤버처럼 기반 생성 시 같은 장기 저장소를 빌리며 값 사본을 만들지 않는다.
    constructor(owner) {
        this.#owner = owner; const transmission = owner.world.storage.gameAux.squidTransmission;
        this.formHeaders = transmission.forms; this.deletionStamp = transmission.deletion; this.formBuffer = transmission.buffer;
    }

    // 원문1152..1186의 typed 캠페인 순서다. 선택된 장치 실패에는 그 앞의 실제 쓰기/결선을 남긴다.
    bind() {
        const o = this.#owner; const s = o.state; const services = o.services;
        if (!o.playersReady || this.#started) throw new Error('Session Player 테이블의 초기화 전/반복 결선');
        this.#started = true;
        if (services.coreDataGlobals) o.coreDataGlobals.select(o.playerState, Number(o.core.ownerMode.challenge), 1, 1);
        // 장기 상태에 짧은 Session 주소를 남기지 않고 실제 장기 전역 owner만 보관한다.
        const gameplay = o.gameplayGlobals;
        s.constructionState.command.chargeBlockedRead = s.constructionState.place.chargeBlockedRead = () => gameplay.world.missionPost;
        const storage = o.world.storage;
        this.broadcast = new RawBattleBroadcast(o.players, services.battlePeers, reference(o.playerState, 'battleOwner'), reference(o.core.processState, 'server'),
            () => storage.assertDisplay('server', '\\Ns\\O\\Ztrans.cpp', 0x1c3));
        this.broadcast.bindMutableSend((target, packet) => this.sendConstructionMutable(target, packet));
        if (!services.assets.sendGunPacket) services.assets.sendGunPacket = (bytes) => this.broadcast.send(bytes);
        if (!services.pathPacket) services.pathPacket = (bytes) => this.broadcast.send(bytes);
        if (!services.formPacket) services.formPacket = (bytes) => this.broadcast.send(bytes);
        this.formWriter = new RawFormPacket(o.core.pool, this.formHeaders, this.deletionStamp);
        if (!services.dependencyTransmit) services.dependencyTransmit = (form, ignored) => {
            const size = this.formWriter.writeData(form, this.formBuffer);
            services.formPacket(this.formBuffer.subarray(0, size));
        };
        const lists = storage.sidLists;
        o.priestState.priests.sharedOwnerState = storage.priestOwnerState;
        o.priestState.priests.sharedPriests = lists.priests; s.assetState.sharedResidences = lists.residences;
        s.assetState.sharedTemples = lists.temples; s.assetState.sharedOutposts = lists.outposts; s.assetState.sharedGeysers = lists.geysers;
        // 같은 Player +0/+4/+8을 조회하는 실제 목록을 연결한 뒤 해당 행의 자기 동맹 기본값을 쓴다.
        for (let owner = 0; owner < 9; owner += 1) {
            o.playerUse.bindFactory(o.core.bookkeeping.ownerFactories[owner], owner);
            s.gunTargetState.allyMasks[owner] = 1 << owner; s.gunState.allies[owner][owner] = true;
        }
        s.gunState.allianceRead = (owner, target) => o.alliances[playerIndex(owner) * 9 + playerIndex(target)];
        s.gunTargetState.allyMaskRead = (owner) => {
            const offset = playerIndex(owner) * 0xb4 + 0x54;
            return new DataView(o.players.buffer, o.players.byteOffset, o.players.byteLength).getUint32(offset, true);
        };
        s.gunTargetState.playerFlagsRead = (owner) => o.players[playerIndex(owner) * 0xb4 + 0x50];
        this.#ready = true;
    }

    // 원문4129의 선택된 건설 송신이다. Login이 있으면 그 현재 Registry만 선택하고 서비스로 우회하지 않는다.
    // 단독 캠페인의 NULL 방송 peer는 여기에 도달하지 않는다. 실제 내부 zlocal도 같은 Registry 송신을 쓴다.
    sendConstructionMutable(target, packet) {
        const o = this.#owner; const registry = o.login ? o.login.transport.registry : o.services.connectionRegistry;
        if (!registry) throw new Error('선택된 건설 방송 연결의 실제 Registry 없음');
        registry.sendMutable(target | 0, packet, o.players);
    }

    // 이 생성자 구간의 정상 복귀만 뜻한다. Session/World Load 완료와 구별한다.
    get ready() { return this.#ready; }
}
