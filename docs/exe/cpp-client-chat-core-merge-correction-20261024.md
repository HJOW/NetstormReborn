# ColorSort 뒤 Core/Chat 사본 적용 검사의 교정

2026-10-24 root의 실제386 적용에서 발견한 **이전 사본 검사 오판**을 별도 기록한다. 최초 owner patch `76c19886…`, ColorSort patch `68ccbf77…`, `merge_check.py`, `merge-check.json` 및 기존 문서는 수정하지 않았다. 이 기록은 [이전 장기수명 문서](cpp-client-long-lifetime-next-20261024.md)의 “순차 네 단계 종료0/수동 합성 불필요” 결론을 **대체한다**. Native/격리 모듈 실행 결과와 구별한다.

원인은 저장소 하위 cwd에서 `git apply`를 실행한 것이다. `diff --git` 형식 ColorSort15파일은 모두 **Skipped patch**였지만 종료코드0을 반환했다. plain unified 형식 owner patch만 사본에 적용됐다. 최초 검사자는 종료코드와 공유 입력 불변만 확인하고 **ColorSort 적용 결과 SHA·선언 생성**을 확인하지 않았다. 이는 담당자의 검사 누락이다. 실제 최초 사본에서13파일은 baseline과 동일하고 ClientMain2파일과 신규 owner 검사만 바뀌어 있었다.

새 [교정 도구](../../extracted/cpp-client-chat-core-owners-20261024-proposed/correct_merge_check.py)는 root가 지정한 **C:\Users\BUZZ\AppData\Local\Temp** 아래 새 디렉터리를 만들고 모든 조상에 `.git`이 없음을 확인했다. `cpp-chat-core-colorsort-correction-cbelo1e7` 사본에서 다음을 확인했다. 공유 H: 소스/`.git`에는 쓰지 않았다.

1. 원래385 baseline15 SHA를 확인하고 ColorSort의 check/apply가 모두 종료0이며 **실제15파일 행본문**이 동결 ColorSort 사본과 전부 일치했다. 적용 결과는 원래CRLF를 유지하고 동결 copy는LF여서 실제 결과 SHA와 정규화 행본문 동일성을 각각 기록했다.
2. 그 뒤 원래 owner patch의 check는 **종료1**이며 ClientMain.h:33/cpp:11의 두 문맥이 실패했다. root가 관측한 결과와 같다. ColorSort의 새 전방선언/include가 기존 owner hunk 문맥 사이에 들어간 것이 원인이다.
3. 원래 동결 owner 생성기를 수정하지 않고 ColorSort 뒤 두 파일을 입력으로 준 별도 사본에서 동일한 owner 의미변경만 합성했다. 그 문맥 patch는 외부사본 check/apply가 모두 종료0이며 실제 결과파일 SHA·신규 검사 생성·**ColorSort/Core/Chat 선언 및 멤버/초기화의 동시 존재**를 확인했다. 나머지13 ColorSort 파일 SHA는 합성 전후 동일하다.

실제 결과 SHA는 ClientMain.h `1e21781c9a2e0e8e1c4fa998f06e89caba608895fcf7ff1361ca055616e15cd4`, ClientMain.cpp `dc835c89ba6729d0c8c28285062f189635e285d04a72a07743c7042f2b9a9c34`, 신규 검사 `b0d5a0af71d0f05799c954b7400331d2d0cffb49fcb787fbc918d38c3a91c761`이다. [교정 JSON](../../extracted/cpp-client-chat-core-owners-20261024-proposed/merge-correction.json)에 verbose 전체와 최초자료 SHA를 보존한다. [문맥 합성 patch](../../extracted/cpp-client-chat-core-owners-20261024-proposed/after-colorsort-context.patch)는 참고용이며 root의 실제 수동 합성을 대신 적용하지 않았다.

교정 첫 시도는 실제 git의 CRLF 보존과 동결 copy의LF 차이를 raw SHA로 같다고 요구해 중단했다. `merge-correction1.log`와 첫 Temp 사본 `cpp-chat-core-colorsort-correction-xn4lrfr1`도 보존한다. 둘째 시도는 실제 결과 SHA와 행본문 동일성을 나누어 검사해 성공했다. 원래 오판의 실패나 교정 전제 차이를 숨기지 않는다.

## 같은 검사 구조의 범위 감사

이 담당자의 다른 제안 검증 스크립트 중 Storage.LoadFile/FileDevices/Session checkpoint/CoreWrite/FullSnapshot의 git check는 **cwd=ROOT**다. 그 결과를 새 시점의 변경된 공유 소스에 억지로 재실행하거나 과거 성공으로 덮지 않고, 최초 사본/검증 범위를 읽기 감사했다. 이번처럼 하위 cwd에서 diff --git 선행 patch를 건너뛰는 구조는 발견하지 못했다.

이전384 owner→Subscriber 순차 사본은 두 patch 모두 plain unified 형식이며 **결과9파일 SHA·신규2검사 생성·random/subscriber 자동주입 각1회**를 실제 사본에서 검증했다. 이 별도 증거도 [범위 감사 JSON](../../extracted/cpp-client-chat-core-owners-20261024-proposed/predecessor-apply-audit.json)에 기록한다. 이번 오판으로 Native 최초 관측이나 실제 root 통합 빌드/검사 결과까지 무효라고 확대하지 않는다. 반대로 종료0만 있는 미확인 적용을 성공으로 인정하지 않는다.
