// 일반 입력의 SetOwner 명령과 편집 대상의 현재 Player Core 선택을 옮긴 모듈이다(cpppj `client/ObjectOwnerCommands.cpp/.h`, 10.78 전용). UI 명령과 현재 월드의 가상 소유자 변경 사이의 경계이다.
// 사용:
//   dispatchObjectOwner(event, revisionBox, { editor(), missionPost(), selected(), owner(sid, 새 owner DWORD), follow(sid) }) → boolean
//     원문 4d817e..4d81cb: `kSetObjectOwnerCommand`(0x020000cc, 상위 플래그 제거 비교)가 아니면 false 이다. 뗌(release)은 소비하고 아무 하위도 요구하지 않는다.
//     편집 또는 미션 후처리 모드일 때만 현재 selected 를 한 번 읽어 보관하고(가상 74 뒤 현재 selected 로 바꾸지 않음) 비영이면 owner(보관 SID, event.y 의 DWORD)를 부른 뒤 **이때 뒤늦게 읽은** editor 가 참이면 follow(보관 SID)를 부른다.
//     선택 유무와 무관하게 모드 안이면 월드 재그리기 revision(0059a8b0, DWORD 감김)을 1 올린다. 미선택 하위는 요구하지 않고 선택된 누락은 앞선 효과를 보존한 채 거부한다.
//   followEditorObjectOwner(pool, sid, revisionBox, selectCore(owner, 1)) — 4d5290: 현재 unsigned owner BYTE(+34)를 읽어 0 이 아니면 SelectPlayerCoreData(owner, 1)와 revision +1 을 한다(owner 0 은 무동작). 0번/예약/free/dead 도 원문 물리 안이면 읽는다.
//   finishEditorPrimarySelection(control, { editor(), selected(), localOwner(), readOwner(sid), flags2(sid), assignOwner(sid, 로컬 owner), cycleBridge(sid), follow(sid) }) — 4da2e8..357: 일반 좌클릭의 Select 뒤 현재 선택과 소유자를 지연 조회한다.
//     Editor 가 꺼졌거나 선택이 0 이면 아무것도 읽지 않고, control 이면 owner 가 로컬과 다를 때 assignOwner 후 follow 로 끝내고 같으면 flags2 의 다리 비트(4)이면 cycleBridge 를 부른 뒤 follow 한다. owner 변경 뒤 Editor 를 재조회하지 않는다.
// revisionBox 는 월드 재그리기 DWORD(`{ value }`)이며 미니맵 요청이 아니다. 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// 원본 관측: `object-owner-command-x86.tsv` 256행과 `editor-primary-selection-x86.tsv` 256행을 tests/object-owner-commands.test.mjs 가 대조한다.
// (2026-11-15 WB04-E531: 4-D/6단계 입력 구획 ObjectOwnerCommands 이식)
import { InputCode } from './input-queue.js';

// 보호 542a24 와 SetOwner 스크립트가 공유하는 실제 명령 번호이다. (2026-11-15 WB04-E531)
export const SET_OBJECT_OWNER_COMMAND = 0x020000cc;
// 명령 사건 코드에서 상위 플래그를 뺀 하위 26비트 마스크, 슬롯의 소유자 BYTE 위치, 다리 genus 비트이다. (2026-11-15 WB04-E531)
const COMMAND_BITS = 0x03ffffff;
const OWNER_OFFSET = 34;
const BRIDGE_GENUS = 4;

// 선택된 하위만 요구하며 없으면 앞선 효과를 보존한 채 도달한 위치에서 거부한다. 미선택 효과는 요구하지 않는다. (2026-11-15 WB04-E531)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('객체 소유자 명령의 실제 하위 미연결');
    return hook;
}

// release 는 일반 입력의 앞선 거부를 반영하고 알 수 없는 명령만 caller 에 돌려준다. (2026-11-15 WB04-E531)
export function dispatchObjectOwner(event, revision, hooks) {
    if (((event.code & COMMAND_BITS) >>> 0) !== SET_OBJECT_OWNER_COMMAND) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    if (need(hooks.editor)() || need(hooks.missionPost)()) {
        // 원문 ESI 에 보관하므로 가상 74 뒤 현재 selected 로 바꾸지 않는다.
        const selected = need(hooks.selected)() >>> 0;
        if (selected) {
            need(hooks.owner)(selected, event.y >>> 0);
            if (need(hooks.editor)()) need(hooks.follow)(selected);
        }
        revision.value = (revision.value + 1) >>> 0;
    }
    return true;
}

// 0번/예약/free/dead 도 원문 물리 안이면 읽으며 유효한 플레이어 범위는 실제 선택 하위가 판정한다. (2026-11-15 WB04-E531)
export function followEditorObjectOwner(pool, sid, revision, selectCore) {
    if (pool?.edition !== 'patch1078' || !(sid >= 0 && sid < pool.capacity) || sid > 65535) throw new RangeError('편집 객체 owner 조회의10.78 물리 SID 범위');
    // 원문 4d529d 의 MOVZX 를 따르며 디컴파일의 char 표기만으로 부호 확장하지 않는다.
    const owner = pool.slot(sid)[OWNER_OFFSET];
    if (owner) {
        need(selectCore)(owner, 1);
        revision.value = (revision.value + 1) >>> 0;
    }
}

// 가상 74 가 Editor/선택을 바꿔도 최초 ESI 의 같은 SID 를 따라가며 일반 SetOwner 명령과 재조회 순서를 섞지 않는다. (2026-11-15 WB04-E531)
export function finishEditorPrimarySelection(control, hooks) {
    if (!need(hooks.editor)()) return;
    // 선택이 0 이면 owner/로컬 Player/TYPE 를 읽지 않는다.
    const selected = need(hooks.selected)() >>> 0;
    if (!selected) return;
    if (control) {
        // 원문 MOVZX 뒤 현재 로컬 DWORD 를 한 번 읽는다.
        const owner = need(hooks.readOwner)(selected);
        const local = need(hooks.localOwner)();
        if (owner !== local) {
            need(hooks.assignOwner)(selected, local);
            need(hooks.follow)(selected);
            return;
        }
        if ((need(hooks.flags2)(selected) & BRIDGE_GENUS) !== 0) need(hooks.cycleBridge)(selected);
    }
    need(hooks.follow)(selected);
}
