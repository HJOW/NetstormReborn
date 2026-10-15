# AI 요구 에너지 공급 중단 인수인계 (2026-10-15)

현재 PC 및 마지막 선택 정밀 디컴파일 PC는 **VM-W11-Codex**이다. 원본 게임/GUI를 실행하지 않았으며 보호 원본을 수정하지 않았다. 사용자의 잠시 중단 요청에 따라 현재 선택 export/미등록 구현 단계에서 멈춘다. 이 단위는 **미완료·빌드 미실행·원본 실행 대조 0개**이며 기존 AI 생성자 160개/초기 갱신 133개에 추가 합산하지 않는다.

## 확인한 원문

- 읽기 전용 선택 export는 `tools/ghidra/type-required-energy-functions.json`의 `0049b0d0`, `0049c3b0`, `0049ebb0` 3개이며 종료 0으로 끝났다. 출력은 `extracted/type-required-energy/originals/{creation.c,functions.tsv}`이다. 새 Ghidra 작업은 남아 있지 않다.
- `0049c3b0`의 실제 속성 키는 **`mana`**다. `requiredEnergy` 키가 아니다. AST의 BT_STRING_CONSTANT를 검사하고 길이 9byte 이하를 assert한 뒤 TYPE 내부 **+0xa0**에 NUL까지 복사한다. 숫자 AST/9byte 초과는 원본 assert 뒤 잘못된 포인터 접근·이웃 필드 손상이 가능한 범위이므로 새 미등록 helper에서는 명시 예외 경계다.
- `0049ebb0`은 500byte TYPE 전부를 zero 초기화하고 theme=5, group=10을 넣는다. +0xa0은 NULL 포인터가 아니라 **내장 C 문자열**이다. 내부 타입 0..69와 자산 밖 빈 타입 186·187의 빈 문자열은 이 초기화에서 나온다.
- `0049b0d0`은 group!=10, flags2의 0x200000이 꺼짐, +0xa0 첫 byte=0일 때 기본 문자열을 만든다. 이미 저장된 `RiftTypeRecord.level`은 AST level-1이다. level>=0이면 원소 문자 1개, level==1이면 s 추가, level==2이면 같은 원소와 s 추가이며 level<0이면 빈 문자열이다. 그 뒤 명시 mana/group10/제외 플래그와 무관하게 모든 문자열 byte를 원본 CRT tolower에 통과시킨다.
- PE 원문 `00540ad8→0050c340`은 `wrtsWRTS.?>` 뒤 NUL이다. 새 helper는 ASCII 원문 범위를 복원하며 표 밖 원소 번호와 비ASCII CRT locale 동작은 명시 미복원 경계다.

## 남겨 둔 구현

- `cpppj/src/o/RawTypeRequiredEnergy.{h,cpp}`: `TypeRequiredEnergy(edition,currentType,definition)`로 기존 TYPE와 AST의 mana/default 후처리를 조회한다. 새로운 RiftTypeRecord 필드나 복제 TYPE 표는 추가하지 않았다. definition==nullptr은 내부/빈 타입의 실제 zero 입력을 뜻하며 반환은 항상 소유 문자열이다.
- `cpppj/src/client/AiTypeRequiredEnergy.{h,cpp}`: `BindAiTypeRequiredEnergy(assets,currentTypes)`로 실제 GameAssets.Types()[type-70].definition과 같은 현재 TYPE span을 getter에 묶는다. PATCH의 188 TYPE/116 자산을 검사한다. 두 참조는 getter보다 오래 살아야 한다.
- 위 **4개 파일은 CMake에 미등록**이며 Session/GameWorld/기존 AI 160·133 소스/API는 변경하지 않았다. 새 코드에 빌드 성공이나 원본 동일성 판정을 아직 부여하지 않는다. `cpppj/type-required-energy.json`은 이 상태와 파일 SHA만 기록한 인수인계 자료이며 native oracle 보고서가 아니다.

## 재개 순서

1. 새 helper가 현재 후처리 직후 TYPE 필드/원본 AST와 같은 의미인지 native `0049b0d0` 대조와 보호 자산 188개 공급 검사로 확인한다. group10/기본/명시/0x200000/level 경계를 포함하고 숫자 AST·길이 초과·비ASCII는 정상 반환 수에 넣지 않는다. 원본 malloc/포인터/OOM 동일성을 주장하지 않는다.
2. 이 helper는 매 조회 기존 TYPE와 AST로 후처리를 계산한다. 원본 +0xa0은 초기화 때 저장된 필드이므로, 나중에 level/theme/group/flags2를 별도로 바꾸는 경로가 생기면 getter 재계산과 실제 저장 필드의 수명을 다시 검토해야 한다. 현재는 미연결 상태이며 미래의 동일성을 보장한 것으로 처리하지 않는다.
3. root가 공용 빌드 freeze를 해제한 뒤 `AiBrainConstructorGlobals.invalidGraph`의 현재 `const uint32_t&`를 필수 `std::function<uint32_t()>` getter로 바꾸고 원본 +0x298/+0x2ac 쓰기 지점에서 각각 읽는다. Session의 실제 uint8_t 장부에 uint32_t 임시 값을 캐스팅해 참조시키면 수명 문제가 생긴다. 수정 뒤 기존160 fixture/SHA를 재생성하고 BYTE 원본 backing의 현재 읽기를 집중 검사한다. **이번 중단 시점에는 이 수정 미실행**이다.
4. 새 모듈 원본 대조/검사를 마친 뒤 CMake 등록·Release 집중 검사·문서/LEFT 갱신·단위 커밋을 root와 조율한다. Session에서는 생성자와 초기 갱신 모두 같은 getter를 받도록 조립한다. AI 전략 프레임은 별도 미복원 경계다.

## 이번 단계의 점검

선택 export 3개 종료0, 새 파일 UTF-8 해독/후행 공백/의도치 않은 한자 점검만 수행했다. PowerShell→Python stdin의 기본 인코딩으로 새 .cpp 2개와 이 문서의 한글이 `?`로 저장된 문제를 발견하여 Unicode를 직접 전달하는 apply_patch로 복구했다. C++ 빌드·단위 검사·새 native 실행은 미실행이다. 기존 AI 생성자/초기 갱신 자료는 freeze를 유지했다.
