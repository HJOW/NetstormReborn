# 건설 표시의 실제 Kernel·ProcessForm·소멸 연결

작업 PC: **HJOW-Athlon**, 2026-10-11. AGENTS.md와 두 LEFT_JOBS의 현재 인계를 확인했다. 마지막 디컴파일 PC와 일치하므로 기존 읽기 전용 내보내기를 사용했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11**(`constructionlifetime` 7/13/13개). 이번 작업은 복원 몸체를 묶는 호스트 어댑터이며 새 원본 함수를 복원하거나 원본 실행 관찰 수를 늘리지 않았다. 독립 관찰 누적은 **498,660개**로 유지한다. 원본 게임/복사본/analyzeManager/클론 창/장치는 실행하지 않았다.

## 실제 소유권과 실행

`ConstructionVisualSystem`은 `RawConstructionVisual`, `RawRenderRegistry`, `RawConstructionFrame`, `DestroyConstructionVisual`을 실제 `SquidProcessHost`와 연결한다. `ConstructionVisualProcess`가 픽셀/입자/시각/등록 필드를 갖고, 부착 전에는 생성 어댑터가 객체를 소유하며 부착 뒤에는 **Kernel이 객체를 소유**한다.

`Add(parent, instant)`는 원본 표시 생성 몸체를 호출한다. 표시 목록의 슬롯과 토큰을 먼저 등록하고, registry의 attach에서 **현재 표시 타입(기본 17), 부모, flags 0x10, 지정 form 0**으로 실제 client ProcessForm을 붙인다. 부모 종속 체인/Kernel DWORD 번호/form SID를 기존 host가 기록한 뒤 캡처와 초기 입자 선택을 수행한다. 초기화가 끝나야 실행/출력을 허용한다. 부모/form 번호는 호스트 주소로 해석하지 않으며 토큰은 비영 DWORD 식별자다.

이후 `Kernel::RunFrame`이 표시 프로세스를 자동 실행한다. 각 호출은 같은 시각/난수/팔레트/좌표 배율을 쓰는 기존 프레임 몸체에 들어간다. 자기 종료의 Kill은 실제 host에 연결한다. Kill이 즉시 객체를 없애므로 callback을 프로세스 멤버에 보관하지 않고 호출 동안 살아 있는 로컬 프레임 객체에 둔다. 실행 뒤에는 저장 토큰 조회로 생존을 확인한 객체에만 현재 사각형/깊이를 등록 필드에 반영한다.

`MakeConstructionVisualSystemHooks`는 건설 Start의 표시 효과를 실제 Add에 연결한다. Start의 기존 순서대로 표시가 HP 진행 타입 **30**보다 먼저 붙는다. 통합 검사에서 Kernel 슬롯 1의 표시와 슬롯 2의 진행이 실행되고, 진행 완료가 abstract를 해제한 뒤 표시가 **다음 Kernel 순회**에서 자기 종료하는 것을 확인했다. 완료 소리/부모 공간 Unpop·재등록은 명시 경계이며 해당 몸체 전체를 새로 연결했다고 판단하지 않는다.

## Draw와 삭제

`system.Draw()`는 현재 등록 슬롯의 살아 있는 표시를 한 번씩 제출하는 어댑터다. 각 객체는 기존 `RawConstructionFrame::Draw`를 호출하므로 현재 부모 좌표/카메라/hotspot과 출력 픽셀을 사용한다. 콜백 중 목록이 확장될 수 있어 span을 장기 보관하지 않고 매 슬롯에서 다시 읽는다. 현재 객체가 출력 콜백에서 지워져도 이후 객체를 읽지 않는다.

이 슬롯 순회는 **원본 클라이언트의 깊이 정렬/가시 영역 선별/장치 렌더링 복원 근거가 아니다**. 실제 클라이언트 프레임 루프는 후속 단계에서 이 출력 또는 완성할 원본 표시 수집 경로를 호출해야 한다. SHP 캡처/팔레트 생성/dirty 큐/장치 blit는 호출자가 공급한다.

표시 form 삭제의 preDestroy는 기존 host가 Kernel 슬롯을 제거하고 가상 소멸자를 실행한다. 표시 소멸자는 **캡처→출력→입자→표시 등록/base 표식→scalar 객체 반납** 순서를 기존 수명 몸체로 수행한다. 이후 host가 form을 부모 체인에서 Unpop하고 SID를 반납한다. 공유 팔레트는 유지한다. 실제 contained 조회로 최근 타입 17 하나만 지울 때 다른 표시/Regular와 링크가 남는 것도 검사했다. 부모 삭제 검사는 이미 void인 부모의 공통 종속 삭제를 사용했고 부모의 공간 해제 전체를 검증하지 않았다.

시스템 종료는 남은 표시를 host를 통해 지운다. **host/Kernel/풀/시각/팔레트/난수는 system보다 오래 살아야 한다.** system의 복사/이동은 금지한다. 반환 포인터/필드 참조는 자기 종료나 부모 삭제 후 사용하면 안 된다. 버퍼/객체 release 콜백은 관찰만 하고 예외나 수명 변경을 일으키면 안 된다. 최대 HP/dirty 경계는 실행 중인 표시를 삭제하면 안 된다. 캡처 중 Kernel/Draw는 생성 중인 버퍼를 건너뛰며 중첩 Add는 예외로 거부한다. 확보/부착/캡처 실패와 생성 중 취소는 새 등록/form/Kernel을 정리한다. 이러한 실패/재진입 처리는 호스트의 안전 계약이며 원본 오류 경로의 동일성 근거가 아니다.

## 검사와 다음 단계

새 C++ 통합 검사 **9개 모두 통과**: 두 판본×즉시/일반의 실제 부착/Run/Draw/자기 삭제/빈 칸 재사용, Start→진행→완료→다음 순회 표시 종료, contained 삭제와 다른 프로세스 보존, 부모 종속 삭제, 시스템 종료, 부착 전후 실패 정리, 생성 중 재진입, 출력 중 삭제와 40→80칸 확장, 다른 풀 거부와 캡처 중 취소다. 합성 최대 HP 100/5×7 캡처/공유 색 표를 사용하며 실제 게임 자료/화면 검사가 아니다.

Release 경고/오류 **0**, 전체 CTest 내부 **689개·실패 0**(154.42초, 전체 154.46초), 기존 저장 원본 근거 **105종 모두 감사 통과**. 기존 oracle/fixture/evidence는 변경하지 않았다. 별도 사본에서 새 연결 변이 **7개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과): Kernel Run 생략, Draw 생략, 현재 사각형 반영 생략, client→서버 form flags, form 타입 변경, scalar 반납 bit, Start 즉시/일반 반전. 사본은 변이마다 원래 바이트로 복구했다. 로그 `extracted/constructionvisualsystem-{build,ctest,audits,mutations}.log`는 UTF-8이다.

재검사: `cmake --build cpppj/build --config Release --parallel 4`, `cpppj/build/bin/Release/netstorm_tests.exe --filter ConstructionVisualSystem_`, `ctest --test-dir cpppj/build -C Release --output-on-failure`. 새 변이는 `tools/cpp_mutation_check.py --focused --only visualsystem-run` 등 `visualsystem-` 7종이며 저장소 밖 사본에서 실행한다.

다음은 **원본 클라이언트 표시 수집/깊이 정렬/가시 영역과 프레임 루프 연결, 실제 SHP 캡처/공유 팔레트 생성/장치 출력**이다. 타입 준비 소리 +0x90, 부모 공간/소유 사제 contained/하위 경로와 건설 GUI도 남아 있다. 게임 내 건설/미션 완주는 아직 불가능하다. AGENTS.md·보호 원본·dotnetpj 수정, 커밋/푸시는 하지 않았다.

관련 파일: `cpppj/src/o/ConstructionVisualSystem.{h,cpp}`, `cpppj/tests/ConstructionVisualSystemTests.cpp`, `tools/cpp_mutation_check.py`. 기존 몸체 근거: [표시 생성](cpp-construction-visual-reconstruction.md), [프레임](cpp-construction-frame-reconstruction.md), [등록/소멸](cpp-construction-lifetime-reconstruction.md), [종속 조회/삭제](cpp-contained-lookup-reconstruction.md), [HP 진행](cpp-construction-process-reconstruction.md).
