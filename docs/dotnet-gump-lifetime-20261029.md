# dotnetpj R3c — Gump 수명과 루트 교체

2026-10-29, HJOW-Athlon. 기준 cpppj는 `6f2b5101bae0`의 `src/client/GumpLifetime.h/.cpp`다. 작업 전에 AGENTS.md·현재 dotnetpj 인계·공통 인계의 제약/디컴파일 호스트를 다시 확인했다. 현재/마지막 정밀 디컴파일 호스트가 모두 HJOW-Athlon이며 이 단위는 커밋된 cpppj 소스와 기존 fixture만 읽었다.

## 1. 구현

`GumpLifetime`은 기존 생성/트리와 같은 노드를 사용한다. 자식 next를 재귀 전에 보관하고 자식부터 Kill 요청 또는 Destroy를 처리한다. idle/overlay/Form/Push/삭제 큐/초점/override/dialog/dead 기록과 등록 해제/소멸자의 원본 순서를 유지한다. Kill 중복 요청은 제거하지 않으며 기본 소멸자가 전체 큐의 중복을 null로 지워 실제 해제를 한 번만 수행하게 한다.

idle은 첫 후보, overlay는 마지막 후보, dialog는 첫 일치를 사용한다. CD는 idle 앞80칸만 처리한다. Cleanup은 null 항목에서는 반복 범위를 갱신하지 않고 실제 소멸자 뒤에만 현재 KillCount를 읽는다. 텍스트 포인터는 해제 콜백 뒤0으로 쓰고 두 번째 포인터는 그 시점의 현재 값을 읽는다. 예외 전에 이미 수행한 쓰기는 원복하지 않는다.

`KillExceptType`은 루트 외 등록999칸을 현재 count와 무관하게 순회한다. 제외 signed BYTE를 부호 확장해 각 슬롯에서 다시 읽도록 C# `ref sbyte`를 사용했다. `ReplaceRoot`는 이전 루트 Kill→현재 큐 Cleanup→새 루트/id0 기록이며 serial/count나 새 객체의 임시 등록 칸을 임의로 초기화하지 않는다.

## 2. 검증

- `gumplifetime-x86.tsv`519행(3판본×173), `uireset-x86.tsv`242행(2판본×121)을 사본 없이 Link로 연결했다. fixture 목록89개·포트 맵146행이며 전체 reflected-commit은 올리지 않았다.
- 전체 노드160바이트·활성 표5개·현재 전역·연속 연산 반환·가상 경계 당시 전체 노드/표/전역을 원본 기대값과 대조했다. 기본 deleting destructor에 실제 `DeleteScalar`를 연결하는 입력도 그대로 재생한다.
- 집중13개가 통과했다. C++의 원본 재생5개와 실제 생성/부착/분리/기본 소멸자/해제/등록 칸 재사용, 트리 교체 지연 삭제, 기본 소멸자의 비등록해제, CD idle 경계, 손상/순환/한도, 콜백 예외 시 부분 쓰기, 두 판본의 생존 자식 이동/루트 교체를 이식했다.
- 제외 BYTE를 부호 확장 대신0확장하는 변이에 두 판본 reset의 전체 바이트 대조가 모두 실패했다. 원복 후 전체 **1,548개 통과(Assets363·Core1030·Client155), 실패/건너뜀0**, Release 전체 빌드 경고/오류0을 확인했다. 기대값 변경은 없다.
- 원시 로그는 Git 제외 `extracted/dotnet-gump-lifetime-focused-20261029.log`·`dotnet-gump-lifetime-mutation-20261029.log`·`dotnet-gump-lifetime-tests-20261029.log`·`dotnet-gump-lifetime-build-20261029.log`에 있다.

## 3. 남은 연결

다음은 같은 저장소 위의 `GumpEvents`·`GumpInput` 사건 필터/선택/캡처/dialog/overlay 전달과 `GumpFrame` 등록 통지/idle/재그리기다. 이어서 Draw·위젯·글꼴/GIF·메뉴를 조립한다. 수명 L1 및 독립 통합을 실제 메뉴 화면/전체 세션 플레이 완료로 세지 않는다. 기존 근사 코드/검사는 은퇴시키지 않았다.

cpppj·공통 인계·원본/CD·AGENTS.md는 변경하지 않았다. 게임/창 실행과 요청에서 제외한 사제 포획/희생·특정 적 공격·승리 재현, 멀티플레이 검사는 수행하지 않았다. 외부 TODO.md 변경은 커밋에 포함하지 않는다.
