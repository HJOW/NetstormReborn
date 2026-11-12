// 미션이 선택한 같은 파일 읽기의 원문 이미지와 실제 Template 파일 이름을 함께 전달하는 값 모양을 옮긴 모듈이다(cpppj `client/MissionFortSource.h`, 헤더만, 10.78/웹 공통).
//  - 논리 미션 이름(예: `Tutorial1`)과 저장에 쓰는 파일 경로의 수명을 분리한다. 한 번 선택한 fortSpec 경로와 그 읽기 결과를 한 값으로 묶어 재조회로 서로 다른 파일을 섞지 않는다.
//  - 사용: `createMissionFortSourceImage(image, fileName)` → `{ image: Uint8Array, fileName: string }`(원본 `MissionFortSourceImage`),
//          `createMissionFortFileSelection(fileName, read)` → `{ fileName, read }`(원본 `MissionFortFileSelection`; read 는 호출할 때만 같은 파일을 읽는다).
//    함수 형식 `MissionFortFileReader`(논리 이름 → 이미지/경로)와 `MissionFortFileSelector`(논리 이름 → 선택)는 별도 타입 없이 일반 함수로 둔다.
//  - 웹은 파일 시스템이 없어 `fileName` 이 `./data/` 아래 자산 경로 문자열이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E337: 4-D B 구획 MissionFortSource 이식)

// 한 번 선택한 경로의 이미지와 이름을 묶는다. 이미지는 호출자가 소유한 복사본이어야 하며 이 함수는 복사하지 않는다. 형식이 틀리면 예외이다. (2026-11-12 WB04-E337)
export function createMissionFortSourceImage(image, fileName) {
    if (!(image instanceof Uint8Array)) throw new TypeError('Fort 원문 이미지는 Uint8Array 여야 합니다');
    if (typeof fileName !== 'string') throw new TypeError('Fort 파일 이름은 문자열이어야 합니다');
    return { image, fileName };
}

// 파일을 열기 전에 같은 manager 이름을 비교할 수 있도록 선택(이름)과 읽기(함수)를 나눈다. read 는 호출될 때만 같은 파일을 읽는다. (2026-11-12 WB04-E337)
export function createMissionFortFileSelection(fileName, read) {
    if (typeof fileName !== 'string') throw new TypeError('Fort 파일 이름은 문자열이어야 합니다');
    if (typeof read !== 'function') throw new TypeError('Fort 파일 선택의 읽기는 함수여야 합니다');
    return { fileName, read };
}
