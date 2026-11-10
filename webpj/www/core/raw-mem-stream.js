// 10.78 MemStream(0044cc20 할당 · 0044cc80 대입 · 0044cce0 쓰기 · 0044cd40 압축)을 옮긴 모듈이다(cpppj `o/RawMemStream.cpp/.h`, 기준선 1f00a25a 와 같음).
// stream 은 원본 MemStream 의 실제 16 바이트이다: +0 버퍼 포인터(불투명 DWORD 핸들), +4 pos, +8 end, +12 alloc. 호스트 포인터로 해석하지 않고 hooks 로 핸들의 실제 버퍼를 빌린다.
//  - allocateMemStream: 이미 버퍼가 있으면 진단(ExistingData)을 낸 뒤 alloc 을 먼저 쓰고, 할당 → 포인터 쓰기 → 그 시점의 alloc 만큼 0 채움 → pos/end 를 0 으로 쓴다.
//  - copyMemStream: 기존 버퍼를 반납하고 첫 16 바이트를 비운 뒤 원문 end 길이로 할당하고, DWORD 단위로 읽은 뒤 쓰며(겹침 허용) 나머지 바이트를 앞에서부터 복사한다. pos 는 0, end 는 원문 end 이다.
//  - writeMemStream: signed 용량 부족이면 진단(WriteCapacity)을 낸 뒤에도 계속하며, 진단 뒤 stream 필드를 다시 읽어 pos 위치에 DWORD/나머지 바이트를 복사하고 pos/end 를 갱신한다.
//  - compactMemStream: end==alloc 이면 무동작, end 가 signed 양수이면 새 버퍼로 옮기고 옛 버퍼를 반납하며, 아니면 소멸(DestroyMemStream)한다.
// 일반 memcpy/memmove 로 바꾸지 않고 원본의 한 DWORD 읽기/쓰기 반복을 유지한다(겹치는 버퍼의 결과가 달라진다). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-10 WB04-E274: 4-B DataManager 스트림 묶음 이식)
import { destroyMemStream } from './raw-data-manager-lifetime.js';

// 진단 뒤에도 원본 본문이 계속하는 두 조건이다. (2026-11-10 WB04-E274)
export const MEM_STREAM_ASSERTION = Object.freeze({ existingData: 'ExistingData', writeCapacity: 'WriteCapacity' });

// 비정렬 DWORD(리틀 엔디언)를 읽는다. 인접 바이트에 영향을 주지 않는다. (2026-11-10 WB04-E274)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 지정한 DWORD 네 바이트만 쓴다. (2026-11-10 WB04-E274)
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 본문의 첫 쓰기 전에 실제 16 바이트를 확인한다. (2026-11-10 WB04-E274)
function physical(stream) {
    if (stream.length < 16) throw new RangeError('MemStream 실제16BYTE 누락');
}

// 진단 소비자가 없으면 빈 소비자를 자동으로 추가하지 않고 거부한다. (2026-11-10 WB04-E274)
function assertion(hooks, kind) {
    if (typeof hooks?.assertion !== 'function') throw new Error('MemStream 실제 진단 미연결');
    hooks.assertion(kind);
}

// 원본이 실제로 읽거나 쓰는 범위만 빌린다. 길이 0 은 버퍼 조회 없이 빈 뷰이다. (2026-11-10 WB04-E274)
function range(hooks, pointer, position, length) {
    if (!length) return new Uint8Array(0);
    if (!pointer || typeof hooks?.buffer !== 'function') throw new Error('MemStream 실제 버퍼 조회 미연결');
    const bytes = hooks.buffer(pointer);
    if (position > bytes.length || length > bytes.length - position) throw new RangeError('MemStream 실제 버퍼 물리 범위');
    return bytes.subarray(position, position + length);
}

// 원본 한 DWORD 읽기 뒤 쓰기를 반복하고 남은 1~3 바이트를 앞에서부터 복사한다(겹침 허용). (2026-11-10 WB04-E274)
function copyForward(target, source, length) {
    let position = 0;
    // DWORD 단위: 같은 네 바이트를 완전히 읽은 뒤 쓴다.
    for (; length - position >= 4; position += 4) put(target, position, read(source, position));
    // 나머지 바이트: 한 바이트씩 읽고 쓴다.
    for (; position < length; position += 1) target[position] = source[position];
}

// 0044cc20: 할당. allocator 가 alloc 을 바꾸면 그 현재 값만큼 0 으로 채운다. 사용: allocateMemStream(stream, size, { allocate, buffer, assertion }). (2026-11-10 WB04-E274)
export function allocateMemStream(stream, size, hooks) {
    physical(stream);
    if (read(stream, 0)) assertion(hooks, MEM_STREAM_ASSERTION.existingData);
    put(stream, 12, size >>> 0);
    if (typeof hooks?.allocate !== 'function') throw new Error('MemStream 실제 할당 미연결');
    const pointer = hooks.allocate(size >>> 0) >>> 0;
    put(stream, 0, pointer);
    const length = read(stream, 12);
    range(hooks, pointer, 0, length).fill(0);
    put(stream, 4, 0);
    put(stream, 8, 0);
}

// 0044cc80: 대입. free/할당 뒤의 원문 길이와 data 를 다시 읽으며 pos 는 0 으로 남긴다. source 는 원문 MemStream 의 실제 16 바이트이다. (2026-11-10 WB04-E274)
export function copyMemStream(stream, source, hooks) {
    physical(stream);
    if (source.length < 16) throw new RangeError('MemStream 대입의 실제 원문16BYTE 누락');
    const previous = read(stream, 0);
    if (previous) {
        if (typeof hooks?.release !== 'function') throw new Error('MemStream 대입의 실제 반납 미연결');
        hooks.release(previous);
    }
    put(stream, 0, 0);
    put(stream, 12, 0);
    put(stream, 8, 0);
    put(stream, 4, 0);
    allocateMemStream(stream, read(source, 8), hooks);
    const length = read(source, 8);
    const input = range(hooks, read(source, 0), 0, length);
    const output = range(hooks, read(stream, 0), 0, length);
    copyForward(output, input, length);
    put(stream, 8, read(source, 8));
}

// 0044cce0: 쓰기. 진단 뒤에도 계속하고 진단이 바꾼 stream 필드를 다시 읽어 대상 위치를 정한다. length 는 DWORD 이고 source 는 읽을 원문 뷰이다. (2026-11-10 WB04-E274)
export function writeMemStream(stream, source, length, hooks) {
    physical(stream);
    length >>>= 0;
    const sum = (read(stream, 4) + length) >>> 0;
    if ((read(stream, 12) | 0) < (sum | 0)) assertion(hooks, MEM_STREAM_ASSERTION.writeCapacity);
    if (length > source.length) throw new RangeError('MemStream 실제 원문 물리 범위');
    const target = range(hooks, read(stream, 0), read(stream, 4), length);
    copyForward(target, source, length);
    const current = (read(stream, 4) + length) >>> 0;
    put(stream, 4, current);
    if ((read(stream, 8) | 0) < (current | 0)) put(stream, 8, current);
}

// 0044cd40: 압축. end==alloc 은 무동작, signed 양의 end 는 재할당·복사·옛 버퍼 반납, 그 밖은 실제 소멸이다. 양의 재할당에서는 반납 뒤 필드 변이를 다시 0 으로 덮지 않는다. (2026-11-10 WB04-E274)
export function compactMemStream(stream, hooks) {
    physical(stream);
    const end = read(stream, 8);
    if (end === read(stream, 12)) return;
    const previous = read(stream, 0);
    if ((end | 0) > 0) {
        put(stream, 0, 0);
        allocateMemStream(stream, end, hooks);
        const length = read(stream, 12);
        const source = range(hooks, previous, 0, length);
        writeMemStream(stream, source, length, hooks);
        if (typeof hooks?.release !== 'function') throw new Error('MemStream 옛 버퍼 반납 미연결');
        hooks.release(previous);
    } else {
        destroyMemStream(stream, hooks?.release);
    }
}
