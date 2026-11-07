// 일반 MakeAi(대본 명령)의 기본 설정과 owner 0 이전/삭제 꼬리, 그리고 전체 객체 이전·삭제 순회를 옮긴 모듈이다(cpppj `o/RawMakeAi.cpp/.h`, 10.78 전용). 미션 시작 override 와 구별한다.
//  - makeAi(owner, settings, reads, hooks): 전투 중이 아니면 설정/저장소에 접근하지 않고 끝낸다. 600바이트 설정에 hooks.initialize(40ef80 기본값만, 공통/owner 미션 override 없음)를 적용한 뒤 현재 missionPost 가 0 이면 +0x220=0, +0x224=1.0f, +0x228=2.0f, +0x230=3, +0x234=6 으로 덮는다.
//    owner 가 0 이면 follower = 현재 로컬 owner + 1 에 이미 brain 이 있으면 끝, 아니면 transferObjects(follower) → create(follower, settings) → clearObjects(0x2000) 이다.
//    owner 가 0 이 아니면 playerKnown 을 읽고 +0x1e 를 0 으로 쓴 뒤(첫 기술 문자열 비움) 알려져 있으면 stormPower(로컬 owner)를, 아니면 0.0 을 _ftol(64비트 절삭의 하위 DWORD)해 +0x22c 에 쓰고 create(owner, settings)한다. 600바이트 미작성 부분은 caller 입력을 유지한다.
//  - transferMakeAiObjects(owner, hooks): 풀 한계를 진입 때 고정하고(free 만 건너뜀) 로컬 첫 region 도 진입 때 캡처한 뒤, 현재 소유자(로컬 또는 중립)·비묻힘(상태 bit 8 없음)·flags 가 있는 객체 중 다른 섬(region 이 다름)이고 플레이어/다리 TYPE 이 아닌 것을
//    로컬 소유였으면 두 카운터(local/built)를 줄이고 setOwner 로 이전한다. flags&0x200000 이면 깊이를 올려 한 번 더 setOwner 하고 내리며(예외 때 깊이 감소를 합성하지 않음), flags&0x200 이면 새 소유자의 첫 region 을 Player 행(+0x78)에 쓴다.
//  - clearMakeAiObjects(mask, hooks): 다음 후보를 flags 조회/가상 삭제보다 먼저 고르고(삭제가 슬롯을 바꿔도 이번 다음 번호 유지) flags&mask 이면 destroy(sid, 0)한다. 현재 풀 상한은 매번 다시 읽고 free 건너뛰기에는 진입 한계를 쓴다.
// 선택된 효과의 지연 검증이며 도달하지 않은 분기는 불필요한 장치를 요구하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E32: 일반 MakeAi 이식으로 추가)
import { MISSION_AI_SETTINGS_SIZE } from './raw-mission-ai-settings.js';

// 도달하지 않은 분기는 요구하지 않고 선택된 효과의 누락만 거부한다. (2026-11-08 WB04-E32)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('MakeAi 실제 현재 부모 미연결');
    return hook;
}

// 정렬 없는 원문 물리 읽기(폭 1 또는 4, 리틀엔디언)와 DWORD 쓰기이다. 범위 밖은 거부한다. (2026-11-08 WB04-E32)
function read(bytes, offset, width = 4) {
    if (offset > bytes.length || width > bytes.length - offset) throw new RangeError('MakeAi 읽기 물리 범위');
    let value = 0;
    // 실제 필드 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}
function put(bytes, offset, value) {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('MakeAi 쓰기 물리 범위');
    // 실제 DWORD 네 바이트만 기록한다.
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 실제 _ftol 의 signed 64비트 절삭 하위 DWORD 이며 indefinite 의 하위도 0 이다. (2026-11-08 WB04-E32)
function ftol(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 40eb40 은 먼저 증가하고 진입 당시 한계 안에서 free 만 건너뛴다(dead/void/buried 에 새 관문을 더하지 않는다). (2026-11-08 WB04-E32)
function next(index, limit, hooks) {
    let result = index + 1;
    while (result < limit && (read(need(hooks.slot)(result), 11, 1) & 1) !== 0) result += 1;
    return result;
}

// 본문의 현재 풀 상한은 iterator 고정 한계와 다르다. (2026-11-08 WB04-E32)
function valid(index, hooks) {
    return index >= 1 && index < need(hooks.capacity)();
}

// 같은 현재 슬롯의 좌표를 callback 뒤 재조회한다. (2026-11-08 WB04-E32)
function position(index, hooks) {
    const bytes = need(hooks.slot)(index);
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, read(bytes, 14), true);
    view.setUint32(4, read(bytes, 18), true);
    return [view.getFloat32(0, true), view.getFloat32(4, true)];
}

// reads = { battle, missionPost, playerKnown, localOwner, brain(owner) }, hooks = { initialize(설정), stormPower(owner), create(owner, 설정), transferObjects(owner), clearObjects(mask) } 이다. (2026-11-08 WB04-E32)
export function makeAi(owner, settings, reads, hooks) {
    if (!need(reads.battle)()) return;
    if (!settings || settings.length !== MISSION_AI_SETTINGS_SIZE) throw new TypeError('MakeAi 실제 600바이트 설정 필요');
    need(hooks.initialize)(settings);
    if (!need(reads.missionPost)()) {
        put(settings, 0x220, 0);
        put(settings, 0x224, 0x3f800000);
        put(settings, 0x228, 0x40000000);
        put(settings, 0x230, 3);
        put(settings, 0x234, 6);
    }
    if (!owner) {
        const follower = (need(reads.localOwner)() + 1) >>> 0;
        if (need(reads.brain)(follower)) return;
        need(hooks.transferObjects)(follower);
        need(hooks.create)(follower, settings);
        need(hooks.clearObjects)(0x2000);
        return;
    }
    const known = need(reads.playerKnown)();
    settings[0x1e] = 0;
    const storm = known ? need(hooks.stormPower)(need(reads.localOwner)()) : 0;
    put(settings, 0x22c, ftol(storm));
    need(hooks.create)(owner, settings);
}

// hooks = { capacity, localOwner, playerType, bridgeType, slot(index) → 현재 50바이트, flags(index), players() → 현재 1620바이트, region(x, y), setOwner(index, owner), destroy(index, 값),
//   decrementLocal(type), decrementBuilt(type), loadingDepth: { value }(5c89b8 의 중첩 깊이), loadingDepthChanged(깊이) } 이다. 로컬 첫 region 만 진입 때 캡처하고 owner/TYPE/flags/슬롯은 각 원문 위치에서 다시 읽는다. (2026-11-08 WB04-E32)
export function transferMakeAiObjects(owner, hooks) {
    const limit = need(hooks.capacity)();
    let index = next(1, limit, hooks);
    const localRegion = read(need(hooks.players)(), need(hooks.localOwner)() * 180 + 0x78);
    // 원문 pool 은 효과 뒤 재조회하며 고정 iterator 상한으로 free 만 건너뛴다.
    while (valid(index, hooks)) {
        const objectOwner = read(need(hooks.slot)(index), 34, 1);
        if ((objectOwner === need(hooks.localOwner)() || !objectOwner) && (read(need(hooks.slot)(index), 11, 1) & 8) === 0 && need(hooks.flags)(index)) {
            const point = position(index, hooks);
            if (need(hooks.region)(point[0], point[1]) !== localRegion) {
                const type = read(need(hooks.slot)(index), 10, 1);
                if (type !== need(hooks.playerType)() && type !== need(hooks.bridgeType)()) {
                    if (read(need(hooks.slot)(index), 34, 1) === need(hooks.localOwner)()) {
                        need(hooks.decrementLocal)(type);
                        need(hooks.decrementBuilt)(read(need(hooks.slot)(index), 10, 1));
                    }
                    need(hooks.setOwner)(index, owner);
                    const flags = need(hooks.flags)(index);
                    if ((flags & 0x200000) !== 0) {
                        // 원문 깊이 증가 뒤 예외는 감소를 합성하지 않는다.
                        hooks.loadingDepth.value = (hooks.loadingDepth.value + 1) >>> 0;
                        if (hooks.loadingDepthChanged) hooks.loadingDepthChanged(hooks.loadingDepth.value);
                        need(hooks.setOwner)(index, owner);
                        hooks.loadingDepth.value = (hooks.loadingDepth.value - 1) >>> 0;
                        if (hooks.loadingDepthChanged) hooks.loadingDepthChanged(hooks.loadingDepth.value);
                    }
                    if ((flags & 0x200) !== 0) {
                        const players = need(hooks.players)();
                        const here = position(index, hooks);
                        put(players, owner * 180 + 0x78, need(hooks.region)(here[0], here[1]));
                    }
                }
            }
        }
        index = next(index, limit, hooks);
    }
}

// 다음 후보는 flags 조회/가상 삭제보다 앞서 찾으며 삭제가 슬롯을 바꿔도 이번 다음 번호는 유지한다. (2026-11-08 WB04-E32)
export function clearMakeAiObjects(mask, hooks) {
    const limit = need(hooks.capacity)();
    let index = next(1, limit, hooks);
    // 현재 상한은 매번 다시 읽고 다음 free 건너뛰기에는 진입 limit 를 쓴다.
    while (valid(index, hooks)) {
        const current = index;
        index = next(index, limit, hooks);
        if ((need(hooks.flags)(current) & mask) !== 0) need(hooks.destroy)(current, 0);
    }
}
