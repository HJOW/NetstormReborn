// 생성자1815..1824의 송신 합성·낙하/다리 효과·같은 로딩 깊이 결선이다(225553df, WB04-E618).
// 사용: 전체 Regular 뒤 bind. Runtime 게시1825와 Native 장기 장치 설치/Load는 별도다.
import { DaisProcess } from './raw-dais-process-data.js';
import { ConstructionProcess } from './raw-construction-process.js';
import { writeConstructionProcessData } from './raw-construction-data.js';
import { BombEffectTimerProcess } from './raw-bomb-effect-timer.js';
import { finalizeBombTimerPacket } from './raw-bomb-timer-packet.js';
import { FlyerProcess } from './raw-flyer-process.js';
import { WindAviaryProcess } from './raw-wind-aviary-process.js';
import { RainAviaryProcess } from './raw-rain-aviary-process.js';
import { SunAviaryProcess } from './raw-sun-aviary-process.js';
import { BRIDGE_LIFECYCLE_EFFECT } from './bridge-lifecycle.js';
import { ShrapnelKind } from './raw-shrapnel-frame.js';

// 필요한 하위 함수는 선택한 위치에서만 요구한다. 빈 성공으로 다른 프로세스를 삼키지 않는다.
function call(rest, name, ...args) { if (typeof rest[name] !== 'function') throw new Error(`다른 Process의 실제 ${name} 미연결`); return rest[name](...args); }
// signed 길이 범위를 검사하고 복사 없이 같은 현재 버퍼 보기를 반환한다.
function packetView(bytes, length) { if (length < 0 || length > bytes.length) throw new RangeError('Process의 실제 작성/송신 버퍼 범위'); return bytes.subarray(0, length); }

// 여러 반환 transmission이 같은 마지막 writer 선택과 실제 패킷 저장소를 공유한다.
export class RawCampaignSessionTransmission {
    // 기반 owner와 한 번의 결선/부분 실패/반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // Impl의 실제 마지막 분배 표식이며 일반/전체 writer와 별도 compose 결과가 공유한다.
    writingDaisProcess = false; writingConstruction = false;
    writingBombTimer = false; writingFlyer = false; writingRainAviary = false; writingSunAviary = false; writingWindAviary = false;
    // 독립 typed의 예약과 Native 공통 버퍼 선택은 생성 순간에 한 번 정한다.
    bombTimerPacket; windAviaryPacket; flyerPacket; rainAviaryPacket; sunAviaryPacket;
    // 실제 World 훅에 게시한 같은 함수 값이다.
    hooks = null;

    // 독립 typed의 원문 예약 크기222/64를 유지하며 실제 프로세스/난수/슬롯을 만들지 않는다.
    constructor(owner) {
        this.#owner = owner;
        const common = this.#native() ? owner.world.storage.gameAux.squidTransmission.buffer : null; // Native만 장기 공통 물리를 빌린다.
        this.bombTimerPacket = common ?? new Uint8Array(222);
        this.windAviaryPacket = common ?? new Uint8Array(64); this.flyerPacket = common ?? new Uint8Array(64);
        this.rainAviaryPacket = common ?? new Uint8Array(64); this.sunAviaryPacket = common ?? new Uint8Array(64);
    }

    // 실제 typed 처리기 조립만 하고 World.BindRuntime을 앞당기지 않는다.
    bind() {
        const o = this.#owner; // 같은 Session의 현재 모듈과 훅 표다.
        if (this.#disposed || !o.regularModules?.ready || this.#started) throw new Error('Session 송신의 초기화 전/반복 결선');
        this.#started = true;
        this.hooks = o.worldHooks.transmit = this.transmission();
        o.worldHooks.walkerFall = (sid) => { this.#live(); return o.priests.fallWalker(sid); };
        o.worldHooks.bridgeEffect = (event) => {
            this.#live();
            if (event.effect === BRIDGE_LIFECYCLE_EFFECT.notifyRemoval) { o.particles.addShrapnel(event.sid, ShrapnelKind.Variant2); return; }
            if (event.effect === BRIDGE_LIFECYCLE_EFFECT.fallSound) {
                if (typeof o.services.sound.playOnceAt !== 'function') throw new Error('실제 다리 낙하 위치 소리 미연결');
                o.services.sound.playOnceAt(event.x, event.y, o.soundNames.lookup('bridgeFall.wav'), 0, 0); return;
            }
            throw new Error('다리 파편/소리 경계의 다른 수명 효과');
        };
        o.worldHooks.loadingDepth = (depth) => {
            this.#live(); o.objectState.loadingDepth = depth >>> 0; o.state.carrierState.loadingDepth = depth >>> 0;
            o.priests.setLoadingDepth(depth); o.assetBindings.setLoadingDepth(depth);
            if (o.daisState) o.runtimeModules.prepareRitual();
        };
        this.#ready = true;
    }

    // 현재 실제 장기 Native 장치가 설치되어 있을 때만 해당 송신 분기를 고른다.
    #native() { return this.#owner.world.storage.usesNativeTransmissionDevices?.() ?? false; }
    // C++의 현재 Gameplay GameAux getter이며 헤더/버퍼 사본을 만들지 않는다.
    #aux() { return this.#owner.gameplayGlobals.gameAux(); }
    // 반납한 Session의 남은 함수 값은 새 World에 재진입하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session 송신'); }

    // Dais→Aviary/Timer/비행→Construction→전투→자산→fallback 순서의 실제 합성이다.
    transmission(fallback = {}) {
        this.#live(); const o = this.#owner; // 전달할 formPacket 함수 값은 C++처럼 합성 순간에 캡처한다.
        return this.daisTransmission(this.aviaryTransmission(this.constructionTransmission(o.combat.transmission(o.services.formPacket, o.assetBindings.transmission(fallback)))));
    }

    // 6ah는 항상 현재 공통 버퍼/헤더에 직접 쓰며 Native일 때도 이 writer를 유지한다.
    daisTransmission(fallback) {
        const rest = { ...fallback }; // C++ 값 인수인 하위 함수 표의 사본이다.
        const write = (process, changed, force) => { const aux = this.#aux(); const header = aux.daisProcessHeader; return process.writePacket(aux.squidTransmission.buffer, changed, force, header[1], header[0]); }; // 현재 헤더 재조회다.
        return {
            // 마지막 파생 선택은 실제 쓰기 실패 전에도 갱신된다.
            writeData: (process, changed) => { this.#live(); this.writingDaisProcess = process instanceof DaisProcess; return this.writingDaisProcess ? write(process, changed, false) : call(rest, 'writeData', process, changed); },
            // force는 DWORD 전체 비영 판정이다.
            writeFullData: (process, force) => { this.#live(); this.writingDaisProcess = process instanceof DaisProcess; return this.writingDaisProcess ? write(process, false, (force >>> 0) !== 0) : call(rest, 'writeFullData', process, force); },
            // 같은 마지막 선택에 따라 현재 공통 버퍼 또는 하위 보기를 돌려준다.
            packet: (length) => { this.#live(); return this.writingDaisProcess ? packetView(this.#aux().squidTransmission.buffer, length) : call(rest, 'packet', length); },
            // 실제 장기 broadcast가 설치됐으면 그 장치를 요구하며 service로 우회하지 않는다.
            transmit: (length) => { this.#live(); if (!this.writingDaisProcess) return call(rest, 'transmit', length); if (this.#native()) { this.#owner.world.storage.broadcastProcessForm(length); return; } this.#owner.services.formPacket(packetView(this.#aux().squidTransmission.buffer, length)); },
        };
    }

    // 건설도 같은 현재 공통 버퍼/헤더와 실제 ConstructionProcess 필드를 사용한다.
    constructionTransmission(fallback) {
        const rest = { ...fallback }; // 원문 값형 fallback을 캡처한다.
        const write = (process, changed, force) => { const aux = this.#aux(); const header = aux.constructionHeader; return writeConstructionProcessData(process, aux.squidTransmission.buffer, changed, force, { protocol: header[1], message: header[0] }); }; // 두 원문 BYTE의 현재 값이다.
        return {
            // 건설 프로세스의 일반 선택/작성이다.
            writeData: (process, changed) => { this.#live(); this.writingConstruction = process instanceof ConstructionProcess; return this.writingConstruction ? write(process, changed, false) : call(rest, 'writeData', process, changed); },
            // 전체 작성도 같은 선택 표식을 사용한다.
            writeFullData: (process, force) => { this.#live(); this.writingConstruction = process instanceof ConstructionProcess; return this.writingConstruction ? write(process, false, (force >>> 0) !== 0) : call(rest, 'writeFullData', process, force); },
            // 현재 common buffer의 복사 없는 보기다.
            packet: (length) => { this.#live(); return this.writingConstruction ? packetView(this.#aux().squidTransmission.buffer, length) : call(rest, 'packet', length); },
            // 실제 현재 Native/typed 송신 분배다.
            transmit: (length) => { this.#live(); if (!this.writingConstruction) return call(rest, 'transmit', length); if (this.#native()) { this.#owner.world.storage.broadcastProcessForm(length); return; } this.#owner.services.formPacket(packetView(this.#aux().squidTransmission.buffer, length)); },
        };
    }

    // 이미 옮긴 다섯 Process의 writer/버퍼를 원문 우선순서로 선택한다. 전용 생산/행동을 새로 실행하지 않는다.
    aviaryTransmission(fallback) {
        const rest = { ...fallback }; // 현재 하위 함수 값의 사본이다.
        const select = (process) => { // 모든 원문 분배 표식은 각 write 전에 갱신한다.
            this.writingBombTimer = process instanceof BombEffectTimerProcess; this.writingFlyer = process instanceof FlyerProcess;
            this.writingRainAviary = process instanceof RainAviaryProcess; this.writingSunAviary = process instanceof SunAviaryProcess; this.writingWindAviary = process instanceof WindAviaryProcess;
        };
        const buffer = () => this.writingBombTimer ? this.bombTimerPacket : this.writingWindAviary ? this.windAviaryPacket : this.writingFlyer ? this.flyerPacket : this.writingRainAviary ? this.rainAviaryPacket : this.writingSunAviary ? this.sunAviaryPacket : null; // 원문 packet 선택 순서다.
        const write = (process, changed, force, full) => { // Native 인자는 DWORD force를 보존하며 typed는 bool로 해석한다.
            select(process); const bytes = buffer(); if (!bytes) return call(rest, full ? 'writeFullData' : 'writeData', process, full ? force : changed);
            if (this.#native()) return this.#owner.world.storage.serializeProcessForm(process, bytes, changed, force);
            if (this.writingBombTimer) return process.writePacket(bytes, 22);
            return full ? process.writePacket(bytes, false, (force >>> 0) !== 0) : process.writePacket(bytes, changed);
        };
        return {
            // 일반 writer는 parentChanged와 force0을 사용한다.
            writeData: (process, changed) => { this.#live(); return write(process, changed, 0, false); },
            // 전체 writer는 parentChanged=false를 사용한다.
            writeFullData: (process, force) => { this.#live(); return write(process, false, force, true); },
            // 같은 실제 개별 버퍼 또는 fallback 보기다.
            packet: (length) => { this.#live(); const bytes = buffer(); return bytes ? packetView(bytes, length) : call(rest, 'packet', length); },
            // 타이머 길이 확정은 현재 송신 대상이 있는 경우에만 먼저 수행한다.
            transmit: (length) => {
                this.#live(); const bytes = buffer(); if (this.#native() && bytes) { this.#owner.world.storage.broadcastProcessForm(length); return; }
                if (!bytes) return call(rest, 'transmit', length);
                packetView(bytes, length); const o = this.#owner; // 범위 검사는 대상 조회/수정에 앞선다.
                if (this.writingBombTimer) {
                    // 실제 활성 Player와 현재 peer를 각 순회에서 다시 읽고 첫 송신 대상에서만 길이를 확정한다.
                    for (let player = 1; player < 9; ++player) if ((o.players[player * 180 + 0x50] & 1) && o.services.battlePeers[player] && player !== o.playerState.battleOwner) { finalizeBombTimerPacket(bytes, true); break; }
                }
                o.services.formPacket(packetView(bytes, length));
            },
        };
    }

    // 실제 Host 반납 뒤 이전 세션 함수 값의 재진입을 차단한다.
    dispose() { this.#disposed = true; }
    // 이 생성자 구간의 정상 결선 상태이며 World 게시/Load는 뜻하지 않는다.
    get ready() { return this.#ready; }
}
