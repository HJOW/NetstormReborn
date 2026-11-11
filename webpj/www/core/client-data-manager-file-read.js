// DataManager 의 전체 파일 읽기(원문 41baa0 호출)를 웹 가상 파일 계층에 연결하는 client 어댑터이다(cpppj `client/DataManagerFileRead.cpp/.h` 의 웹 대응, 분류 adapt).
// cpppj 는 BaseFile 의 FILE·키·scratch 해독을 실제 CRT 로 연결하지만, 웹은 `GameFiles.read`(느슨한 파일 → 아카이브 조회, 해독 완료된 바이트)로 대체한다. 보존하는 계약은 다음과 같다:
//  - 파일 없음(조회 실패)은 null 이다. 길이 0 인 존재 파일도 **최소 4BYTE 할당을 소유**한다(원본 41baa0 의 최소 할당; 빈 파일을 새 파싱 오류나 삭제로 주장하지 않고 정상 false 정리로 이어짐).
//  - 반환 블록은 { storage, length } 이다: storage 는 DataManager 가 반납할 때까지 유지하는 물리 할당(Uint8Array, 길이는 max(length, 4)이며 논리 길이 뒤의 미작성 padding 은 0 이지만 원문 미정 바이트의 관측값으로 주장하지 않음),
//    length 는 원문 41baa0 의 별도 출력 길이로 storage.length 와 같다고 가정하지 않는다. storage 는 파일 원본 바이트의 사본이라 변경해도 보관 파일에 영향이 없다.
//  - 이름은 첫 NUL 까지만 쓴다(원문 C 문자열). 경로 형식(역슬래시/슬래시, 대소문자)은 GameFiles 가 같은 이름으로 취급한다.
// bindDataManagerFileRead(hooks, files)는 실제 Native caller 가 명시한 경우에만 `readWhole` 을 설치하며 기존 독립 read 와 다른 훅은 그대로 둔다(원본 BindDataManagerFileRead 와 같은 역할).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E284: 4-B DataManagerFileRead 웹 어댑터)

// 원본 41baa0 이 allocate 하는 최소 크기이다. (2026-11-11 WB04-E284)
const MINIMUM_ALLOCATION = 4;

// 가상 파일 계층에서 전체 파일을 읽어 최소 4BYTE 의 소유 블록으로 돌려준다. files 는 GameFiles(read: 없으면 null), name 은 문자열이다. 미발견이면 null 이다. 사용: readDataManagerFileImage(files, 'd/map.fort') → { storage, length } 또는 null. (2026-11-11 WB04-E284)
export function readDataManagerFileImage(files, name) {
    if (!files || typeof files.read !== 'function') throw new TypeError('DataManager 전체 읽기에는 가상 파일 계층(read)이 필요합니다.');
    const nul = String(name).indexOf('\0');
    const path = nul < 0 ? String(name) : String(name).slice(0, nul);
    const bytes = files.read(path);
    if (bytes === null || bytes === undefined) return null;
    // 논리 길이와 물리 할당을 분리한다: 길이 0 이어도 최소 4BYTE 를 소유하고 사본이라 원본 파일 바이트에 영향이 없다.
    const storage = new Uint8Array(Math.max(bytes.length, MINIMUM_ALLOCATION));
    storage.set(bytes);
    return { storage, length: bytes.length };
}

// 실제 Native caller 의 readWhole 만 설치한 새 훅 묶음을 돌려준다(기존 훅과 독립 read 는 그대로). 사용: bindDataManagerFileRead({ read, typeCount, … }, files). (2026-11-11 WB04-E284)
export function bindDataManagerFileRead(hooks, files) {
    return { ...hooks, readWhole: (name) => readDataManagerFileImage(files, name) };
}
