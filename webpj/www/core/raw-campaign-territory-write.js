// Session의 공용 WriteEditorTerritory(3489..3505)를 실제 Template9/동일 MemStream에 연결한다(225553df, WB04-E588).
// 이름과 달리 RawCampaignVortex도 사용하는 공유 쓰기다. Editor UI/저장 파일·회복 경로는 만들지 않는다.
import { writeFortTerritoryRecords } from './raw-fort-territory-write.js';
import { writeMemStream } from './raw-mem-stream.js';

// 같은 고정 Template 슬롯의 현재 manager를 매번 읽고, 반환 당시 실제 노드의 생성 수명을 동기 호출 동안 보존한다.
export function writeCampaignTerritory(storage, slot, records) {
    slot >>>= 0;
    const streams = [];
    // 실제 manager/Create/current/일반 writer만 공급한다. 경로별 실패 전에 끝난 효과는 그대로 둔다.
    const hooks = {
        manager: (owner) => { if (owner >= 9) throw new RangeError('Editor 지형 기록의 실제 Template 범위'); const bytes = storage.headers(); return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(owner * 20, true); },
        assertion: (line) => { throw new Error(`실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:3492 (Territory ${line})`); },
        create: (manager, name) => { checkManager(manager); storage.createSection(slot, name); },
        current: (manager) => { checkManager(manager); streams.push(storage.currentWriteStream(slot)); return streams.length; },
        write: (token, input, length) => {
            if (!token || token > streams.length) throw new RangeError('Editor 지형 기록의 실제 stream 토큰');
            storage.accessSection(streams[token - 1], (fields, memory) => { writeMemStream(fields, input ?? new Uint8Array(0), length, memory); });
        },
    };
    // Create 뒤 바뀐 manager도 같은 고정 this 슬롯에서 다시 읽는다. 이전 반환값을 되살리지 않는다.
    function checkManager(manager) { if (manager !== hooks.manager(slot)) throw new Error('Editor 지형 기록의 현재 manager 불일치'); }
    writeFortTerritoryRecords(slot, records, hooks);
}
