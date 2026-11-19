// 원본 Fort 현재 이름 선택(00460980)과 고정 Template의 파일 이름 설정(004bf630)을 이식한다.
// 사용: setCurrentFortName(nameBytes, scratch256, globals, hooks). 이름과 경로는 NUL을 포함한 실제 BYTE 배열이다.
// Editor 파일 삭제 등 외부 효과는 호출자가 연결하며 이 모듈이 파일을 수정하지 않는다.

// 선택된 실제 효과만 요구하며 미연결을 성공으로 대체하지 않는다.
function need(effect) { if (typeof effect !== 'function') throw new Error('Fort 현재 이름의 실제 경계 미연결'); return effect; }
// 현재 물리의 비정렬 DWORD만 읽는다.
function read(bytes, offset) {
    if (!(bytes instanceof Uint8Array) || offset < 0 || offset + 4 > bytes.length) throw new RangeError('Fort 현재 이름 헤더 조회 범위');
    return new DataView(bytes.buffer, bytes.byteOffset + offset, 4).getUint32(0, true);
}
// 선택된 같은 물리에 DWORD를 쓰고 주변 바이트는 보존한다.
function put(bytes, offset, value) {
    if (!(bytes instanceof Uint8Array) || offset < 0 || offset + 4 > bytes.length) throw new RangeError('Fort 현재 이름 헤더 쓰기 범위');
    new DataView(bytes.buffer, bytes.byteOffset + offset, 4).setUint32(0, value >>> 0, true);
}
// 첫 BYTE만 검사하며 물리가 없으면 빈 이름으로 취급하지 않는다.
function empty(bytes) { if (!(bytes instanceof Uint8Array) || !bytes.length) throw new RangeError('Fort 현재 이름 첫 BYTE 범위'); return bytes[0] === 0; }
// 첫 점 또는 NUL까지만 읽으며 미종료 입력은 거부한다.
function hasDot(bytes) {
    if (!(bytes instanceof Uint8Array)) throw new TypeError('Fort 새 이름 BYTE 필요');
    // 원문 strchr처럼 현재 BYTE를 순서대로 확인한다.
    for (const byte of bytes) { if (byte === 46) return true; if (byte === 0) return false; }
    throw new RangeError('Fort 새 이름 NUL 범위');
}
// 한 BYTE씩 읽고 쓴 뒤 NUL을 검사해 겹친 배열과 부분 실패를 보존한다.
function copy(target, source) {
    if (!(target instanceof Uint8Array) || !(source instanceof Uint8Array)) throw new TypeError('Fort 이름 복사 BYTE 필요');
    // 물리 범위를 넘으면 그 이전에 쓴 BYTE를 유지하고 중단한다.
    for (let index = 0; ; index += 1) {
        if (index >= source.length || index >= target.length) throw new RangeError('Fort 이름 NUL 복사 물리 범위');
        const byte = source[index]; target[index] = byte; if (byte === 0) return;
    }
}
// 원본이 요구하는 위치마다 현재 owner와 헤더를 다시 조회한다.
function currentHeader(globals, hooks) { return need(hooks.header)(need(globals.owner)()); }
// 경로 선택은 파일 키 포인터를 먼저 고르고 그 내용은 바꾸지 않는다.
function path(name, hooks) { need(hooks.selectFileSpec)(); return need(hooks.pathSpec)(name); }

// 고정 헤더의 진단 뒤 현재 manager 이름에 경로를 복사하고 같은 슬롯에 통지한다.
export function setFortManagerFileName(slot, value, hooks) {
    const header = need(hooks.header)(slot);
    if (!read(header, 0)) need(hooks.assertion)(429);
    if (read(header, 4)) need(hooks.assertion)(430);
    copy(need(hooks.managerName)(read(header, 0)), value); need(hooks.notify)(slot);
}

// 현재 Fort 이름의 이전 경로·Snapshot·복사·새 경로·제목·설정 효과를 원본 순서로 수행한다.
export function setCurrentFortName(name, scratch, globals, hooks) {
    if (!(scratch instanceof Uint8Array) || scratch.length !== 256) throw new TypeError('Fort 이전 경로 scratch 256BYTE 필요');
    if (read(currentHeader(globals, hooks), 0)) {
        if (empty(need(hooks.currentName)())) need(hooks.assertion)(39);
        copy(scratch, path(need(hooks.currentName)(), hooks));
    }
    if (hasDot(name)) need(hooks.assertion)(88);
    const before = currentHeader(globals, hooks);
    if (read(before, 0) && !read(before, 8)) need(hooks.snapshot)();
    copy(need(hooks.currentName)(), name);
    const after = currentHeader(globals, hooks);
    if (read(after, 0)) {
        const readonly = read(after, 8); put(after, 8, 0);
        if (empty(need(hooks.currentName)())) need(hooks.assertion)(39);
        const selected = path(need(hooks.currentName)(), hooks);
        setFortManagerFileName(need(globals.owner)(), selected, hooks); need(hooks.snapshot)();
        if (!readonly) {
            const next = path(name, hooks);
            if (!need(hooks.samePath)(scratch, next)) need(hooks.deleteRequest)(scratch);
        }
    }
    need(hooks.refreshTitle)(); const dirty = globals.dirty; const fort = need(globals.fort)(); globals.dirty = (dirty + 1) >>> 0;
    if (fort) {
        if (need(globals.editor)()) { need(hooks.config)('lastEditFort', need(hooks.currentName)()); return; }
        if (need(globals.mission)()) return;
        const slot = need(globals.owner)(); if (!read(need(hooks.header)(slot), 0)) return;
        const state = need(hooks.state)(slot); if (!(state instanceof Uint8Array) || state.length <= 0x11) throw new RangeError('Fort 현재 State 관문 BYTE 범위');
        if (!state[0x11]) need(hooks.config)('lastMultiplayerFort', need(hooks.currentName)());
    }
    if (need(globals.editor)()) need(hooks.config)('lastEditFort', need(hooks.currentName)());
}
