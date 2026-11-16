# Session TypeNames·Objects·Contents reader 결선 (WB04-E621)

기준선 `225553df`의 `RawCampaignSession.cpp:3194..3224`
(`MakeFortContentsReader`·`ReadFortObjectStreams`)와3345의
`PrepareFortTypeNames`를 같은 실제 owner의 세션 helper로 연결했다.
대상 파일과 `RawFortTypeNamesLoad`·`RawFortObjectStream`·
`RawFortContentsLoad`는 기준선 뒤 변경0이다. 전체 Session/Load 완료는 아니다.

## 구현

- Select의 실제 반환 노드와 생성 수명을 호출별 토큰 표에 보관한다.
  manager/current 변경 뒤에도 선택 당시의 같은 stream에 동기로 접근하며
  이미 읽은 파일을 다시 파싱하거나 완성 객체 목록을 만들지 않는다.
- TypeNames는 같은 저장 깊이와255BYTE 변환표의 현재 DWORD를 읽고 쓴다.
  현재 TYPE 내부 narrow 이름+NUL과 실제 hash reader를 연결했다.
  본문 예외는 별도 unwind로 깊이만 복구한다. 이전/부분 할당을 임의로
  해제하지 않는다. 표시용 번역문으로 내부 TYPE 이름을 바꾸지 않는다.
- Objects는 기본20 Terr/Chaff와 부분 Terr 범위를 같은 raw reader로 읽는다.
  변환표 반납은 정상 꼬리만 수행하며 실패 시 객체/커서/변환표 접두를 유지한다.
  지역 `sourceStorage`를 써도 전역 Template manager를 교체하지 않는다.
- Contents는 현재 변환표·두 TYPE 조회·부모의 현재 raw TYPE와 목록 flags를
  실제 `ContainedCreate`에 연결한다. 전체 DWORD 부모를 실제 Pool 접근 시
  검증하고 quantity/listFlags/attachFlags/quality를 그대로 전달한다.
- owner의 두 공개 helper는 반납 후 호출을 거부한다. Contents reader의
  access와 owner는 동기 읽기 전체 동안 살아 있어야 한다.

신규 실제 조립 helper는 `tests/campaign-session-support.mjs`다. 기존 실제
World·Player·HP·내용물 모듈만 조립하며 별도 근사 월드를 만들지 않는다.
검사의 Objects 로더도 같은 실제 `RawCampaignWorldRuntime.objects`다.

## 검증과 최초 실패

신규6건과 관련17건, 집중23/23 통과다. 같은 manager/현재 이름/변환표,
지역 Template 격리, 실제20 Terr/Chaff·부분 범위 커서, 정상 반납,
기존 변환 진단과 중첩 unwind, 실제 Form6 생성/부착·현재 부모/TYPE를 확인했다.

초기 신규6건은 통과했다. 손상 파일 보강에서 논리 end5를 물리 끝으로
가정하여23건 중1건이 실패했다. 실제 Template 동기 빌림은16KB 할당이며
원시 reader는 그 물리 범위를 사용한다. 별도 빌림 사이의 vector 확장도
새 칸을0으로 채우므로 임의 꼬리가 유지된다고 가정할 수도 없었다.
그 보강은 같은 현재 변환표가 남아 있는 실제 원문 진단 입력으로 바꾸어,
단일/중첩 unwind와 현재 할당 보존을 대조했다. 제품/fixture의 기존 물리
메모리 계약과 기대값을 느슨하게 바꾸지 않았다.

기존 원본 관측의 직접 대조는 다음을 유지한다. 새 fixture0이며 manifest는
806항목이다(행 수는 머리글 포함, 각 자료의 실제 소비 범위는 manifest 참조).

| 자료 | 줄 | SHA-256 |
| --- | --- | --- |
| fort-object-load | 1219 | `31fb91aabb4fe3c403e9a9780e0f14f211508cbbce6523cef1b53a5bdf439519` |
| fort-contents-load | 669 | `22851fa72e8905ac9465aadb64cb5669695dfd7d2847435fb0dcb25d4048948e` |
| fort-type-names-load | 665 | `4c824ec764b85b73ef1d2387c7e6043611b767e983f66abc3f1a98da2d0ef86c` |

Node v22.23.3·v26.13.0 전체는 각각5,939건 통과,
실패·취소·건너뜀0·종료0이다(`--test-concurrency=4`). v24는 기존 지시로
생략·호환 가정이다. 대응표 오류0·구문/UTF-8·git diff --check 통과다.
로그 접두는 `extracted/webpj-e621-`이며 기존3개 manifest 항목의 범위와
계산 칸을 갱신했다.
공중3종 새 전용 생성/생산/행동·원본/클론 게임 실행·실제 미션 플레이·
포획/희생/승리·Native 장기 설치는 실행하지 않았다. 검사 parent의 VT는
기존 Player 통지 계약 입력이며 세계에 Pop하지 않는다.

다음은 생성자1866의 Template 설치와 `MakeRuntime`·승패/AI 부모 구성,
외곽 Session 공개와 실제 Load다. 이번 reader 결선만으로 저장 미션의
전체 지형/객체 Pop·Kernel·로그인 완료를 선언하지 않는다.
