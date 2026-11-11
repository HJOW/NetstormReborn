// 미션 로그인 서버 8종 패킷 수신(`raw-mission-client-packets.js`)과 로그인 몸체(`raw-mission-login.js`)의 시험이 함께 쓰는 현재 저장소 장면이다(cpppj `MissionClientPacketsTests.cpp` 의 `ClientPacketFixture` 대응).
// 모든 수신기와 실제 참여/Ready 송신/SP 가 같은 Player9·옵션·동맹 표·전역을 공유하며, 입력만 명시하고 원본 결과를 계산하지 않는다. 시험 전용 도우미이며 제품 코드가 아니다. (2026-11-11 WB04-E319: 클라이언트 시험에서 분리)
import assert from 'node:assert/strict';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { PlayerParticipationState, RawPlayerParticipation } from '../www/core/raw-player-participation.js';
import { PlayerWorldState } from '../www/core/raw-player-world.js';
import { RawMissionClientPackets, observeMissionClientPacket } from '../www/core/raw-mission-client-packets.js';
import { RawMissionLogin } from '../www/core/raw-mission-login.js';

// 바이트를 소문자 16진 문자열로, 문자열(이진)을 바이트열로 바꾼다. (2026-11-11 WB04-E314)
export const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
export const hexBytes = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
export const textHex = (text) => hex(Array.from(text, (char) => char.charCodeAt(0)));

// little endian DWORD 를 쓴다. (2026-11-11 WB04-E314)
export function put(bytes, offset, value) {
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// float 비트를 만든다. (2026-11-11 WB04-E314)
export function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 모든 수신기와 실제 참여/SP 가 공유하는 현재 저장소이다(C++ ClientPacketFixture 대응). 입력만 명시하고 원본 결과를 계산하지 않는다. (2026-11-11 WB04-E314)
export class ClientPacketScene {
    // 생성: values = 입력 행의 쉼표 열(case,server,boss,mission,known,owner,variant,mutate,sender), input = 수신 packet 바이트이다.
    constructor(values, input) {
        this.packet = Uint8Array.from(input);
        this.players = new Uint8Array(1620);
        this.options = new Uint8Array(87).fill(0xa5);
        this.allies = new Uint32Array(81);
        this.hidden = this.options.subarray(0x3e, 0x3e + 9);
        this.zone = Uint8Array.of(0x11, 0x12, 0x13, 0x14, 0x15, 0x16);
        this.cached = new Uint8Array(10);
        this.handle = Uint8Array.of(...Array.from('My Name', (char) => char.charCodeAt(0)), 0);
        this.player = new PlayerWorldState();
        this.participationState = new PlayerParticipationState();
        this.trace = [];
        this.server = Number(values[1]) !== 0;
        this.boss = Number(values[2]) !== 0;
        this.mission = Number(values[3]) !== 0;
        this.saved = true;
        this.metamap = false;
        this.battle = true;
        this.simulconnect = false;
        this.mutated = false;
        this.editor = 0;
        this.outpost = 0;
        this.seed = 7; this.battleSeed = 8; this.revision = 9; this.option = 10; this.begin = 0;
        this.clockStamp = 11; this.baseClock = 12; this.phase = 0; this.current = 13; this.aux = 14; this.pending = 15; this.ticks = 16; this.coreActive = 17;
        this.type = 122; this.energy = 0;
        this.capture = 77.25; this.loginEnd = 30.0;
        this.player.playerKnown = Number(values[4]);
        this.player.battleOwner = this.player.templateOwner = Number(values[5]);
        this.variant = Number(values[6]);
        this.mutate = Number(values[7]);
        this.editor = this.outpost = this.variant & 1;
        this.phase = [10, 16, 9][(this.variant >>> 1) % 3];
        this.loginEnd = (this.variant & 1) ? 0.0 : 30.0;
        this.cached.fill(0x55);
        this.core = this.players.subarray(180 + 0x6c, 180 + 0x6c + 10);
        this.sp = new ScrambledSpStore(new GameRandom(42), () => 9);
        this.sp.initialize();
        // 입력값만 명시한다: Player 마다 이름 "N<번호>", flags 0x200, 동맹 mask 자기 bit, 방향표 대각선 1, SP 900+번호.
        for (let owner = 0; owner < 9; owner += 1) {
            const name = `N${owner}`;
            this.players.set(Array.from(name, (char) => char.charCodeAt(0)), owner * 180 + 12);
            put(this.players, owner * 180 + 0x50, 0x200);
            put(this.players, owner * 180 + 0x54, 1 << owner);
            this.allies[owner * 9 + owner] = 1;
            this.sp.set(owner, floatBits(900 + owner));
        }
        this.participation = new RawPlayerParticipation(this.players, this.allies, this.hidden, this.participationState, {
            clearChat: (flags) => { this.trace.push(`Clear,${flags}`); },
            addChat: (sub, name, flags) => { this.chat(sub, name, flags); },
            assertion: () => { assert.fail('참여 모듈의 미선택 진단'); },
        });
        this.participation.bindCurrentInputs({ localOwner: () => this.player.battleOwner, playerKnown: () => this.player.playerKnown, battle: () => this.battle, metamap: () => this.metamap, localTransport: () => this.simulconnect });
        // 모든 수신기와 실제 참여/Ready 송신/SP 가 공유하는 로그인 몸체이다: Ready 의 boss 서버 경로가 이 몸체의 sendReady 를 부른다. 로컬 송신 장치는 실제 observer 뒤 wire 를 기록한다.
        this.login = new RawMissionLogin(this.players, this.participation, this.loginGlobals(), {
            send: (target, bytes) => {
                observeMissionClientPacket(bytes[1], bytes, 1, (text) => { this.assert(text); });
                this.send(target, bytes.subarray(0, bytes[2] | (bytes[3] << 8)));
            },
            broadcast: (bytes) => { this.send(0, bytes); },
            assertion: (text) => { this.assert(text); },
        });
        this.packets = new RawMissionClientPackets(this.players, this.participation, this.sp, this.state(), this.hooks());
    }

    // 로그인 몸체에 같은 현재 저장소를 복사 없이 빌려 주는 전역 묶음이다(속성으로 현재값을 읽고 쓴다). (2026-11-11 WB04-E319)
    loginGlobals() {
        const scene = this;
        return {
            get server() { return scene.server; },
            get boss() { return scene.boss; },
            get simulconnect() { return scene.simulconnect; },
            get missionWorld() { return scene.mission; },
            get missionPost() { return scene.mission; },
            get metamap() { return scene.metamap; },
            player: scene.player,
            get mySubscriber() { return scene.subscriber; },
            fortCount: 0,
            lastLoginPlayer: 0,
            predictableCount: 0,
            get begin() { return scene.begin; }, set begin(value) { scene.begin = value; },
            get beginSeed() { return scene.seed; },
            get battleSeed() { return scene.battleSeed; },
            debugBurst: 0,
            get clockStamp() { return scene.clockStamp; }, set clockStamp(value) { scene.clockStamp = value; },
            get baseClock() { return scene.baseClock; },
            get captureEnd() { return scene.capture; },
            get currentCoreData() { return scene.core; }, set currentCoreData(value) { scene.core = value; },
            cachedCoreData: scene.cached,
            zoneAddress: scene.zone,
            battleOptions: scene.options,
        };
    }

    // 현재 원본 참조를 정확히 같은 수명으로 빌려주는 상태 묶음이다: 속성으로 현재값을 읽고 쓴다. (2026-11-11 WB04-E314)
    state() {
        const scene = this;
        return {
            get server() { return scene.server; },
            get boss() { return scene.boss; },
            get missionPost() { return scene.mission; },
            get savedGame() { return scene.saved; },
            get metamap() { return scene.metamap; },
            get inBattle() { return scene.battle; },
            get editor() { return scene.editor; },
            get outpost() { return scene.outpost; },
            player: scene.player,
            get beginSeed() { return scene.seed; }, set beginSeed(value) { scene.seed = value; },
            get battleSeed() { return scene.battleSeed; }, set battleSeed(value) { scene.battleSeed = value; },
            get revision() { return scene.revision; }, set revision(value) { scene.revision = value; },
            get optionValue() { return scene.option; }, set optionValue(value) { scene.option = value; },
            get begin() { return scene.begin; }, set begin(value) { scene.begin = value; },
            get clockStamp() { return scene.clockStamp; }, set clockStamp(value) { scene.clockStamp = value; },
            get baseClock() { return scene.baseClock; }, set baseClock(value) { scene.baseClock = value; },
            get phase() { return scene.phase; },
            get currentState() { return scene.current; }, set currentState(value) { scene.current = value; },
            get currentAux() { return scene.aux; }, set currentAux(value) { scene.aux = value; },
            get pendingState() { return scene.pending; }, set pendingState(value) { scene.pending = value; },
            get sourceTicks() { return scene.ticks; }, set sourceTicks(value) { scene.ticks = value; },
            get coreDataActive() { return scene.coreActive; }, set coreDataActive(value) { scene.coreActive = value; },
            get captureEnd() { return scene.capture; }, set captureEnd(value) { scene.capture = value; },
            get loginEnd() { return scene.loginEnd; },
            get currentCoreData() { return scene.core; }, set currentCoreData(value) { scene.core = value; },
            cachedCoreData: scene.cached,
            zoneAddress: scene.zone,
            battleOptions: scene.options,
            currentHandle: scene.handle,
            hidden: scene.hidden,
        };
    }

    // 외부 진단을 원본 좁은 문자열 바이트로 기록한다. (2026-11-11 WB04-E314)
    assert(text) {
        this.trace.push(`Assert,${textHex(text)}`);
    }

    // 채널 요청을 기록하고 변이 2 번 행에서는 현재 서버 모드를 한 번 바꾼다. (2026-11-11 WB04-E314)
    chat(sub, name, flags) {
        this.trace.push(`Chat,${sub},${textHex(name)},${flags}`);
        if (this.mutate === 2 && !this.mutated) {
            this.server = false;
            this.mutated = true;
        }
    }

    // 송신 packet 을 그대로 관찰하고 변이 4 번 행에서는 다음 타이머 조회를 바꾼다. (2026-11-11 WB04-E314)
    send(target, bytes) {
        this.trace.push(`Send,${target | 0},${hex(bytes)}`);
        if (this.mutate === 4) this.loginEnd = 0.0;
    }

    // 미복원 큰 소비자의 인자/호출 순서만 외부 입력으로 관찰한다. (2026-11-11 WB04-E314)
    hooks() {
        const scene = this;
        const trace = this.trace;
        return {
            assertion: (text) => { scene.assert(text); },
            configInt: (key) => { trace.push(`Config,${key}`); return scene.variant & 32 ? 1 : 0; },
            tell: (text) => { trace.push(`Tell,${textHex(text)}`); },
            runState: () => { trace.push(`State,${scene.current}`); },
            resetCamera: () => { trace.push('Camera'); },
            refreshSp: () => { trace.push('Refresh'); if (scene.mutate === 3) put(scene.packet, 4, 2); },
            wallSeconds: () => { trace.push('Wall'); if (scene.mutate === 1) scene.phase = 16; return 100.5; },
            clock: () => { trace.push('Clock'); return 7501; },
            outpostOptions: (sender) => { trace.push(`Outpost,${sender}`); },
            updateRange: () => { trace.push('Range'); },
            requiredEnergy: () => { trace.push('Energy'); return 1251; },
            setEnergyType: (value) => { trace.push(`Type,${scene.type}`); scene.energy = floatBits(value); },
            readOptionValue: () => { trace.push('Option'); return 17; },
            reconnectUi: () => { trace.push('ReconnectUI'); },
            openFortImage: (owner) => { trace.push(`ImageName,${owner}`); trace.push(`Open,${owner}`); },
            readFortImage: (owner, image) => { trace.push(`Image,${owner},${hex(image)}`); },
            buildFort: (owner) => { trace.push('Builder'); trace.push(`Attach,${owner}`); trace.push('Build'); },
            peerAddress: (owner) => { trace.push(`IP,${owner}`); return '127.0.0.1'; },
            addChat: (sub, name, flags) => { scene.chat(sub, name, flags); },
            removeChat: (sub, flags) => { trace.push(`Remove,${sub},${flags}`); },
            restoreCoreData: (owner) => { trace.push(`Restore,${owner}`); },
            playerUiBusy: () => { trace.push('Busy'); return (scene.variant & 64) !== 0; },
            saveReconnect: () => { trace.push('Save'); },
            playerNotice: (kind, owner, argument) => { trace.push(`Notice,${kind},${owner},${argument}`); },
            send: (target, bytes) => { scene.send(target, bytes); },
            sendReady: (target) => { scene.login.sendReady(target); },
        };
    }

    // 원본 현재 전역과 double 비트를 정해진 순서로 읽어 출력한다. (2026-11-11 WB04-E314)
    stateText() {
        const core = this.core;
        let identity = 0;
        if (core && core.length) identity = core === this.cached ? 0xfffffffe : core.byteOffset - this.players.byteOffset + 1;
        const doubleBits = (value) => {
            const view = new DataView(new ArrayBuffer(8));
            view.setFloat64(0, value, true);
            return view.getBigUint64(0, true);
        };
        const values = [this.player.battleOwner, this.player.templateOwner, this.player.playerKnown, this.seed, this.battleSeed, this.revision, this.option, this.begin, this.clockStamp, this.baseClock, this.phase,
            this.current, this.aux, this.pending, this.ticks, this.coreActive, this.server ? 1 : 0, identity, doubleBits(this.capture), doubleBits(this.loginEnd)];
        return values.map(String).join(',');
    }

    // 옵션/주소/cache/TYPE 쓰기의 전체 바이트를 직렬화한다. (2026-11-11 WB04-E314)
    storageText() {
        const raw = new Uint8Array(4);
        put(raw, 0, this.energy);
        return hex(this.zone) + hex(this.options) + hex(this.cached) + hex(raw);
    }

    // 같은 실제 분산 SP 저장소의 9 소유자 현재 비트를 읽는다. (2026-11-11 WB04-E314)
    spText() {
        const raw = new Uint8Array(36);
        for (let owner = 0; owner < 9; owner += 1) put(raw, owner * 4, this.sp.get(owner));
        return hex(raw);
    }

    // 방향별 81DWORD 를 원본 little endian 바이트로 직렬화한다. (2026-11-11 WB04-E314)
    alliesText() {
        const raw = new Uint8Array(324);
        for (let index = 0; index < 81; index += 1) put(raw, index * 4, this.allies[index]);
        return hex(raw);
    }

    // 실제 발생 순서의 전체 외부 효과 문자열이다. (2026-11-11 WB04-E314)
    traceText() {
        return this.trace.length ? this.trace.join(';') : '-';
    }
}

