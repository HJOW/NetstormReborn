# !color.dat 초기화 부모의 다음 최소 복원 범위

단위386 후속 **정적 설계·제품 미작성**이다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon /2026-10-23이다. 원본 게임·GUI·공용 빌드·x86 격리 실행을 하지 않았고 동결 Native 자료/기존 소스는 바꾸지 않았다. [PE 원시 바이트·명령·EH 근거](../../extracted/cpp-colorsort-input-20261024-current385/cache-parent-static.json), SHA256 `4855bdcf20d26c8e115aaa9b7a70e55894114acf76f666fbfb565e661ab14784`를 새로 보존했다.

## 확정된 외곽 순서

정밀C49436~49662와 PE `0043c4e0~0043c96a`(1163BYTE), 실제 catch `0043c96b~0043ca6a`를 읽었다. 최초 시작 호출자는 `004396e9`다.

1. `0043c4fe/503`에서 현재 `00540cf8`, `00540cec`를 이 순서로 읽는다. `0043c51e`의 `00459c60(-2,cec,-2,cf8,-3,"!color.dat",0)` 반환 경로를 한 번 보관한다. 두 전역의 의미를 임의 Config 키로 치환하지 않는다.
2. 경로 callback 복귀 뒤 `0043c523`에서 현재 `00555d88`을 **한 번** 읽고 그 raw ColorSort의 기록들을 사용한다. 화면 팔레트를 읽거나 primary를 매 색마다 다시 조회하지 않는다.
3. `005319c8~005319e7`의8DWORD에서 각 행 시작 때 lowBYTE 하나를 읽는다. 현재 값은84/249/184/140/150/112/156/222다. `00556538`과 `00556e38`에 각각8×256BYTE 표를 만든다. 뒤 표는 모든 색을 해당 행 색으로, 앞 표는 항등으로 먼저 쓴 뒤 기록의 `+8` double 명도가 `0.800000011920929`보다 엄격히 클 때만 행 색으로 바꾼다. 원문은 매 색의56BYTE를14DWORD 순방향 복사한 뒤 비교하며 NaN은 교체하지 않는다.
4. `0043c877`에서 `0043be30`의 실제 TYPE별 소유자 표 등록을 호출한다. 이것은 캐시 hit/miss와 무관하게 먼저 실행된다. 이미 복원한 일반/전용 소유자 표 계산을 재사용하되 위의 두8행 표를 그 다른 계산으로 대체하면 안 된다.
5. 지역 File24BYTE의 마지막 두DWORD만0으로 쓴다. `0043c891`: reader `0041b4c0(path,0,1)`→아래9개 순서의 `0041a7a0(target,256,ffffffff)`→정상 `0041b410` close다. 읽기 반환 길이는 부모가 검사하지 않는다. 정상 cachehit에서는 `0043c010`을 호출하지 않는다.

| 파일 offset | 전역 표 주소 | 고정 Input의9표 인덱스 |
|---|---|---|
|0|`00556038`|2|
|256|`00557738`|7|
|512|`00556438`|6|
|768|`00556238`|4|
|1024|`00556138`|3|
|1280|`00556338`|5|
|1536|`00555f38`|1|
|1792|`00555e38`|0|
|2048|`00557838`|8|

## 정확한 catch와 close 수명

`004f9c33`은 FuncInfo `00522240`을 사용한다. TryMap `0052222c`는 상태0~1과 catch 상태3을 기록하고 Handler `0052221c`는 TypeDescriptor `0052f460`의 **`.?AVEX_UNABLE_TO_OPEN_FILE@@`** 하나를 선택한다. 모든 파일/표 계산 오류를 잡는 catch-all이 아니다.

이 일치 예외에서만 catch `0043c96b`가 `0043c010`으로9표를 재생성하고, writer `0041b8d0(path,1,1)`→같은9개 순서의 `0041a910(source,256,ffffffff)`→`0041b410`을 호출한 뒤 `0043c95a`로 정상 복귀한다. 짧은 read나 일반 `std::runtime_error`를 이 분기로 바꾸지 않는다. writer의 실패를 같은 reader catch로 재시도하지 않는다.

UnwindMap `005221fc`의 상태1은 실제 funclet `004f9c20`(EBP-58 reader→0041b410), 상태3은 `004f9c28`(EBP-94 writer→0041b410)에 연결된다. 두 생성자 이전 상태0/2에는 outer close가 없다. reader 정상 close 전에는 상태0, writer 정상 close 전에는 상태2로 내린다. 따라서 미완성 생성자를 외곽에서 임의 close하거나 정상 close 실패에서 두 번 close하면 안 된다. close 자체가 unwind 도중 다시 예외를 던지는 경우도 실제 C++ EH 계약을 별도 확인해야 하며 빈 close로 덮지 않는다.

## 현존 장치와 제안 API

`o/BaseFile.h/.cpp`의 `BaseFileSystem::Read/TryRead`는 vector를 소유하는 VFS 읽기다. 파일 없음·TAFF 범위/서명·파일 읽기 실패는 일반 runtime_error이며, 정확한 `EX_UNABLE_TO_OPEN_FILE` 형이나 원문 File24BYTE의 생성/close 소유자는 없다. 같은 파일 경계를 조사한 SavedStateRead 담당자도 이를 확인했다. `RawSavedStateRead` 역시 open/read/close를 필수 hook으로 유지한다. 오류 문자열로 예외를 분류해서는 안 된다.

작은 신규 `RawColorMapCacheInitialize` 부모는 다음 필수 경계를 받는 안이 가능하다.

- 같은 장기 primary DWORD의 현재 getter와 반환 당시 raw0x3800BYTE 물리 빌림, 두8행 출력과 현재8DWORD 입력표, 실제9×256BYTE 표의 고정 span. 이 부모가 경로→primary 캡처→두 표→TYPE표→파일의 순서를 소유한다.
- 한 번 반환한 경로 할당의 수명을 reader/catch/writer 끝까지 보장하는 PathSpec 장치. 서로 다른 reader/writer24BYTE 지역 scratch의 미정 초기값도 caller가 명시 입력한다.
- 정확한 `EX_UNABLE_TO_OPEN_FILE`에 대응하는 별도 typed marker만 발생시키는 실제 file open/read/write/close 장치. 일반 BaseFile 오류 전체를 marker로 바꾸지 않는다. 최초 모듈은 reader/writer ctor와 같은 객체/token 수명, 부분 read/write를 각 요청 시점에서 필수로 요구한다.
- `0043be30` 실제 소유자 표 등록과 기존 `ClientColorSortInput::Rebuild(currentRed)`/43c010 계산의 필수 연결. full raw ColorSort/search 소유자를 고정 RGB 사본의 기본 성공으로 가장하지 않는다.

현재 실제 File 객체/typed 예외 장치가 없으므로 이 부모를 Client에 즉시 연결해 완료 처리할 수는 없다. 부모의 독립 복원에서는 그 경계를 명시 장치로 남기는 것이 가능하다. cache 쓰기의 실제 파일 경로는 보호 원본/CD에 자동 연결하지 않으며, 이후 root가 선택할 쓰기 가능한 작업 cache namespace와 같은 PathSpec 수명에 연결해야 한다.

## Native 선관측의 최소 분할

첫 작은 검증 단위는 `0043c4e0` 진입→두8행 표→실제 `0043be30` 호출 경계까지다. 경로와 TYPE표 등록은 명시 장치로 두고, source 물리/초기표/8DWORD 입력을 제품과 독립적으로 공급해 finite·같음·NaN·무한대·source 교체를 관측할 수 있다.

그 다음 normal cachehit은1163BYTE 본문에서 실제9read/close 순서와 짧은 read의 잔여를 고정한다. typed miss는 실제 catch256BYTE와 두 실제 unwind funclet을 각각 실행해 경계 순서를 먼저 고정할 수 있으나, 현재 oracle의 hook 중단을 MSVCRT의 실제 typed throw/dispatch 실행으로 표시해서는 안 된다. 분류는 위 EH 메타데이터 근거, catch/cleanup은 별도 명시 frame 실행이라는 범위를 보고서에 구별해야 한다. 필요하면 전체 `_CxxFrameHandler` 관측을 다음 하위 단위로 남긴다.

이번에는 위 정적 원문 범위와 API 설계까지만 완료했다. 첫 fixture/tool/report를 아직 만들지 않았으므로 Native 관측이나 제품 구현/테스트 통과로 세지 않는다. 최초 자료가 고정되기 전 제품을 작성하지 않는다.
