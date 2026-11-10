// 10.78 DataManager 연결 목록의 추가/제거 본문을 옮긴 모듈이다(cpppj `o/RawDataManagerList.cpp/.h`, 기준선 1f00a25a 와 같음). manager 의 head(+4)/current(+8)와 노드 next(+0x38)를 같이 쓴다.
//  - appendDataManagerStream(0044ce10): 빈 목록은 새 노드 조회나 압축 없이 head/current 두 필드만 쓴다. 비어 있지 않으면 next 를 따라 마지막 노드를 찾아 압축(compact)한 뒤 그 노드의 next 에 새 핸들을 쓰고 current 를 새 핸들로 쓴다.
//  - removeDataManagerStream(0044d0f0): 먼저 unlink 하고 같은 노드의 MemStream 소멸 → 노드 반납을 수행한다. 선행 노드가 없으면 진단(missingPredecessor)을 낸 뒤 같은 선행 노드를 다시 읽는다. current 는 해제 도중 외부 효과가 바꿔도 별도로 0 으로 만들지 않는다.
// hooks = { lifetime: { stream(핸들), releaseBuffer(핸들), releaseStream(핸들) }, compact(핸들), missingPredecessor() }. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-10 WB04-E274: 4-B DataManager 스트림 묶음 이식)
import { destroyMemStream } from './raw-data-manager-lifetime.js';

// 노드 next 필드의 위치(바이트)이다. (2026-11-10 WB04-E274)
const NEXT = 0x38;

// 비정렬 DWORD(리틀 엔디언)를 읽는다. (2026-11-10 WB04-E274)
function read(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 지정한 DWORD 네 바이트만 쓴다. (2026-11-10 WB04-E274)
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 핸들의 실제 60 바이트 노드를 매번 다시 조회한다(호스트 주소로 자르지 않음). (2026-11-10 WB04-E274)
function node(hooks, handle) {
    if (!handle || typeof hooks?.lifetime?.stream !== 'function') throw new Error('DataManager 목록의 실제 노드 조회 미연결');
    const bytes = hooks.lifetime.stream(handle);
    if (bytes.length < 60) throw new RangeError('DataManager 목록의 실제60BYTE 누락');
    return bytes;
}

// 원본이 보고한 뒤 계속하는 선행 링크 진단이다. 소비자가 실제 수리/중단 동작을 고른다. (2026-11-10 WB04-E274)
function missing(hooks) {
    if (typeof hooks?.missingPredecessor !== 'function') throw new Error('DataManager 목록의 선행 링크 진단 미연결');
    hooks.missingPredecessor();
}

// 0044ce10: 추가. 사용: appendDataManagerStream(manager, 새노드핸들, hooks). (2026-11-10 WB04-E274)
export function appendDataManagerStream(manager, stream, hooks) {
    if (manager.length < 12) throw new RangeError('DataManager 추가의 실제12BYTE 누락');
    let tail = read(manager, 4);
    if (tail) {
        // 현재 next 를 따라 마지막 노드를 찾는다.
        for (let next = read(node(hooks, tail), NEXT); next; next = read(node(hooks, tail), NEXT)) tail = next;
        if (typeof hooks?.compact !== 'function') throw new Error('DataManager 마지막 stream 압축 미연결');
        hooks.compact(tail);
        put(node(hooks, tail), NEXT, stream >>> 0);
    } else {
        put(manager, 4, stream >>> 0);
    }
    put(manager, 8, stream >>> 0);
}

// 0044d0f0: 제거. 목표가 head 이면 head 를 다음으로 옮기고, 아니면 직전 노드를 찾아 건너뛰게 한다(없으면 진단). stream 이 0 이 아니면 MemStream 소멸 → 노드 반납. 사용: removeDataManagerStream(manager, 핸들, hooks). (2026-11-10 WB04-E274)
export function removeDataManagerStream(manager, stream, hooks) {
    if (manager.length < 12) throw new RangeError('DataManager 제거의 실제12BYTE 누락');
    stream >>>= 0;
    let previous = read(manager, 4);
    if (stream === previous) {
        put(manager, 4, read(node(hooks, stream), NEXT));
    } else {
        if (!read(node(hooks, previous), NEXT)) {
            missing(hooks);
        } else {
            // 목표의 직전 노드를 유지하고 다음 링크가 0 이면 같은 선행 노드로 진단한다.
            for (;;) {
                const next = read(node(hooks, previous), NEXT);
                if (next === stream) break;
                previous = next;
                if (!read(node(hooks, previous), NEXT)) break;
            }
            if (!read(node(hooks, previous), NEXT)) missing(hooks);
        }
        const current = read(node(hooks, previous), NEXT);
        put(node(hooks, previous), NEXT, read(node(hooks, current), NEXT));
    }
    if (stream) {
        destroyMemStream(node(hooks, stream), hooks.lifetime.releaseBuffer);
        if (typeof hooks.lifetime.releaseStream !== 'function') throw new Error('DataManager 실제 노드 반납 미연결');
        hooks.lifetime.releaseStream(stream);
    }
}
