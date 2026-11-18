// State 정리의 Native 주소를 같은 Client 장기 전역에 연결한다. 미이식 주소는 명시적으로 거부한다.
// 사용: bindClientStateWorldGlobals({ gameplay, local, activity, storage, currentStorage }).read32(address) / write32(address, value).

// 원문 bool 필드는 임의 DWORD를 0/1로 조용히 바꾸지 않는다.
function stateBoolean(value) {
    if (value > 1) throw new RangeError('State bool 전역의 정상0/1 밖 쓰기');
    return value !== 0;
}

// 같은 권한·보조 전역·표면을 확인하고 직접 접근 callback을 반환한다. 바인딩 자체는 값을 바꾸지 않는다.
export function bindClientStateWorldGlobals(context) {
    const { gameplay, local, activity, storage, currentStorage, nsInfo, kencloud, players, templates, random, audio } = context;
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
        [0x59a8b0, () => kencloud.worldRedraw()],
    ]);
    // 읽기 전용 주소와 직접 읽기·쓰기가 가능한 Player/모드 필드를 구분한다.
    const values = new Map([
        [0x540bc0, { get: () => Number(gameplay.server().value), set: (v) => { gameplay.server().value = stateBoolean(v); } }],
        [0x540bc4, { get: () => Number(gameplay.boss().value), set: (v) => { gameplay.boss().value = stateBoolean(v); } }],
        [0x540c70, { get: () => gameplay.player.battleOwner }], [0x540cac, { get: () => gameplay.player.templateOwner }],
        [0x5424b0, { get: () => storage.pool.predictableCursor, set: (v) => { storage.pool.predictableCursor = v; } }],
        [0x5467b8, { get: () => nsInfo.zone, set: (v) => { nsInfo.zone = v; } }],
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
    const aux = gameplay.gameAux();
    // GameAux의 DWORD는 원래 signed/unsigned 저장 폭과 읽기 전용 주소를 유지한다.
    const auxFields = [
        [0x540bdc, 'defaultSp', 'signed', false], [0x541240, 'geyserType', 'unsigned', false],
        [0x5424bc, 'refundPercent', 'unsigned', true], [0x5453e4, 'allowAnyCapture', 'signed', true],
        [0x5467bc, 'subscriberMarker', 'unsigned', true], [0x595088, 'resourcePool', 'signed', true],
        [0x59508c, 'totalCost', 'signed', true], [0x595090, 'savedGeyserCount', 'unsigned', true],
        [0x5950ac, 'ordinaryGeysers', 'unsigned', true], [0x5950b0, 'geyserFrameFlag', 'unsigned', true],
        [0x595124, 'incomePrimed', 'unsigned', true], [0x5c8bb0, 'archipelago', 'unsigned', true],
        [0x5ca8f0, 'priestQuarterHp', 'bool', true],
    ];
    // 각 효과는 같은 실제 GameAux 필드를 현재 시점에 읽고 쓴다.
    for (const [address, name, kind, writable] of auxFields) {
        const entry = { get: () => Number(aux[name]) };
        if (writable) entry.set = (value) => { aux[name] = kind === 'bool' ? stateBoolean(value) : kind === 'signed' ? value | 0 : value; };
        values.set(address, entry);
    }
    values.set(0x54c8a0, { get: () => storage.chunks.mode, set: (value) => { storage.chunks.mode = value; } });
    // 중앙 좌표는 float 변환 없이 원본 DWORD 비트를 같은 Float32Array에 쓴다.
    for (const [address, offset] of [[0x568afc, 0], [0x568b00, 4]]) {
        const view = () => { const point = aux.centralPosition(); return new DataView(point.buffer, point.byteOffset, point.byteLength); };
        values.set(address, { get: () => view().getUint32(offset, true), set: (value) => view().setUint32(offset, value, true) });
    }
    // 손실 좌표도 같은 float32 물리에서 직접 DWORD 비트를 보존한다.
    for (const [address, offset] of [[0x5c848c, 0], [0x5c8490, 4]]) {
        const view = () => { const point = aux.lostPosition(); return new DataView(point.buffer, point.byteOffset, point.byteLength); };
        values.set(address, { get: () => view().getUint32(offset, true), set: (value) => view().setUint32(offset, value, true) });
    }
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
        // Fort 핵심 시드의 하위 WORD만 바꾸고 상위 16비트를 보존한다.
        write16(address, value) {
            if (address !== 0x5c89b4) throw new RangeError('State 직접 쓰기의 미등록 Native WORD 주소');
            random.fortCoreSeed = ((random.fortCoreSeed & 0xffff0000) | (value & 0xffff)) >>> 0;
        },
        // 독립 double 기한은 각 장기 소유자에 64비트 그대로 쓰고 음악 장치는 선택된 경우에만 요구한다.
        write64Bits(address, bits) {
            switch (address) {
            case 0x565e68: if (typeof audio?.writeSongEndBits !== 'function') throw new Error('State 음악 기한의 실제 음향 장치 미연결'); audio.writeSongEndBits(bits); return;
            case 0x5950b8: aux.writeWideBits('geyserDeadline', bits); return;
            case 0x5950c0: aux.writeWideBits('cleanupDeadline', bits); return;
            case 0x5950c8: aux.writeWideBits('secondaryGeyserDeadline', bits); return;
            case 0x595358: aux.writeWideBits('scheduledDrawDeadline', bits); return;
            case 0x5c85f0: local.loginEnd().bits = bits; return;
            case 0x5c85f8: local.connectEnd().bits = bits; return;
            case 0x5c8610: local.reconnectSaveEnd().bits = bits; return;
            case 0x5c8618: local.captureEnd().bits = bits; return;
            default: throw new RangeError('State 직접 쓰기의 미등록 Native double 주소');
            }
        },
        // 매 호출마다 현재 Player 물리를 다시 빌린다.
        players() { return players.records(); },
        // 현재 Template 9개 헤더의 첫 DWORD를 지정 슬롯에서 읽는다.
        templateManager(slot) {
            if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('State Template9의 슬롯 범위');
            const headers = templates.headers(); return new DataView(headers.buffer, headers.byteOffset, headers.byteLength).getUint32(slot * 20, true);
        },
        // 지정 TYPE의 실제 float 비용을 빌려 원본처럼 단정도로 저장한다.
        typeDefault(type) {
            if (!Number.isInteger(type) || type < 0 || type >= storage.types.length) throw new RangeError('State 현재 TYPE 범위');
            const record = storage.types[type]; return { get value() { return record.cost; }, set value(value) { record.cost = Math.fround(value); } };
        },
    };
}
