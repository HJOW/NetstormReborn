// Gump 생성/트리 검사가 공유하는160BYTE 필드 직렬화다. 기대 생성/정렬/등록 결과는 계산하지 않는다. (2026-11-14 WB04-E410)
import { GumpCreateNode, gumpNodeFloatBits } from '../www/ui/gump-create.js';
// 주소 순서의 실제 DWORD와 float32 필드다.
const DWORDS = ['vtable', 'flags', 'alignFlags', 'next', 'previous', 'parent', 'child', 'id', 'command', 'object', 'form', 'serial', 'depth', 'secondaryDepth'];
const FLOATS = ['scale', 'baseWidth', 'baseHeight', 'width', 'height', 'x', 'y'];
// 전체160BYTE 입력을 같은 기본 노드 필드로 읽는다. 원문 float 비트도 함께 공급한다.
export function decodeGumpNode(hex) {
    const raw = Uint8Array.from(Buffer.from(hex, 'hex')); if (raw.length !== 160) throw new RangeError('Gump 입력160BYTE'); const view = new DataView(raw.buffer); const node = new GumpCreateNode();
    // 기본 정수/float 필드의 위치만 해석한다.
    for (let i = 0; i < DWORDS.length; i += 1) node[DWORDS[i]] = view.getUint32(i * 4, true);
    for (let i = 0; i < FLOATS.length; i += 1) { node[FLOATS[i]] = view.getFloat32(56 + i * 4, true); node.floatBits[FLOATS[i]] = view.getUint32(56 + i * 4, true); }
    // 위치/속도와 글자 문맥은 DWORD 비트 그대로다.
    for (let i = 0; i < 6; i += 1) node.positioning[i] = view.getUint32(84 + i * 4, true);
    node.cached.x = view.getInt32(108, true); node.cached.y = view.getInt32(112, true); node.untouched = view.getUint32(116, true); node.textPointer = view.getUint32(120, true); node.alternateTextPointer = view.getUint32(124, true);
    for (let i = 0; i < 8; i += 1) node.textContext[i] = view.getUint32(128 + i * 4, true); return node;
}

// 현재 필드를 원래 offset에 쓰며 변경하지 않은 float의 NaN payload는 입력 비트를 유지한다.
export function encodeGumpNode(node) {
    const raw = new Uint8Array(160); const view = new DataView(raw.buffer);
    // 기본 정수/float를 현재 값으로 기록한다. 그대로인 float는 NaN 조용화도 하지 않는다.
    for (let i = 0; i < DWORDS.length; i += 1) view.setUint32(i * 4, node[DWORDS[i]], true);
    for (let i = 0; i < FLOATS.length; i += 1) view.setUint32(56 + i * 4, gumpNodeFloatBits(node, FLOATS[i]), true);
    // 뒤쪽 필드는 현재 저장소를 그대로 직렬화한다.
    for (let i = 0; i < 6; i += 1) view.setUint32(84 + i * 4, node.positioning[i], true);
    view.setInt32(108, node.cached.x, true); view.setInt32(112, node.cached.y, true); view.setUint32(116, node.untouched, true); view.setUint32(120, node.textPointer, true); view.setUint32(124, node.alternateTextPointer, true);
    for (let i = 0; i < 8; i += 1) view.setUint32(128 + i * 4, node.textContext[i], true); return Buffer.from(raw).toString('hex');
}
