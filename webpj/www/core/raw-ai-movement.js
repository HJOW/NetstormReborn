// AI 의 실제 경로 조회/정지·대상/좌표 이동·자기 신전 복귀를 옮긴 모듈이다(cpppj `o/RawAiMovement.cpp/.h`, 10.78: 패치 00411630·00411680·004116d0·004117e0·004118a0·0041d840).
//  - aiDistanceToFootprint(point, bounds): 0041d840 — 축별 signed 거리이다. x 차이만 float 에 저장하고 y 는 배정도이며 unordered(NaN)에서는 x 를 선택한다(내부 점은 음수 거리를 허용한다).
//  - new RawAiMovement(memory, pool, boss, board, packet, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, boss() 는 00540bc4 의 현재 권한(원본 위치마다 다시 읽는다), board 는 `RawBoardTarget`(41d530 일반 접근점),
//    packet 은 사용자 이동과 같은 현재 헤더(`CarrierMoveState`)이다. hooks = { movement(type)→경로 TYPE(00488fe0), lookup(parent, type, index, remove)→SID(004b1a90), footprint(sid)→{left, top, right, bottom}(004ade70),
//    send(channel, bytes)(004e2dc0 채널 9, 18바이트), sacrifice(actor, target)(00494eb0), target(actor, target, 0, 0)→경로(004894e0), start(request)→경로(00489080, request = { parent, destination, replaceStart, helper, tailWord, tailByte, reserved }),
//    name(sid)→이름(가상 68), logTarget(token, source, target)·logPoint(token, name, x, y)(현재 AI/Move 로그 종류, 본체 +0xb00 이 켜진 때만), distance(point, sid)→거리(0041d840) } 이며 도달한 위치에서만 요구한다.
//  - movementForm(actor): 현재 TYPE 의 첫 현재 경로 Form 을 조회한다. stop(actor): actor 0 은 TYPE/목록에 접근하지 않는 무동작, 그 외는 같은 현재 TYPE/순번 0/remove 1 이다.
//  - toTarget(token, actor, target, sacrifice): 비서버는 대상 발자국의 일반 접근점을 계산해 현재 헤더의 18바이트를 채널 9 로 보내고(사용자 선택/중복 목적지 관문 없음), 서버이고 희생이면 의식만 시작하고 돌아간다. 송신 뒤 현재 권한이 서버이면
//    실제 대상 접근 경로가 없을 때 이름/로그 없이 끝낸다. 성공하면 대상 → 출발 순으로 이름을 읽고(디버그 여부 조회 전) +0xb00 이 켜져 있으면 로그를 낸다.
//  - toPoint(token, actor, point, sacrifice): 비서버는 송신, 현재 권한이 서버이면 인자 0 의 실제 경로 시작(실패하면 이름/로그도 생략) 뒤 좌표를 ftol(y 먼저, 무효는 0)하고 이름 → 로그.
//  - home(token, actor): 본체 +0x294 신전이 있으면 현재 위치/현재 신전 발자국 거리가 2 이상(또는 unordered)일 때 효과 뒤 다시 읽은 현재 신전으로 이동한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E167: AI 이동 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { writeCarrierMoveCommand } from './raw-carrier-move.js';

// 원본 복귀 거리 경계(PE 00501444, float 2.0), 이동 명령 길이, 송신 채널, 슬롯의 TYPE/좌표 위치이다. (2026-11-09 WB04-E167)
const HOME_DISTANCE = 2;
const PACKET_BYTES = 18;
const CHANNEL = 9;
const TYPE_AT = 10;
const X_AT = 14;
const Y_AT = 18;
const TWO_63 = 9223372036854775808;

// 비정렬 DWORD 를 범위 검사 뒤 낮은 BYTE 부터 읽는다. (2026-11-09 WB04-E167)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 이동 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E167)
function floatAt(raw, at) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(raw, at), true);
    return view.getFloat32(0, true);
}

// 선택된 실제 하위 누락을 가짜 경로 성공/빈 이름으로 대신하지 않는다. (2026-11-09 WB04-E167)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 이동 실제 하위 미연결');
    return hook;
}

// 원본 ftol 의 signed64 절삭 하위 DWORD(signed)이며 무효 입력은 0 이다. (2026-11-09 WB04-E167)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 0041d840: 원본은 내부 점에서 음수 거리를 허용하며 최종 unordered 도 x 차이를 돌려준다(x 차이만 float 에 저장). (2026-11-09 WB04-E167)
export function aiDistanceToFootprint(point, bounds) {
    const px = Math.fround(point.x);
    const py = Math.fround(point.y);
    const x = px < bounds.left ? Math.fround(bounds.left - px) : Math.fround(px - bounds.right);
    const y = py < bounds.top ? bounds.top - py : py - bounds.bottom;
    return y > x ? y : x;
}

// AI 이동 처리기이다. 사용자 이동의 선택 해제/동일 목적지 관문을 섞지 않고 AI 원문 흐름만 처리한다. (2026-11-09 WB04-E167)
export class RawAiMovement {
    #memory;
    #pool;
    #boss;
    #board;
    #packet;
    #hooks;

    // 같은 PATCH 풀/현재 권한/기하/헤더를 빌린다. 생성은 이동/전송/이름 조회 효과를 실행하지 않는다. (2026-11-09 WB04-E167)
    constructor(memory, pool, boss, board, packet, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 이동 본체 소유자 누락');
        if (pool?.edition !== 'patch1078') throw new TypeError('AI 이동 같은PATCH풀 누락');
        if (typeof boss !== 'function' || typeof board?.nearest !== 'function' || !packet?.request) throw new TypeError('AI 이동 현재 권한/기하/헤더 누락');
        this.#memory = memory;
        this.#pool = pool;
        this.#boss = boss;
        this.#board = board;
        this.#packet = packet;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E167)
    get pool() { return this.#pool; }

    // 가짜/해제 토큰과 다른 크기의 본체를 거부하고 현재 본체 DWORD 만 읽는다. (2026-11-09 WB04-E167)
    #field(token, offset) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 이동 실제0xb08본체 누락');
        return read(raw, offset);
    }

    // 위치 조회에 소유자/생존 필터를 추가하지 않고 현재 raw 의 두 float 좌표를 읽는다. (2026-11-09 WB04-E167)
    #position(actor) {
        const raw = this.#pool.slot(actor);
        return { x: floatAt(raw, X_AT), y: floatAt(raw, Y_AT) };
    }

    // 잘못된 DWORD 를 다른 SID 로 절단하지 않고 물리 검사 뒤 SID 로 좁힌다. (2026-11-09 WB04-E167)
    #physicalSid(value) {
        if (value >= this.#pool.capacity || value > 0xffff) throw new RangeError('AI 이동 현재목표SID 범위');
        return value;
    }

    // 00411630: 순번 0/remove 0 은 경로 시작의 다른 추가 인자가 아니다. (2026-11-09 WB04-E167)
    movementForm(actor) {
        const type = need(this.#hooks.movement)(this.#pool.slot(actor)[TYPE_AT]);
        return need(this.#hooks.lookup)(actor, type, 0, false);
    }

    // 00411680: actor 0 일 때는 TYPE 나 목록에 접근하지 않는다. (2026-11-09 WB04-E167)
    stop(actor) {
        if (!actor) return;
        const type = need(this.#hooks.movement)(this.#pool.slot(actor)[TYPE_AT]);
        need(this.#hooks.lookup)(actor, type, 0, true);
    }

    // 헤더는 실제 송신 직전 현재 공유값이며 사용자 선택/중복 목적지 검사를 추가하지 않는다. (2026-11-09 WB04-E167)
    #send(actor, point, sacrifice) {
        const packet = new Uint8Array(PACKET_BYTES);
        writeCarrierMoveCommand(this.#packet, actor, point, sacrifice, packet);
        need(this.#hooks.send)(CHANNEL, packet);
    }

    // 004116d0: 처음 서버였던 희생 분기만 즉시 의식으로 돌아가며 송신 뒤 권한 변경은 대상 접근 분기를 따른다. (2026-11-09 WB04-E167)
    toTarget(token, actor, target, sacrifice) {
        const hooks = this.#hooks;
        if (!this.#boss()) {
            const footprint = need(hooks.footprint)(target);
            const source = this.#position(actor);
            const point = this.#board.nearest(footprint, source, 1);
            this.#send(actor, point, sacrifice);
        } else if (sacrifice) {
            need(hooks.sacrifice)(actor, target);
            return;
        }
        if (this.#boss() && !need(hooks.target)(actor, target, 0, 0)) return;
        const targetName = need(hooks.name)(target);
        const sourceName = need(hooks.name)(actor);
        if (this.#field(token, 0xb00)) need(hooks.logTarget)(token, sourceName, targetName);
    }

    // 004117e0: 송신 뒤 현재 권한이 생겼으면 동일한 인자 0 의 실제 경로를 시작한다. 실패하면 이름/로그도 생략한다. (2026-11-09 WB04-E167)
    toPoint(token, actor, point, sacrifice) {
        const hooks = this.#hooks;
        if (!this.#boss()) this.#send(actor, point, sacrifice);
        if (this.#boss() && !need(hooks.start)({ parent: actor, destination: point, replaceStart: 0, helper: 0, tailWord: 0, tailByte: 0, reserved: 0 })) return;
        const y = ftol(point.y);
        const x = ftol(point.x);
        const name = need(hooks.name)(actor);
        if (this.#field(token, 0xb00)) need(hooks.logPoint)(token, name, x, y);
    }

    // 004118a0: 거리 판정 뒤 현재 신전 SID 를 다시 읽어 원본 이동에 전달한다(거리 2 이상 또는 unordered 이면 복귀). (2026-11-09 WB04-E167)
    home(token, actor) {
        const temple = this.#field(token, 0x294);
        if (!temple) return;
        const point = this.#position(actor);
        const distance = need(this.#hooks.distance)(point, this.#physicalSid(temple));
        if (!(distance < HOME_DISTANCE)) this.toTarget(token, actor, this.#physicalSid(this.#field(token, 0x294)), 0);
    }
}
