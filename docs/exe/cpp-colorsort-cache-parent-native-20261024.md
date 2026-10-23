# !color.dat 부모·catch·cleanup의 독립 복원

단위386 후속 **미등록 모듈**이다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon /2026-10-23이다. 기존 공유 제품/CMake/LEFT/git/공용 build/lib·원본/CD·GUI·게임은 변경하거나 실행하지 않았다. [이전 정적 설계](cpp-colorsort-cache-parent-next-20261024.md)와 [현재385 ColorSort 사본](cpp-colorsort-input-current385-20261024.md)을 바꾸지 않고 다음 자료를 추가했다.

## 제품 전에 고정한 원문 범위

[도구](../../tools/decomp_colorsort_cache_parent.py)·[최초 fixture](../../cpppj/tests/fixtures/colorsort-cache-parent-x86.tsv)·[원문 보고서](../../cpppj/colorsort-cache-parent-native.json)를 먼저 만들고 첫 관측을 고정한 뒤 C++를 작성했다. 실제 PE 부모 `0043c4e0`1163BYTE, 독립 catch `0043c96b`256BYTE, reader cleanup `004f9c20`8BYTE, writer cleanup `004f9c28`11BYTE를 실행했다.

65입력×두 FPU control의 **130관측**, 실제4,753,814명령이며 두 정밀도 출력 차이는0이다. prefix checkpoint8·정상 반환76·장치 stop26·정확 형식 전파10·다른 오류 전파10이다. 실제 독립 cleanup은36회, 외부 장치가 분류해 catch에 넘긴 typed 분기는6회다.

**전체 MSVCRT typed throw/dispatch는 실행하지 않았다.** `EX_UNABLE_TO_OPEN_FILE`의 형식과 상태0~1의 catch 선택은 기존 PE의 FuncInfo/TryMap/Handler/TypeDescriptor 정적 근거다. 실제 body에서 예외를 선택한 장치는 CPU 실행을 멈추고 같은 EBP로 필요한 실제 cleanup을 독립 호출한다. 일치 형식만 별도 분류 장치가 실제 catch entry에 넘긴다. 이 분류 과정 자체를 원문 CRT 실행 관측으로 세지 않는다. reader/writer constructor·파일 read/write/close·PathSpec·TYPE 소유자 표 등록·43c010의9표 계산은 명시 하위 장치이며 해당 큰 몸체의 복원 완료를 주장하지 않는다.

원문 도구는 첫 관측 이후 덮어쓰기를 거부하고 `--verify`로 최초 SHA만 확인한다. tool/fixture/report는 이후 제품이나 기대에 맞춰 수정하지 않았다.

## 새 모듈의 실제 순서와 API

[RawColorMapCache.h](../../cpppj/src/o/RawColorMapCache.h)/[cpp](../../cpppj/src/o/RawColorMapCache.cpp)는 다음 한정 부모만 소유한다.

- `PrepareColorMapCache`: PathSpec 반환→그 뒤 현재 primary 물리 한 번 빌림→각 행의 현재8DWORD lowBYTE→solid/항등을 먼저 저장→56BYTE14DWORD 복사→기록+8 double이 `0.800000011920929`보다 엄격히 큰 경우 교체→실제 TYPE 소유자 표 등록을 요청한다. 같음/NaN은 교체하지 않는다. 같은 source가 현재 Screen RGB로 바뀌지 않는다.
- `InitializeColorMapCache`: 위 prefix 뒤 reader24BYTE 마지막8만0으로 쓴다. reader `(path,0,1)`→정확한 파일 순서9read→close다. read 반환 길이를 검사하지 않으며 짧은 읽기의 잔여를 보존한다. ctor가 정상 복귀한 뒤에만 cleanup한다.
- `RegenerateColorMapCache`: 정확 `ColorCacheUnableToOpen` marker에서만 선택하는 catch 본문이다. 9표 생성→writer `(path,1,1)`→같은 순서9write→close다. **writer24BYTE를 parent가 미리0으로 쓰지 않는다.** writer/catch의 예외는 원래 reader catch를 재진입하지 않는다.
- `CloseColorMapCacheFile`: 같은24BYTE this를 필수 close 장치에 전달한다. 정상 close 호출 전에는 constructed 관문을 내리므로 그 호출이 실패해도 같은 객체를 다시 close하지 않는다.

Global은 현재 primary의 raw 물리 getter와 실제8DWORD 입력, 두2048BYTE/아홉256BYTE 출력 span이다. 같은 경로 전체 물리 span은 한 번 반환받아 reader와 writer 양쪽이 빌린다. callback이 같은 경로 할당을 수정하면 writer도 현재 NUL 이름을 읽는다. 원문 외곽 PathSpec의 현재 `00540cf8/00540cec` 인수 보존은 실제 외부 장치의 책임이다.

호출자는 서로 다른 reader/writer24BYTE 지역 물리와 실제 장치 소유자를 제공해야 한다. 현재 BaseFileSystem의 vector 읽기나 일반 runtime_error는 이 정확한 File 객체/예외 형식의 대체품이 아니다. 실제 파일을 열거나 캐시를 쓰는 장치는 이 모듈에 기본 제공하지 않으며 원본/CD 쓰기로 자동 연결하지 않는다. 현재 raw ColorSort 전체 소유자·TYPE별 등록/표 계산·읽기/쓰기 가능한 작업 cache namespace의 실제 접합은 후속이다.

## 정상 close 실패와 이중 예외의 구별

정상 reader close는 Native state가 이미0이고 writer close는2다. 최초 fixture의 `closeR-stop/unable/other`, `closeW-stop`은 이 정상 close 실패를 별도로 관측한다. 정확 reader marker는 그 상태에서도 catch로 들어가지만 추가 reader cleanup은 없다.

반면 **unwind 중 실행한 close가 다시 예외를 던지는 입력은 지원 계약 밖**이다. 최초130관측은 첫 예외 뒤 cleanup 성공만 포함한다. 모듈의 장치는 unwind 중 close에서 두 번째 예외를 던지지 않아야 한다. 계약 밖에서 두 번째 marker가 C++ outer catch로 흐를 수 있는 결과를 원문 MSVCRT double-exception 정책으로 동일 단정하지 않는다. 전체 CRT 정책을 임의 구현하거나 첫 Native 자료를 바꾸지 않았다.

root의 읽기 검토 후 이 한정 계약을 header 주석에 추가했다. 제품 제어 흐름/최초 Native는 그대로이며 정상 close 관문 하강과 unwind close의 두 상황을 구별해 기록한다.

## 격리 검증과 후속 연결

[ColorMapCacheTests.cpp](../../cpppj/tests/ColorMapCacheTests.cpp)는 Native130개 전부의 두8행4096BYTE·9표2304BYTE·두 File48BYTE·실제 write 출력·현재 source/owner 입력·호출 순서·분류/cleanup 횟수를 대조한다. 정상 cachehit에서 미선택 buildMaps/writer 장치를 요구하지 않는 자연 관문과 raw source 물리 실패 전에 이미 기록한 첫 solid/항등 BYTE도 별도 검사한다. 명시 물리 거부를 Native NULL 메모리 성공으로 표현하지 않는다.

신규 모듈 cpp·검사·Runner만 직접 컴파일/링크한 [격리 build1](../../extracted/colorsort-cache-parent-20261024-isolated/build1.log)은 종료0·관측 경고/오류0, [검사3개](../../extracted/colorsort-cache-parent-20261024-isolated/test1.log)는 실패0이다. 공용 라이브러리나 기존 build 디렉터리를 읽거나 쓰지 않았다. 첫 도구 `--verify`도130관측의 최초 SHA 일치로 통과했다. root의 읽기 검토는 기본 부모 순서에 명백한 오류를 찾지 못했으며 전체 EH/OS 장치 검증으로 확대하지 않는다.

실제 공용 등록은 하지 않았다. root가 후속에 module cpp·검사 cpp·fixture를 등록할 수 있지만 원문 기본 Source/9표/TYPE 등록과 정확한 File 장치를 연결하기 전 전체 ColorSort startup 완료로 세지 않는다. 보존 경로·최종 SHA는 [verification.json](../../extracted/colorsort-cache-parent-20261024-isolated/verification.json)에 기록한다.
