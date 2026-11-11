// PATCH 로그인 004a7800 의 현재 Player/일반 전투·미션/패킷/Ready 부모 순서를 옮긴 모듈이다(cpppj `o/RawMissionLogin.cpp/.h`, 10.78: 004a7800 로그인 수신 부모, 004a7510 재접속 63h, 004a6920 응답, 004a6990 Begin, 004a6e70 SP, 004a6ab0/00491270 가입 통지, 004a6f70 Ready mask).
// 사용: const login = new RawMissionLogin(players, participation, globals, hooks); registerMissionLogin(dispatch, login); login.receive(pending, connection, packet); login.sendReady(0); login.sendReply(owner, accepted);
//   players = Player9 1620BYTE(+0x2c subscriber·+0x50 flags·+0x58 watcher·+0x5c 순서·+0x60 예측수·+0x64 시작 완료 표시), participation = `RawPlayerParticipation`(실제 flags 3/연결 재배치/거부·해제),
//   globals = 로그인과 실제 Session/타이머가 공유하는 현재 전역 묶음(속성으로 매번 현재값을 읽고, 쓰는 필드는 직접 갱신; 별도 Ready 성공 사본 없음): server·boss·simulconnect·missionWorld·missionPost·metamap(bool)·player(PlayerWorldState)·mySubscriber·fortCount·lastLoginPlayer·predictableCount·begin·
//   beginSeed·battleSeed(또는 sharedBattleSeed `{ value }` int32 상자)·debugBurst·clockStamp·baseClock·captureEnd·currentCoreData(Uint8Array|null)·cachedCoreData(10)·zoneAddress(6)·battleOptions(87)·savedGame/inBattle(일반 자원 초기화가 선택될 때만 필요한 선택 필드)·debugPacketHeader([메시지, 프로토콜])/debugPacketInitialLength(디버그 63000BYTE 패킷 전용).
//   hooks = 큰 외부 몸체: disconnect·send(target, bytes)(로컬 writer 의 전체 mutable 예약을 현재 연결 gate 에 넘기고 장치가 observer 뒤 현재 선언 길이를 사용)·broadcast·pendingReply·flush·wallSeconds·clock·spBits·applyOptions·clearTypes·sendPlayer·sendFortMetadata·loadCurrentFort·loadFortImage·peerAddress·sendFort·
//   synchronizePeer·loadWorld·sendSettings·assertion·restoreCoreData·setSp·makeBattleIsland·cleanupReconnectClients·checkingAssertions·log. 선택한 외부 몸체는 도달한 위치에서 필수이며 패킷을 버리는 성공 처리기를 공급하지 않는다(없으면 앞선 부분 효과를 남긴 채 거부).
// 기준선 뒤 cpppj 변경(7ce6f1298·5844bfb56·25e32b7a9·a60d547f0·392517096·ede61f05d 등, 3.6절 ②): 재접속 63h(`receiveReconnect`)·일반 자원 초기화(watcher/시작 SP)·디버그 burst·mutable 전체 예약 송신·공유 전투 seed 상자·Native 고정 진입값 없는 등록을 반영했다.
//   Native Login 고정 수신기를 분배기에 대여하는 분기는 E306 의 ③ 과 같은 묶음이라 옮기지 않았다. CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E319: 4-C RawMissionLogin 이식)
import { MissionLoginPlayerCounts } from './raw-mission-login-timer.js';

// 실제 Player 폭/개수와 flags 필드 위치(10.78)이다. (2026-11-11 WB04-E319)
const STRIDE = 0xb4;
const PLAYERS = 9;
const FLAGS = 0x50;

// 선택된 외부 몸체만 도달 시 필수로 검사한다. (2026-11-11 WB04-E319)
function need(device) {
    if (typeof device !== 'function') throw new Error('미션 로그인 실제 외부 몸체 미연결');
    return device;
}

// 비정렬 little endian 필드의 지정 폭을 기록한다(범위 밖은 거부). (2026-11-11 WB04-E319)
function write(bytes, offset, value, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset + width > bytes.length) throw new RangeError('미션 로그인 쓰기 범위');
    // 실제 폭의 낮은 바이트만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 입력 배열의 실제 폭을 현재 위치에서 읽는다(범위 밖은 거부). (2026-11-11 WB04-E319)
function readBytes(bytes, offset, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset + width > bytes.length) throw new RangeError('미션 로그인 읽기 범위');
    let value = 0;
    // 주변 Player 필드나 헤더를 섞지 않는다.
    for (let index = 0; index < width; index += 1) value |= bytes[offset + index] << (index * 8);
    return value >>> 0;
}

// fixed packet 은 생성자의 실제 초기값을, variable 은 004df590 의 0xff 채움을 사용한다: version 1 / type / 선언 길이(주어지지 않으면 전체 크기). (2026-11-11 WB04-E319)
function makePacket(type, size, fill = 0, initialLength = 0) {
    const bytes = new Uint8Array(size).fill(fill);
    bytes[0] = 1;
    bytes[1] = type;
    write(bytes, 2, initialLength || size, 2);
    return bytes;
}

// CRT _ftol 의 64비트 절삭값 하위 DWORD 이며 비유한/64비트 범위 밖은 하위 0 이다. (2026-11-11 WB04-E319)
function ftol(value) {
    if (!(value > -(2 ** 63) && value < 2 ** 63)) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 비트(uint32)를 단정도 float 로 읽는다. (2026-11-11 WB04-E319)
function floatFromBits(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 로그인 수신 부모와 응답/Begin/SP/Ready/가입 통지를 같은 현재 Player 에서 실행한다. (2026-11-11 WB04-E319)
export class RawMissionLogin {
    // 사용: 호출자는 현재 Player9 와 참여 처리기에 동일 저장소를 공급해야 하며 모든 참조 대상이 더 오래 살아야 한다. 물리 크기가 다르면 거부한다. (2026-11-11 WB04-E319)
    constructor(players, participation, globals, hooks = {}) {
        if (!(players instanceof Uint8Array) || players.length !== STRIDE * PLAYERS || globals?.zoneAddress?.length !== 6 || globals.battleOptions?.length !== 87 || globals.cachedCoreData?.length !== 10) {
            throw new RangeError('미션 로그인 실제 Player/Zone/옵션 크기 오류');
        }
        this.players = players;
        this.participation = participation;
        this.globals = globals;
        this.hooks = hooks;
        this.counts = new MissionLoginPlayerCounts(players);
    }

    // 원본 0번을 포함한 현재 Player 필드만 읽는다. (2026-11-11 WB04-E319)
    read(owner, offset, width = 4) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= PLAYERS || offset + width > STRIDE) throw new RangeError('미션 로그인 Player 번호/필드 오류');
        return readBytes(this.players, owner * STRIDE + offset, width);
    }

    // 원본 DWORD 쓰기의 다른 비트는 호출자가 보존한다. (2026-11-11 WB04-E319)
    put(owner, offset, value) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= PLAYERS || offset + 4 > STRIDE) throw new RangeError('미션 로그인 Player 쓰기 오류');
        write(this.players, owner * STRIDE + offset, value);
    }

    // 전투 seed 는 공유 자원 상자가 있으면 그 int32 를 unsigned 비트로 읽는다. (2026-11-11 WB04-E319)
    battleSeed() {
        const globals = this.globals;
        return globals.sharedBattleSeed ? globals.sharedBattleSeed.value >>> 0 : globals.battleSeed >>> 0;
    }

    // 004a6920: 응답 0x28(18바이트)을 만든다. 현재 벽시각을 얻은 뒤의 captureEnd 를 읽으므로 callback 변경도 반영된다: 남은 시간 double 의 비트를 +10/+14 에 쓴다. (2026-11-11 WB04-E319)
    reply(owner, accepted) {
        const bytes = makePacket(0x28, 18);
        write(bytes, 4, 10, 2);
        write(bytes, 6, 0x4e, 2);
        bytes[8] = owner & 0xff;
        bytes[9] = accepted & 0xff;
        const now = need(this.hooks.wallSeconds)();
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.globals.captureEnd - now, true);
        write(bytes, 10, view.getUint32(0, true));
        write(bytes, 14, view.getUint32(4, true));
        return bytes;
    }

    // 같은 응답 writer 를 CheckLogin 의 실제 거부 경계에도 빌려 준다(owner 를 대상으로 송신). (2026-11-11 WB04-E319)
    sendReply(owner, accepted) {
        const bytes = this.reply(owner, accepted);
        need(this.hooks.send)(owner | 0, bytes);
    }

    // 004a6990: Begin 0x29(34바이트)는 생성자 초기화와 실제 현재 필드(owner·zone 6·시작 seed·전투 seed)로 정해진다. 디버그 burst 가 켜져 있으면 같은 대상에 별도 63012바이트 예약/작성/송신을 정확히 세 번 수행하고 각 송신 뒤 다음 예약에서 현재 헤더/길이를 다시 읽는다. (2026-11-11 WB04-E319)
    sendBegin(owner) {
        const globals = this.globals;
        const bytes = makePacket(0x29, 34);
        write(bytes, 4, owner);
        bytes.set(globals.zoneAddress, 8);
        write(bytes, 22, globals.beginSeed);
        write(bytes, 30, this.battleSeed());
        need(this.hooks.send)(owner | 0, bytes);
        if (globals.debugBurst) {
            // 같은 대상에 별도 63012바이트 예약/작성/송신을 세 번 수행한다.
            for (let repeat = 0; repeat < 3; repeat += 1) {
                if (!globals.debugPacketHeader || globals.debugPacketInitialLength === undefined || globals.debugPacketInitialLength === null) throw new Error('로그인 디버그의 현재0ch 생성 헤더 미연결');
                const message = globals.debugPacketHeader[0];
                const protocol = globals.debugPacketHeader[1];
                const initial = globals.debugPacketInitialLength;
                const packet = new Uint8Array(63012).fill(0xff);
                packet[1] = message;
                packet[0] = protocol & 0x7f;
                write(packet, 2, initial < 4 ? 4 : initial, 2);
                write(packet, 8, 63000);
                // 40a060 의 원문 payload+12 에 하위 BYTE(index)만 저장한다.
                for (let index = 0; index < 63000; index += 1) packet[12 + index] = index & 0xff;
                need(this.hooks.send)(owner | 0, packet);
            }
        }
    }

    // 004a6e70: 0x81(44바이트, 선언 길이 4 로 시작해 observer 가 44 로 확정)의 owner 0 슬롯은 0xff 로 남고 현재 1..8 float SP 를 각각 절삭해 쓴다(조회마다 현재 값). (2026-11-11 WB04-E319)
    sendSp(owner) {
        const bytes = makePacket(0x81, 44, 0xff, 4);
        // 각 SP 조회는 실제 0040ec50 을 한 번 실행하여 그 시점의 현재 값을 쓴다.
        for (let player = 1; player < 9; player += 1) write(bytes, 8 + player * 4, ftol(floatFromBits(need(this.hooks.spBits)(player))));
        write(bytes, 4, 9);
        need(this.hooks.send)(owner | 0, bytes);
    }

    // 004a6ab0→00491270: 가입한 Player 를 구독자/주소/flags 3 으로 확정하고 현재 접속한 모든 대상에게 보낸 뒤 현재 사람 참여 Player 각각을 새 대상에게 다시 보낸다(두 순회는 합치지 않고 콜백 뒤 flags 를 매번 다시 읽음). (2026-11-11 WB04-E319)
    joined(owner, subscriber) {
        const globals = this.globals;
        this.put(owner, 0x2c, subscriber);
        if (owner !== globals.player.battleOwner) {
            const address = need(this.hooks.peerAddress)(owner);
            const base = owner * STRIDE + 0x30;
            if (address.length + 1 > this.players.length - base) throw new RangeError('미션 로그인 peer주소 범위');
            for (let index = 0; index < address.length; index += 1) this.players[base + index] = address.charCodeAt(index) & 0xff;
            this.players[base + address.length] = 0;
        }
        this.put(owner, FLAGS, ((this.read(owner, FLAGS) & ~0x100) | 3) >>> 0);
        // 00491270 은 가입한 Player 하나를 현재 접속한 모든 대상에게 보낸다.
        for (let target = 1; target < 9; target += 1) if (this.read(target, FLAGS, 1) & 1) need(this.hooks.sendPlayer)(owner, target);
        // 004a6ab0 은 현재 사람 참여 Player 각각을 새 대상에게 다시 보낸다.
        for (let player = 1; player < 9; player += 1) if (this.read(player, FLAGS, 1) & 2) need(this.hooks.sendPlayer)(player, owner);
    }

    // 004a6f70: boss 가 아니면 진단 뒤 계속하고, 현재 owner 1..8 의 flags 0x40 으로 Ready mask 를 만든다(실제 참여/접속 여부와 무관). target 0 은 원본 전체 분배(broadcast), 그 밖은 해당 대상 송신이다. (2026-11-11 WB04-E319)
    sendReady(target) {
        if (!this.globals.boss) need(this.hooks.assertion)('boss');
        const bytes = makePacket(0x6e, 8);
        let mask = 0;
        // 실제 비트 번호는 owner 번호와 같다.
        for (let owner = 1; owner < 9; owner += 1) if (this.read(owner, FLAGS, 1) & 0x40) mask |= 1 << owner;
        write(bytes, 4, mask >>> 0);
        if (!target) need(this.hooks.broadcast)(bytes);
        else need(this.hooks.send)(target | 0, bytes);
    }

    // 004a7800 의 일반/미션 부모를 같은 현재 Player 와 실제 선택 효과로 실행한다. (2026-11-11 WB04-E319)
    receive(pending, connection, packet) {
        this.receiveDirect(pending, connection, packet);
    }

    // Native F 의 nullable 두 번째 인자를 거부 응답의 실제 사용 지점까지 보존한다: NULL 연결은 거부 응답이 실제 선택될 때만 요구하며 앞선 server/Player 부분 효과를 유지한다. (2026-11-11 WB04-E319)
    receiveDirect(pending, connection, packet) {
        const globals = this.globals;
        const hooks = this.hooks;
        if (!globals.server) {
            need(hooks.disconnect)(pending | 0);
            return;
        }
        const subscriber = readBytes(packet, 8);
        let owner = 1;
        // subscriber 가 같은 현재 첫 실제 Player 만 선택한다.
        for (; owner < 9; owner += 1) if (this.read(owner, 0x2c) === subscriber) break;
        if (owner === 9) owner = 0;
        if (!globals.player.playerKnown) {
            // 최초 조건은 한 번만 읽고 루프 안에서는 현재 subscriber 전역을 다시 읽는다.
            for (let player = 1; player < 9; player += 1) {
                if (this.read(player, 0x2c) === (globals.mySubscriber >>> 0)) {
                    globals.player.templateOwner = player;
                    globals.player.battleOwner = player;
                    globals.player.playerKnown = 1;
                    globals.currentCoreData = globals.metamap ? globals.cachedCoreData : this.players.subarray(player * STRIDE + 0x6c, player * STRIDE + 0x6c + 10);
                }
            }
        }
        if (!owner) {
            const bytes = this.reply(0, 0);
            if (!connection) throw new Error('선택된 Login 거부 응답의 실제 연결 미연결');
            need(hooks.pendingReply)(connection, bytes);
            return;
        }
        if (globals.simulconnect) {
            // 실제 새 pending 을 이전 owner 연결과 구별하여 먼저 이전 끝만 닫는다.
            if ((pending | 0) < 0) need(hooks.disconnect)(owner);
            else if ((pending | 0) < 1 || (pending | 0) > 8) need(hooks.assertion)('FIRST_REAL_PLAYER <= pendingId && pendingId < END_REAL_PLAYERS');
        }
        // CheckLogin 이전의 사람 참여 bit 2 를 끝까지 보존한다.
        const previous = (this.read(owner, FLAGS) >>> 1) & 1;
        // 실제 Client 는 원문처럼 previous 0 에서 부모를 호출한다. 독립 미션 Post 1 의 장치 누락만 하위 첫 자연 반환과 같다.
        if (!previous && (hooks.restoreCoreData || !globals.missionPost)) need(hooks.restoreCoreData)(owner);
        this.participation.checkLogin(pending | 0, owner);
        need(hooks.flush)();
        this.sendBegin(owner);
        need(hooks.flush)();
        // 004a7998 의 df590 초기 선언 4 를 유지하고 전체 94BYTE 예약을 같은 송신 gate 에 전달한다.
        const options = makePacket(0x67, 94, 0xff, 4);
        options[4] = 0;
        write(options, 5, 87, 2);
        options.set(globals.battleOptions, 7);
        need(hooks.send)(owner, options);
        if (this.counts.participating() < 2) need(hooks.applyOptions)(options);
        need(hooks.flush)();
        if (!globals.fortCount) need(hooks.clearTypes)();
        need(hooks.flush)();
        // 004a6a70 은 subscriber 가 있는 모든 현재 Player 를 순서대로 보낸다.
        for (let player = 1; player < 9; player += 1) if (this.read(player, 0x2c)) need(hooks.sendPlayer)(player, owner);
        need(hooks.flush)();
        need(hooks.sendFortMetadata)(owner);
        need(hooks.flush)();
        if (!previous) {
            // 실제 0040a4b0 위치이며 name 을 별도 모델로 해석하지 않는다.
            const size = readBytes(packet, 13);
            const start = 17 + readBytes(packet, 12, 1);
            if (start > packet.length || size > packet.length - start) throw new RangeError('미션 로그인 Fort image 물리 범위');
            this.put(owner, 0x5c, globals.fortCount);
            this.put(owner, 0x60, globals.predictableCount);
            globals.fortCount = (globals.fortCount + 1) >>> 0;
            globals.lastLoginPlayer = owner;
            if (globals.server && owner === globals.player.battleOwner) need(hooks.loadCurrentFort)(owner);
            else need(hooks.loadFortImage)(owner, packet.subarray(start, start + size));
        }
        this.joined(owner, readBytes(packet, 8));
        this.sendReply(owner, 1);
        if (!globals.missionPost && !this.selected(globals.savedGame) && this.selected(globals.inBattle)) {
            // 원문 조건의 쉼표식처럼 watcher 쓰기는 +64 검사보다 먼저 실행한다.
            this.put(owner, 0x58, globals.battleOptions[0x3e + owner] === 1 ? 1 : 0);
            if (!this.read(owner, 0x64)) {
                // 각 지정 효과 뒤 watcher/현재 시작 SP 옵션을 다시 읽는다.
                for (let player = 1; player < 9; player += 1) {
                    // watcher 는 실제 SP 0 이며 다른 Player 만 시작 SP + 4000 을 받는다.
                    let amount = 0;
                    if (globals.battleOptions[0x3e + player] !== 1) {
                        // 0041c7d0 의 실제 3항목이며 옵션+0e 가 현재 색인을 고른다.
                        const startingSp = [1000, 2500, 3000];
                        const index = globals.battleOptions[0x0e];
                        if (index >= startingSp.length) throw new RangeError('일반 로그인 시작 SP 옵션 색인');
                        amount = startingSp[index] + 4000;
                    }
                    need(hooks.setSp)(player, amount);
                }
            }
        }
        need(hooks.flush)();
        if (globals.metamap) need(hooks.assertion)('!metamap');
        this.sendSp(owner);
        need(hooks.flush)();
        globals.clockStamp = need(hooks.clock)() >>> 0;
        const difference = Math.trunc(((globals.clockStamp - (globals.baseClock >>> 0)) | 0) / 1000);
        // 시간 packet 은 실제 1..8 전체를 요청한다. 실제 접속 gate/연결 부재 처리는 send 가 담당한다.
        for (let target = 1; target < 9; target += 1) {
            const bytes = makePacket(0x84, 8);
            write(bytes, 4, difference >>> 0);
            need(hooks.send)(target, bytes);
        }
        need(hooks.flush)();
        this.put(owner, 0x64, 1);
        // 현재 fortCount 는 바깥 반복마다 다시 읽으며 새 자기 Fort 는 이 첫 순회에서 제외한다.
        for (let order = 0; (order | 0) < (globals.fortCount | 0); order += 1) {
            // 각 효과 뒤 현재 flags/순서/예측수를 다시 읽는다.
            for (let player = 1; player < 9; player += 1) {
                if ((this.read(player, FLAGS, 1) & 2) && this.read(player, 0x5c) === order && (previous || player !== owner)) {
                    need(hooks.sendFort)(player, owner, this.read(player, 0x60));
                    need(hooks.flush)();
                }
            }
        }
        if (owner !== globals.player.battleOwner) need(hooks.synchronizePeer)(owner);
        need(hooks.flush)();
        if (!previous) {
            // 두번째 순회는 자기 대상도 포함하여 새 Fort 를 현재 접속자에게 보낸다.
            for (let target = 1; target < 9; target += 1) {
                if (this.read(target, FLAGS, 1) & 1) {
                    if (this.read(globals.lastLoginPlayer, 0x60) !== (globals.predictableCount >>> 0)) need(hooks.assertion)('predictableCount');
                    need(hooks.sendFort)(owner, target, globals.predictableCount);
                }
            }
            // 원문 미션 저장 월드만 현재 Load 를 실행한다.
            if (globals.missionWorld) need(hooks.loadWorld)();
            // 일반 watcher 는 실제 세 단계의 전투섬 생성 없이 settings/Ready 로 간다.
            else if (globals.missionPost || !this.read(owner, 0x58)) need(hooks.makeBattleIsland)(owner);
        }
        need(hooks.sendSettings)(owner);
        need(hooks.flush)();
        need(hooks.flush)();
        const current = globals.player.battleOwner;
        this.put(current, FLAGS, (this.read(current, FLAGS) | 0x40) >>> 0);
        this.sendReady(owner);
        need(hooks.flush)();
        this.finish();
    }

    // 선택 필드(저장 경로/전투 여부 같은 bool 포인터)를 읽는다. 일반 자원 초기화가 선택될 때 필드가 없으면 임의 값을 공급하지 않고 거부한다. (2026-11-11 WB04-E319)
    selected(value) {
        if (value === undefined || value === null) throw new Error('미션 로그인 실제 외부 몸체 미연결');
        return Boolean(value);
    }

    // 로그인 마무리: 구독자가 하나뿐이거나(끊긴 사람 제외) 하나만 남으면 시작 표시 begin 을 1 로 쓴다(뒤 4437f0 은 빈 함수라 새 효과가 없음). (2026-11-11 WB04-E319)
    finish() {
        if (this.counts.subscribers() !== 1) {
            const disconnected = this.counts.disconnected();
            if (((this.counts.subscribers() - disconnected) >>> 0) !== 1) return;
        }
        this.globals.begin = 1;
    }

    // 63h 재접속은 CheckLogin 거부 뒤에도 원문 후반을 계속하며 새 Fort/월드 생성 분기를 실행하지 않는다: 실제 8바이트 입력만 받는다. (2026-11-11 WB04-E319)
    receiveReconnect(pending, packet) {
        const globals = this.globals;
        const hooks = this.hooks;
        if (packet.length !== 8 || packet[1] !== 0x63 || readBytes(packet, 2, 2) !== 8) throw new TypeError('재접속63h의 실제8BYTE 입력 오류');
        need(hooks.log)(`Reconnecting ${pending | 0}\n`);
        let owner = 1;
        // 원문은 subscriber 0 도 비교하며 별도 flags/서버 권한 관문을 추가하지 않는다.
        for (; owner < 9; owner += 1) if (readBytes(packet, 4) === this.read(owner, 0x2c)) break;
        if (owner === 9) {
            // Serverboard.cpp 1b2 의 실제 진단 복귀 뒤 반환이다.
            if (need(hooks.checkingAssertions)()) need(hooks.assertion)('0');
            return;
        }
        if (this.participation.checkLogin(pending | 0, owner)) this.sendReply(owner, 1);
        if (readBytes(packet, 4) === (globals.mySubscriber >>> 0)) this.put(owner, FLAGS, (this.read(owner, FLAGS) | 0x18) >>> 0);
        this.joined(owner, readBytes(packet, 4));
        globals.fortCount = 0;
        globals.lastLoginPlayer = 0;
        let marker = 0;
        // PE 는 counter 의 최대값 대신 앞서 고른 owner 번호와 다음 +5c 를 비교한다. 이 원문 선택을 정규화하지 않는다.
        for (let player = 1; player < 9; player += 1) {
            if (this.read(player, FLAGS, 1) & 2) {
                globals.fortCount = (globals.fortCount + 1) >>> 0;
                if ((this.read(player, 0x5c) | 0) > marker) {
                    marker = player;
                    globals.lastLoginPlayer = player;
                }
            }
        }
        need(hooks.cleanupReconnectClients)();
        const local = globals.player.battleOwner;
        this.put(local, FLAGS, (this.read(local, FLAGS) | 0x40) >>> 0);
        this.sendReady(owner);
        this.finish();
    }
}

// 원본 초기화처럼 0x27 의 실제 버전 1/정책 0/처리기와 0x63 재접속(길이 8)을 등록한다. login 은 dispatch 보다 오래 살아야 한다. (2026-11-11 WB04-E319)
export function registerMissionLogin(dispatch, login) {
    dispatch.observe(0x27, null);
    dispatch.version(0x27, 1, 0);
    dispatch.register(0x27, (pending, connection, packet) => { login.receive(pending | 0, connection, packet); });
    // 원문 99 의 observer 40b380 은 빈 함수이며 실제 63h/1/8 수신을 등록한다.
    dispatch.observe(0x63, null);
    dispatch.version(0x63, 1, 0);
    dispatch.requireLength(0x63, 8);
    dispatch.register(0x63, (pending, connection, packet) => { login.receiveReconnect(pending | 0, packet); });
}
