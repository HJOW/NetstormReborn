// 로그인 서버가 보낸 실제 8종 패킷(0x29 LoginBegin·0x67 Options·0x2b Player·0x2a Fort·0x28 Reply·0x81 SP·0x84 시간·0x6e Ready)의 현재 Player/State/SP/Template 수신 처리를 옮긴 모듈이다
// (cpppj `o/RawMissionClientPackets.cpp/.h`와 기준선 뒤 새 `o/RawStateBattleOptions`의 67h 전체 부모, 10.78: 004b3b40·004b4900·004912d0·004b3d30·004b8940·004b3e90·004a6f30·004a7050, 송신 observer 40a540/40a590/40ae80/40b460, 이름 정제 0042b0e0).
// 사용: const packets = new RawMissionClientPackets(players, participation, sp, state, hooks); registerMissionClientPackets(dispatch, packets); packets.receive(sender, connection, packet);
//   players = Player9 기록 9×180BYTE(`Uint8Array`), participation = `RawPlayerParticipation`, sp = `ScrambledSpStore`, state = 원본 현재 전역 묶음(아래 필드를 속성으로 가진 객체, 읽는 시점마다 현재값을 조회하고 쓰는 필드는 직접 갱신),
//   hooks = 도달한 위치에서만 요구하는 큰 외부 소비자(진단·UI·전송·Fort 이미지 등). 빈 수신 성공은 허용하지 않고 선택한 장치가 없으면 앞선 쓰기를 남긴 채 거부한다.
//   state 의 읽기 전용 필드: server·boss·missionPost·savedGame·metamap·inBattle·editor·outpost·phase·loginEnd·currentHandle(NUL 종료 이름)·hidden(battleOptions+0x3e 의 9BYTE 같은 메모리).
//   쓰기 필드: player({ playerKnown, battleOwner, templateOwner })·beginSeed·battleSeed(또는 sharedBattleSeed `{ value }` int32 상자)·revision·optionValue·begin·clockStamp·baseClock·currentState·currentAux·pendingState·sourceTicks·coreDataActive·captureEnd(double)·currentCoreData(Uint8Array 또는 null)·cachedCoreData(10)·zoneAddress(6)·battleOptions(87).
// 기준선 뒤 cpppj 변경(46d64b2a7·3f942573d·aa91f53b1·ede61f05d·eeae45c69, 3.6절 ②): 67h 옵션 수신의 공통 State 부모(`receiveStateBattleOptions`, 대응표 밖 신규 파일 RawStateBattleOptions 의 동작)·송신 observer(`observeMissionClientPacket`)·고정 진입값 직접 수신(`receiveFixed`)을 반영했다.
//   Native Login 고정 수신기를 분배기에 대여하는 분기는 E306 의 ③(고정 Native 수신기)와 같은 묶음이라 옮기지 않는다. 8종 중 Ready(boss 서버 경로)가 부르는 `sendReady` 의 본체는 RawMissionLogin(다음 단위들)이다.
// CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E314: 4-C RawMissionClientPackets 이식)

// 실제 Player 기록 폭과 개수(10.78)이다. (2026-11-11 WB04-E314)
const STRIDE = 0xb4;
const COUNT = 9;

// 원본 이름 정제가 쓰는 금지문자표(0xa0 과 % . ? = ( ) [ ] ~ " ` ; : 공백)의 바이트 집합이다. (2026-11-11 WB04-E314)
const FORBIDDEN = new Set([0xa0, 0x25, 0x2e, 0x3f, 0x3d, 0x28, 0x29, 0x5b, 0x5d, 0x7e, 0x22, 0x60, 0x3b, 0x3a, 0x20]);

// 선택한 외부 장치가 함수가 아니면 도달한 위치에서 오류를 낸다(앞선 쓰기는 남음). (2026-11-11 WB04-E314)
function need(device) {
    if (typeof device !== 'function') throw new Error('미션 클라이언트 패킷의 실제 소비자 미연결');
    return device;
}

// 비정렬 BYTE/WORD/DWORD 를 낮은 바이트부터 읽는다(물리 범위 밖은 오류). (2026-11-11 WB04-E314)
function get(bytes, offset, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset + width > bytes.length) throw new RangeError('미션 수신 읽기 물리 범위');
    let value = 0;
    // 필요한 폭의 낮은 바이트부터 조립한다.
    for (let index = 0; index < width; index += 1) value |= bytes[offset + index] << (index * 8);
    return value >>> 0;
}

// 지정 폭의 낮은 바이트만 쓰고 인접 stale 바이트는 유지한다(물리 범위 밖은 오류). (2026-11-11 WB04-E314)
function write(bytes, offset, value, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset + width > bytes.length) throw new RangeError('미션 수신 쓰기 물리 범위');
    // 지정 BYTE/WORD/DWORD 의 낮은 바이트만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}

// NUL 을 포함한 strcpy 입력 구간만 고른다(NUL 이 없으면 오류). (2026-11-11 WB04-E314)
function cString(bytes) {
    const end = bytes.indexOf(0);
    if (end < 0) throw new RangeError('미션 수신 이름 NUL 누락');
    return bytes.subarray(0, end + 1);
}

// float 의 비트(uint32)를 만든다. (2026-11-11 WB04-E314)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 0042b0e0: 금지 문자를 현재 NUL 까지 한 칸씩 당겨 지우고(삭제한 위치는 다시 검사하므로 연속 금지문자도 제거) NUL 뒤 stale 바이트는 지우지 않는다.
// 이름이 비면 `Cheat<구독자 & 0xff>` 로 대체하며 이름 저장소보다 길면 오류이다. Player 와 이후 송신 이름 처리가 같은 몸체를 쓴다. (2026-11-11 WB04-E314)
export function sanitizeMissionPlayerName(subscriber, name) {
    let length = cString(name).length - 1;
    let index = 0;
    // 삭제한 위치는 다시 검사해 연속 금지문자도 원본처럼 제거한다.
    while (index < length) {
        if (!FORBIDDEN.has(name[index])) {
            index += 1;
            continue;
        }
        // NUL 을 포함해 현재 문자 뒤를 왼쪽으로 한 칸 옮긴다.
        for (let next = index; next < length; next += 1) name[next] = name[next + 1];
        length -= 1;
    }
    name[length] = 0;
    if (!length) {
        const replacement = `Cheat${subscriber & 0xff}`;
        if (replacement.length + 1 > name.length) throw new RangeError('미션 빈 이름 대체 범위');
        // 대체 이름과 NUL 을 쓴다.
        for (let at = 0; at < replacement.length; at += 1) name[at] = replacement.charCodeAt(at);
        name[replacement.length] = 0;
    }
}

// 40a540/40a590/40ae80/40b460: 송신(sending ≠ 0)에서만 현재 필드를 검사하고 선언 WORD 를 원문 폭으로 다시 쓴다(수신 방향에서는 읽지 않음). 진단이 복귀하면 현재 packet 을 다시 읽어 길이를 쓴다. 나머지 등록 4종의 observer 는 무효과이다. (2026-11-11 WB04-E314)
export function observeMissionClientPacket(type, packet, sending, assertion) {
    if (!sending) return;
    switch (type) {
    case 0x2a:
        if (get(packet, 0xc) === 0xffffffff) need(assertion)('fortFileSize != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x672);
        write(packet, 2, get(packet, 0xc, 2) + 0x11, 2);
        return;
    case 0x2b:
        if (get(packet, 0x33, 1) === 0xff) need(assertion)('nameLen != (unsigned char)-1', '\\Ns\\O\\_p_packets.cpp', 0x695);
        write(packet, 2, get(packet, 0x33, 1) + 0x3b, 2);
        return;
    case 0x67:
        if (get(packet, 5, 2) === 0xffff) need(assertion)('dataLen != (unsigned short)-1', '\\Ns\\O\\_p_packets.cpp', 0xa07);
        write(packet, 2, get(packet, 5, 2) + 7, 2);
        return;
    case 0x81: {
        if (get(packet, 4) === 0xffffffff) need(assertion)('numPlayers != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0xc3d);
        // 40b48a 의 현재 하위 WORD 와 ADD AX 의 16비트 감김을 SHL 전에 보존한다.
        const count = (get(packet, 4, 2) + 2) & 0xffff;
        write(packet, 2, count * 4, 2);
        return;
    }
    default:
        return;
    }
}

// 004b4900 전체: 길이(87 초과면 진단, 복귀 뒤 현재 길이 재조회) → 옵션 DWORD 단위 복사 → 나머지 0..3 BYTE 복사 → (CS 가 있으면 0042c920) → 범위 → 비용 조회/저장(float) → 환불 비율 조회/저장 → (전투·미션 아님·저장 경로면) 재접속 UI.
// 일반 수신과 메타 수신이 같은 순서로 쓰는 대응표 밖 신규 파일 `RawStateBattleOptions` 의 동작이다. globals 는 { options(87), challenge, battle, mission, saved, setValue(v) } 로 읽는 시점마다 현재값을 조회한다. (2026-11-11 WB04-E314)
export function receiveStateBattleOptions(sender, packet, globals, hooks) {
    if (get(packet, 5, 2) > 87) need(hooks.assertion)('z->dataLen <= sizeof(globalBattleOptions)');
    const length = get(packet, 5, 2);
    let at = 0;
    // 원문 rep movsd 의 현재 한 DWORD 씩 읽기/쓰기이다.
    for (let word = length >>> 2; word; word -= 1, at += 4) write(globals.options, at, get(packet, 7 + at, 4), 4);
    // 마지막 0..3BYTE 만 추가로 복사하며 미복사 꼬리를 지우지 않는다.
    for (let byte = length & 3; byte; byte -= 1, at += 1) write(globals.options, at, get(packet, 7 + at, 1), 1);
    if (globals.challenge) need(hooks.challengeOptions)(sender, packet);
    need(hooks.range)();
    const cost = need(hooks.cost)();
    need(hooks.storeCost)(Math.fround(cost | 0));
    globals.setValue(need(hooks.currentValue)() >>> 0);
    if (globals.battle && !globals.mission && globals.saved) need(hooks.reconnect)();
}

// 같은 Session 의 Player9/SP/참여 모듈을 참조하며 실제 8종 수신 분기를 실행한다. (2026-11-11 WB04-E314)
export class RawMissionClientPackets {
    // 사용: 저장소는 처리기보다 오래 살아야 하며 호출자는 같은 Player9 를 participation 에 공급한다. 물리 크기가 어긋나면 거부한다. (2026-11-11 WB04-E314)
    constructor(players, participation, sp, state, hooks = {}) {
        const hidden = state?.hidden;
        const options = state?.battleOptions;
        if (!(players instanceof Uint8Array) || players.length !== STRIDE * COUNT || state?.cachedCoreData?.length !== 10 || state.zoneAddress?.length !== 6 || options?.length !== 87 || hidden?.length !== 9
            || hidden.buffer !== options.buffer || hidden.byteOffset !== options.byteOffset + 0x3e) {
            throw new RangeError('미션 수신의 실제 Player/현재 전역 크기 오류');
        }
        this.players = players;
        this.participation = participation;
        this.sp = sp;
        this.state = state;
        this.hooks = hooks;
    }

    // 현재 9기록에서 원본 폭만 읽는다. (2026-11-11 WB04-E314)
    read(owner, offset, width = 4) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= COUNT || offset + width > STRIDE) throw new RangeError('미션 수신 Player 읽기 범위');
        return get(this.players, owner * STRIDE + offset, width);
    }

    // 같은 현재 9기록의 필요한 필드만 쓴다. (2026-11-11 WB04-E314)
    put(owner, offset, value, width = 4) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= COUNT || offset + width > STRIDE) throw new RangeError('미션 수신 Player 쓰기 범위');
        write(this.players, owner * STRIDE + offset, value, width);
    }

    // 원본 문자열은 선언 필드 폭을 넘더라도 실제 Player9 물리 저장소 안에서는 NUL 까지 복사한다. (2026-11-11 WB04-E314)
    copy(owner, offset, source) {
        const text = cString(source);
        if (!Number.isInteger(owner) || owner < 0 || owner >= COUNT || owner * STRIDE + offset + text.length > this.players.length) throw new RangeError('미션 수신 Player 문자열 범위');
        this.players.set(text, owner * STRIDE + offset);
    }

    // 현재 정제된 Player 이름의 NUL 이전 문자열(바이트 = 문자 코드의 이진 문자열)이며 채팅에 전달한다. (2026-11-11 WB04-E314)
    name(owner) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= COUNT) throw new RangeError('미션 수신 이름 Player 범위');
        const text = cString(this.players.subarray(owner * STRIDE + 12));
        return String.fromCharCode(...text.subarray(0, text.length - 1));
    }

    // 004b3b40: 서버도 현재 번호/CoreData 를 지정하고 원본 여섯 바이트(zone)와 두 seed 를 쓴다. owner 0 이면 이전 CoreData 를 캐시에 보관하고 선택을 비운다. (2026-11-11 WB04-E314)
    loginBegin(packet) {
        const state = this.state;
        if (state.server && get(packet, 4) !== state.player.battleOwner) need(this.hooks.assertion)('z->yourPlayerId == myPlayerId()');
        const owner = get(packet, 4);
        if (owner >= COUNT) throw new RangeError('LoginBegin Player 범위');
        state.player.templateOwner = owner;
        state.player.battleOwner = owner;
        if (!owner) {
            if (state.currentCoreData && state.currentCoreData.length) {
                if (state.currentCoreData.length < 10) throw new RangeError('LoginBegin 이전CoreData 범위');
                state.cachedCoreData.set(state.currentCoreData.subarray(0, 10));
            }
            state.currentCoreData = null;
            state.coreDataActive = 0;
        } else {
            state.player.playerKnown = 1;
            state.currentCoreData = state.metamap ? state.cachedCoreData : this.players.subarray(owner * STRIDE + 0x6c, owner * STRIDE + 0x6c + 10);
        }
        state.beginSeed = get(packet, 22);
        state.zoneAddress.set(packet.subarray(8, 14));
        // unsigned 입력을 명시 비트 변환해 저장한다(실제 자원 DWORD 를 빌렸으면 그곳에 직접 쓴다).
        const seed = get(packet, 30);
        if (state.sharedBattleSeed) state.sharedBattleSeed.value = seed | 0;
        else state.battleSeed = seed;
    }

    // 004b4900: 현재 옵션 복사와 모든 파생 효과를 순서대로 실행한다. 각 원문 위치에서 필요한 현재 장치만 같은 공통 몸체가 선택한다. (2026-11-11 WB04-E314)
    options(sender, packet) {
        const state = this.state;
        const hooks = this.hooks;
        receiveStateBattleOptions(sender, packet, {
            options: state.battleOptions,
            get challenge() { return state.outpost; },
            get battle() { return state.inBattle; },
            get mission() { return state.missionPost; },
            get saved() { return state.savedGame; },
            setValue: (value) => { state.optionValue = value; },
        }, {
            assertion: hooks.assertion, challengeOptions: hooks.outpostOptions, range: hooks.updateRange, cost: hooks.requiredEnergy, storeCost: hooks.setEnergyType, currentValue: hooks.readOptionValue, reconnect: hooks.reconnectUi,
        });
    }

    // 004b3d30: 로컬 미션 서버에서는 재설치를 생략하고 revision 만 실제로 증가한다. 클라이언트는 숨김이 아니면(또는 미션 후처리) Fort 이미지를 읽어 배치한다. (2026-11-11 WB04-E314)
    fort(packet) {
        const state = this.state;
        const owner = get(packet, 4);
        if (owner >= COUNT) throw new RangeError('미션 Fort Player 범위');
        if (owner === state.player.battleOwner) {
            if (!state.missionPost) need(this.hooks.localFort)(owner);
        } else {
            if (state.server) {
                state.revision = (state.revision + 1) >>> 0;
                return;
            }
            need(this.hooks.openFortImage)(owner);
        }
        if (!state.server) {
            if (state.missionPost || !state.hidden[get(packet, 4)]) {
                const size = get(packet, 12);
                if (packet.length < 17 || size > packet.length - 17) throw new RangeError('미션 Fort image 실제 범위');
                need(this.hooks.readFortImage)(owner, packet.subarray(17, 17 + size));
                need(this.hooks.buildFort)(get(packet, 4));
            }
        }
        state.revision = (state.revision + 1) >>> 0;
    }

    // 004912d0: 이름/flags/채널/방향별 동맹/metadata/UI 순서를 현재 읽기로 실행한다. 콜백이 packet 을 바꿀 수 있으므로 packet 필드는 쓰는 위치마다 다시 읽는다. (2026-11-11 WB04-E314)
    player(sender, packet) {
        const state = this.state;
        const hooks = this.hooks;
        const owner = get(packet, 0x32, 1);
        if (owner >= COUNT || packet.length < 0x3b) throw new RangeError('미션 Player packet 범위');
        this.copy(owner, 12, packet.subarray(0x3b));
        if (state.missionPost && get(packet, 0x32, 1) === state.player.battleOwner) this.copy(owner, 12, state.currentHandle);
        sanitizeMissionPlayerName(get(packet, 0x26), this.players.subarray(owner * STRIDE + 12));
        this.put(owner, 0x2c, get(packet, 0x26));
        this.put(owner, 0x71, get(packet, 0x3a, 1), 1);
        if (!state.server) {
            this.copy(owner, 0x30, packet.subarray(4));
            // '*' 주소는 현재 sender 연결의 실제 주소 문자열로 바꾼다.
            if (this.players[owner * STRIDE + 0x30] === 0x2a) {
                const address = need(hooks.peerAddress)(sender);
                const bytes = new Uint8Array(address.length + 1);
                for (let index = 0; index < address.length; index += 1) bytes[index] = address.charCodeAt(index) & 0xff;
                this.copy(owner, 0x30, bytes);
            }
            this.put(owner, 0x50, ((this.read(owner, 0x50) & 0x40) | get(packet, 0x2a)) >>> 0);
            this.put(owner, 0x54, get(packet, 0x22));
        }
        if (!(this.read(owner, 0x50, 1) & 1)) need(hooks.removeChat)(this.read(owner, 0x2c), 0x20);
        else need(hooks.addChat)(this.read(owner, 0x2c), this.name(owner), 0x20);
        this.participation.setAlliance(get(packet, 0x32, 1), get(packet, 0x22));
        if (!state.server) {
            this.put(owner, 0x5c, get(packet, 0x34, 1));
            this.put(owner, 0x60, get(packet, 0x35, 2));
            this.put(owner, 0xa0, get(packet, 0x37, 1));
            this.put(owner, 0xa4, get(packet, 0x38, 1), 1);
            this.put(owner, 0xa5, get(packet, 0x39, 1), 1);
        }
        if (!state.missionPost && this.read(owner, 0x2c)) need(hooks.restoreCoreData)(get(packet, 0x32, 1));
        if (!need(hooks.playerUiBusy)()) {
            if (!state.missionPost) need(hooks.saveReconnect)();
            if (this.read(owner, 0x50, 1) & 1) {
                need(hooks.playerNotice)(5, get(packet, 0x32, 1), 0);
                need(hooks.playerNotice)(3, get(packet, 0x32, 1), 1);
                return;
            }
            need(hooks.playerNotice)(3, get(packet, 0x32, 1), 0);
            need(hooks.playerNotice)(2, get(packet, 0x32, 1), 0);
            if (this.read(owner, 0x50, 1) & 2) need(hooks.playerNotice)(5, get(packet, 0x32, 1), 1);
        }
    }

    // 004b8940: 버전 확인(10.0x4e 미만이고 안내 억제가 꺼져 있으면 GameVerDiff 안내→상태 12→동기 State 실행), 남은 시간(double 비트)을 벽시각에 더해 종료 시각으로 저장한 뒤 현재 phase 를 다시 읽어 State 를 요청한다. (2026-11-11 WB04-E314)
    reply(packet) {
        const state = this.state;
        const hooks = this.hooks;
        const major = get(packet, 4, 2);
        const minor = get(packet, 6, 2);
        if ((major !== 10 || minor < 0x4e) && !need(hooks.configInt)('suppressVersionMessage')) {
            need(hooks.tell)(`GameVerDiff(${major}.${minor})`);
            state.currentState = 12;
            state.currentAux = 0;
            need(hooks.runState)();
            return;
        }
        // 남은 시간은 두 DWORD 가 이루는 double 비트이다.
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, get(packet, 10), true);
        view.setUint32(4, get(packet, 14), true);
        const now = need(hooks.wallSeconds)();
        state.captureEnd = view.getFloat64(0, true) + now;
        if (state.phase === 16) {
            state.currentState = 18 - (get(packet, 9, 1) !== 0 ? 1 : 0);
            need(hooks.runState)();
            return;
        }
        if (state.phase === 10) {
            state.currentState = (get(packet, 9, 1) !== 0 ? 1 : 0) * 2 + 11;
            need(hooks.resetCamera)();
            need(hooks.runState)();
            return;
        }
        state.sourceTicks = 0;
        state.pendingState = 3;
    }

    // 004b3e90→00447810: 각 반복 끝에 현재 count 를 다시 읽는다. 음수 SP 는 전달하지 않고 편집기 모드에서는 194162.0 으로 쓰며, 로컬이면 갱신을 요청한다. (2026-11-11 WB04-E314)
    receiveSp(packet) {
        const state = this.state;
        // 현재 count(signed)는 각 반복 끝에 다시 읽는다.
        for (let owner = 0; owner < (get(packet, 4) | 0); owner += 1) {
            const value = get(packet, 8 + owner * 4);
            if ((value | 0) < 0) continue;
            const amount = get(packet, 8 + owner * 4);
            const local = Boolean(state.player.playerKnown) && owner === state.player.battleOwner;
            if (local || (owner > 0 && owner < COUNT)) {
                const number = state.editor ? 194162.0 : Math.fround(amount | 0);
                this.sp.set(local ? state.player.battleOwner : owner, floatBits(number));
                if (local) need(this.hooks.refreshSp)();
            }
        }
    }

    // 004a7050: boss 서버의 9 입력은 자연 반환하고, 클라이언트는 mask 를 받아 자기 0x40 응답을 별도로 실행하며, boss 는 보낸 owner 를 준비 상태로 두고 전체 분배를 요청한다. (2026-11-11 WB04-E314)
    ready(sender, packet) {
        const state = this.state;
        const hooks = this.hooks;
        if (!state.boss) {
            if ((get(packet, 4) | 0) < 0) state.begin = 1;
            // 원본 1..8 각 조건은 현재 packet 을 다시 읽으며 해당 Player 0x40 만 OR 한다.
            for (let owner = 1; owner < COUNT; owner += 1) {
                if (get(packet, 4) & (1 << owner)) this.put(owner, 0x50, (this.read(owner, 0x50) | 0x40) >>> 0);
            }
            const self = state.player.battleOwner;
            if (!(this.read(self, 0x50) & 0x40)) {
                this.put(self, 0x50, (this.read(self, 0x50) | 0x40) >>> 0);
                if (packet.length < 8) throw new RangeError('미션 Ready 응답 8BYTE 범위');
                need(hooks.send)(9, packet.subarray(0, 8));
            }
        } else if (sender !== 9) {
            this.put(sender, 0x50, (this.read(sender, 0x50) | 0x40) >>> 0);
            need(hooks.sendReady)(0);
            if (state.loginEnd === 0) need(hooks.send)(sender | 0, Uint8Array.of(1, 0x6e, 8, 0, 0, 0, 0, 0x80));
        }
    }

    // 원본 등록이 고른 실제 작은 몸체를 분배한다: 현재 packet 종류(BYTE 1)를 고정 진입값으로 바꿔 실행하며 다른 종류를 무시하는 성공은 없다. (2026-11-11 WB04-E314)
    receive(sender, connection, packet) {
        switch (get(packet, 1, 1)) {
        case 0x29: this.receiveFixed(0x004b3b40, sender, packet); return;
        case 0x67: this.receiveFixed(0x004b4900, sender, packet); return;
        case 0x2b: this.receiveFixed(0x004912d0, sender, packet); return;
        case 0x2a: this.receiveFixed(0x004b3d30, sender, packet); return;
        case 0x28: this.receiveFixed(0x004b8940, sender, packet); return;
        case 0x81: this.receiveFixed(0x004b3e90, sender, packet); return;
        case 0x84: this.receiveFixed(0x004a6f30, sender, packet); return;
        case 0x6e: this.receiveFixed(0x004a7050, sender, packet); return;
        default: throw new TypeError('미등록 미션 클라이언트 packet 종류');
        }
    }

    // 저장된 원문 진입값만 선택해 같은 본문을 실행하며 현재 packet 종류/연결을 다시 읽지 않는다(0x004a6f30 은 시간 응답: 현재 clock 과 정수 틱 차의 하위 DWORD 를 기준 시계로 쓴다). (2026-11-11 WB04-E314)
    receiveFixed(entry, sender, packet) {
        const state = this.state;
        switch (entry) {
        case 0x004b3b40: this.loginBegin(packet); break;
        case 0x004b4900: this.options(sender, packet); break;
        case 0x004912d0: this.player(sender, packet); break;
        case 0x004b3d30: this.fort(packet); break;
        case 0x004b8940: this.reply(packet); break;
        case 0x004b3e90: this.receiveSp(packet); break;
        case 0x004a6f30: {
            const ticks = get(packet, 4) | 0;
            state.clockStamp = need(this.hooks.clock)() >>> 0;
            // 004a6f50 의 double 1000 곱과 fsubp 는 decomp 본문에서 누락됐다: 정수 입력의 정확한 64비트 차의 하위 DWORD 이다.
            const difference = BigInt(state.clockStamp | 0) - BigInt(ticks) * 1000n;
            state.baseClock = Number(BigInt.asUintN(32, difference));
            break;
        }
        case 0x004a7050: this.ready(sender, packet); break;
        default: throw new TypeError('미등록 미션 클라이언트 packet 종류');
        }
    }

    // 송신 observer 를 같은 Client 진단 장치(`hooks.observerAssertion`)로 실행한다. 부품 검사와 송신 등록이 서로 다른 길이 수식을 복제하지 않는다. (2026-11-11 WB04-E314)
    observe(type, packet, sending) {
        observeMissionClientPacket(type, packet, sending, this.hooks.observerAssertion);
    }
}

// 원본 8종의 observer/버전 1/정책 0/수신기를 실제 분배기에 등록한다: 가변 길이 4종(0x2a·0x2b·0x67·0x81)은 mutable 송신 observer 를, 나머지는 빈 observer 를 건다. 원본 길이 표는 분배기가 이미 소유한다. (2026-11-11 WB04-E314)
export function registerMissionClientPackets(dispatch, packets) {
    const types = [[0x29, 0x004b3b40], [0x67, 0x004b4900], [0x2b, 0x004912d0], [0x2a, 0x004b3d30], [0x28, 0x004b8940], [0x81, 0x004b3e90], [0x84, 0x004a6f30], [0x6e, 0x004a7050]];
    // 각 타입은 버전 1/정책 0 과 같은 처리기를 가진다.
    for (const [type, entry] of types) {
        if (type === 0x2a || type === 0x2b || type === 0x67 || type === 0x81) dispatch.observePacket(type, (packet, sending) => { packets.observe(type, packet, sending); });
        else dispatch.observe(type, () => {});
        dispatch.version(type, 1, 0);
        dispatch.register(type, (sender, connection, packet) => { packets.receiveFixed(entry, sender, packet); });
    }
}
