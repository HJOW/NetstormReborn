// 경로 패킷(Path 데이터, 패킷 번호 0x30)의 송신 작성기·수신 복원기·길이 관찰자·Form 직렬화 분배기를 옮긴 모듈이다(cpppj `o/RawPathData.cpp/.h`, 10.78 전용).
// 패킷은 24바이트 머리(+0 플래그 하위 7비트·+1 번호·+2 WORD 길이·+4 Form·+6 부모·+11 helper WORD·+13 nodeCount WORD·+15/+16 시작 좌표 BYTE·+17 시작 시각 DWORD·+21 명령 꼬리 WORD·+23 꼬리 BYTE)와
// 방향/거리 또는 직접 좌표 payload 로 이루어진다. 공유 송신 버퍼의 반환 길이 밖 바이트와 쓰지 않는 머리 3바이트는 지우지 않는다.
//  - RawPathData.write(image, form, parent, parentGenus, header, buffer) (패치 004895b0 typed): 경로 이미지(100,080바이트 또는 RawPathRuntime)를 읽어 머리와 payload 를 쓴다. 꼭짓점이 없거나 5000 을 넘으면 거부,
//    부모 genus 0x120000 이면 이후 꼭짓점의 절삭 좌표 두 BYTE 를, 아니면 방향(하위 3비트)과 31칸 단위로 쪼갠 거리(홀수 방향은 한 축, 짝수 방향은 두 축 합) BYTE 를 쓴다. 반환은 `written + 24`.
//  - RawPathData.writeCurrent(runtime, form, parent, header, buffer, hooks) (Native 4895b0): typed 값 writer 의 스냅샷/자격 guard 없이 원문 순서로 쓴다. 매 꼭짓점 읽기마다 현재 nVert 를 다시 읽어 `i < nVert` 진단(정상 복귀하면 계속),
//    좌표는 signed64 절삭의 하위 DWORD(비유한/범위 밖은 0), 점/run 로그를 trace 훅으로 내고, 부모 genus 는 고정 머리 뒤 현재 패킷 +6 의 SID 로 한 번 조회하며, payload 주소와 반환은 현재 signed WORD 이다.
//  - RawPathData.read(runtime, packet, pool, types, hooks) (패치 00489a10): 패킷을 기존 경로 배열에 순서대로 덧붙이고(방향 run 은 방향이 바뀔 때 모서리를 이전/새 방향으로 두 번 추가, 끝에 마지막 점), 의식 끝점(스테이지 0..4)을 보정하고,
//    Prepare 를 실행한 뒤 첫 5105 애니메이션을 지운다. 경로 탐색/직선 대체는 하지 않는다.
//  - observePathData(packet, sending, assertion) (40a660): 송신일 때만 nodeCount 가 -1 이면 진단을 내고 WORD 길이를 `nodeCount + 24` 로 확정한다. 수신은 무동작이다.
//  - makePathFormTransmission(pool, types, header, buffer, transport, other, nativeWriter): 같은 풀의 RawPathProcess 만 위 writer 로 직렬화하고 다른 프로세스는 `other` 로 위임한다(없으면 거부).
//    반환은 호스트의 `setFormTransmit` 에 넘기는 `{ writeData(process, parentChanged), transmit(size), packet(size), writeFullData(process, force) }` 이다.
// 10.72(CD) 판본(WORD 길이 헤더·배열 600)은 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// 근거: cpppj `o/RawPathData.cpp/.h`(기준선 1f00a25a 뒤 `20261106-63`·`20261107-46`·`20261108-205 CP` 의 수신 복원·Native 본문·헤더 DWORD 길이 반영), 원본 관측 pathdata-x86.tsv 10.78 384행.
// (2026-11-13 WB04-E350: 6-P RawPathData 이식으로 추가)
import { PATH_LAYOUT } from './raw-path-runtime.js';
import { RawPathProcess } from './raw-path-process.js';

// 패킷 payload 가 시작하는 오프셋, 첫 도착 시각에 곱하는 송신 배율(ROM 상수), 수신 시각 복원 배율, 의식 시작점 보정 거리이다. (2026-11-13 WB04-E350)
export const PATH_PACKET_PAYLOAD = 24;
const TIME_SCALE = 8;
const READ_TIME_SCALE = 0.125;
const STAGE_SNAP_DISTANCE = 1.5;
// 경로 이미지의 시작 좌표 위치, 패치 전용 명령 꼬리(WORD)·꼬리 단계(BYTE) 위치, 꼭짓점 개수 위치이다. (2026-11-13 WB04-E350)
const POINT_AT = 0x14;
const COUNT_AT = 0x10;
const TAIL_AT = 0x186e4;
const STAGE_AT = 0x186e8;
// 8방향의 x/y 단위 이동(보호 PE 0052f83c/5c)이다. (2026-11-13 WB04-E350)
const DX = Object.freeze([0, 1, 1, 1, 0, -1, -1, -1]);
const DY = Object.freeze([-1, -1, 0, 1, 1, 1, 0, -1]);
// 진단이 보고하는 원문 파일 이름과 줄, 거리 한 조각의 최대 칸 수이다. (2026-11-13 WB04-E350)
const PATH_HEADER_FILE = 'c:\\ns\\o\\path.h';
const PATHPROCESS_FILE = '\\Ns\\O\\Pathprocess.cpp';
const PACKETS_FILE = '\\Ns\\O\\_p_packets.cpp';
const RUN_LIMIT = 31;
// 부모 genus 중 직접 좌표 payload 를 쓰는 비트이다. (2026-11-13 WB04-E350)
const DIRECT_GENUS = 0x120000;

// float 비트 변환용 공용 보기이다. (2026-11-13 WB04-E350)
const SCRATCH = new DataView(new ArrayBuffer(8));

// 새 패킷 머리 전역 값이다: 패킷 번호 0x30, 플래그 하위 7비트 1, 공통 헤더에 기록할 현재 DWORD 길이 0(df590 이 최소 4 를 비교한 뒤에만 낮은 WORD 를 쓴다). (2026-11-13 WB04-E350)
export function createPathDataHeader() {
    return { packetId: 0x30, packetFlags: 1, packetLength: 0 };
}

// 비정렬 raw 필드를 폭만큼 리틀엔디언 부호 없는 수로 읽는다. 범위 밖이면 호스트 오류로 거부한다(최대 4바이트). (2026-11-13 WB04-E350)
function readBits(bytes, offset, width = 4) {
    if (!Number.isInteger(offset) || offset < 0 || offset + width > bytes.length) throw new RangeError('Path WriteData 이미지 범위');
    let result = 0;
    // 필드 폭만 읽으며 주변 여백을 합치지 않는다.
    for (let index = 0; index < width; index += 1) result += bytes[offset + index] * 2 ** (8 * index);
    return result;
}

// 비정렬 float32 필드를 읽는다. (2026-11-13 WB04-E350)
function readFloat(bytes, offset) {
    SCRATCH.setUint32(0, readBits(bytes, offset), true);
    return SCRATCH.getFloat32(0, true);
}

// 비정렬 double 필드를 읽는다. (2026-11-13 WB04-E350)
function readDouble(bytes, offset) {
    if (!Number.isInteger(offset) || offset < 0 || offset + 8 > bytes.length) throw new RangeError('Path WriteData 이미지 범위');
    for (let index = 0; index < 8; index += 1) SCRATCH.setUint8(index, bytes[offset + index]);
    return SCRATCH.getFloat64(0, true);
}

// float 값의 비트 DWORD 이다. (2026-11-13 WB04-E350)
function floatBits(value) {
    SCRATCH.setFloat32(0, value, true);
    return SCRATCH.getUint32(0, true);
}

// 원본 _ftol 의 유효 signed32 입력 범위를 보존하고 정상 원본 범위 밖은 명시적으로 진단한다(typed writer). (2026-11-13 WB04-E350)
function truncate(value) {
    const result = Math.trunc(value);
    if (!Number.isFinite(result) || result < -0x80000000 || result > 0x7fffffff) throw new RangeError('Path WriteData _ftol 범위');
    return result;
}

// Native 4e49c0 의 masked signed64 절삭 뒤 낮은 DWORD 이다. 비유한/범위 밖 불확정값은 낮은 DWORD 0 이다. (2026-11-13 WB04-E350)
function nativeFtol(value) {
    const limit = 9223372036854775808;
    if (!Number.isFinite(value) || value < -limit || value >= limit) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// typed 좌표 읽기: 실제 float32 를 읽어 0 쪽 절삭한다. (2026-11-13 WB04-E350)
function coordinate(image, offset) {
    return truncate(readFloat(image, offset));
}

// 선택 훅이 없으면 실행 가능한 빈 효과로 대체하지 않고 거부한다. (2026-11-13 WB04-E350)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// 패킷 전체 길이 헤더와 경로 이미지 크기를 확인한다. (2026-11-13 WB04-E350)
function imageOf(source) {
    const image = source instanceof Uint8Array ? source : source?.bytes;
    if (!(image instanceof Uint8Array) || image.length !== PATH_LAYOUT.size) throw new RangeError('Path WriteData 판본/버퍼 크기');
    return image;
}

// 송신 버퍼에 폭만큼 하위 바이트를 쓰는 함수를 만든다. 범위 밖이면 거부하고 앞선 prefix 는 되돌리지 않는다. (2026-11-13 WB04-E350)
function writer(buffer, message) {
    return (offset, value, width = 1) => {
        if (!Number.isInteger(offset) || offset < 0 || offset + width > buffer.length) throw new RangeError(message);
        // 원본 BYTE/WORD/DWORD 의 하위 바이트만 저장한다.
        for (let index = 0; index < width; index += 1) buffer[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
    };
}

// 경로 패킷의 작성기·복원기이다. 판본이 10.78 하나라 상태가 없다. (2026-11-13 WB04-E350)
export class RawPathData {
    // 판본은 10.78 하나이며 모든 raw 필드 조회에 같은 위치를 쓴다.
    constructor() {
        Object.freeze(this);
    }

    // typed 작성기(004895b0)이다. image 는 100,080바이트 이미지 또는 RawPathRuntime 이고 header 는 { packetId, packetFlags, packetLength }, buffer 는 24바이트 이상의 공유 송신 버퍼이다.
    // 각 필드의 쓰기 순서와 두 payload 분기를 그대로 유지하며 image 는 바꾸지 않는다. 반환은 패킷 길이(`written + 24`)이다. (2026-11-13 WB04-E350)
    write(source, form, parent, parentGenus, header, buffer) {
        const image = imageOf(source);
        if (buffer.length < PATH_PACKET_PAYLOAD) throw new RangeError('Path WriteData 판본/버퍼 크기');
        const put = writer(buffer, 'Path WriteData 송신 버퍼 부족');
        // 실제 공통 헤더는 이 호출에서 fillLength0 을 받는다. 따라서 기존 여백/나머지 버퍼를 보존한다.
        put(1, header.packetId);
        put(0, header.packetFlags & 0x7f);
        put(2, Math.max(header.packetLength >>> 0, 4), 2);
        put(4, form, 2);
        put(6, parent, 2);
        const count = readBits(image, COUNT_AT) | 0;
        if (count < 1 || count > PATH_LAYOUT.capacity) throw new RangeError('Path WriteData i < nVert');
        let x = coordinate(image, POINT_AT);
        let y = coordinate(image, POINT_AT + 4);
        put(15, x >>> 0);
        put(16, y >>> 0);
        put(17, truncate(readDouble(image, PATH_LAYOUT.times) * TIME_SCALE) >>> 0, 4);
        put(11, readBits(image, PATH_LAYOUT.helper, 2), 2);
        put(21, readBits(image, TAIL_AT, 2), 2);
        put(23, readBits(image, TAIL_AT + 4, 1));
        put(13, 0, 2);
        let written = 0;
        // 원본 Path 는 부모 genus 0x120000 이면 방향/거리 대신 이후 꼭짓점의 절삭 좌표 두 BYTE 를 그대로 쓴다.
        for (let index = 1; index < count; index += 1) {
            const nextX = coordinate(image, POINT_AT + index * 8);
            const nextY = coordinate(image, POINT_AT + 4 + index * 8);
            if ((parentGenus & DIRECT_GENUS) !== 0) {
                put(PATH_PACKET_PAYLOAD + written, nextX >>> 0);
                written += 1;
                put(13, written, 2);
                put(PATH_PACKET_PAYLOAD + written, nextY >>> 0);
                written += 1;
                put(13, written, 2);
            } else if (x !== nextX || y !== nextY) {
                const direction = readBits(image, PATH_LAYOUT.directions + index * 4);
                let distance = Math.abs(x - nextX);
                if ((direction & 1) === 0) distance += Math.abs(y - nextY);
                if (distance > 0x7fffffff) throw new RangeError('Path WriteData 거리 범위');
                // 원본은 31칸씩 분할하며 동일한 하위 3비트 방향을 각 BYTE 에 붙인다.
                while (distance > 0) {
                    const step = Math.min(distance, RUN_LIMIT);
                    put(PATH_PACKET_PAYLOAD + written, (step << 3) | (direction & 7));
                    written += 1;
                    put(13, written, 2);
                    distance -= step;
                }
            }
            x = nextX;
            y = nextY;
        }
        if (written > 0x7fff) throw new RangeError('Path WriteData signed 길이 범위');
        return written + PATH_PACKET_PAYLOAD;
    }

    // Native 4895b0 의 현재 본문이다. hooks = { parentGenus(sid) → genus(고정 머리 뒤 현재 packet+6 의 부모), assertion(expression, file, line), trace(text) } 이며 도달하는 위치에서만 요구한다.
    // typed writer 의 스냅샷/자격 guard/예외 정책을 가져오지 않는다. (2026-11-13 WB04-E350)
    writeCurrent(runtime, form, parent, header, buffer, hooks) {
        const image = imageOf(runtime);
        const put = writer(buffer, 'NativePath writer의 현재 송신 예약 범위');
        // 각 진단/루프 조건마다 원문 현재 nVert 를 다시 읽는다.
        const count = () => readBits(image, COUNT_AT) | 0;
        // 정상 복귀 뒤 count 를 다시 자격 검사하지 않고 선택된 물리를 읽는다.
        const check = (index, line) => {
            if (index >= count()) need(hooks?.assertion, 'NativePath writer의 현재 진단 장치 미연결')('i < nVert', PATH_HEADER_FILE, line);
        };
        // 좌표는 signed64 절삭의 낮은 DWORD 를 signed 로 본다.
        const point = (index, vertical) => {
            const at = POINT_AT + index * 8 + (vertical ? 4 : 0);
            if (at < 0) throw new RangeError('NativePath writer의 현재 좌표 물리 범위');
            return nativeFtol(readFloat(image, at)) | 0;
        };
        // 진단/로그가 바꾼 현재 WORD 를 각 원문 위치에서 읽는다.
        const nodeWord = () => readBits(buffer, 13, 2);
        const signedWord = () => (nodeWord() << 16) >> 16;
        // 각 point 의 현재 로그 형식을 그대로 사용한다.
        const tracePoint = (index, px, py, direction) => {
            need(hooks?.trace, 'NativePath writer의 실제 로그 장치 미연결')(`[${index}] (${px},${py}) , ${direction | 0}\n`);
        };
        put(1, header.packetId);
        put(0, header.packetFlags & 0x7f);
        put(2, Math.max(header.packetLength >>> 0, 4), 2);
        put(4, form, 2);
        put(6, parent, 2);
        check(0, 0x1f);
        put(15, point(0, false) >>> 0);
        check(0, 0x1f);
        put(16, point(0, true) >>> 0);
        check(0, 0x24);
        put(17, nativeFtol(readDouble(image, PATH_LAYOUT.times) * TIME_SCALE), 4);
        put(11, readBits(image, PATH_LAYOUT.helper, 2), 2);
        put(21, readBits(image, TAIL_AT, 2), 2);
        put(23, readBits(image, STAGE_AT, 1));
        check(0, 0x1f);
        let x = point(0, false);
        check(0, 0x1f);
        let y = point(0, true);
        put(13, 0, 2);
        // 고정 prefix 뒤 현재 packet+6 에서 부모 genus 를 한 번 선택한다.
        const parentGenus = need(hooks?.parentGenus, 'NativePath writer의 현재 부모 genus 장치 미연결')(readBits(buffer, 6, 2)) >>> 0;
        if ((parentGenus & DIRECT_GENUS) === 0) {
            check(0, 0x29);
            tracePoint(0, x, y, readBits(image, PATH_LAYOUT.directions));
            let index = 1;
            if (index < count()) {
                check(index, 0x1f);
                // 원문은 최초 x 진단 뒤 각 y/방향 진단과 현재 count 루프 조건을 다시 실행한다.
                do {
                    const nextX = point(index, false);
                    check(index, 0x1f);
                    const nextY = point(index, true);
                    check(index, 0x29);
                    const direction = readBits(image, PATH_LAYOUT.directions + index * 4);
                    tracePoint(index, nextX, nextY, direction);
                    if (x !== nextX || y !== nextY) {
                        // 원문 SUB/ABS/ADD 의 DWORD 감김을 보존한다.
                        const distance = (from, to) => {
                            const delta = ((from >>> 0) - (to >>> 0)) >>> 0;
                            const sign = (delta & 0x80000000) !== 0 ? 0xffffffff : 0;
                            return (((delta ^ sign) >>> 0) - sign) >>> 0;
                        };
                        let bits = distance(x, nextX);
                        if ((direction & 1) === 0) bits = (bits + distance(y, nextY)) >>> 0;
                        let remaining = bits | 0;
                        // 양수 signed 거리만 31칸씩 쓰고 각 run 의 실제 로그 뒤 현재 WORD 를 다시 읽는다.
                        while (remaining > 0) {
                            const step = Math.min(remaining, RUN_LIMIT);
                            need(hooks?.trace, 'NativePath run의 실제 로그 장치 미연결')(`n[${signedWord()}] ${direction & 7} | ${step & 0x1f}\n`);
                            put(PATH_PACKET_PAYLOAD + signedWord(), (step << 3) | (direction & 7));
                            put(13, (nodeWord() + 1) & 0xffff, 2);
                            remaining = (remaining - step) | 0;
                        }
                    }
                    x = nextX;
                    y = nextY;
                    index += 1;
                } while (index < count());
            }
        } else {
            let index = 1;
            // 직접 좌표는 각 WORD 증가 뒤 진단/현재 좌표 읽기/store 를 수행한다.
            while (index < count()) {
                let before = signedWord();
                put(13, (nodeWord() + 1) & 0xffff, 2);
                check(index, 0x1f);
                put(PATH_PACKET_PAYLOAD + before, point(index, false) >>> 0);
                before = signedWord();
                put(13, (nodeWord() + 1) & 0xffff, 2);
                check(index, 0x1f);
                put(PATH_PACKET_PAYLOAD + before, point(index, true) >>> 0);
                index += 1;
            }
        }
        // 별도 written 스냅샷/INT16 상한 guard 대신 원문 현재 signed WORD 로 반환한다.
        return signedWord() + PATH_PACKET_PAYLOAD;
    }

    // 패치 00489a10 수신 복원이다. 기존 배열에 순서대로 덧붙이고 의식 끝점/Prepare/5105 삭제를 실행한다.
    // packet 은 24바이트 이상의 수신 바이트, pool 은 10.78 SidPool, types 는 188칸 TYPE 표(flags2), hooks = { stagePoint(sid, stage) → { x, y }, metadata(parent) → [speed, turningSpeed], now() → 게임 시각,
    // clearAnimation(parent), trace(text), assertion(expression, file, line) } 이며 도달하는 위치에서만 요구한다. 각 필드/꼭짓점의 실제 쓰기를 끝낸 뒤 진단하며 뒤 실패에서 앞선 상태를 되돌리지 않는다. (2026-11-13 WB04-E350)
    read(runtime, packet, pool, types, hooks) {
        if (!(runtime?.bytes instanceof Uint8Array) || runtime.bytes.length !== PATH_LAYOUT.size || pool?.edition !== 'patch1078' || types.length !== 188) throw new TypeError('Path 수신의 실제 PATCH 저장소/판본 불일치');
        if (packet.length < PATH_PACKET_PAYLOAD) throw new RangeError('Path 수신의 실제24BYTE 머리 부족');
        const image = runtime.bytes;
        const view = new DataView(image.buffer, image.byteOffset, image.byteLength);
        // 경로 이미지의 지정 위치에 DWORD(또는 8바이트 0)를 쓴다.
        const write = (offset, value, width = 4) => {
            if (!Number.isInteger(offset) || offset < 0 || offset + width > image.length) throw new RangeError('경로 raw 범위');
            if (width === 8) view.setBigUint64(offset, BigInt(value), true);
            else view.setUint32(offset, value >>> 0, true);
        };
        const parent = readBits(packet, 6, 2);
        if (parent >= pool.capacity) throw new RangeError('Path 수신의 부모 물리 범위');
        let x = packet[15];
        let y = packet[16];
        const startTime = ((readBits(packet, 17) | 0) * READ_TIME_SCALE);
        write(PATH_LAYOUT.finished, 0);
        write(PATH_LAYOUT.targetA, 0);
        write(PATH_LAYOUT.targetB, 0);
        write(PATH_LAYOUT.helper, readBits(packet, 11, 2));
        write(TAIL_AT, readBits(packet, 21, 2));
        write(STAGE_AT, packet[23]);
        // 고정 헤더 뒤 기존 payload 만 읽는다.
        const byte = (index) => {
            if (index >= packet.length - PATH_PACKET_PAYLOAD) throw new RangeError('Path 수신의 선택된 payload 부족');
            return packet[PATH_PACKET_PAYLOAD + index];
        };
        // 직접 좌표 루프는 원문처럼 조건마다 현재 WORD 를 재조회한다.
        const nodeCount = () => (readBits(packet, 13, 2) << 16) >> 16;
        // 원문의 현재 count 의 point/direction/time 쓰기 → count++ → 진단이다.
        const append = (point, direction) => {
            const index = runtime.count;
            if (index < 0 || index >= PATH_LAYOUT.capacity) throw new RangeError('Path 수신 append의 실제 배열 범위');
            write(POINT_AT + index * 8, floatBits(point.x));
            write(POINT_AT + 4 + index * 8, floatBits(point.y));
            write(PATH_LAYOUT.directions + index * 4, direction);
            write(PATH_LAYOUT.times + index * 8, 0, 8);
            write(COUNT_AT, index + 1);
            if (index + 1 >= PATH_LAYOUT.capacity) need(hooks?.assertion, 'Path 수신의 실제 선택된 장치 미연결')('nVert < MAX_PATH_LENGTH', PATH_HEADER_FILE, 0x16);
        };
        // Native printf 의 DWORD 비트를 signed %d 로 해석한다.
        const tracePoint = (index, px, py, direction) => {
            need(hooks?.trace, 'Path 수신의 실제 선택된 장치 미연결')(`[${index}] (${px | 0},${py | 0}) , ${direction | 0}\n`);
        };
        // 좌표 DWORD 감김 뒤 signed int → float 저장이다.
        const point = () => ({ x: Math.fround(x | 0), y: Math.fround(y | 0) });
        const type = pool.slot(parent)[10];
        if (type >= types.length) throw new RangeError('Path 수신의 현재 부모 TYPE 범위');
        let firstDirection = 0;
        if ((types[type].flags2 & DIRECT_GENUS) === 0) {
            const count = nodeCount();
            firstDirection = count > 0 ? byte(0) & 7 : 0;
            tracePoint(0, x, y, firstDirection);
            append(point(), firstDirection);
            let previous = firstDirection;
            let last = firstDirection;
            // 방향/거리 BYTE 를 순서대로 읽는다. 매 run 의 방향/거리 로그가 실제 append 보다 앞선다.
            for (let index = 0; index < count; index += 1) {
                const packed = byte(index);
                const length = packed >> 3;
                last = packed & 7;
                need(hooks?.trace, 'Path 수신의 실제 선택된 장치 미연결')(`n[${index}] ${last} | ${length}\n`);
                if (last !== previous) {
                    append(point(), previous);
                    tracePoint(runtime.count - 1, x, y, previous);
                    append(point(), last);
                    tracePoint(runtime.count - 1, x, y, last);
                }
                x = (x + Math.imul(DX[last], length)) >>> 0;
                y = (y + Math.imul(DY[last], length)) >>> 0;
                previous = last;
            }
            if (count !== 0) append(point(), last);
            tracePoint(runtime.count - 1, x, y, last);
        } else {
            append(point(), 0);
            if ((packet[13] & 1) !== 0) need(hooks?.assertion, 'Path 수신의 실제 선택된 장치 미연결')('(zwp->nodeCount & 1) == 0', PATHPROCESS_FILE, 0x1ca);
            // 짝수 좌표쌍을 순서대로 추가하며 방향/시간은 0 이다.
            for (let index = 0; index < nodeCount(); index += 2) append({ x: byte(index), y: byte(index + 1) }, 0);
        }
        const target = readBits(image, TAIL_AT);
        if (target && target < pool.capacity && (pool.slot(target)[11] & 1) === 0) {
            let stage = readBits(image, STAGE_AT) | 0;
            if (stage > 0 && stage < 5) {
                const corner = need(hooks?.stagePoint, 'Path 수신의 실제 선택된 장치 미연결')(target, stage - 1);
                const raw = pool.slot(parent);
                const differenceX = readFloat(raw, 14) - corner.x;
                const differenceY = readFloat(raw, 18) - corner.y;
                if (Math.sqrt(differenceX * differenceX + differenceY * differenceY) < STAGE_SNAP_DISTANCE) {
                    write(POINT_AT, floatBits(corner.x));
                    write(POINT_AT + 4, floatBits(corner.y));
                }
            }
            stage = readBits(image, STAGE_AT) | 0;
            if (stage >= 0 && stage < 5) {
                const corner = need(hooks?.stagePoint, 'Path 수신의 실제 선택된 장치 미연결')(target, stage);
                const at = runtime.count * 8 + 0xc;
                if (at < 0) throw new RangeError('Path 수신 마지막 점의 물리 범위');
                write(at, floatBits(corner.x));
                write(at + 4, floatBits(corner.y));
            }
        }
        const turning = need(hooks?.metadata, 'Path 수신의 실제 선택된 장치 미연결')(parent)[1];
        const speed = hooks.metadata(parent)[0];
        runtime.prepare(need(hooks?.now, 'Path 수신의 실제 선택된 장치 미연결')(), startTime, speed, turning, firstDirection);
        need(hooks?.clearAnimation, 'Path 수신의 실제 선택된 장치 미연결')(parent);
    }
}

// 40a660 의 송신 observer 이다. 수신 방향은 무동작이며 송신은 nodeCount 가 -1 이면 진단(정상 복귀 시 계속)을 낸 뒤 WORD 길이를 `nodeCount + 24` 의 하위 WORD 로 확정한다. packet 은 15바이트 이상이어야 한다.
// 사용: observePathData(buffer, 1, assertion). (2026-11-13 WB04-E350)
export function observePathData(packet, sending, assertion) {
    if (!sending) return;
    if (packet.length < 15) throw new RangeError('Path observer의 실제 nodeCount 부족');
    const count = (readBits(packet, 13, 2) << 16) >> 16;
    if (count === -1) need(assertion, 'Path observer의 실제 진단 미연결')('nodeCount != (short)-1', PACKETS_FILE, 0x716);
    const length = (((readBits(packet, 13, 2) << 16) >> 16) + PATH_PACKET_PAYLOAD) & 0xffff;
    packet[2] = length & 0xff;
    packet[3] = length >> 8;
}

// 같은 pool 을 쓰는 Host 의 flags 0 서버 부착이 실제 Path 패킷을 만들도록 ProcessForm 직렬화를 분배한다. 다른 프로세스는 other 로 위임하며 없으면 거부한다.
// header/buffer/types/pool/송신 효과는 Host 보다 오래 살아야 하며 transport(size)는 이미 쓴 같은 buffer 를 전송해야 한다. nativeWriter(process, buffer, parentChanged, force) → int32 가 있으면 Native 본문을 쓴다.
// 반환: { writeData(process, parentChanged), packet(size), writeFullData(process, force), transmit(size) } — 호스트 `setFormTransmit` 에 그대로 넘긴다. (2026-11-13 WB04-E350)
export function makePathFormTransmission(pool, types, header, buffer, transport, other = {}, nativeWriter = null) {
    if (pool?.edition !== 'patch1078' || types.length !== 188 || buffer.length < PATH_PACKET_PAYLOAD || typeof transport !== 'function') throw new TypeError('Path 서버 직렬화 연결 누락');
    // 반환된 writer 와 버퍼 조회가 마지막 실제 파생 선택을 공유한다.
    const selected = { path: false };
    const data = new RawPathData();
    const writeData = (process, parentChanged) => {
        selected.path = process instanceof RawPathProcess;
        if (selected.path) {
            // Native 는 동일 정확 본문에서 prefix 뒤 부모를 선택하고 typed 만 종전 자격 guard 를 쓴다.
            if (typeof nativeWriter === 'function') return nativeWriter(process, buffer, parentChanged, 0);
            if (process.pool !== pool || !process.form || !process.parent) throw new TypeError('Path 서버 직렬화 풀/부착 불일치');
            const type = pool.slot(process.parent)[10];
            return data.write(process.runtime, process.form, process.parent, types[type].flags2, header, buffer);
        }
        return need(other.writeData, '다른 프로세스의 실제 WriteData 미복원 경계')(process, parentChanged);
    };
    return {
        writeData,
        // 마지막 writer 가 작성한 같은 버퍼를 빌린다. 대상 지정 전송은 방송 콜백을 실행하지 않고 이 바이트만 복사한다.
        packet: (size) => {
            if (!selected.path) return need(other.packet, '다른 Process의 실제 작성 버퍼 미연결')(size);
            if (!Number.isInteger(size) || size < 0 || size > buffer.length) throw new RangeError('Path 실제 작성 버퍼 범위');
            return buffer.subarray(0, size);
        },
        // ProcessForm+54 의 (buffer, 부모 변경 0, force 인자)이다. Path writer 는 force 로 필드를 선별하지 않고 현재 전체 경로를 작성한다.
        writeFullData: (process, force) => {
            if (process instanceof RawPathProcess) {
                if (typeof nativeWriter === 'function') {
                    selected.path = true;
                    return nativeWriter(process, buffer, false, force);
                }
                return writeData(process, false);
            }
            // 파생 fallback 뒤 버퍼 조회가 앞선 Path 버퍼를 잘못 선택하지 않게 같은 writer 선택을 갱신한다.
            selected.path = false;
            return need(other.writeFullData, '다른 Process의 실제 전체 writer 미연결')(process, force);
        },
        // 버퍼 조회와 같은 직전 writer 선택을 송신에서도 사용한다.
        transmit: (size) => {
            if (selected.path) transport(size);
            else need(other.transmit, '다른 Process의 실제 송신 장치 미연결')(size);
        },
    };
}
