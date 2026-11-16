// 비용 환불6ch/배치 취소2fh의 실제 wire와 예측 객체 삭제다(225553df, WB04-E602).
// 사용: writeConstructionMoneyPacket/RejectPacket으로 송신하고 receiveConstructionReject(packet, world, destroy)로 같은 Pool/Hash를 정리한다.
import { decodeCanonType } from './raw-canon-type.js';
import { cellArea } from './raw-construction-clear.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 패킷의 비정렬 필드를 같은 바이트 구간에 little endian으로 접근한다.
function view(bytes) { return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength); }

// 현재 signed 금액과 Player/source/kind의 하위 BYTE를 패딩 없는11BYTE에 쓴다.
export function writeConstructionMoneyPacket(notice) {
    const packet = new Uint8Array([1, 0x6c, 11, 0, notice.player, notice.source, notice.kind, 0, 0, 0, 0]);
    view(packet).setInt32(7, notice.amount, true); return packet;
}

// 좌표의 단정도 비트와 type/argument/direction의 하위 BYTE를 실제15BYTE에 쓴다.
export function writeConstructionRejectPacket(notice) {
    const packet = new Uint8Array(15); packet.set([1, 0x2f, 15, 0]); const raw = view(packet);
    raw.setFloat32(4, notice.x, true); raw.setFloat32(8, notice.y, true);
    packet[12] = notice.type; packet[13] = notice.argument; packet[14] = notice.direction; return packet;
}

// 같은 현재 TYPE/Canon의 모든 칸에서 matching abstract를flags8로 지운다. SID 영역/소유자/첫 후보 제한을 더하지 않는다.
// world는 {pool, hash, types, frames, patternTypes, noIslandType, diagnostics}이며 현재 값은 getter도 된다.
export function receiveConstructionReject(packet, world, destroy) {
    if (!(packet instanceof Uint8Array) || packet.length < 15 || world.pool.edition !== 'patch1078') throw new TypeError('배치 취소의 실제 PATCH15BYTE 계약');
    const type = packet[12]; if (type >= world.types.length || type >= world.frames.length) throw new RangeError('배치 취소 현재 TYPE 범위');
    const meta = world.frames[type]; const raw = view(packet);
    const decoder = decodeCanonType(meta.frames, meta.defaultFrame, world.patternTypes, { type, argument: packet[13], direction: packet[14], x: raw.getFloat32(4, true), y: raw.getFloat32(8, true), explicitFrame: false }, world.diagnostics);
    // 앞선 삭제 뒤 다음 칸은 같은 현재 TYPE 발자국을 다시 읽는다.
    for (; decoder.valid; decoder.advance()) {
        const record = world.types[type]; const finder = new RawSquidFinder(world.pool, world.hash, world.types);
        // Finder가 보존한 다음 후보를 계속 방문하고 후보마다 현재 noIslandType을 읽는다.
        for (let sid = finder.begin(cellArea(decoder.x, decoder.y, record.footX, record.footY)); sid; sid = finder.next()) {
            const candidate = world.pool.slot(sid);
            if ((candidate[40] & 1) && (candidate[10] === type || candidate[10] === world.noIslandType)) {
                if (typeof destroy !== 'function') throw new Error('배치 취소의 실제 삭제 가상 미연결'); destroy(sid, 8);
            }
        }
    }
}
