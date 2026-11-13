// 실제 선택용200×200 원색 버퍼의255 판정을 옮긴다(cpppj client/RawObjectHit.cpp/.h, 10.78 +b0=498000).
// 사용: new RawObjectHit(pool,types,shapes,{frame,plain,additional}).hit(sid,x,y,[cameraX,cameraY]). 화면 배율/모바일 여유를 추가하지 않는다.
// 첫 TYPE는 고정하고 가상 getter 뒤 raw·TYPE/SHP 필드의 현재 값을 읽으며 선택한 헤더/상대점은 보존한다. (2026-11-14 WB04-E408)
import { projectCameraCoordinates } from '../render/camera-view.js';

// little endian DWORD를 현재 물리 슬롯에서 읽는다.
const dword = (raw, at) => new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(at, true);
// 선택된 하위 효과만 요구한다. opaque/음수점은 그림 장치를 읽지 않는다.
function required(callback) { if (typeof callback !== 'function') throw new Error('픽셀 선택의 도달한 가상/출력 하위 미연결'); return callback; }

// 실제 pool/현재 TYPE/물리 SHP 헤더와 가상+78/plain/+a4를 빌린다.
export class RawObjectHit {
    // 단일 raw 저장소와188칸 표·가상 효과다.
    #pool; #types; #shapes; #hooks;
    // 다른 판본/TYPE/SHP 길이는 생성할 때 거부한다.
    constructor(pool, types, shapes, hooks = {}) {
        if (pool?.edition !== 'patch1078' || types?.length !== 188 || shapes?.length !== 188) throw new TypeError('픽셀 선택의10.78 TYPE/SHP 계약 오류');
        this.#pool = pool; this.#types = types; this.#shapes = shapes; this.#hooks = hooks;
    }
    // 원문은 -1이 아닐 때 +78을 다시 호출하며 음수점/opaque/200밖에서 도달 순서가 다르다.
    hit(sid, x, y, camera) {
        const raw = this.#pool.physical(sid); const type = raw[10]; const hooks = this.#hooks;
        if (type >= this.#types.length) throw new RangeError('픽셀 선택의 현재TYPE 범위');
        const first = required(hooks.frame)(sid) | 0; const frame = first === -1 ? dword(raw, 36) | 0 : hooks.frame(sid) | 0;
        const record = this.#types[type]; const shape = this.#shapes[type]; const limit = shape.frameCount * ((record.flags1 & 0x440000) ? 2 : 1);
        if (!shape.loaded || frame < 0 || frame >= limit || frame >= shape.frames.length || (record.maxHitPoints >>> 0) === 0xdddddddd || (record.maxHitPoints >>> 0) === 0xcdcdcdcd) throw new Error('픽셀 선택의 frameCheck 원본 진단');
        const header = { ...shape.frames[frame] }; const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); const point = projectCameraCoordinates(camera[0], camera[1], view.getFloat32(14, true), view.getFloat32(18, true));
        const rx = (x - (((point[0] - record.hotspotX) | 0) - header.hotspotX | 0)) | 0; const ry = (y - (((point[1] - record.hotspotY) | 0) - header.hotspotY | 0)) | 0;
        if (rx < 0 || ry < 0) return false;
        if (record.flags1 & 0x800000) return rx < header.width && ry < header.height;
        const pixels = new Uint8Array(200 * 200).fill(255);
        if (!(raw[40] & 2)) required(hooks.plain)(type, dword(raw, 36) | 0, header.hotspotX, header.hotspotY, pixels);
        else {
            const main = hooks.frame(sid) | 0; if (main !== -1) required(hooks.plain)(type, main, header.hotspotX, header.hotspotY, pixels);
            required(hooks.additional)(sid, header.hotspotX, header.hotspotY, pixels);
        }
        return rx < 200 && ry < 200 && pixels[ry * 200 + rx] !== 255;
    }
}
