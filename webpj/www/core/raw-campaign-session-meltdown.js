// 같은 Session의 Meltdown을 현재 World·피해·방송 장치에 연결한다(225553df).
import { RawMeltdown } from './raw-meltdown.js';

// 요청·패킷·효과가 같은 월드와 현재 권한을 빌리는 생성 구간이다.
export class RawCampaignSessionMeltdown {
    // Session 소유자와 한 번 설치·정상 복귀·반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 현재 역할과 실제 장치에 연결한 값형 훅이다.
    state = null; hooks = null;

    // 생성 자체는 효과를 실행하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 반납 뒤 기존 함수 값이 다른 월드에 재진입하지 못한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session Meltdown'); }

    // 파괴 주문 뒤 실제 검색·피해·시청각·방송을 한 번 설치한다.
    bind() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !o.devastationModules?.effectReady || this.#started || o.meltdown) throw new Error('Session Meltdown 설치 순서 오류');
        this.#started = true;
        const state = {
            boss: () => { this.#live(); return o.state.assetState.readBoss(); },
            localOwner: () => { this.#live(); return o.playerState.battleOwner; },
            alliances: o.alliances, request: { message: 0x76, protocol: 1, length: 6 }, effect: { message: 0x75, protocol: 1, length: 0 },
        };
        const hooks = {
            hasHitPoints: (sid) => { this.#live(); return o.hp.hasHitPoints(sid); },
            assertion: (line) => { throw new Error(`원본 Meltdown 권한 진단 ${line}`); },
            find: (x, y, radius, list) => { this.#live(); o.meltdown.find(x, y, radius, list); },
            send: (target, packet) => { this.#live(); if (typeof o.services.carrierRequest !== 'function') throw new Error('Meltdown 실제 로컬 요청 전송 미설치'); o.services.carrierRequest(target, packet); },
            broadcast: (packet) => {
                this.#live(); if (o.world.storage.usesNativeTransmissionDevices?.()) throw new Error('Native 장기 Meltdown 방송은 4D-8에서 연결');
                // 실제 연결이 있는 비로컬 참가자가 있을 때만 패킷 길이를 확정한다.
                for (let owner = 1; owner < 9; owner += 1) {
                    if ((o.players[owner * 180 + 0x50] & 1) && o.services.battlePeers[owner] && owner !== o.playerState.battleOwner) { RawMeltdown.finalizePacket(packet, true); break; }
                }
                const length = packet[2] | (packet[3] << 8); if (length > packet.length) throw new RangeError('Meltdown 실제 배포 범위');
                o.tables.broadcast.send(packet.subarray(0, length));
            },
            receive: (sender, connection, packet) => { this.#live(); o.meltdown.receiveEffect(packet); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            damage: (sid, amount, owner, parent) => { this.#live(); const damage = o.hits.damageHooks({}).damage; if (typeof damage !== 'function') throw new Error('Meltdown 실제 피해 미연결'); damage(sid, amount, owner & 255, parent); },
            ground: (x, y) => { this.#live(); o.hits.impact.ground(x, y); },
            edge: (sid, x, y) => { this.#live(); c.bridgeEvents.convertEnd(sid, x, y); },
            visual: (x, y, variant, interval) => { this.#live(); o.hits.impact.visual(x, y, variant, interval); },
            sound: (name) => { this.#live(); if (typeof o.services.sound.play !== 'function') throw new Error('Meltdown 실제 전역 음향 미설치'); o.services.sound.play(o.soundNames.lookup(name), 0, 0, 0, 0, 0); },
        };
        o.meltdown = new RawMeltdown(c.pool, c.hash, c.types, c.frames, state, hooks);
        this.state = state; this.hooks = hooks; this.#ready = true;
    }

    // World 반납 뒤 현재 Session의 효과 함수를 닫는다.
    dispose() { this.#disposed = true; }
    // 이 생성 구간의 정상 복귀이며 실제 Load는 별도다.
    get ready() { return this.#ready; }
}
