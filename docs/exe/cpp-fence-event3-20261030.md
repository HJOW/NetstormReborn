# 썬 바리케이드 사건3과 실제 Regular 호출자의 빈 x87 반환 — 단위613

2026-10-30, HJOW-Athlon. AGENTS.md·공통/DN 인계를 다시 읽었고 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23이다. 보호 원본·정밀 산출물은 변경하지 않았다.

## 원본 제어와 실제 반환

기존 RawFenceRegular.Handle은 보호bit20 없는 사건3을 무조건 미복원 예외로 중단했다. 원본00457a50은 방향0/2/4/6의 이웃 SID를 먼저 모두 모으고, 각 이웃의 기존 fencemark를 삭제한 뒤 그 효과 이후 현재 좌표로 연결선을 삭제한다. owner 캐시+2a와 연결 수 캐시+2e를 쓰지 않는다. 마지막 new40 성공일 때 사건2/payload0.1의 Regular를 생성하고 명시 float load 없이 반환한다.

반환값을 임의0/-1로 정하지 않고 실제 호출자00496cb0과 생성자00496e80을 함께 실행했다. 정상 ABI의 빈 x87 스택에서 일반271사례 모두 실제 FST32가`0xffc00000` 음수 indefinite NaN을 생성했다. 호출자는 이를 payload와 다음시각에 저장하고 count를1 증가시킨다. count의32비트 감김도 그대로다. NaN due 때문에 같은 Regular의 다음 호출은 사건을 다시 선택하지 않는다. 이미 생성한 사건2는 별도 프로세스로 남아 정상 실행된다. bit20은 사건3보다 먼저 기존 Damageable에 위임한다.

C++는 같은 이웃 스냅샷/현재 좌표 재조회와 조건부 예약을 연결했다. 공통 ClearNeighbors의 owner 갱신은 사건0만 선택하고 사건3은 캐시를 보존한다. float를 반환해야 하는 C++ Handler에는 실제 호출자가 저장하는 음수 NaN을 전달해 기존 ProcessHost가 같은 payload/시각/count를 처리하게 했다. 빈 성공/건너뜀이나 임의 지연을 넣지 않았다. 일반 ABI의 빈 스택 입력을 관측한 범위이며 비표준 잔류 x87 입력 전체를 정의한 표본은 아니다.

## 실제 CPU를 기준으로 삼은 이유

최초 Unicorn 관측은 태그가 빈 상태의 FST를0으로 처리해 원본 의미의 기준에서 제외했다. 이 실패 입력/출력과 최초 도구는 원시 자료에 보존했다. 별도 콘솔 도우미 tools/native_fence_event3.cpp는 게임 창이나 원본 EXE 프로세스를 실행하지 않고 선택된 원문 명령을 실제32비트 CPU에서 실행한다. 실제 CALL/FST7BYTE의 동작을 유지한 관찰 trampoline은 x87 환경·레지스터/플래그를 보존하고 FST 값/직전 태그만 읽는다.

10.78은 재배치 자료가 없고 원래0x400000 주소가 검사 프로세스 메모리와 충돌했다. private 이미지의 선택된 네 함수에서 절대주소 피연산자33개, switch/vtable 포인터5개를 기계적으로0x30000000 기준으로 옮겼다. opcode·상대분기·수치상수는 바꾸지 않으며38개 내역을 address-relocations.json에 보존한다. 출력의 다섯 raw 가상표 DWORD만 원본 논리주소로 정규화한다. 원본 파일에 쓰거나 정밀 산출물을 다시 생성한 것이 아니다. 이웃/표식/선·할당·공통 부착/삭제·Damageable은 명시 외부 경계다. 실제 생성자의 필드·시각 복사와 x87 명령은 실행했다.

검사 도우미를 다시 만들 때는 Visual Studio의 **x86 Native Tools Command Prompt**에서 저장소 루트를 기준으로 다음과 같이 실행한다. 게임/배포 빌드는 이 도우미를 포함하지 않는다. 출력 폴더는 미리 만들고, `--output`에는 아직 없는 새 관측 경로를 사용한다.

```bat
cl /nologo /std:c++20 /EHsc /O2 /arch:SSE2 /fp:strict /utf-8 /W4 /WX tools\native_fence_event3.cpp /Fe:extracted\cpp-fence-event3-613-20261030\native_fence_event3.exe /Fo:extracted\cpp-fence-event3-613-20261030\native_fence_event3.obj /link /BASE:0x10000000 /DYNAMICBASE:NO
python -X utf8 tools/decomp_fence_event3.py --helper extracted/cpp-fence-event3-613-20261030/native_fence_event3.exe --output extracted/cpp-fence-event3-new-check --verify
```

## 근거와 확인

[원문 기록](../../cpppj/fence-event3-native-20261030.json)은 실제 CPU301사례/두 정밀도602관측 차이0이다. 일반271/보호위임30이며 네 방향16개 조합·할당 성공/실패·검색/표식/할당 효과 후 변이·현재 두 캐시 보존·payload/음수0/NaN·count감김을 포함한다. 최초 성공/fixture 생성/재관측의6개 출력과 세 private 이미지가 같은 바이트다. FST 직전 태그·실제 반환/현재 Regular·새 생성자 필드와 다음 호출 보존을 관측했다. Native 도우미도 최종 경고/오류0으로 빌드했다.

C++는301행의 전체250BYTE·반환비트·하위 순서를 비교했다. 실제 같은 Host에서는 할당 실패/성공·네 이웃과 좌표 변이·count감김의 세 장면으로 NaN 재예약/재호출 보존, 새 사건2 실행·삭제와 마지막 Form 반납을 원문 관측에 대조했다. 기존 펜스/프로세스/장기Host 저장소를 포함한 고유57개/실패0·등록3520개다. 전체3520개 실행은 아니다. 최종 Release 게임/검사 빌드는 종료0·경고/오류0이다.

새 검사 최초 빌드의 spots 준비형식uint16을 실제 API의uint8로 수정했다. 첫 Host 검사에서 Kernel.Clear가 Form까지 반납한다는 잘못된 검사 가정의6체크가 두 필터에 보고됐으며, 실제 Host.Kill로 Form 정리를 마치도록 검사만 수정했다. 제품 제어를 그 실패에 맞춰 바꾸지 않았다. 실제 명령 FADD qword를 확인한 결과 공통 Regular 다음시각 계산은 기존 C++와 같았으므로 변경하지 않았다. 최초 실패와 최종 동일 EXE 결과는 별도로 보존한다.

새 ZIP1376항목/게임1358의 압축/추출/원천 바이트가 모두 일치한다. 게임SHA256은`a86629f49be5ea8dff4289b2b217b2da1951df9762082ccf5e0b55651452ae7d`, ZIP은`13a962946efb1f505d361107ce01fdcdda00586bde2e9c45a07920d25ea2c68f`다. 한글 배포 사본의 Tutorial6을28.723666초 실행해 실제Start/같은Session·정지0·시각24.547000초 증가·실제 창전체RGB·정상 종료를 확인했다. 실제 특정적공격/포획/희생/승리 재현은 생략했으며 실제 사건3 적중/삭제를 강제로 유발한 근거는 아니다.

보호2580파일/기존fixture757개는 불변이고 새fixture 포함758개다. 마지막 전체34는 같은609제품의610 검증이다. 이번613 제품의 전체34/전체C++/자연완주/원본 전체동일성/Windows11 실기·PCM·활성 전체화면과10.72 복원은 별도다.612의 원본/현재 스택 배치·미정값 자체 동일성 한계도 유지한다. 공중3종전용/신규MP 검사는 하지 않았다. 남은 수명·UI/Edit·Native·시나리오/파일·호환성과 후순위MP 무시험/4~6차를 계속한다.

[집계](../../cpppj/fence-event3-runtime-20261030.json), 원시자료는 extracted/cpp-fence-event3-613-20261030/이다. 전체 목표는 active/미완료다.
