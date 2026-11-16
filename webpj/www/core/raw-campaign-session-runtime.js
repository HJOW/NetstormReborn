// Session1611..1649의 실제 World Runtime 훅을 같은 하위 모듈/현재 상태에 구성한다(225553df, WB04-E595).
// 사용: connectAssets 뒤 bind. 이 단계는 훅 값만 준비하며 World.BindRuntime/전체 Session 공개는1825의 부모가 맡는다.
// 뒤에서 생성할 모듈은 호출 시점에 조회한다. 미설치 몸체를 빈 성공이나 다른 주문으로 대체하지 않는다.
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { connectedCampaignBombTable } from './raw-campaign-session-particles.js';

// 현재 raw의 DWORD/float를 실제 바이트에서 읽는다. 반환 슬롯은 복사하지 않는다.
function dword(raw, offset) { return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0; }
// 원문 bit_cast<float>이며 정렬되지 않은 같은 슬롯의 현재 네 BYTE를 해석한다.
function float(raw, offset) { return new DataView(raw.buffer, raw.byteOffset + offset, 4).getFloat32(0, true); }
// 뒤 생성자가 설치할 같은 모듈을 선택 지점에서 요구한다.
function module(owner, key, message) { const value = owner[key]; if (!value) throw new Error(message); return value; }
// 원문 FillMissing의 최초 연결 위치를 가진 선택된 실패 장치다. 성공 기본값을 만들지 않는다.
function missing(line) { return () => { throw new Error(`실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:${line}`); }; }

// 같은 Impl 상태와 하위 포인터를 빌리는 훅 조립 단계다. 현재 World Runtime/Native 수명 자체를 소유하지 않는다.
export class RawCampaignSessionRuntime {
    // 기반 owner·반복/반납 관문과 원문 값형 훅 표다.
    #owner; #started = false; #ready = false; #disposed = false;
    hooks = null;

    // 생성 자체는 현재 상태/World/하위 효과를 실행하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 자산 뒤 실제 훅 표만 구성한다. InstallContents와 최종 BindRuntime은 후속 생성 구간이다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.assetModules?.ready || this.#started) throw new Error('Session Runtime 훅의 초기화 전/반복 결선');
        this.#started = true;
        const assets = { ...o.services.deletion }; if (!assets.selected) assets.selected = missing(1630);
        const other = assets.emit;
        const hooks = {
            vortexRegular: { ...o.assetModules.vortexRuntime },
            // 표적/피해는 실제 현재 몸체를 사용하며 입자 포인터는 선택 시점에 조회한다.
            attacks: {
                mayTarget: (sid, target) => {
                    // Center가 효과를 낸 뒤에도 C++ span처럼 같은 슬롯의 다음 필드를 읽는다.
                    this.#live(); const raw = c.pool.physical(sid); const center = o.gunTarget.center(sid);
                    return o.gunTarget.mayTarget({ x: float(raw, 14), y: float(raw, 18), centerX: center[0], centerY: center[1], type: raw[10], direction: o.gunMode.get(sid) | 0, owner: raw[34], frame: dword(raw, 36) | 0 }, target);
                },
                damage: o.hits.damageHooks({}).damage,
                beam: (source, target) => { this.#live(); module(o, 'particles', '아크의 실제 빔 몸체 설치 전 호출').addBeam(source, target, 'shortZap.wav'); },
                dissolve: (victim, first, second, orientation) => { this.#live(); module(o, 'particles', '애시드의 실제 분해 몸체 설치 전 호출').addDissolve(victim, first, second, orientation); },
            },
            // 경로 목적지는 RawPathPoint 값으로 변환하며 같은 Dude/사제 부모를 사용한다.
            dude: {
                movementType: (type) => { this.#live(); return o.priests.movementProcessType(type); },
                entrance: (target) => { this.#live(); const point = o.hits.dudes.residencePoint(target); return { x: point[0], y: point[1] }; },
                move: (parent, point, quickly) => { this.#live(); return o.hits.dudes.move(parent, [point.x, point.y], quickly); },
            },
            residence: { entrance: o.assetModules.residence.entrance, spawn: o.assetModules.residence.spawn, task: o.assetModules.residence.task },
            factoryRegular: { ...o.assetModules.factoryRuntime },
            // 각 의식 부모는 두 실제 포인터 관문 뒤 공통 현재 상태를 준비한다.
            ritualRegular: {
                progress: (sid) => { this.#live(); const body = this.#ritual('daisRegular', '실제 룬 진행 설치 전 호출'); this.prepareRitual(); body.progress(sid); },
                release: (sid) => { this.#live(); const body = this.#ritual('altarSacrifice', '실제 제단 해방 설치 전 호출'); this.prepareRitual(); body.releaseFromAltar(sid); },
                removeAnimation: (sid) => { this.#live(); const remove = o.ritualExplosion; if (typeof remove !== 'function') throw new Error('실제 제단 애니메이션 제거 미설치'); remove(sid); },
                escapeFrame: (sid, number) => { this.#live(); const body = this.#ritual('altarSacrifice', '실제 다이스 이탈 설치 전 호출'); this.prepareRitual(); body.escapeFrame(sid, number); },
                sacrificeAdvance: (sid) => { this.#live(); const body = this.#ritual('daisRegular', '실제 희생 진행 설치 전 호출'); this.prepareRitual(); return body.sacrificeAdvance(sid); },
                ritual: (sid) => { this.#live(); const body = this.#ritual('templeRitual', '실제 다이스 의식 설치 전 호출'); this.prepareRitual(); const priest = body.findStrongOwnedPriest(sid); body.ritual(sid, priest, 1); },
            },
            damageable: { hitPoints: o.decorationHit },
            squidPacket: o.services.formPacket,
            squidConnected: (target) => { target |= 0; this.#live(); return target > 0 && target < 9 && (o.players[target * 180 + 0x50] & 1) !== 0; },
            squidSend: (target, packet) => { target |= 0; this.#live(); if (target < 0 || target >= 9) throw new RangeError('현재 대상별 Squid 연결 범위'); const send = o.services.battlePeers[target]; if (send) send(packet); },
            formTransmit: o.services.dependencyTransmit || missing(1629),
            contentNotice: (parent, form, reason) => { this.#live(); module(o, 'contentNotice', '현재 내용물 통지 몸체 미설치').notice(parent, form, reason); },
            capturedContent: (parent, previous) => { this.#live(); module(o, 'priestTakenNotice', '현재 사제 포획 안내 몸체 미설치').notify(parent, previous); },
            gumpNotice: o.services.gumpNotice,
            assets: { ...assets, emit: (event) => { this.#live(); if (event.effect === SQUID_DESTROY_EFFECT.transmit) { o.world.transmitDeletion(event.sid, event.flags); return; } if (typeof other !== 'function') throw new Error('실제 세션의 다른 파생 삭제/선택 효과 미연결'); other(event); } },
            dependencyNotice: (watcher, watched, form) => {
                this.#live(); const raw = c.pool.slot(watcher);
                if (raw[10] === 110 && dword(raw, 0) === 0x50eb98) { o.world.destroy(watcher, 0); return; }
                c.processes.notifyDependency(watcher, watched, form);
            },
            // 주문은 같은 준비 상태/대상 예약을 빌리며 sender/connection을 읽지 않는 수신 부모를 사용한다.
            bombRegular: {
                notice: (parent, count, event) => { this.#live(); module(o, 'flare', '현재 주문 플레어 몸체 미설치').spawn(parent, count | 0, event); },
                targets: (parent, storage) => { this.#live(); this.prepareDevastation(); const result = { storage, capacity: storage.length | 0, count: 0 }; module(o, 'devastationTargets', '현재 주문 검색 몸체 미설치').find(parent, result); return result.count >>> 0; },
                receive: (sender, connection, packet) => { this.#live(); this.receiveDevastationEffect(packet); },
            },
        };
        this.hooks = o.worldHooks = hooks; this.#ready = true;
    }

    // 원문 의식 호출의 두 현재 포인터를 먼저 확인한다. 준비 효과는 그 다음이다.
    #ritual(key, message) { const o = this.#owner; if (!o.daisState || !o[key]) throw new Error(message); return o[key]; }

    // 같은 현재 Player flag9BYTE와 방향별 동맹81DWORD를 실제 Rune/Dais 상태에 순서대로 복사한다.
    prepareRitual() {
        this.#live(); const o = this.#owner; const dais = module(o, 'daisState', '현재 의식 상태 미설치');
        // Rune은 원문0..8의 Player+50 BYTE를 먼저 반영한다.
        for (let owner = 0; owner <= 8; owner += 1) o.state.runeState.playerFlags[owner] = o.players[owner * 180 + 0x50];
        // 같은 Dais의 signed 동맹 배열로 비트를 복사하며 Player 배열을 별칭으로 넘기지 않는다.
        for (let index = 0; index < o.alliances.length; index += 1) dais.alliances[index] = o.alliances[index] | 0;
    }

    // 독립 typed 주문은 각 실제 효과 앞에서 현재 권한/소유자와 동맹 값 사본을 공급한다.
    prepareDevastation() {
        this.#live(); const o = this.#owner; const s = o.state; const boss = s.assetState.readBoss() ? 1 : 0;
        s.devastationTargetState.boss = s.devastationBlastState.boss = s.devastationEffectState.boss = s.devastationRegularState.boss = boss;
        s.devastationRegularState.localOwner = o.playerState.battleOwner;
        s.devastationBlastState.alliances.set(o.alliances);
    }

    // 같은420bf0 앞단이다. free/dead 조기는 실제 수신 몸체가 처리하며 다른 살아 있는 VT를 주문으로 가장하지 않는다.
    receiveDevastationEffect(packet) {
        this.#live(); const o = this.#owner;
        if (packet.length < 6) throw new TypeError('Devastation 패킷 부모 WORD 부족');
        const parent = packet[4] | (packet[5] << 8); const raw = o.core.pool.slot(parent);
        if (!(raw[11] & 3) && !connectedCampaignBombTable(dword(raw, 0))) throw new Error('다른 주문 Session 연결은 미복원');
        this.prepareDevastation(); module(o, 'devastationEffect', '현재 주문 효과 수신 몸체 미설치').receive(packet);
    }

    // 연결이 완성된 현재 생성 구간만 실행한다. 반납 뒤 선택한 자기 closure는 수명 오류다.
    #live() { if (!this.#ready || this.#disposed) throw new Error('반납/미연결 Session Runtime 훅'); }
    // 큰 하위 모듈은 부모가 원문 순서로 반납한다. 이 표만 무효화하며 World Runtime을 등록/해제하지 않는다.
    dispose() { this.#disposed = true; }
    // 훅 구성의 정상 복귀이며 전체 Session/World Runtime 게시 완료가 아니다.
    get ready() { return this.#ready; }
}
