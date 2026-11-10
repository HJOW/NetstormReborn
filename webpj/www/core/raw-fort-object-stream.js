// 10.78 의 실제 Objects stream 을 읽으면서 같은 객체/내용물/월드에 즉시 적용하는 모듈이다(cpppj `o/RawFortObjectStream.cpp/.h` = `RawFortObjectLoad::ObjectStream/ChunkStream/SectionStream/LoadStream`,
// 원본 004bdc60 · 004be160 · 004be2c0 · 004be3c0 · 004be610, 기준선 1f00a25a 와 같음). 값형 경로(`FortObjectLoad.object/chunk/section/load`)와 같은 로더(`pool` · `types` · `state` · `hooks`)를 쓰고
// 완성 레코드 목록을 만들지 않는다: 필드를 읽기 전에 객체를 만들고, 각 실제 효과 뒤에 현재 TYPE/전역을 다시 읽는다.
//  - objectStream: 위치 BYTE · 저장 종류 BYTE(0xf6 이상은 다리와 owner=255-저장값, 아니면 현재 변환표/checking 으로 치환) → 종류 0 이면 추가 BYTE 가 있으면 진단 0x2e8 · island 는 추가 BYTE 를 읽지 않고 pos 만 증가 →
//    create → 프레임/QA/QB/다리 프레임/legacy·factory state → owner 결정 → HP → 지형·난수 프레임 → pop(9) → container 면 contents 훅 → Fort 후처리 삭제.
//  - chunkStream: 표지 BYTE(99 아니면 진단 0x37a) · WORD 개수 뒤 객체를 읽은 즉시 적용(뒤 실패 때 앞선 객체/난수 유지).
//  - sectionStream: 현재 지도 크기(chunksX/Y)와 Terr 허용 정책을 읽은 뒤에만 stream 을 소비한다. scope 는 Terr 1, Chaff 2.
//  - loadStream: 깊이 증가 → 변환표 준비 → Terr[start,end) → (Fort/Chaff 모드면) Chaff → 정상 꼬리의 변환표 반납 → 소유자 마감. 예외는 깊이만 복구하고 변환표와 이미 만든 월드는 보존한다.
// input(FortObjectStreamHooks) = { manager(slot), select(manager, 이름, 1) → stream, access(stream, 작업(fields, memory)), prepareTypes(slot), conversion(), checking(), conversionBuffer(token),
//   releaseConversion(), contents(sid, stream), assertion(번호) }. 진단 뒤 복귀하면 중간 효과를 유지하고 계속한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-10 WB04-E276: 4-B Objects stream 적재 이식)

// 원본 진단 행 번호이다. (2026-11-10 WB04-E276)
export const FORT_OBJECT_STREAM_ASSERTION = Object.freeze({ wordPastEnd: 0x2e, missingTypeConversion: 0x79, nonZeroAfterEmpty: 0x2e8, largeIsland: 0x334, chunkMarker: 0x37a, noManager: 0x3f5, emptyRange: 0x3f9 });

// 타입 flags1 의 저장 필드·특수 프레임·그릇 비트이다(값형 경로와 같다). (2026-11-10 WB04-E276)
const SAVE_FRAME = 0x20;
const SAVE_QA = 0x1000;
const SAVE_QB = 0x2000;
const CONTAINER = 0x20000;
const TERRAIN_FRAME = 0x200000;
const RANDOM_FRAME = 0x100000;
// 타입 flags2 의 다리 프레임·factory·dais 부류이다. (2026-11-10 WB04-E276)
const BRIDGE_FRAME = 0x4;
const FACTORY = 0x4000;
const DAIS = 0x400000;
// 슬롯 안의 타입·WORD 상위 바이트·HP·프레임 위치이다. (2026-11-10 WB04-E276)
const TYPE = 10;
const WORD_HIGH = 13;
const HP = 26;
const FRAME = 36;

// 선택한 하위만 요구한다. (2026-11-10 WB04-E276)
function need(device) {
    if (typeof device !== 'function') throw new Error('Fort 원시 Objects 실제 장치 미연결');
    return device;
}

// 비정렬 DWORD(리틀 엔디언) 읽기/쓰기. 물리 범위를 넘으면 거부한다. (2026-11-10 WB04-E276)
function dword(bytes, at) {
    if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Fort Objects stream DWORD 물리범위');
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}
function putDword(bytes, at, value) {
    if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Fort Objects DWORD 쓰기 물리범위');
    for (let byte = 0; byte < 4; byte += 1) bytes[at + byte] = (value >>> (8 * byte)) & 255;
}

// BYTE 직접 읽기는 논리 end 검사를 추가하지 않고 실제 버퍼 끝만 거부한다. (2026-11-10 WB04-E276)
function streamByte(token, input) {
    let value = 0;
    need(input.access)(token, (fields, memory) => {
        const position = dword(fields, 4);
        const bytes = need(memory.buffer)(dword(fields, 0));
        if (position >= bytes.length) throw new RangeError('Fort Objects 직접 BYTE 물리범위');
        value = bytes[position];
        putDword(fields, 4, (position + 1) >>> 0);
    });
    return value;
}

// WORD 를 먼저 읽고 pos 증가 뒤 signed end 진단을 한다. signedValue 면 16 비트 부호 확장 DWORD 를 돌려준다. (2026-11-10 WB04-E276)
function streamWord(token, input, signedValue) {
    let value = 0;
    need(input.access)(token, (fields, memory) => {
        const position = dword(fields, 4);
        const bytes = need(memory.buffer)(dword(fields, 0));
        if (position > bytes.length || bytes.length - position < 2) throw new RangeError('Fort Objects 직접 WORD 물리범위');
        value = bytes[position] | (bytes[position + 1] << 8);
        const end = dword(fields, 8);
        putDword(fields, 4, (position + 2) >>> 0);
        if (((position + 2) | 0) > (end | 0)) need(input.assertion)(FORT_OBJECT_STREAM_ASSERTION.wordPastEnd);
    });
    return signedValue ? ((value << 16) >> 16) >>> 0 : value;
}

// 원본 signed 좌표의 DWORD 감김 덧셈이다. (2026-11-10 WB04-E276)
function add(a, b) {
    return (a + b) | 0;
}

// 원문 BYTE 종류의 현재 변환표/검사 모드를 재조회해 치환한다. 0xf6 이상이나 표가 없으면 그대로이다. (2026-11-10 WB04-E276)
function streamType(type, input) {
    if (type >= 0xf6) return type;
    let conversion = need(input.conversion)();
    if (!conversion) return type;
    // 진단 뒤 바뀐 현재표도 같은 원래 종류로 읽는다.
    const read = (pointer) => {
        const bytes = need(input.conversionBuffer)(pointer);
        if (type >= bytes.length) throw new RangeError('Fort Objects 변환표 BYTE 물리범위');
        return bytes[type];
    };
    if (need(input.checking)() && !read(conversion)) {
        need(input.assertion)(FORT_OBJECT_STREAM_ASSERTION.missingTypeConversion);
        conversion = input.conversion();
    }
    return read(conversion);
}

// 버전 BYTE 를 읽은 뒤 원문 data/end/pos 관문만 판단한다: data 비0, end 비0, signed pos<end. (2026-11-10 WB04-E276)
function streamRemaining(token, input) {
    let present = false;
    need(input.access)(token, (fields) => {
        present = dword(fields, 0) !== 0 && dword(fields, 8) !== 0 && (dword(fields, 4) | 0) < (dword(fields, 8) | 0);
    });
    return present;
}

// 값형 경로와 같은 판본/풀 검사이다(10.78 전용). (2026-11-10 WB04-E276)
function requirePatch(loader, message) {
    if (loader.pool.edition !== 'patch1078') throw new TypeError(message);
}

// 004be160 객체 하나: 필드를 읽기 전에 객체를 만들고 각 실제 효과 뒤 현재 TYPE/전역을 읽는다. 삭제된 SID 도 호출자에게 돌려준다(원문은 void). (2026-11-10 WB04-E276)
export function objectStream(loader, stream, originX, originY, defaultOwner, input) {
    requirePatch(loader, 'Fort 원시 Objects는10.78 전용');
    const state = loader.state;
    const hooks = loader.hooks;
    const position = streamByte(stream, input);
    const stored = streamByte(stream, input);
    let type = stored;
    let owner = defaultOwner >>> 0;
    if (stored < 0xf6) {
        type = streamType(stored, input);
    } else {
        owner = (255 - stored) >>> 0;
        type = state.bridgeType & 255;
    }
    if (!type) {
        if (streamByte(stream, input)) need(input.assertion)(FORT_OBJECT_STREAM_ASSERTION.nonZeroAfterEmpty);
        return 0;
    }
    // island 의 추가 BYTE 는 읽지 않고 pos 만 증가한다.
    if (type === state.islandType) {
        need(input.access)(stream, (fields) => putDword(fields, 4, (dword(fields, 4) + 1) >>> 0));
        return 0;
    }
    const x = Math.fround(add(originX, position >> 4));
    const y = Math.fround(add(originY, position & 15));
    const sid = hooks.create(type, 0);
    loader.type(sid);
    if (loader.type(sid).flags1 & SAVE_FRAME) loader.frame(sid, streamByte(stream, input));
    if (loader.type(sid).flags1 & SAVE_QA) {
        const byte = streamByte(stream, input);
        loader.word(sid, ((loader.pool.slot(sid)[WORD_HIGH] << 8) | byte) >>> 0);
    }
    if (loader.type(sid).flags1 & SAVE_QB) loader.word(sid, streamWord(stream, input, true));
    if (loader.type(sid).flags2 & BRIDGE_FRAME) loader.frame(sid, streamByte(stream, input));
    // 원문의 legacy 판정 004ad0a0 은 OR 때문에 항상 참이다: version 0 에서 BYTE 를 읽는다.
    if (!state.version) hooks.setState(sid, streamByte(stream, input));
    if ((state.version | 0) > 1 && (loader.type(sid).flags2 & FACTORY)) hooks.setState(sid, streamByte(stream, input));
    if ((state.version | 0) > 0) {
        if (loader.type(sid).flags2 & state.ownerSavedMask) {
            owner = streamByte(stream, input);
            if (!owner || owner > 8) owner = 1;
            if (!state.special && !state.mission) owner = defaultOwner >>> 0;
        }
    } else {
        owner = defaultOwner >>> 0;
    }
    if (loader.type(sid).flags2 & state.ownerClearedMask) owner = 0;
    const current = loader.pool.slot(sid)[TYPE];
    if (current === state.islandType || current === state.noIslandType) owner = 0;
    if (current === state.largeIslandType) need(input.assertion)(FORT_OBJECT_STREAM_ASSERTION.largeIsland);
    hooks.setOwner(sid, owner);
    // 가상 HP 조회가 풀 저장소를 바꿀 수 있으므로 반환 뒤 현재 할당 슬롯을 다시 빌려 쓴다.
    if (hooks.hasHitPoints(sid)) putDword(loader.pool.allocated(sid), HP, hooks.maxHitPoints(sid) >>> 0);
    if (loader.type(sid).flags1 & TERRAIN_FRAME) {
        const ground = loader.terrain(x, y);
        if (ground) {
            const frame = dword(loader.pool.slot(ground), FRAME);
            loader.frame(sid, frame === 0xffffffff ? 0 : frame);
        }
    }
    if (loader.type(sid).flags1 & RANDOM_FRAME) {
        const frame = loader.randomFrame(sid);
        loader.frame(sid, frame === -1 ? 0 : frame);
    }
    hooks.pop(sid, x, y, 9);
    if (loader.type(sid).flags1 & CONTAINER) need(input.contents)(sid, stream);
    if (state.fort && !(state.readFullDeck ? state.readFullDeck() !== 0 : state.fortDestroyBlocked)) {
        const genus = loader.type(sid).flags2;
        if (!(genus & (state.readFortKeepGenus ? state.readFortKeepGenus() : state.fortKeepMask)) ||
            (!state.special && !state.mission && (loader.type(sid).flags2 & DAIS) && hooks.virtualCount(sid) === 1)) hooks.destroy(sid, 0);
    }
    return sid;
}

// 004be2c0 청크 하나: 표지/WORD 개수 뒤 각 객체를 읽은 즉시 적용한다. 뒤 레코드 실패 때 앞선 객체/난수를 유지한다. (2026-11-10 WB04-E276)
export function chunkStream(loader, stream, x, y, owner, input) {
    const originX = (x << 4) | 0;
    const originY = (y << 4) | 0;
    if (streamByte(stream, input) !== 99) need(input.assertion)(FORT_OBJECT_STREAM_ASSERTION.chunkMarker);
    const count = streamWord(stream, input, false);
    for (let index = 0; index < count; index += 1) objectStream(loader, stream, originX, originY, owner, input);
}

// 004be3c0 섹션 하나: 원문 y/x 순서와 각 현재 지도 크기/정책을 그대로 사용한다. territory<0 이면 Chaff(scope 2)이다. (2026-11-10 WB04-E276)
export function sectionStream(loader, stream, owner, territory, input) {
    const state = loader.state;
    state.scope = territory < 0 ? 2 : 1;
    // 매 행 끝에서 현재 높이를 다시 비교한다.
    for (let y = 0; y < state.chunksY; y = add(y, 1)) {
        // 같은 현재 청크와 Terr 허용을 읽은 뒤에만 파일을 소비한다.
        for (let x = 0; x < state.chunksX; x = add(x, 1)) {
            const region = loader.hooks.chunkRegion(x, y);
            if (territory >= 0 && !loader.hooks.territoryAllowed(region, owner, territory)) continue;
            chunkStream(loader, stream, x, y, owner, input);
        }
    }
}

// 004be610 상위 순회: 깊이/변환표 준비 → Terr → Chaff → 정상표 반납 → 후처리. 예외는 깊이만 반납하고 변환표·이미 만든 월드는 보존한다. (2026-11-10 WB04-E276)
export function loadStream(loader, slot, input, start = 0, end = 20) {
    requirePatch(loader, 'Fort 원시 Objects 부모는10.78 전용');
    const state = loader.state;
    state.loadingDepth = (state.loadingDepth + 1) >>> 0;
    loader.hooks.loadingDepthChanged(state.loadingDepth);
    try {
        if (!need(input.manager)(slot)) need(input.assertion)(FORT_OBJECT_STREAM_ASSERTION.noManager);
        if (start === end) need(input.assertion)(FORT_OBJECT_STREAM_ASSERTION.emptyRange);
        need(input.prepareTypes)(slot);
        const owner = slot >= 1 && slot <= 8 ? slot >>> 0 : 0;
        // 원문과 같은 현재 manager 를 각 선택 직전에 다시 읽는다.
        for (let territory = start; territory < end; territory = add(territory, 1)) {
            const name = `Terr${String(territory).padStart(2, '0')}`;
            const stream = need(input.select)(input.manager(slot), name, 1);
            state.version = streamByte(stream, input);
            if (streamRemaining(stream, input)) sectionStream(loader, stream, owner, territory, input);
        }
        if (state.fort || state.chaffMode) {
            const stream = need(input.select)(input.manager(slot), 'Chaff', 1);
            state.version = streamByte(stream, input);
            if (streamRemaining(stream, input)) sectionStream(loader, stream, owner, -1, input);
        }
        need(input.releaseConversion)();
        loader.hooks.finishOwner(owner);
    } catch (error) {
        state.loadingDepth = (state.loadingDepth - 1) >>> 0;
        loader.hooks.loadingDepthChanged(state.loadingDepth);
        throw error;
    }
    state.loadingDepth = (state.loadingDepth - 1) >>> 0;
    loader.hooks.loadingDepthChanged(state.loadingDepth);
}
