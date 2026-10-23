# Dot 명령 등록 부모의 signed 관문과 count 캡처 교정

2026-10-24 **후속 단위386 독립 준비**다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. PATCH10.78 **0043f660~0043f6ec,141BYTE**와 정밀 C를 읽기 대조해 최초 구현의 두 차이를 교정했다. CD10.72 정밀 대응은 **0043ee80**이며 이번에 CD 몸체를 실행하지 않았다. 실제 Client 명령 registry 접합·Chat 전체 초기화·게임/GUI 실행은 이 작업 범위가 아니다.

**최종 격리 build4 종료0·컴파일 경고/오류0,4검사/실패check0**이다. 기존 Native250관측의 전체 표/trace가 그대로 통과했고, 새 보충 PE12관측 중 지원된 정상/표 끝 중단8관측의 전체2568BYTE/trace도 일치했다. 음수4관측은 별도 외부 물리 경계로 보존하고 제품 성공 대조에 합치지 않았다. 공유 기존 소스/CMake/LEFT/git·공용 build/lib·원본/CD·GUI에는 변경·실행이 없다.

## 교정 전 자료와 실패 재현

최초 `tools/decomp_dot_command_register.py`, `dot-command-register-x86.tsv`, `cpppj/dot-command-register-native.json`은 수정하지 않았다. 최초 **125입력×2=250관측(정상248/복사중단2),5144명령**과 정밀도 차이0은 그대로다. 원문 부모를 CPU 실행하고 strncpy31은 명시 외부 장치로 둔 관측이다. count0/1/63/64/65와 기존 복사 뒤 count0/2/63/64 입력은 아래 두 차이가 있어도 결과가 같아 최초2검사를 통과했다.

교정 전 제품h/cpp·검사·최초 tool/fixture/report·최초 격리 실행파일/obj/runner/log를 **13파일 그대로** `extracted/cpp-dot-command-register-20261024-before-correction/`에 복사하고 SHA를 보존했다. 이 원시 사본과 첫2검사 통과 기록은 교정 후 결과로 덮어쓰지 않는다.

보충 Native를 먼저 고정하고 새 검사 두 개를 추가한 뒤 **교정 전 제품**으로 별도 MSVC build3을 실행했다. 컴파일 종료0·경고/오류0이며 실행은 **4검사/11개 실패check/종료1**이었다. 기존250 대조는 통과하고 count65 부분효과 관련8check와 음수 명시 거부3check가 실패했다. 이 실행파일과 `test-before.log`를 보존한 뒤 제품 두 지점을 수정했다. 교정 후 성공을 위해 fixture/검사 기대를 바꾸지 않았다.

## 실제 PE가 보관하는 값

`0043f660 mov eax,[00532534]`→`cmp eax,40`→`jl`은 **signed count<64** 관문이다. 최초 uint32 `>63` 구현은 상위비트가 켜진 값을 자연0 반환으로 처리했다. 교정 제품은 같은 최초DWORD를 int32 비트로 해석해 signed 관문을 적용한다. 음수 초기count의 원시32비트 주소감김은 지원 host span의 계약 밖이므로 **out_of_range로 명시 거부**한다. 음수의 원본 실행 전체를 동일하게 복원하거나 모두 OS 접근위반이라고 주장하지 않는다. signed양수64 이상은 외부 복사 장치가 없어도 원문처럼 자연0이다.

`0043f683 mov ecx,[00532534]`는 strncpy가 복귀한 뒤 count를 **한 번** 읽는다. 이후 callback표 쓰기→이름32번째BYTE0→visible표 쓰기→ECX 증가→count 쓰기→증가한ECX의 visible종단0까지 같은 ECX를 사용한다. 최초 제품의 각 store별 count 재조회는 실제 명령 순서와 달랐다.

복사 뒤 count64이면 callback/NUL/visible 쓰기를 거쳐 count65를 쓰고 visible65 종단이 **같은 count DWORD를0으로 덮는다**. 이 기존 관측은 계속 보존한다. 복사 뒤 count65이면 visible65가 먼저 count 저장소를 pointer값으로 덮지만, 원문은 보관한 ECX65를 증가해 **count66을 다시 쓴다**. 최초 제품은 중간에 덮인 pointer를 다시 읽어 증가시켜 이 부분효과가 달랐다. 교정은 복사 직후 `selected`를 한 번 캡처하고 모든 직접 쓰기와 `updated=selected+1`에 계속 사용한다.

## 별도 v2 선관측의 정확한 범위

[보충 도구](../../tools/decomp_dot_command_register_v2.py)는 기존 도구의 PE/strncpy 명시장치를 재사용하고, 교정 전 제품 SHA가 보존 사본과 같은 것을 검사한 뒤 새 [fixture](../../cpppj/tests/fixtures/dot-command-register-v2-x86.tsv)와 [report](../../cpppj/dot-command-register-v2-native.json)를 처음 고정했다. **6입력×2=12관측/284명령**, 정상4·명시표밖쓰기중단4·음수복사인자중단4이며 두 정밀도 차이0이다. 최초 자료 덮어쓰기는 거부한다.

copy-count65의 중단은 **CPU/OS 접근위반이 아니라 명시2568BYTE 표 계약을 넘는 `00532538` DWORD0 쓰기 직전의 관측 장치 중단**이다. 해당 주소의 실제 PE 메모리가 유효하지 않다고 주장하지 않는다. 이 중단 앞의 전체 표와 count66 효과를 제품의 같은 span 범위 검사 직전 부분효과와 대조했다. 최초 도구/fixture/report는 새 범위를 넣기 위해 고쳐 쓰지 않았다.

초기count−1은 원문 signed 관문을 통과하고 표 앞 `00531b10`의 strncpy 인자 경계까지 도달한다. v2는 실제 CRT가 그 주소에 쓰기 전에 중단한다. 제품은 음수 원시주소 산술을 앞에서 명시 거부하므로 외부 복사 호출 trace까지 동일한 성공이라고 세지 않는다. 별도 검사에서 같은 입력의 표 보존·복사 미호출·out_of_range와 signed양수 상한의 자연0을 구분한다. 상위비트 입력0x80000000/0x80000001의 추가 host 검사도 원문 전체 실행 fixture라고 포장하지 않는다.

## 격리 실행과 등록

추가 `DotCommandRegister_V2CapturedCountPartialEffects`와 `DotCommandRegister_NegativeCountIsExplicitPhysicalContract`를 포함해 총4검사다. 첫 두 검사/최초250관측은 그대로 보존한다. 제품은 [RawDotCommandRegister.h](../../cpppj/src/client/RawDotCommandRegister.h)/[cpp](../../cpppj/src/client/RawDotCommandRegister.cpp), 검사는 [DotCommandRegisterTests.cpp](../../cpppj/tests/DotCommandRegisterTests.cpp)다.

공용 등록 시 제품 `src/client/RawDotCommandRegister.cpp`, 검사 `tests/DotCommandRegisterTests.cpp`, **`NETSTORM_DOT_COMMAND_REGISTER_FIXTURE`**와 **`NETSTORM_DOT_COMMAND_REGISTER_V2_FIXTURE`**에 각각 최초/보충 TSV 절대경로를 공급한다. 사본 격리에서는 `__FILE__` 기반의 같은 고정경로를 사용했다. 실제 명령 callback/visible 값은 원문 주소 식별자이며 host 함수 포인터로 호출하지 않는다. 실제 Client registry 소유자는 다음 접합 작업이다.

격리 경로는 `extracted/cpp-dot-command-register-20261024-correction-isolated/`다. 첫 build1은 LF cmd의 한국어 rem 해석 오류가 있었고 컴파일 검증으로 세지 않는다. CRLF로 정규화한 build2는 VsDevCmd 대기 상태에서 해당 작업의 확인된 자식 프로세스만 중단했으며 빈 로그도 보존했다. 이후 직접 VC14.51.36231/SDK10.0.26100.0 환경을 하위 프로세스에만 공급하고 `/W4 /permissive- /utf-8` 단일 cl로 build3의 이전 실패와 build4의 최종 통과를 확인했다. 공용 정적 라이브러리와 `/MP`는 사용하지 않았다.

원문/최초/보충 자료와 교정 전·후 소스/실행파일/로그의 SHA는 [검증 manifest](../../extracted/cpp-dot-command-register-20261024-correction-validation.json)에 고정한다. 원본 정적 초기 count1/name say/callback0042f1e0/visible00531b30은 root의 별도 정적 초기표 자료로 보존하며 이번 보충 입력을 실제 최초 Client registry의 전체 생성 증거로 세지 않는다.
