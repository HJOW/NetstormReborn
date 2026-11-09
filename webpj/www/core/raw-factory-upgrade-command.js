// 원본 Factory 업그레이드 요청(00453bf0)과 연구 수신 부모(00453d70)의 58h mode 1 업그레이드 수신 접두를 옮긴 모듈이다(cpppj `o/RawFactoryUpgradeCommand.cpp/.h`, 10.78 전용).
//  - request(00453bf0): 원본 자체에는 비용/소유자/이미 진행 중인지 검사가 없다(그런 관문은 입력 부모에 있다). 실제 SharedRegular 예약이 성공하면 event 4(payload 20초)를 붙이고, 실패해도 나머지 요청은 계속한다.
//    그 뒤 **현재 시각 + 15초**(요청 완료 간격, 00508ed0)를 wire 의 완료 시각으로 쓴 58h kind 0/mode 1 의 16 BYTE 를 채널 9 로 송신한다. 대기 20초와 wire 완료 15초를 구분한다.
//  - receive(00453d70 의 업그레이드 분기): 부모가 free/dead 이면 무시, kind 0·mode 1 이 아니면 무시(연구 분기와 구별, sender/owner 일치를 추가하지 않음). 검사 모드에서는 Factory genus 가 아니면 진단 뒤 현재 genus 를 다시 읽는다.
//    비용(0041df60) → 소유자 SP 에서 비용만큼 차감(0041dff0) → 원본 예약 로그 → SharedRegular 예약이 성공하면 event 3(최소 1초, **unordered(NaN)는 1초로 바꾸지 않음**, x87 test ah,41) → 현재 boss 일 때만 원문 16 BYTE 방송.
// 실제 하위(SP·SharedRegular·송신·방송·로그·진단)는 선택한 위치에서만 요구한다(빈 성공 금지). event 3 진행/완료/예측 그림은 `RawFactoryUpgrade` 가 맡는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-10 WB04-E218: Factory 업그레이드 요청/수신 이식으로 추가)

import { FACTORY_RESEARCH_PACKET_BYTES, writeFactoryResearchCommand } from './raw-factory-research.js';

// 00453bf0 의 event 4 SharedRegular payload(20초), 00508ed0 의 요청 완료 간격(15초), 00501580 의 수신 최소 지연(1초)이다. (2026-11-10 WB04-E218)
const REQUEST_WAIT = Math.fround(20);
const UPGRADE_DURATION = 15;
const MINIMUM_WAIT = 1;
const TYPE_OFFSET = 10;
const OWNER_OFFSET = 34;
const FACTORY_GENUS = 0x4000;

// 패킷에서 선택된 필드의 실제 바이트만 물리 범위 검사 뒤 읽는다(BigInt: 8 BYTE 까지). (2026-11-10 WB04-E218)
function packetField(raw, at, width) {
    if (at > raw.length || raw.length - at < width) throw new RangeError('Factory 업그레이드 수신 필드 범위');
    let value = 0n;
    for (let index = 0; index < width; index += 1) value |= BigInt(raw[at + index]) << BigInt(index * 8);
    return value;
}

// 실제 선택된 하위만 요구하며 누락을 빈 성공으로 처리하지 않는다. (2026-11-10 WB04-E218)
function need(effect, message) {
    if (typeof effect !== 'function') throw new Error(message);
    return effect;
}

// double 비트 변환 도구이다. (2026-11-10 WB04-E218)
const doubleView = new DataView(new ArrayBuffer(8));

// Factory 업그레이드 요청/수신이다. pool 은 10.78 SidPool, types 는 TYPE 표(flags2), state = { boss(), checking(), now() } 현재 전역 getter(복사하지 않고 빌림),
// hooks = { cost(type) → 비용 단위, addSp(owner, delta), reserveShared() → boolean(원본 new(0x28) 성공/실패), addShared(sid, event, delay), send(channel, packet), broadcast(packet), log(kind, text), assertion(text) } 이다.
// 사용: new RawFactoryUpgradeCommand(pool, types, state, hooks).request(factory). (2026-11-10 WB04-E218)
export class RawFactoryUpgradeCommand {
    // 현재 Factory 의 상태/타입/소유자를 읽는 실제 풀, 같은 현재 TYPE 비용 분류, 현재 원본 전역, 선택된 실제 하위이다.
    #pool;
    #types;
    #state;
    #hooks;

    // 같은 PATCH 풀/TYPE 만 먼저 확인하며 생성 시 게임 효과나 장치 호출을 하지 않는다. (2026-11-10 WB04-E218)
    constructor(pool, types, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length === 0) throw new TypeError('Factory 업그레이드 같은 PATCH/TYPE 계약 누락');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 같은 현재 슬롯의 지정 BYTE 를 읽는다. (2026-11-10 WB04-E218)
    #read(factory, offset) {
        const raw = this.#pool.slot(factory);
        if (offset >= raw.length) throw new RangeError('Factory 업그레이드 현재 raw 범위');
        return raw[offset];
    }

    // 진단 효과 뒤 다른 TYPE 이 되었으면 그 현재 genus(flags2)로 다시 판정한다. (2026-11-10 WB04-E218)
    #genus(factory) {
        const type = this.#read(factory, TYPE_OFFSET);
        if (type >= this.#types.length) throw new RangeError('Factory 업그레이드 현재 TYPE 범위');
        return this.#types[type].flags2;
    }

    // 00453bf0: event 4/20초 예약 → 현재 now+15초의 58h kind 0/mode 1 송신이다. 예약 실패도 송신은 계속하고 예약/부착 효과 뒤의 현재 시각으로 메시지를 만든다. (2026-11-10 WB04-E218)
    request(factory) {
        const hooks = this.#hooks;
        if (need(hooks.reserveShared, 'Factory 업그레이드 SharedRegular 예약 미연결')()) need(hooks.addShared, 'Factory 업그레이드 SharedRegular 실제 부착 미연결')(factory, 4, REQUEST_WAIT);
        const packet = new Uint8Array(FACTORY_RESEARCH_PACKET_BYTES);
        writeFactoryResearchCommand(factory, 0, this.#state.now() + UPGRADE_DURATION, packet);
        packet[7] = 1;
        need(hooks.send, 'Factory 업그레이드 실제 요청 송신 미연결')(9, packet);
    }

    // 00453d70 의 업그레이드 분기: 비용 → event 3/최소 1초(unordered 보존) 예약 → 현재 boss 방송이다. 비용 차감과 방송은 예약 실패 때도 되돌리지 않고, 선택된 누락은 앞선 실제 효과를 보존한 채 그 위치에서 거부한다. (2026-11-10 WB04-E218)
    receive(packet) {
        const hooks = this.#hooks;
        const factory = Number(packetField(packet, 4, 2));
        if (this.#read(factory, 11) & 3) return;
        if (packetField(packet, 6, 1) !== 0n || packetField(packet, 7, 1) !== 1n) return;
        if (this.#state.checking() && !(this.#genus(factory) & FACTORY_GENUS)) need(hooks.assertion, 'Factory 업그레이드 genus 진단 미연결')('factory.getGenus() & gFACTORY');
        if (!(this.#genus(factory) & FACTORY_GENUS)) return;
        const cost = need(hooks.cost, 'Factory 업그레이드 현재 비용 조회 미연결')(this.#read(factory, TYPE_OFFSET));
        const delta = Math.fround(-cost);
        need(hooks.addSp, 'Factory 업그레이드 같은 실제 SP 가산 미연결')(this.#read(factory, OWNER_OFFSET), delta);
        need(hooks.log, 'Factory 업그레이드 원본 예약 로그 미연결')(4, 'new ftUPGRADE sharedRegPro\n');
        // 실제 x87 test ah,41 분기는 unordered 에서도 원래 차이를 선택하므로 NaN 을 1초로 바꾸지 않는다.
        if (need(hooks.reserveShared, 'Factory 업그레이드 수신 SharedRegular 예약 미연결')()) {
            doubleView.setBigUint64(0, packetField(packet, 8, 8), true);
            const difference = doubleView.getFloat64(0, true) - this.#state.now();
            const wait = Math.fround(difference < MINIMUM_WAIT ? MINIMUM_WAIT : difference);
            need(hooks.addShared, 'Factory 업그레이드 수신 SharedRegular 실제 부착 미연결')(factory, 3, wait);
        }
        if (this.#state.boss()) {
            if (packet.length < FACTORY_RESEARCH_PACKET_BYTES) throw new RangeError('Factory 업그레이드 원문 16바이트 방송 범위');
            need(hooks.broadcast, 'Factory 업그레이드 실제 원문 방송 미연결')(packet.slice(0, FACTORY_RESEARCH_PACKET_BYTES));
        }
    }

    // 실제 Session 조립의 동일 풀이다. (2026-11-10 WB04-E218)
    get pool() { return this.#pool; }
}
