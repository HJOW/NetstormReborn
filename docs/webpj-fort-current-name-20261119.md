# 단독 미션 Fort 현재 이름 선택 (WB04-E884)

기준선 `225553df`의 Tutorial Init은 `RequestMissionFort` → `SelectCurrentFortName`을 거쳐 `FortCurrentNameDevices`와 `RawFortCurrentName`을 호출한다. 두 파일을 편집기 전용으로 제외한 기존 판정은 단독 미션에 잘못 적용되었으므로 이식 대상으로 재분류했다. Editor 작업 파일 삭제와 원격 Fort 조회는 웹 범위에서 계속 제외한다.

웹에 현재 이름 선택과 manager 파일명 setter를 이식했다. 원본 순서로 현재 owner·헤더를 재조회하고 NUL 바이트 복사, 제목·dirty·조건별 효과와 중간 실패를 유지한다. 실제 파일 삭제는 하지 않으며 해당 경계는 명시적으로 주입받는다. 보호 10.78 x86 관측 876행 중 단독의 manager 없는 40행을 이름 물리·scratch·dirty·호출 순서로 대조하고, Snapshot 뒤 owner 교체와 부분 복사도 검사했다. 나머지 836행과 실제 Client 파일·Template 장치 및 Guide Init 결선은 후속이다.

Node v26 집중·연관 검사 41건이 모두 통과했다. 작업 단위 주기에 따라 이번에는 전체 회귀를 실행하지 않았다. 기존 이력 전체를 조회하는 선택적 도구 실행은 대응표·상태 개수 확인 뒤 중단했다. 게임 창은 실행하지 않았다.
