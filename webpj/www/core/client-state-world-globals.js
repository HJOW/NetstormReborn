// State 정리의 Native 주소를 같은 Client 장기 전역에 연결한다. 미이식 주소는 명시적으로 거부한다.
// 사용: bindClientStateWorldGlobals({ gameplay, local, activity, storage, currentStorage }).read32(address) / write32(address, value).

// 원문 bool 필드는 임의 DWORD를 0/1로 조용히 바꾸지 않는다.
function stateBoolean(value) {
    if (value > 1) throw new RangeError('State bool 전역의 정상0/1 밖 쓰기');
    return value !== 0;
}

// 같은 권한·보조 전역·표면을 확인하고 직접 접근 callback을 반환한다. 바인딩 자체는 값을 바꾸지 않는다.
export function bindClientStateWorldGlobals(context) {
    const { gameplay, local, activity, storage, currentStorage } = context;
    if (storage.pool.edition !== 'patch1078' || storage.authority !== gameplay.authority() || storage.gameAux !== gameplay.gameAux() || (currentStorage && currentStorage !== storage)) {
        throw new TypeError('State 전역의 같은 실제 Client/표면/권한 owner 필요');
    }
    // 각 주소는 호출 시점에 같은 장기 상자를 다시 빌린다.
    const boxes = new Map([
        [0x542534, () => local.cachedPeerIndex()], [0x542538, () => local.cachedPeerValue()],
        [0x54253c, () => local.cachedPeerFieldA4()], [0x542540, () => local.cachedPeerFieldA8()], [0x542544, () => local.scriptBreak()],
        [0x54db14, () => gameplay.fullDeck()], [0x55a420, () => local.sourceType()], [0x5949d4, () => gameplay.mapRevision()],
        [0x59a8ac, () => gameplay.hideBuildings()], [0x5c7a74, () => local.begin()], [0x5c85a0, () => local.centralCreated()],
        [0x5c85b4, () => local.connectionNotified()], [0x5c85c0, () => local.zonePending()], [0x5c85c4, () => local.metaConnected()],
        [0x5c85c8, () => local.metadataMode()], [0x5c85cc, () => local.stateAuxCC()], [0x5c85e0, () => local.abortPending()],
    ]);
    // 읽기 전용 주소와 직접 읽기·쓰기가 가능한 Player/모드 필드를 구분한다.
    const values = new Map([
        [0x540bc0, { get: () => Number(gameplay.server().value), set: (v) => { gameplay.server().value = stateBoolean(v); } }],
        [0x540bc4, { get: () => Number(gameplay.boss().value), set: (v) => { gameplay.boss().value = stateBoolean(v); } }],
        [0x540c70, { get: () => gameplay.player.battleOwner }], [0x540cac, { get: () => gameplay.player.templateOwner }],
        [0x594fa0, { get: () => Number(storage.links.mission) }],
        [0x594fa4, { get: () => Number(gameplay.world.missionPost), set: (v) => { gameplay.world.missionPost = stateBoolean(v); } }],
        [0x594fb8, { get: () => Number(storage.ownerMode.fort), set: (v) => { storage.ownerMode.fort = stateBoolean(v); gameplay.player.fortMode = storage.ownerMode.fort; } }],
        [0x594fbc, { get: () => Number(storage.ownerMode.battle), set: (v) => { storage.ownerMode.battle = stateBoolean(v); storage.links.battle = storage.ownerMode.battle; } }],
        [0x594fc8, { get: () => Number(storage.ownerMode.challenge), set: (v) => { storage.ownerMode.challenge = stateBoolean(v); } }],
        [0x595344, { get: () => gameplay.player.playerKnown, set: (v) => { gameplay.player.playerKnown = v; } }],
        [0x5c85a4, { get: () => gameplay.world.editor, set: (v) => { gameplay.world.editor = v; } }],
        [0x5c85d4, { get: () => Number(local.savedGame().value), set: (v) => { local.savedGame().value = stateBoolean(v); } }],
        [0x5c8758, { get: () => local.state().phase[0] }],
    ]);
    // 22칸 프레임 표의 정렬된 원문 주소만 인덱스로 변환한다.
    const frameIndex = (address) => address >= 0x5c84e8 && address < 0x5c8540 && (address - 0x5c84e8) % 4 === 0 ? (address - 0x5c84e8) / 4 : -1;
    return {
        // 알려진 주소만 읽고 인접 표나 아직 이식하지 않은 소유자는 읽지 않는다.
        read32(address) {
            const frame = frameIndex(address); if (frame >= 0) return activity.frames()[frame] >>> 0;
            if (boxes.has(address)) return boxes.get(address)().value >>> 0;
            if (values.has(address)) return values.get(address).get() >>> 0;
            throw new RangeError('State 직접 읽기의 미등록 Native DWORD 주소');
        },
        // 알려진 쓰기 위치만 갱신하고 읽기 전용 주소는 거부한다.
        write32(address, value) {
            value >>>= 0; const frame = frameIndex(address); if (frame >= 0) { activity.frames()[frame] = value; return; }
            if (boxes.has(address)) { boxes.get(address)().value = value; return; }
            const entry = values.get(address); if (entry?.set) { entry.set(value); return; }
            throw new RangeError('State 직접 쓰기의 미등록 Native DWORD 주소');
        },
    };
}
