// 현재 Template SaveImage 와 로그인 sendFort 의 실제 0x2a 예약/필드/길이 확정을 옮긴 모듈이다(cpppj `o/RawMissionLoginPackets.cpp/.h`, 10.78: 004a7240 sendFort, 004df590 예약 헤더, 0040a540 길이 observer).
// 사용: const packets = new RawMissionLoginPackets(players, reads, imageHooks, hooks); packets.sendFort(owner, target, predictable);
//   players = Player9 1620BYTE(+0x58 의 watcher BYTE 를 사본 없이 읽음), reads = { current(owner) → 현재 Template 또는 null(미설치), initialLength() → 00544c48 의 현재 WORD }, imageHooks = `RawFortImage` 의 { typeCount, assertion } 현재 조회/진단,
//   hooks = { log(owner, target)(004a68e0 "sendFort from %d to %d\n"), send(target, bytes)(image+20 전체 mutable 예약을 현재 연결 gate 에 넘기고 장치가 40a540 뒤 image+17 선언만 복사하며 3BYTE 꼬리는 보존), missingTemplate()(004bf520 manager 누락 진단 뒤 현재 manager 를 다시 읽음) }.
// 원본 동적 stack 예약은 wire 보다 3바이트 크며 그 꼬리도 0xff 로 보존한다(최종 wire 는 imageSize + 17). 길이 observer 는 별도 원본 몸체(`finalizeMissionFortPacket`)가 송신 장치 gate 뒤에서 수행한다.
// Template 선택은 같은 현재 레코드를 각 원문 위치에서 다시 읽으며 VFS/파일 로더를 이 모듈이 대신하지 않는다.
// 기준선 뒤 cpppj 변경(ede61f05d, 3.6절 ②): 로그인 writer 의 실제 mutable 송신 예약(image+20 전체)을 반영했다. CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E317: 4-C RawMissionLoginPackets 이식)
import { FORT_IMAGE_CAPACITY, RawFortImage } from './raw-fort-image.js';

// 실제 Player 폭/개수, 이미지 시작 위치(0x11)와 예약 초과 폭(0x14)이다: 0x2a 의 두 다른 크기(예약 imageSize+20, wire imageSize+17). (2026-11-11 WB04-E317)
const STRIDE = 0xb4;
const OWNERS = 9;
const IMAGE_OFFSET = 0x11;
const RESERVATION_OVERHEAD = 0x14;

// 비정렬 DWORD/WORD 를 원본 little endian 필드에 쓴다. (2026-11-11 WB04-E317)
function write(packet, offset, value, size = 4) {
    // 지정 폭의 하위 BYTE 만 원문 순서로 기록한다.
    for (let byte = 0; byte < size; byte += 1) packet[offset + byte] = (value >>> (8 * byte)) & 0xff;
}

// 실제 packet 필드의 원본 폭을 현재 배열에서 읽는다. (2026-11-11 WB04-E317)
function read(packet, offset, size) {
    let value = 0;
    // 정렬 가정 없이 낮은 BYTE 부터 조립한다.
    for (let byte = 0; byte < size; byte += 1) value |= packet[offset + byte] << (8 * byte);
    return value >>> 0;
}

// 선택된 실제 소비자 누락은 해당 위치에서 거부한다. (2026-11-11 WB04-E317)
function need(device) {
    if (typeof device !== 'function') throw new Error('sendFort 실제 선택 소비자 미연결');
    return device;
}

// 길이 observer 뒤 실제 wire 범위만 반환하는 송신 예약이다(예약 tail 은 전송하지 않음). (2026-11-11 WB04-E317)
export class MissionFortPacket {
    // storage 는 imageSize + 20 바이트의 실제 예약이다. (2026-11-11 WB04-E317)
    constructor(storage) {
        this.storage = storage;
    }

    // 현재 길이 WORD 만 빌린다. 길이 확정 전/변조된 범위 밖 선언은 거부한다. (2026-11-11 WB04-E317)
    bytes() {
        if (this.storage.length < 4) throw new RangeError('sendFort packet 헤더 물리 범위 오류');
        const length = read(this.storage, 2, 2);
        if (length < 4 || length > this.storage.length) throw new RangeError('sendFort 선언 길이 물리 범위 오류');
        return this.storage.subarray(0, length);
    }
}

// 004df590/004a7240 의 실제 예약을 작성한다: 전체를 0xff 로 채우고 version 1·type 0x2a·초기 길이(최소 4)·owner·예측수·이미지 크기·watcher·이미지만 쓰며 끝 3BYTE 는 건드리지 않는다. 이미지가 고정 할당(0x8000)을 넘으면 거부한다. (2026-11-11 WB04-E317)
export function makeMissionFortPacket(owner, predictable, watcher, image, initialLength) {
    if (image.length > FORT_IMAGE_CAPACITY) throw new RangeError('sendFort 실제 이미지 할당 범위 초과');
    const storage = new Uint8Array(image.length + RESERVATION_OVERHEAD).fill(0xff);
    storage[0] = 1;
    storage[1] = 0x2a;
    write(storage, 2, Math.max(initialLength & 0xffff, 4), 2);
    write(storage, 4, owner >>> 0);
    write(storage, 8, predictable >>> 0);
    write(storage, 12, image.length);
    storage[16] = watcher & 0xff;
    storage.set(image, IMAGE_OFFSET);
    return new MissionFortPacket(storage);
}

// 0040a540 의 실제 bool 인자 / 0xffffffff 진단 / 하위 WORD 감김: writeLength 가 거짓이면 아무것도 읽지 않고, 참이면 이미지 크기 DWORD 가 -1 일 때 진단(복귀하면 계속)한 뒤 현재 하위 WORD + 0x11 을 선언 WORD 에 쓴다. 물리 packet 은 17바이트 이상이어야 한다. (2026-11-11 WB04-E317)
export function finalizeMissionFortPacket(packet, writeLength, assertion = null) {
    if (!writeLength) return;
    if (packet.length < IMAGE_OFFSET) throw new RangeError('sendFort 길이 observer 물리 범위 오류');
    const size = read(packet, 12, 4);
    if (size === 0xffffffff) {
        if (typeof assertion !== 'function') throw new Error('sendFort 실제 크기 진단 미연결');
        assertion();
    }
    // 진단이 정상 복귀한 뒤 현재 하위 WORD 를 다시 읽으며 앞서 검사한 DWORD 사본으로 대체하지 않는다.
    write(packet, 2, read(packet, 12, 2) + IMAGE_OFFSET, 2);
}

// 실효 image 와 현재 Player+0x58 watcher 를 작성해 보내는 로그인 sendFort 몸체이다. 메타데이터/다른 Player 패킷을 합성하지 않는다. (2026-11-11 WB04-E317)
export class RawMissionLoginPackets {
    // 사용: 같은 Player9 와 현재 Template/타입 개수/송신 수명은 이 객체보다 오래 살아야 한다. 현재 조회 없이 값형 기본 Template 을 삽입하지 않는다. (2026-11-11 WB04-E317)
    constructor(players, reads, imageHooks, hooks = {}) {
        if (!(players instanceof Uint8Array) || players.length !== STRIDE * OWNERS || typeof reads?.current !== 'function' || typeof reads.initialLength !== 'function' || typeof imageHooks?.typeCount !== 'function') {
            throw new TypeError('sendFort의 PATCH 현재 Player/Template/길이/타입 조회 누락');
        }
        this.players = players;
        this.reads = reads;
        this.imageHooks = imageHooks;
        this.hooks = hooks;
    }

    // 004a7240: Template 없음은 자연 false, 그 밖은 로그 → (manager 누락 진단 뒤 현재 manager 재조회) → 현재 image 작성 → 현재 길이 WORD → 헤더 작성 뒤 현재 watcher → 전체 예약 송신이다. 길이 확정은 실제 장치 gate 뒤 observer 가 수행한다. (2026-11-11 WB04-E317)
    sendFort(owner, target, predictable) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= OWNERS) throw new RangeError('sendFort 현재 Template owner 범위 오류');
        if (!this.reads.current(owner)) return false;
        need(this.hooks.log)(owner, target | 0);
        if (!this.reads.current(owner)) need(this.hooks.missingTemplate)();
        const current = this.reads.current(owner);
        if (!current) throw new Error('sendFort Save의 현재 Template manager NULL');
        const image = new RawFortImage('patch1078', current, this.imageHooks).save();
        const initialLength = this.reads.initialLength();
        // 원본은 길이 WORD 조회/헤더 작성 뒤 현재 watcher 를 읽는다.
        const watcher = this.players[owner * STRIDE + 0x58];
        const packet = makeMissionFortPacket(owner, predictable, watcher, image.bytes(), initialLength);
        // 원문 004a7240 은 길이를 미리 확정하지 않으며 실제 송신 장치가 gate 뒤 observer 와 현재 선언 범위를 사용한다.
        need(this.hooks.send)(target | 0, packet.storage);
        return true;
    }
}
