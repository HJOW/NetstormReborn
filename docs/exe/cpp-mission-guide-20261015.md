# 실제 Guide Run/Advance의 현재 미션과 단계 진행

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 같다. AGENTS와 양 LEFT의 최신 기록을 다시 읽고 같은 PC의 전체 정밀 소스 및 보호 PATCH PE를 재확인했다. 새 디컴파일·원본/클론 게임·GUI·OS 진입은 없고 보호 원본·AGENTS·dotnetpj를 변경하지 않았다.

## 원문과 실제 상태

`o/RawMissionGuide.{h,cpp}`는 Guide `00463bb0`과 Mission Advance `00482c80`을 복원한다. 원문은 `extracted/refined/originals/Netstorm.c`의 두 동명 함수이며 capstone 명령도 확인했다. 실제 공유 타입은 `RawMissionStart.h`의 **`MissionStartState`**다. 새 Guide 전용 sequence/flags 사본을 만들지 않고 Start와 동일한 `flags(+54)`·`sequence(+58)`를 직접 참조한다.

`Advance(state)`는 unsigned DWORD `sequence++` 뒤 `flags &= fffffffe`이며 `ffffffff→0` 감김과 높은 flags를 보존한다. 다른 Start 상태 `tellAtIndex(+a0)`·`fieldC0(+c0)`·`fieldC4(+c4)`를 변경하지 않는다. CD Advance의 대응은 전체 정밀 표의 `00480ce0`이지만 Guide 상위 대응이 확정되지 않아 이번 제품은 PATCH만 허용한다.

Guide의 현재 미션 getter, pending `004b32e0`, 다이얼로그 부재 `004c8e90`, 가상 stage0..6, postpone `+20`, release `00463ab0`, invalidStage 진단 `004e0620`은 필수 외부 경계다. 다른 큰 소비자를 빈 성공으로 채우지 않으며 이 모듈만으로 실제 UI/로그인/Stage3가 연결된 것은 아니다.

## 실제 Run 순서와 재조회

1. 현재 포인터가NULL이면 종료한다. 현재 sequence를 비교용으로 보관하고 flags bit8이면 현재 포인터를 다시 확인한 뒤 release하고 종료한다. pending/dialog/stage는 호출하지 않는다.
2. pending이0일 때만 dialogsClear를 읽는다. 둘 다 허용하면 현재 미션의 flags bit1을 읽어 실제 Advance를 실행한다. 막혔으면 현재 미션의 postpone만 호출한다.
3. Advance 뒤 현재 포인터를 다시 읽어 switch의 sequence를 선택한다. **가상 stage 호출 대상 포인터는 그 뒤 다시 조회한다.** 원문 `00463c06`의 switch용 조회와 `00463c1b/28/35/42/4f/5c/69`의 가상 호출 대상 조회를 합치지 않는다. 초안의 target 참조 재사용은 이 단위에서 해당 경계에 맞게 교정했다.
4. sequence0..6은 가상 `+4..1c`,7이상은 원본 진단이다. 실제 진단 인자는 `0050161c`, `.\\Guide.cpp` 주소 `0050abe0`, line `aa`다. stage6이면 callback이 현재 미션을 바꿔도 원문 위치에서 release를 호출한다.
5. callback 뒤 현재 포인터가NULL이거나 현재 sequence가6이거나 시작 때 sequence와 같으면 그 Run을 끝낸다. 다른 sequence면 같은 Run 안에서 처음부터 반복하며 pending/dialog도 다시 읽는다. 같은 번호로 active 객체만 바뀌면 번호 비교에 따라 종료할 수 있다.

**Stage5가 실제 Advance로5→6을 만들면 현재 Run은 종료한다. 다음 Run이 stage6과 release를 수행한다.** 반대로 Run 앞 flags bit1이5에서 켜졌다면 그 Run의 Advance로6을 선택하여 stage6/release에 바로 도달한다. 두 경로를 합치지 않는다.

현재 getter는 각 원문 포인터 읽기 위치에서 호출된다. 생성자는 모든 경계와 판본을 확인한다. 허용 검사 뒤 callback이 포인터를NULL로 바꿔 원문 역참조 위치가 손상된 입력은 제품의 `RequireCurrent`가 호스트 예외로 명시 거부한다. native 정상 반환 표본은 그런 access violation 입력을 포함하지 않는다.

## 독립 보호 PE 관찰

`tools/decomp_mission_guide.py`는 기존 격리 PE 메모리 로더만 재사용한다. 명령 허용 범위는 같은 PC의 전체 정밀 함수표에 기록된 **00463bb0/00482c80 두 몸체만**이다. Run 본체의 직접 Advance 호출과 외부 가상 효과가 요청한 Advance는 실제 `00482c80`에 진입한다. 기대 단계/종료/짧은평가 결과를 Python으로 계산하지 않는다.

보존한 고유 입력은 **1,550개 = Run 입력1,462 + 직접 Advance 입력88**이다. 반복 Run과 실제 가상 효과의 Advance를 포함한 실제 진입은 **Run1,834회·Advance607회**다. 각 함수 호출 직후 정상 반환 주소와 스택 `ESP=호출전+4`, 보존 EBX/ESI/EDI/EBP를 확인했다. 실제 명령 쓰기 guard는 두 미션의 `+54..5b`와 한정 스택만 허용한다. 다른 주소 쓰기·미검토 명령·게임/OS 진입을 거부한다.

pending1,728회·dialogs1,474회·postpone304회·stage0..6 각각195/192/209/102/104/161/122회·release334회·invalidStage339회는 **명시 외부 대체**다. 원본 큰 pending/UI/가상 몸체/release/assert 내부의 실행 근거로 합산하지 않는다. current는 `Guide+800`의 실제 메모리 읽기 hook으로 관찰하고, 명시 입력의 포인터/상태 변경은 해당 읽기 전에 공급한다. 이 적대적 입력은 실제 게임 getter의 부수효과를 주장하는 것이 아니라 현재 재조회 위치를 검증하기 위한 자료다.

fixture는 `cpppj/tests/fixtures/mission-guide-x86.tsv`다. active 번호, 두 미션 각각 **전체208바이트**, current/pending/dialog/stage/postpone/release/진단 순서를 보존한다. C++ 검사는 같은 `MissionStartState`의 다섯 필드를 원문 offset에 직렬화하여 독립 native의 전체 객체 표현과 비교한다. 원문 물리 객체의 vtable/나머지 stale 바이트는 명시 입력이며 C++ 상태 구조체가 원본 물리 배치를 가졌다고 주장하지 않는다.

입력에는 빈 미션,0..8/최대DWORD sequence, advance/취소/높은 flags, pending 짧은평가, 두 번의 Run, stage의 Advance/flags요청/취소/종료/단계 변경/active 교체, postpone/진단 뒤 변경, getter별 active 재조회, DWORD 감김을 포함한다. 새 관찰은 기존 Chat/Start/FrameGate 입력과 별도로 관리한다.

## 제품 검사와 남은 연결

`MissionGuideTests.cpp`는 **4개 검사**를 제공한다. native1,550개 전체 출력/trace 재생, stage5→다음Run stage6/release, switch 번호와 현재 가상 대상의 분리, 필수7경계/판본 계약이다. 도구의 `--verify`는 현재 원문·제품·검사·도구·fixture SHA와 고유입력 수를 감사한다.

소스 고정 뒤 CMake 등록·Release 경고0·오류0, 이4개를 포함한 집중60개 실패0·전체 CTest1/1·내부1,283개 실패0·216.33초를 확인했다. `extracted/goal-mission-entry-final-{build,focused}.log`, `goal-mission-entry-{ctest,tests}.log`에 보존했다. `cpppj/mission-guide.json`에서 원본 관찰과 제품 검증을 구분한다. 게임/GUI 실행은0이며 실제 로그인 종료/현재 미션 수명/Stage3/UI/정지/프레임 게이트 연결과 자연 완주는 별도 후속이다.
