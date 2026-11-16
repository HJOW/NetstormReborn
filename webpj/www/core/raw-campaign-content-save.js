// Session2566..2584와5899..5901의 기술/객체 공용 내용물 writer를 같은 owner에 연결한다(225553df, WB04-E629).
// 현재 typed World의 실제 ContentForm Robust를 사용한다. Native 장기 가상 대여는 그 설치 단계에 남긴다.
import { RawContainedFinder } from './raw-contained-finder.js';
import { saveFortContents } from './raw-fort-content-save.js';

// 같은 실제50BYTE 슬롯의 접근만 허용한다. 원문 DWORD를 다른 SID로 잘라 성공시키지 않는다.
function physicalSid(owner, sid, name) { if (!Number.isInteger(sid) || sid < 0 || sid >= owner.core.pool.capacity) throw new RangeError(`실제 ${name} SID 범위`); return sid; }
// 선택 시점의 현재 TYPE 세 DWORD만 값으로 넘긴다. 값형 반환과 원래 레코드의 후속 변경을 구별한다.
function contentType(owner, kind) {
    if (!Number.isInteger(kind) || kind < 0 || kind >= owner.core.types.length) throw new RangeError('실제 Technology TYPE 범위');
    const type = owner.core.types[kind]; return { flags1: type.flags1 >>> 0, contentListFlags: type.contentListFlags >>> 0, containerListFlags: type.containerListFlags >>> 0 };
}
// caller가 현재 Session 수명을 보유하는 동기 writer다. stream은 생성 당시의 같은 manager/노드 참조로 캡처한다.
export function makeCampaignFortContentWriter(owner, stream) {
    return {
        begin: (selected) => { const finder = new RawContainedFinder(owner.core.pool, owner.contentFinder); let current = finder.begin(physicalSid(owner, selected, 'Technology 부모')); return { current: () => current, next: () => { current = finder.next(); } }; },
        poolLimit: () => owner.core.pool.capacity, robust: (candidate) => { if (!owner.contentForms) throw new Error('실제 Technology ContentForm 미설치'); return owner.contentForms.robust(physicalSid(owner, candidate, 'Technology 현재')); },
        form: (candidate) => owner.core.pool.slot(physicalSid(owner, candidate, 'Technology 현재')), type: (kind) => contentType(owner, kind),
        parentType: (selected) => contentType(owner, owner.core.pool.slot(physicalSid(owner, selected, '내용물 부모'))[10]),
        assertion: (reason) => { throw new Error('실제 Technology 원문 진단 장치 미연결:' + reason); },
        access: (token, operation) => { if (token !== 1) throw new Error('실제 Technology stream 선택 불일치'); owner.templateStorage.accessSection(stream, operation); },
    };
}
// 외곽 전체 부모가 이미 생성한 명시 stream을 재생성하지 않고 현재 기술 내용물만 기록한다.
export function writeCampaignFortTechnology(owner, parent, stream) { saveFortContents(parent, 1, makeCampaignFortContentWriter(owner, stream)); }
