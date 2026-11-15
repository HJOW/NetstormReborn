// 캠페인 Objects도 쓰는004bd800 TypeNames 부모와 별도 unwind를 옮긴다(cpppj o/RawFortTypeNamesLoad.cpp/.h, 225553df).
// globals={loadingDepth,conversion}는 같은 현재DWORD getter/setter다. hooks.access(token, operation(fields16,memory)) 또는 stream/buffer를 제공한다.
// 모든 효과는 도달한 위치에서만 요구하며 자동 cleanup/rollback은 하지 않는다. 해제·전체 Session 설치는 상위 원문 부모의 책임이다. (WB04-E573)

// 원문 변환 폭과 기존 변환 진단의 Template.cpp 행이다.
const CONVERSION_BYTES = 255, EXISTING_CONVERSION_LINE = 608;
// 선택된 실제 하위가 없으면 그 접두에서 실패한다.
function need(device) { if (typeof device !== 'function') throw new Error('TypeNames 실제 경계 미연결'); return device; }
// 현재 실제 바이트에서 비정렬 DWORD를 읽는다. 부족한 범위를0으로 확장하지 않는다.
function read(bytes, offset) { if (!Number.isSafeInteger(offset) || offset < 0 || offset > bytes.length || bytes.length - offset < 4) throw new RangeError('TypeNames DWORD 물리 읽기 범위'); return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(offset, true); }
// 지정DWORD 전체가 들어갈 때만 네BYTE를 쓴다. 짧은 블록에 반DWORD를 쓰지 않는다.
function put(bytes, offset, value) { if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('TypeNames stream 쓰기 범위'); new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).setUint32(offset, value >>> 0, true); }
// 첫NUL까지 signed char를 위치mod4만큼 밀어 원문32비트 합을 감는다.
function hashName(name) {
    let hash = 0;
    // NUL 뒤 바이트는 포함하지 않으며 실제 물리 안에 NUL이 없으면 거부한다.
    for (let index = 0; index < name.length; index += 1) { const byte = name[index]; if (!byte) return hash >>> 0; hash = (hash + (((byte << 24) >> 24) << ((index & 3) * 8))) >>> 0; }
    throw new RangeError('TypeNames 현재 이름NUL 물리 범위');
}
// 같은 현재DWORD에 먼저 쓰고 소비자에게 새 깊이를 통지한다.
function depth(value, globals, hooks) { globals.loadingDepth = value >>> 0; need(hooks.loadingDepthChanged)(value >>> 0); }

// count는 할당 전, hash 주소는 할당/zero 뒤, 각 변환 쓰기 주소는 일치 순간의 현재값으로 읽는다.
export function loadFortTypeNames(slot, globals, hooks) {
    depth((globals.loadingDepth + 1) >>> 0, globals, hooks);
    const token = need(hooks.select)(need(hooks.manager)(slot | 0), 'TypeNames', 1);
    // 선택된 같은 fields/data의 동기 대여 안에서 본문을 전부 실행한다.
    const load = (fields, buffer) => {
        const data = read(fields, 0);
        if (!data || !read(fields, 8)) globals.conversion = 0;
        else {
            const position = read(fields, 4), bytes = need(buffer)(data);
            if (position >= bytes.length) throw new RangeError('TypeNames count BYTE 물리 범위');
            const count = bytes[position]; put(fields, 4, (position + 1) >>> 0);
            if (globals.conversion) need(hooks.assertion)(EXISTING_CONVERSION_LINE);
            const allocated = need(hooks.allocate)(CONVERSION_BYTES); globals.conversion = allocated >>> 0; const conversion = need(hooks.conversionBuffer)(allocated >>> 0);
            // rep stosd의63회 쓰기를 보존한다.
            for (let word = 0; word < 63; word += 1) put(conversion, word * 4, 0);
            if (conversion.length < 254) throw new RangeError('TypeNames 할당WORD 물리 범위'); conversion[252] = conversion[253] = 0;
            if (conversion.length < CONVERSION_BYTES) throw new RangeError('TypeNames 할당BYTE 물리 범위'); conversion[254] = 0;
            const hashPosition = read(fields, 4), hashData = read(fields, 0);
            if ((hashPosition | 0) > (read(fields, 8) | 0)) put(fields, 8, hashPosition);
            // 매 반복 조건마다 현재 signed TYPE 수를 읽고256 이상 TYPE의 낮은BYTE도 그대로 저장한다.
            for (let type = 0; type < (need(hooks.typeCount)() | 0); type += 1) {
                const hash = hashName(need(hooks.typeName)(type));
                // 저장 중복 hash가 있으면 첫 칸만 갱신한다. 버퍼 내용은 매 비교에서 현재 물리를 다시 읽는다.
                for (let stored = 0; stored < count; stored += 1) {
                    if (read(need(buffer)(hashData), hashPosition + stored * 4) !== hash) continue;
                    const current = need(hooks.conversionBuffer)(globals.conversion);
                    if (stored >= current.length) throw new RangeError('TypeNames 현재 변환 쓰기 물리 범위'); current[stored] = type & 255; break;
                }
            }
        }
    };
    if (hooks.access) hooks.access(token, (fields, memory) => { load(fields, memory.buffer); });
    else load(need(hooks.stream)(token), hooks.buffer);
    depth((globals.loadingDepth - 1) >>> 0, globals, hooks);
}
// 정상 본문과 별개인 원문 cleanup의 현재 깊이 한 번 감소만 수행한다.
export function unwindFortTypeNamesLoad(globals, hooks) { depth((globals.loadingDepth - 1) >>> 0, globals, hooks); }
