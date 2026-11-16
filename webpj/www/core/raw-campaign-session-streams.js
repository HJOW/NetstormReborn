// Session의 TypeNames·Objects·Contents 원시 reader를 같은 Template/변환표/월드에 연결한다(225553df, WB04-E621).
// 사용: 실제 owner.prepareFortTypeNames(slot), owner.readFortObjectStreams(objects, slot, start, end, sourceStorage).
// 파일을 다시 파싱하거나 완성 레코드 사본을 만들지 않고 반환 당시 같은 stream의 동기 접근만 빌린다.
import { CampaignTemplateStorage } from './campaign-template-storage.js';
import { loadFortTypeNames, unwindFortTypeNamesLoad } from './raw-fort-type-names-load.js';
import { loadFortContents } from './raw-fort-contents-load.js';
import { loadStream } from './raw-fort-object-stream.js';

// 현재 manager DWORD를 읽는다. 헤더 슬롯은 호출한 각 부모의 원문 범위로 검증한다.
function manager(storage, slot) {
    if (slot < 0 || slot >= 9) throw new RangeError('저장 reader의 실제 Template 범위');
    const headers = storage.headers(); return new DataView(headers.buffer, headers.byteOffset, headers.byteLength).getUint32(slot * 20, true);
}
// 같은 상위가 빌린 Template owner를 고른다. 지역 Template도 전역 현재 manager를 교체하지 않는다.
function source(owner, slot, storage) {
    if (slot >= 9 || !owner.fortTypeConversion) throw new Error('저장 reader의 실제 Template/변환표 소유자 미연결');
    const selected = storage ?? owner.templateStorage;
    if (!(selected instanceof CampaignTemplateStorage)) throw new TypeError('저장 reader의 실제 Template 저장소 필요');
    return selected;
}
// Select의 반환 당시 노드/생성 수명을 호출별 작은 토큰 표로 보유한다. current가 바뀌어도 같은 반환 노드를 빌린다.
function streams(storage, slot) {
    const selected = [];
    return {
        manager: (owner) => manager(storage, owner),
        select: (current, name, writable) => {
            if (current !== manager(storage, slot)) throw new Error('저장 reader의 현재 manager 불일치');
            selected.push(storage.selectSection(slot, name, writable)); return selected.length;
        },
        access: (token, operation) => {
            if (!Number.isInteger(token) || token < 1 || token > selected.length) throw new RangeError('저장 reader 반환 stream 수명 범위');
            storage.accessSection(selected[token - 1], operation);
        },
    };
}
// 같은 실제 저장 깊이/현재 변환 DWORD를 읽고 쓴다. 변환표 할당 자체의 해제는 정상 Objects 꼬리가 맡는다.
function globals(owner) {
    return { get loadingDepth() { return owner.objectState.loadingDepth; }, set loadingDepth(value) { owner.objectState.loadingDepth = value; },
        get conversion() { return owner.fortTypeConversion.current().value; }, set conversion(value) { owner.fortTypeConversion.current().value = value; } };
}
// 원문 TYPE의 내부 narrow 이름과 NUL이다. 영어 내부 TYPE 이름을 표시용 번역문으로 치환하지 않는다.
function typeName(owner, type) {
    if (type < 0 || type >= owner.core.types.length) throw new RangeError('TypeNames 실제 TYPE 범위');
    const name = owner.core.types[type].name; const bytes = new Uint8Array(name.length + 1);
    // 현재 이름의 각 ANSI 바이트와 끝 NUL만 전달한다.
    for (let index = 0; index < name.length; index += 1) { const byte = name.charCodeAt(index); if (byte > 255) throw new RangeError('내부 TYPE narrow 이름 범위'); bytes[index] = byte; }
    return bytes;
}
// 같은 저장소의 TypeNames를 읽고 실패한 부모 깊이만 원문 unwind로 되돌린다.
export function prepareCampaignFortTypeNames(owner, slot, sourceStorage = null) {
    slot >>>= 0; const storage = source(owner, slot, sourceStorage); const hooks = streams(storage, slot); const current = globals(owner);
    Object.assign(hooks, {
        allocate: (bytes) => owner.fortTypeConversion.allocate(bytes), conversionBuffer: (token) => owner.fortTypeConversion.bytes(token),
        typeCount: () => owner.core.types.length | 0, typeName: (type) => typeName(owner, type),
        assertion: () => { throw new Error('실제 Session TypeNames 진단 미연결'); }, loadingDepthChanged: (depth) => owner.world.setLoadingDepth(depth),
    });
    try { loadFortTypeNames(slot | 0, current, hooks); } catch (error) { unwindFortTypeNamesLoad(current, hooks); throw error; }
}
// 원시 Contents reader의 실제 변환표/현재 TYPE·부모 TYPE·ContainedCreate를 같은 동기 access에 연결한다.
export function makeCampaignFortContentsReader(owner, access) {
    return {
        access, conversionBuffer: (token) => owner.fortTypeConversion.bytes(token),
        type: (kind) => { if (kind >= owner.core.types.length) throw new RangeError('Contents 현재 TYPE 물리 범위'); const record = owner.core.types[kind]; return { flags1: record.flags1, contentListFlags: record.contentListFlags }; },
        parentListFlags: (parent) => { checkParent(owner, parent); const kind = owner.core.pool.slot(parent)[10]; if (kind >= owner.core.types.length) throw new RangeError('Contents 부모 현재 TYPE 물리 범위'); return owner.core.types[kind].containerListFlags; },
        create: (parent, kind, quantity, listFlags, attachFlags, quality) => {
            checkParent(owner, parent);
            if (!owner.contained || owner.contained.pool !== owner.core.pool) throw new Error('Contents 실제 ContainedCreate 미연결/Pool 불일치');
            return owner.contained.create(parent, kind, quantity, listFlags, attachFlags, quality) >>> 0;
        },
        assertion: () => { throw new Error('실제 Session Contents 진단 미연결'); },
    };
}
// 원문 전체 DWORD의 부모 SID를 실제 Pool에 접근하는 위치에서 확인한다.
function checkParent(owner, parent) { if (parent > 65535 || parent >= owner.core.pool.capacity) throw new RangeError('Contents 부모의 실제 Pool 범위'); }
// 기본20 Terr와 Chaff를 실제 현재 stream에서 읽으며 앞선 객체 생성/커서/변환표 실패 접두를 유지한다.
export function readCampaignFortObjectStreams(owner, objects, slot, start = 0, end = 20, sourceStorage = null) {
    slot >>>= 0; const storage = source(owner, slot, sourceStorage); const hooks = streams(storage, slot);
    Object.assign(hooks, {
        prepareTypes: (selected) => prepareCampaignFortTypeNames(owner, selected >>> 0, storage),
        conversion: () => owner.fortTypeConversion.current().value, checking: () => owner.state.gunState.checking >>> 0,
        conversionBuffer: (token) => owner.fortTypeConversion.bytes(token),
        releaseConversion: () => { const current = owner.fortTypeConversion.current(); if (current.value) owner.fortTypeConversion.release(current.value); current.value = 0; },
        contents: (parent, stream) => { const reader = makeCampaignFortContentsReader(owner, hooks.access); loadFortContents(parent, stream, 9, 0, { get conversion() { return owner.fortTypeConversion.current().value; }, checking: hooks.checking }, reader); },
        assertion: (line) => { throw new Error(`원본 Objects Template.cpp 진단 ${line >>> 0}`); },
    });
    loadStream(objects, slot | 0, hooks, start | 0, end | 0);
}
