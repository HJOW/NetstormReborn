# CoreData의 인라인 SP 난수 writer 004be960

2026-10-24 **후속 단위386 독립 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. PATCH10.78 **004be960~004bea6a,267BYTE**를 신규 제품 작성 전에 독립 PE 관측으로 고정했다. 현재 정밀 C/functions.tsv에는 CD10.72 대응이 없으며 임의 주소를 확정하지 않았다. CD 몸체·게임·GUI·OS 시작은 실행하지 않았다.

**최초99입력×두x87정밀도=198관측(정상178/명시중단20),2257176명령과 C++ 전체 대조가 일치했다. 첫 격리 build1/실행 종료0·컴파일 경고/오류0,4검사/실패check0**이다. 원문 첫 실행 실패나 fixture 교정은 없으며 최초 tool/fixture/report를 제품 작성 뒤 다시 쓰지 않았다. 공유 기존 소스/CMake/LEFT/git·공용 build/lib·원본/CD에는 변경이 없다.

## 실제 원문과 신규 API

[신규 제품](../../cpppj/src/o/RawFortSpWrite.h)의 `WriteFortSp(stream,value,hooks)`는 같은 MemStream16BYTE와 caller의 float32 값을 받는다. `FortSpWriteHooks::nextRandom(limit)`는 같은 실제 `GameRandom.Next(limit)`를 빌린다. `stream`에는 기존 `MemStreamHooks`를 전달하며 실제 buffer/WriteCapacity 진단만 사용한다. 할당/free/확장이나 일반 `WriteMemStream` 호출은 없다.

원문은 두 번째 caller 인자를 `FLD DWORD`로 읽어 실제 **004e49c0~004e4a34,117BYTE _ftol**을 호출하고 EAX 하위DWORD만 보관한다. 정밀 C의 복원 서명에는 이 float 인자가 빠져 있으므로 PE/caller를 우선했다. float32 입력을 signed64로0방향 절삭한 하위32비트이며, masked invalid인 NaN/Inf/signed64 범위 밖은 정수 불확정값의 하위DWORD0이다. −2^63은 유효 signed64 입력이다. C++는 유한/범위를 확인한 뒤 안전한 변환을 수행한다. 원문 FPU 예외 마스크를 해제한 환경의 예외·status flag 전체 동일성은 지원 계약이 아니다.

각 bit0..31은 `integer & (1<<bit)`에서 시작하고 같은 **Next(32)를 정확히16번**, 정상 한 호출에서 총512번 소비한다. 난수 결과가 현재 bit와 같으면 **OR만 생략**한다. 다른 경우 x86 SHL의하위5bit로 잡음을 OR한다. bit0..7은 BYTE8개,bit8..15는 WORD8개,bit16..31은 DWORD16개를 직접 써 **총88BYTE**다. 숫자 하나를4BYTE로 쓰거나 일반 stream writer로 묶는 방식과 다르다.

## 인라인 MemStream 순서와 alias

매 bit의16난수가 끝난 뒤 해당 폭의 `pos+width`를 DWORD로 감기게 계산하고 alloc과 **signed 비교**한다. 원문 진단은 `pos+length <= alloc`, `c:\ns\o\memstream.h`, **line36**이다. 진단이 복귀하면 현재 pos/data를 다시 읽어 같은 BYTE/WORD/DWORD를 쓴다. 그 출력 자체가 stream 필드를 바꾸었을 수도 있으므로 **다시 현재 pos를 읽어** 다음 위치를 구하고, 현재 size를 pos store 전에 읽어 signed 최대값 관문으로 확장한다. 일반 writer를 호출하거나 진단 앞의 pos/data/size 사본으로 진행하지 않는다.

같은 물리 버퍼가 stream16BYTE 자체와 겹쳐도 분리하지 않는다. 신규 대조는 세 번째256BYTE 배열의 **같은 첫16BYTE**를 stream으로 빌리고, 그 배열 자체를 data 버퍼로 선택한 원문 입력을 포함한다. 따라서 size/alloc에 대한 출력과 부모의 다음 재조회가 실제 같은 BYTE에 작용한다. 버퍼 getter는 같은 살아 있는 할당의 순수 span을 반환해야 하며 숨은 필드 변경 callback을 추가하는 장치가 아니다.

필수 장치는 도달한 위치에서만 요구한다. 첫 buffer 또는 선택된 진단이 없어도 이미 소비한16난수는 되돌리지 않는다. 물리가 짧아 세 번째 BYTE를 쓸 수 없으면 앞두BYTE·pos/size2·48회 난수 효과를 보존한다. 범위 밖 host 접근은 out_of_range이며 원본 접근위반/OS 예외 처리의 동일성이라고 주장하지 않는다.

## Native 선관측과 독립 검사

[도구](../../tools/decomp_fort_sp_write.py)는 제품h/cpp·검사 세 파일이 없는 것을 먼저 확인했다. 부모267BYTE와 실제 _ftol117BYTE, **004558c0~004558e7 GameRandom40BYTE**를 모두 CPU 실행했다. 난수 결과나 난수 상태 전이를 별도 기대 계산기로 대체하지 않았다. 외부 장치는 line36 진단뿐이며 그 callback의 현재 필드/난수 상태 변경·명시중단을 입력으로 제공했다.

float32 입력은 ±0/최소 subnormal/양음수 소수/65535/32비트 감김/±2^63 경계/Inf/quiet·signaling NaN 비트를 포함한다. 초기 난수0/55/ffffffff, 용량의 signed 경계, 이전 size의 signed 경계, BYTE→WORD→DWORD 경계의 현재 data/pos/size/alloc 변경과 각 단계의 부분중단도 포함한다. 정상반환은512난수·비휘발 레지스터·cdecl stack·FPCW/TOP 균형을 확인했다. 두 정밀도(027f/037f)의 같은 입력별 전체 결과가 같다. 난수가 해당 bit와 같은 **2658회**를 실제 관측했다.

[고정 fixture](../../cpppj/tests/fixtures/fort-sp-write-x86.tsv)는 세 실제256BYTE 전체·_ftol EAX·현재 RNG DWORD·난수/진단/동일bit 횟수·전체 호출 trace와 명령 수를 보존한다. [최초 report](../../cpppj/fort-sp-write-native.json)는 원본/정밀 소스/선행 장치/도구/fixture SHA와 제품 전 관측 사실을 기록한다. `--verify`는 이 최초 SHA만 확인하며 덮어쓰기를 허용하지 않는다.

[신규 검사4개](../../cpppj/tests/FortSpWriteTests.cpp)는 다음을 구분한다.

- 최초198행의 정상178/부분중단20 전체 물리·RNG·난수/진단 trace를 대조한다. 변환값은 안정된 출력의 보호bit를 실제88BYTE에서 모아 Native EAX와 비교하며 가짜 변환 hook를 넣지 않는다. buffer getter는 같은 배열의 span이다.
- 명시 난수 장치가 매번 현재 bit를 반환하는 별도 검사로512회 소비·OR 생략·각1/2/4BYTE 폭·88BYTE 밖 보존을 확인한다. 이 장치를 실제 게임 난수라고 주장하지 않는다.
- 선택된 buffer/진단/난수 장치 누락에서 앞선 효과를 보존한다.
- 짧은 실제 버퍼에서 앞선 BYTE/pos/size/난수 효과가 남는 host 물리 계약을 확인한다.

격리 빌드는 신규 부모·기존 `SpStore.cpp`·신규 검사·작은 runner만 MSVC14.51.36231/SDK10.0.26100.0의 `/W4 /permissive- /utf-8`로 단일 컴파일했다. `RawMemStream.cpp`를 대신 호출하지 않고 공용 정적 라이브러리를 읽거나 링크하지 않았다. 첫 build/test 로그와 실행파일은 `extracted/cpp-fort-sp-write-20261024-isolated/`에 보존한다.

root가 별도로 제품h/cpp·검사·문서와 실제267BYTE PE를 읽기 대조해 명백한 오류가 없음을 확인했다.16난수 선행·원값 비교 뒤SHL mask·BYTE/WORD/DWORD의 필드 읽기 순서·진단 뒤현재값·출력 뒤pos/end·masked_ftol 하위DWORD 계약을 확인한 읽기 검토이며 추가 Native/공용 빌드·게임 실행을 뜻하지 않는다.

## 실제 Session 접합의 남은 범위

기존 `FortCoreWriteHooks::sp(stream,value)`의 세 호출은 고정 Player의 byte+70/word+6c/word+6e를 각 시점에 다시 읽은 float32다. 이 신규 writer를 연결할 때는 **반환 당시 같은 TemplateWriteStream**을 기존 `CampaignTemplateStorage::AccessSection`으로 동기 빌리고, 그 본문 안에서 받은 fields/`MemStreamHooks`와 같은 Session/Client `GameRandom.Next`를 전달해야 한다. span이나 hooks의 실제 물리 빌림을 AccessSection 밖으로 탈출시키지 않는다. 일반 WriteSection으로 결과 배열을 다시 복사하는 우회는 동일 alias가 아니다.

공용 등록 대상은 `src/o/RawFortSpWrite.cpp`, `tests/FortSpWriteTests.cpp`이며 **`NETSTORM_FORT_SP_WRITE_FIXTURE`**에 `tests/fixtures/fort-sp-write-x86.tsv` 절대경로를 공급한다. 기존 `SpStore.cpp`가 필요하다. 실제 CoreWrite 부모 접합/Client SP 장기 수명/Player·Chat 정리 및 전체 Snapshot·Editor 최종loaded는 이번 독립 모듈 검사로 완료했다고 세지 않는다. 최종 SHA와 검증 범위는 [manifest](../../extracted/cpp-fort-sp-write-20261024-validation.json)에 고정한다.
