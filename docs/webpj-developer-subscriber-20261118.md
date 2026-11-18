# WB04-E757 개발자 Subscriber 캠페인 분기

기준선 `225553df`의 `RawCampaignSession.cpp` 31h `setsubid`/`ssi` 분기를 웹 캠페인에 연결했다. 두 입력이 있으면 현재 Template 소유자와 manager를 확인하며 manager가 없을 때는 fort 모드를 조회하지 않는다. 캠페인에서는 기존 Fort를 바꾸지 않고 현재 공유 Client Subscriber DWORD와 handle의 NUL 앞 바이트를 수준 29로 출력한다. 기존 Fort의 Subscriber 변경은 웹에서 제외된 편집 기능이므로 해당 경계에 도달하면 명시적으로 거부한다.

집중 검사에서 별칭·불완전 입력·manager 선행 관문·NUL 및 Template 물리 경계를 확인했다. 실제 TheWarBegins Session을 적재하고 같은 31h 분배기로 호출해 현재 공유 ID/handle을 출력하고 변경하지 않는 것을 확인했다. 최초 통합 검사는 Session Load가 이미 공유 Subscriber와 handle을 로그인 값으로 바꾸는 점을 반영하지 않아 두 번 실패했다. Load 후 공유 칸을 바꾸어 현재값을 관찰하도록 고친 뒤 통과했다.

나머지 31h 명령과 기준선 이후 Login 변경 판정, 선택 UI·미션 Start·앱 수명은 남아 있다. 포획·희생·승리, 세 공중 유닛 전용 행동과 게임 창 실행은 하지 않았다.

Node v26.13.0 최종 전체 회귀 6,256건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E753의 v22 검사 뒤 네 번째 v26 전체 회귀다.
