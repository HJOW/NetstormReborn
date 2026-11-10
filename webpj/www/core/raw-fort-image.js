// 현재 Template 의 35개 원문 섹션을 PATCH DataManager SaveImage 형식으로 직렬화하는 모듈이다(cpppj `o/RawFortImage.cpp/.h`, 원본 004bf520 → 0044d640 / 004bcd80, 기준선 1f00a25a 와 같음).
// 이미지: 첫 바이트 0x46, 둘째 바이트 파일 flag(0xfe 가 아니면 0), 이어서 고정 35 이름 순서로 [길이 WORD(자기 2바이트 포함, 낮은 바이트 먼저) + payload] 를 쓴다. 없는 tail 섹션도 빈 stream 의 길이 2 를 쓴다.
// 원본 0x8000 메모리 stream 의 고정 배열 전체를 0 으로 시작하고 논리 끝(end)과 할당의 0 tail 을 구별한다. 이전 길이 필드는 다음 섹션 진입 직전과 마지막 반환 직전에 수정한다.
// 진단 3종은 원본이 선택된 위치에서만 알린 뒤 계속하는 조건이다: 현재 타입 개수 signed >= 256, stream 길이 필드 위치 자체가 용량을 넘음(알린 뒤 물리 범위 거부), payload 가 남은 공간 이상(엄격한 '<' 진단,
//   정확히 32768 인 정상 물리 쓰기는 막지 않고 초과만 거부). 해석된 Money/Technology 필드나 예전 파일 바이트로 대체하지 않고 현재 raw 섹션 끝까지 원문 그대로 쓴다. 10.78(patch1078) 전용이다.
// hooks = { typeCount() → 현재 00541348, assertion(종류), signatureFlag()? (실제 manager 의 서명 flag), section(index)? (실제 manager 의 현재 섹션) } — 뒤 둘은 선택이며 없으면 값형 입력 fort.flag / fort.sections 를 쓴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E277: 4-B Fort 이미지 직렬화 이식)

// 원본 0044d640 이 실제 memstream 에 확보하는 고정 물리 크기이다. (2026-11-10 WB04-E277)
export const FORT_IMAGE_CAPACITY = 0x8000;

// 고정 섹션 개수(FortTemplate::kSectionCount, SECTION_NAMES 길이)이다. (2026-11-10 WB04-E277)
export const FORT_IMAGE_SECTION_COUNT = 35;

// 원본 SaveImage 의 진단 후 계속 조건이다. (2026-11-10 WB04-E277)
export const FORT_IMAGE_ASSERTION = Object.freeze({ typeCount: 'TypeCount', streamCapacity: 'StreamCapacity', sectionCapacity: 'SectionCapacity' });

// 선택된 진단 소비자가 없으면 빈 성공으로 계속하지 않고 거부한다. (2026-11-10 WB04-E277)
function assertion(hooks, kind) {
    if (typeof hooks.assertion !== 'function') throw new Error('Fort 이미지의 실제 선택 진단 미연결');
    hooks.assertion(kind);
}

// 길이 WORD 를 낮은 바이트부터 쓴다. 값은 자기 2바이트를 포함한다. (2026-11-10 WB04-E277)
function writeLength(bytes, offset, length) {
    bytes[offset] = length & 255;
    bytes[offset + 1] = (length >> 8) & 255;
}

// 직렬화 결과이다: storage 는 32768 바이트 0 초기화 후 현재 섹션을 쓴 단일 배열, end 는 원본 stream+8 의 논리 끝이다. (2026-11-10 WB04-E277)
export class FortImageData {
    constructor(storage, end) {
        this.storage = storage;
        this.end = end;
    }

    // 실제 이미지 끝까지만 빌린다. 배열 밖으로 변조된 end 는 거부한다. (2026-11-10 WB04-E277)
    bytes() {
        if (this.end > this.storage.length) throw new RangeError('Fort 이미지의 논리 끝이 물리 배열을 초과함');
        return this.storage.subarray(0, this.end);
    }
}

// 사용: new RawFortImage('patch1078', fort, { typeCount, assertion }).save() → FortImageData. fort 는 { sections[], flag } (parseFort 결과 모양)이며 저장이 끝날 때까지 살아 있어야 한다. (2026-11-10 WB04-E277)
export class RawFortImage {
    // 미확정 CD 의 비슷한 몸체나 누락된 현재 타입 개수를 추정하지 않는다. (2026-11-10 WB04-E277)
    constructor(edition, fort, hooks) {
        if (edition !== 'patch1078' || typeof hooks?.typeCount !== 'function' || fort.sections.length > FORT_IMAGE_SECTION_COUNT) throw new TypeError('Fort 이미지의 PATCH 현재 Template/타입 개수 계약 오류');
        this.fort = fort;
        this.hooks = hooks;
    }

    // 004bf520 → 0044d640/004bcd80: F/flag + 35개 길이 WORD/현재 payload/마지막 길이 수정. (2026-11-10 WB04-E277)
    save() {
        const storage = new Uint8Array(FORT_IMAGE_CAPACITY);
        // 원본 004bcd8f 의 signed JL 관문이다.
        if ((this.hooks.typeCount() | 0) >= 256) assertion(this.hooks, FORT_IMAGE_ASSERTION.typeCount);
        storage[0] = 0x46;
        storage[1] = this.hooks.signatureFlag ? this.hooks.signatureFlag() & 255 : this.fort.flag === 0xfe ? 0xfe : 0;
        let position = 2;
        let previous = FORT_IMAGE_CAPACITY;
        // 고정 35 이름 순서로 없는 tail 섹션도 실제 빈 stream 의 길이 2 를 쓴다.
        for (let index = 0; index < FORT_IMAGE_SECTION_COUNT; index += 1) {
            // 원본 검색/current 는 이전 길이 수정에 앞서 수행한다.
            const section = this.hooks.section ? this.hooks.section(index) : index < this.fort.sections.length ? this.fort.sections[index] : new Uint8Array(0);
            if (previous !== FORT_IMAGE_CAPACITY) writeLength(storage, previous, position - previous);
            if (position > FORT_IMAGE_CAPACITY - 2) {
                assertion(this.hooks, FORT_IMAGE_ASSERTION.streamCapacity);
                throw new RangeError('Fort 이미지 길이 필드의 물리 범위 초과');
            }
            previous = position;
            writeLength(storage, position, 0);
            position += 2;
            if (section.length >= FORT_IMAGE_CAPACITY - position) assertion(this.hooks, FORT_IMAGE_ASSERTION.sectionCapacity);
            if (section.length > FORT_IMAGE_CAPACITY - position) throw new RangeError('Fort 이미지 payload의 물리 범위 초과');
            storage.set(section, position);
            position += section.length;
        }
        writeLength(storage, previous, position - previous);
        return new FortImageData(storage, position);
    }
}
