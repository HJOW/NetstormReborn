// 10.78의 저장 오브젝트/내용물 적용과 Terr→Chaff 순회를 같은 현재 월드 경계에서 수행한다.
// 근거: cpppj/o/RawFortObjectLoad.cpp의 값형 Fort 경로(004bdc60/004bd130/004be160/004be610).
// 실제 생성·가상 동작·공간 finder는 필수 주입한다. 슬롯/게임 월드의 대체 구현은 만들지 않는다. (2026-11-02 WB03-AB)

// 패치판 슬롯의 타입·WORD·HP·프레임 위치이다. 인접 필드를 덮지 않고 해당 폭만 쓴다.
const TYPE = 10, WORD = 12, HP = 26, FRAME = 36;
// 타입 flags1의 저장 필드·특수 프레임·그릇 플래그이다.
const SAVE_FRAME = 0x20, SAVE_QA = 0x1000, SAVE_QB = 0x2000, CONTAINER = 0x20000;
const TERRAIN_FRAME = 0x200000, RANDOM_FRAME = 0x100000;
// 타입 flags2의 다리·섬·팩토리·다이스 부류와 목록 플래그이다.
const BRIDGE = 0x4, ISLAND = 2, FACTORY = 0x4000, DAIS = 0x400000;
const CONTENT_LOCAL = 0x10, CONTENT_FILTERED = 0x100;
// 원본 고정 P/P 요청에서 보관하는 후보 최대 개수이다.
const MAX_CANDIDATES = 256;
// 원본 생성·가상·월드/장부 효과이다. 누락은 constructor에서 실행 전에 거부한다.
const REQUIRED_HOOKS = Object.freeze(['create', 'notifySurface', 'setState', 'setOwner', 'hasHitPoints', 'maxHitPoints',
    'virtualCount', 'pop', 'destroy', 'createContent', 'chunkRegion', 'territoryAllowed', 'loadingDepthChanged', 'finishOwner']);

// little endian 좁은 쓰기이다. raw는 같은 풀의 살아 있는 슬롯이며 검증은 풀 소유자가 수행한다.
function put(raw, offset, value, width) {
    // 지정 폭의 하위 바이트만 쓰고 주변 payload를 보존한다.
    for (let byte = 0; byte < width; byte += 1) raw[offset + byte] = (value >>> (byte * 8)) & 255;
}

// DWORD/WORD를 호스트 정렬과 무관하게 읽는다.
function read(raw, offset, width = 4) {
    let value = 0;
    // 지정 폭만 조립하므로 프레임 옆 extra를 섞지 않는다.
    for (let byte = 0; byte < width; byte += 1) value |= raw[offset + byte] << (byte * 8);
    return value >>> 0;
}

// 현재 타입이 요구한 선택 필드의 누락은 오류이다. 저장값0은 정상이다.
function required(value, name) {
    if (value === null || value === undefined) throw new Error(`Fort 현재 타입이 요구한 저장 필드 누락: ${name}`);
    return value;
}

// versionOwner={value:DWORD}를 공유하는 로더 전역이다. 다른 로더의 저장 판본과 사본을 만들지 않는다.
// readFullDeck/readFortKeepGenus는 실제 공유 전역을 조회할 때 지정한다. null이면 독립 입력의 fallback 필드를 쓴다.
export function createFortObjectLoadState(versionOwner) {
    if (!versionOwner || typeof versionOwner.value !== 'number') throw new TypeError('공유 Fort 판본 저장소가 필요합니다.');
    return {
        // 원본005c89c0의 현재 판본을 호출할 때마다 같은 소유자에서 읽고 쓴다.
        get version() { return versionOwner.value >>> 0; },
        set version(value) { versionOwner.value = value >>> 0; },
        // 중첩 로딩 깊이와 Terr1/Chaff2의 독립 scope 전역이다.
        loadingDepth: 0, scope: 0,
        // 실제10.78의 다리/받침 윗면/큰 섬(바위)/noIsland 타입 전역이다.
        bridgeType: 82, islandType: 94, largeIslandType: 95, noIslandType: 157,
        // 소유자 저장/지움·Fort 후처리에서 남길 부류의 원본 마스크이다.
        ownerSavedMask: 0x5d77cf00, ownerClearedMask: 0x10002000, fortKeepMask: 0x40480002,
        // 현재 편집/미션/Fort/Chaff 관문과 독립 입력의 삭제 억제값이다.
        special: false, mission: false, fort: false, chaffMode: false, fortDestroyBlocked: false,
        // 각 반복에서 현재 값으로 읽는 청크 지도 크기이다.
        chunksX: 16, chunksY: 16,
        // 실제 공유 전역을 빌리는 선택형 읽기 함수이다. 마지막 Pop/Contents 뒤에도 다시 읽는다.
        readFullDeck: null, readFortKeepGenus: null,
    };
}

// parseFort 결과를 실제 pool/finder/효과에 적용하는 로더이다. 화면 미리보기에는 자동 연결하지 않는다.
// pool: {edition:'patch1078', slot(sid), allocated(sid)}; finder:{pool,begin(rect),next()}는 같은 풀을 사용한다.
// types=188개 레코드, frames=188개 {codes[]}, random.next(limit), state는 위 공유 상태이다.
export class FortObjectLoad {
    // 원본과 같은 판본/표 크기·필수 효과·동일 풀 연결을 실제 쓰기 전에 검사한다.
    constructor({ pool, finder, types, frames, random, state, hooks }) {
        if (pool?.edition !== 'patch1078' || typeof pool.slot !== 'function' || typeof pool.allocated !== 'function' ||
            finder?.pool !== pool || typeof finder.begin !== 'function' || typeof finder.next !== 'function' ||
            types?.length !== 188 || frames?.length !== 188 || typeof random?.next !== 'function' || !state ||
            REQUIRED_HOOKS.some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('Fort 로더의10.78 자료/같은 풀/필수 효과 연결 오류');
        // 현재 raw 슬롯/공간 탐색을 소유하는 실제 장치 참조이다.
        this.pool = pool;
        this.finder = finder;
        // 현재 타입·프레임 표와 게임 난수 참조이다. 호출 중 바뀐 값도 다시 읽는다.
        this.types = types;
        this.frames = frames;
        this.random = random;
        // 공유 전역과 필수 생성/가상 효과 묶음이다.
        this.state = state;
        this.hooks = hooks;
    }

    // callback 뒤의 현재 타입을 같은 할당 슬롯에서 읽는다.
    type(sid) {
        const type = this.pool.allocated(sid)[TYPE];
        if (type >= this.types.length) throw new RangeError('Fort 현재 타입 범위 오류');
        return this.types[type];
    }

    // 저장/특수 프레임은 가상 SetFrame/표시 갱신을 부르지 않고 DWORD만 직접 쓴다.
    frame(sid, value) { put(this.pool.allocated(sid), FRAME, value, 4); }

    // QA/QB의 WORD 쓰기 뒤 현재 다리 전역과 일치할 때만 표면을 알린다.
    word(sid, value) {
        put(this.pool.allocated(sid), WORD, value, 2);
        if (this.pool.slot(sid)[TYPE] === this.state.bridgeType) this.hooks.notifySurface(sid);
    }

    // 같은 일반 finder의 첫 island 부류를 고른다. 상태/footprint 필터를 이 helper에 추가하지 않는다.
    terrain(x, y) {
        const ix = Math.trunc(x), iy = Math.trunc(y);
        // finder는 자체 현재 공간/연결 규칙으로 다음 객체를 반환한다.
        for (let sid = this.finder.begin({ left: ix, top: iy, right: ix, bottom: iy }); sid; sid = this.finder.next()) {
            if (this.type(sid).flags2 & ISLAND) return sid;
        }
        return 0;
    }

    // 원본 고정 wildcard 요청 P/P→변환P/P→P/A→P/모든 변형의 첫 비지 않은 후보 집합에서 고른다.
    randomFrame(sid) {
        const type = this.pool.slot(sid)[TYPE];
        if (type >= this.frames.length) throw new RangeError('Fort 랜덤 프레임 타입 오류');
        const codes = this.frames[type].codes;
        const candidates = [];
        // P의 원본 방향 mask가0이므로 변환 fallback도 P/P이다.
        for (let pass = 0; pass < 4 && candidates.length === 0; pass += 1) {
            // 최대256개만 담으며 번호/flags는 후보 판정에 참여하지 않는다.
            for (let index = 0; index < codes.length; index += 1) {
                const code = codes[index];
                if (code.side === 80 && (pass === 3 || code.variant === (pass === 2 ? 65 : 80)) && candidates.length < MAX_CANDIDATES) candidates.push(index);
            }
        }
        return candidates.length ? candidates[this.random.next(candidates.length)] : -1;
    }

    // 저장 레코드 하나: 초기 필드→소유자→HP→특수 프레임→Pop9→내용물→Fort 후처리이다.
    object(object, originX, originY, defaultOwner) {
        const s = this.state, h = this.hooks;
        let owner = defaultOwner >>> 0;
        const type = object.storedType >= 0xf6 ? s.bridgeType : object.type;
        if (object.storedType >= 0xf6) owner = 255 - object.storedType;
        if (!type || type === s.islandType) return 0;
        if (type < 0 || type >= this.types.length) throw new RangeError('Fort 저장 타입 범위 오류');
        const x = Math.fround((originX + (object.position >> 4)) | 0);
        const y = Math.fround((originY + (object.position & 15)) | 0);
        const sid = h.create(type, 0);
        this.type(sid);
        if (this.type(sid).flags1 & SAVE_FRAME) this.frame(sid, required(object.frame, 'frame'));
        if (this.type(sid).flags1 & SAVE_QA) this.word(sid, (this.pool.slot(sid)[WORD + 1] << 8) | required(object.quantityByte, 'quantityByte'));
        if (this.type(sid).flags1 & SAVE_QB) this.word(sid, required(object.quantityWord, 'quantityWord'));
        if (this.type(sid).flags2 & BRIDGE) this.frame(sid, required(object.bridgeFrame, 'bridgeFrame'));
        if (s.version === 0) h.setState(sid, required(object.legacyState, 'legacyState'));
        if ((s.version | 0) > 1 && (this.type(sid).flags2 & FACTORY)) h.setState(sid, required(object.factoryState, 'factoryState'));
        if ((s.version | 0) > 0) {
            if (this.type(sid).flags2 & s.ownerSavedMask) {
                owner = required(object.owner, 'owner');
                if (!owner || owner > 8) owner = 1;
                if (!s.special && !s.mission) owner = defaultOwner >>> 0;
            }
        } else owner = defaultOwner >>> 0;
        if (this.type(sid).flags2 & s.ownerClearedMask) owner = 0;
        const currentType = this.pool.slot(sid)[TYPE];
        if (currentType === s.islandType || currentType === s.noIslandType) owner = 0;
        if (currentType === s.largeIslandType) throw new Error('Fort isleBig 로드 원본 assert');
        h.setOwner(sid, owner);
        if (h.hasHitPoints(sid)) {
            // 가상 HP 조회가 풀 저장소를 바꿀 수 있으므로 반환 뒤 현재 할당 슬롯을 다시 빌린다.
            const hp = h.maxHitPoints(sid);
            put(this.pool.allocated(sid), HP, hp, 4);
        }
        if (this.type(sid).flags1 & TERRAIN_FRAME) {
            const ground = this.terrain(x, y);
            if (ground) {
                const frame = read(this.pool.slot(ground), FRAME);
                this.frame(sid, frame === 0xffffffff ? 0 : frame);
            }
        }
        if (this.type(sid).flags1 & RANDOM_FRAME) {
            const frame = this.randomFrame(sid);
            this.frame(sid, frame === -1 ? 0 : frame);
        }
        h.pop(sid, x, y, 9);
        if (this.type(sid).flags1 & CONTAINER) this.contents(sid, required(object.contents, 'contents'), 9);
        if (s.fort && !(s.readFullDeck ? s.readFullDeck() !== 0 : s.fortDestroyBlocked)) {
            const genus = this.type(sid).flags2;
            if (!(genus & (s.readFortKeepGenus ? s.readFortKeepGenus() : s.fortKeepMask)) ||
                (!s.special && !s.mission && (this.type(sid).flags2 & DAIS) && h.virtualCount(sid) === 1)) h.destroy(sid, 0);
        }
        return sid;
    }

    // 내용물의 실제 현재 타입/부모 listFlags를 사용한다. Local 플래그 변경은 뒤 형제에게 남고 재귀에는 값으로 전달한다.
    contents(parent, contents, flags = 9) {
        // 저장 순서대로 attach한 뒤 그 항목의 중첩 목록을 처리한다.
        for (const item of contents.items) {
            if (!item.type) return false;
            if (item.type < 0 || item.type >= this.types.length) throw new RangeError('Fort 내용물 타입 범위 오류');
            const savedFlags = this.types[item.type].flags1;
            let listFlags = this.types[item.type].contentListFlags;
            if (!listFlags) listFlags = this.type(parent).containerListFlags;
            if (!listFlags) {
                if (!item.listFlagsStored) throw new Error('Fort 현재 내용물 부모가 요구한 listFlags 저장 BYTE 누락');
                listFlags = item.listFlags;
            }
            if (listFlags & CONTENT_LOCAL) flags = ((flags & ~8) | 16) >>> 0;
            const child = this.hooks.createContent(parent, item.type >>> 0, item.quantity >>> 0, (listFlags & ~CONTENT_FILTERED) >>> 0, flags, 0);
            if ((savedFlags & CONTAINER) && !this.contents(child, { items: item.contents, complete: item.contentsComplete }, flags)) return false;
        }
        return contents.complete;
    }

    // 청크 좌표의16배는 원본 DWORD 감김을 유지하며 파일 안 객체 순서대로 즉시 적용한다.
    chunk(chunk, x, y, defaultOwner) {
        // 비어 있는 레코드도 object에서 원본 분기로 소비한다.
        for (const object of chunk.objects) this.object(object, x << 4, y << 4, defaultOwner);
    }

    // 섹션마다 현재 version/scope와 지도/지역 정책을 읽는다. 저장 청크 소진은 앞선 정상 효과 뒤에 오류이다.
    section(section, owner, territory) {
        const s = this.state;
        s.version = section.version;
        if (!section.present) return;
        s.scope = territory < 0 ? 2 : 1;
        let index = 0;
        // 현재 행 수는 callback 뒤 다음 조건 비교에서도 다시 읽는다.
        for (let y = 0; y < s.chunksY; y += 1) {
            // x가 안쪽이며 Chaff도 현재 지역을 읽는다.
            for (let x = 0; x < s.chunksX; x += 1) {
                const region = this.hooks.chunkRegion(x, y);
                if (territory >= 0 && !this.hooks.territoryAllowed(region, owner, territory)) continue;
                if (index >= section.chunks.length) throw new RangeError('Fort 선택 청크 저장 자료 소진');
                this.chunk(section.chunks[index++], x, y, owner);
            }
        }
    }

    // Terr[start,end) 뒤 허용 시 Chaff, 소유자 마감, 깊이 복구 순서이다. raw 스트림/월드 생성 부모는 별도다.
    load(fort, templateSlot, start = 0, end = 20) {
        if (start === end || start < 0 || end > 20) throw new RangeError('Fort Terr 범위 오류');
        const owner = templateSlot >= 1 && templateSlot <= 8 ? templateSlot >>> 0 : 0;
        const s = this.state;
        s.loadingDepth = (s.loadingDepth + 1) >>> 0;
        this.hooks.loadingDepthChanged(s.loadingDepth);
        try {
            // Terr 번호별 순회와 Chaff의 뒤쪽 실행 순서를 유지한다.
            for (let territory = start; territory < end; territory += 1) this.section(fort.territories[territory], owner, territory);
            if (s.fort || s.chaffMode) this.section(fort.chaff, owner, -1);
            this.hooks.finishOwner(owner);
        } finally {
            s.loadingDepth = (s.loadingDepth - 1) >>> 0;
            this.hooks.loadingDepthChanged(s.loadingDepth);
        }
    }
}
