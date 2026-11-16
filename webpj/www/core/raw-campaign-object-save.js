// Session2590..2623·6009..6028의 현재 객체 가상/필드와 독립 청크 검색기를 연결한다(225553df, WB04-E630).
// 현재 typed 경로를 보존하며 Native 장기 FortSaveVirtual 대여는 설치 단계에 남긴다.
import { saveFortObject, saveFortChunk, fortSaveCoordinate } from './raw-fort-object-save.js';
import { makeCampaignFortContentWriter } from './raw-campaign-content-save.js';
import { saveFortContents } from './raw-fort-content-save.js';
import { supportsConstructedVtable } from './squid-constructors.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawCursorPosition } from './raw-cursor-position.js';
import { RawCarrierDropPoint } from './raw-carrier-drop-point.js';

// 실제 생성 범위 중 가상60이 항상0인 아홉 VT다. TYPE와 VT의 인위적인 쌍 일치는 요구하지 않는다.
const REJECTED_VTABLES = new Set([0x5070c0, 0x508cc8, 0x5092d0, 0x5096a0, 0x509758, 0x509810, 0x50a8e8, 0x50db78, 0x50ee08]);
// 같은 raw50BYTE의 현재 DWORD/float32를 읽는다.
function bits(raw, at) { return new DataView(raw.buffer, raw.byteOffset + at, 4).getUint32(0, true); }
// 캡처한 float32 원시 비트를 같은 숫자로 넓혀 실제 섬 조회에 전달한다.
function number(bits_) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits_, true); return view.getFloat32(0, true); }
// 선택된 같은 현재 SID/TYPE의 물리만 확인한다.
function rawAt(owner, sid) { if (!Number.isInteger(sid) || sid < 0 || sid >= owner.core.pool.capacity) throw new RangeError('실제 Fort 저장 SID 범위'); return owner.core.pool.slot(sid); }
// 앞선 가상 효과 뒤 현재 TYPE BYTE와 같은 레코드를 다시 읽는다.
function typeAt(owner, sid) { const kind = rawAt(owner, sid)[10]; if (kind >= owner.core.types.length) throw new RangeError('Fort 저장 현재 TYPE 범위'); return owner.core.types[kind]; }
// 실제 현재 전역과 가상 후단을 원문 소비 위치에서 읽는 공용 객체 writer다. stream은 생성 수명으로 캡처한다.
export function makeCampaignFortObjectWriter(owner, stream) {
    const o = owner; const c = o.core;
    return {
        global: (key) => {
            if (key === 'battle') return Number(c.ownerMode.battle); if (key === 'mission') return Number(c.bridgeState.mission); if (key === 'editor') return o.globals.editor;
            if (key === 'scope') return o.objectState.scope; if (key === 'version') return o.objectState.version; if (key === 'localOwner') return o.playerState.battleOwner;
            if (key === 'bridgeType') return o.objectState.bridgeType; if (key === 'daisType') return o.state.runeState.readDaisType(); if (key === 'battleMask') return o.objectState.battleSavedMask; if (key === 'ownerMask') return o.objectState.ownerSavedMask;
            throw new Error('Fort 저장 전역 분배 누락');
        },
        form: (sid) => rawAt(o, sid), flags1: (sid) => typeAt(o, sid).flags1, flags2: (sid) => typeAt(o, sid).flags2,
        robust: (sid) => { const table = bits(rawAt(o, sid), 0); if (!supportsConstructedVtable(table)) throw new Error('미복원 Fort 저장 가상 함수 표'); if (REJECTED_VTABLES.has(table)) return 0; if (!o.balloonBody) throw new Error('Fort 저장 벌룬 가상 미설치'); if (o.balloonBody.handles(sid)) return o.balloonBody.robust(sid); return !(typeAt(o, sid).flags1 & 0x20000000); },
        hasLevel: (sid) => { if (!o.level) throw new Error('Fort 저장 레벨 미설치'); return o.level.has(sid); }, level: (sid) => { if (typeof o.rewardCount !== 'function') throw new Error('Fort 저장 가상80 미설치'); return o.rewardCount(sid); },
        island: (x, y) => c.building.hasIsland(number(x), number(y)),
        write: (token, bytes_) => { if (token !== 1) throw new Error('객체 저장 stream 선택 불일치'); o.templateStorage.writeSection(stream, bytes_); },
        contents: (sid, token) => { if (token !== 1) throw new Error('객체 내용물 stream 선택 불일치'); saveFortContents(sid, token, makeCampaignFortContentWriter(o, stream)); },
        assertion: (reason) => { throw new Error('Fort 저장 원문 진단 장치 미연결:' + reason); },
    };
}
// 실제 원본 float32 경계 변환과 현재 Pool/해시/TYPE를 쓰는 청크별 독립 finder다.
export function writeCampaignFortObjectChunk(owner, stream, chunkX, chunkY) {
    const c = owner.core; const objects = makeCampaignFortObjectWriter(owner, stream);
    const hooks = {
        begin: (area) => { const finder = new RawSquidFinder(c.pool, c.hash, c.types); const lower = RawCursorPosition.truncatePoint([area.left, area.top]); const upper = RawCarrierDropPoint.snap([area.right, area.bottom]); let current = finder.begin({ left: fortSaveCoordinate(lower[0]), top: fortSaveCoordinate(lower[1]), right: fortSaveCoordinate(upper[0]), bottom: fortSaveCoordinate(upper[1]) }, 0); return { current: () => current, next: () => { current = finder.next(); } }; },
        poolLimit: () => c.pool.capacity, object: (token, sid, x, y) => saveFortObject(token, sid, x, y, objects),
        access: (token, operation) => { if (token !== 1) throw new Error('청크 저장 stream 선택 불일치'); owner.templateStorage.accessSection(stream, operation); },
    };
    saveFortChunk(1, chunkX, chunkY, hooks);
}
