# Guide 연속 교체와 세 번의 실제 월드 재적재 — 단위944

2026-11-06, HJOW-Athlon. LEFT_JOBS.md 3절3번 첫 항목을 진행했다. 마지막 정밀 디컴파일은 같은 PC/2026-11-03이다. 제품939/11.01 코드는 수정하지 않았다.

기존 원문286행의 Begin/Fork/Restart/End·이름/인자/고리와 미션 수명831행을 현재 집중 실행 파일로 재생해 통과했다. 실제 Guide/Client/Session의 기존 세 검사도 먼저 통과했다. 성공/실패 상태나 특정 적 공격을 재현한 것은 아니다.

새 검사는 기존 작업본과 실제 Client/Session 설치 도우미에서 **Begin → Restart → Fork → Close → Begin → Init → Native State 정리 → 재적재**를 세 차례 이어간다. 각 세대에서 다음을 확인했다.

- 재시작의 캡처 이름/모드 인자와 demo0 복원, Fork의 현재 설정 조회, Close 뒤 미션 Config 반납과 Begin 뒤 등록 수 복원.
- Guide만 교체하는 동안 이전 Session 주소·공유 surface/gameplay 대여·Player base·Kernel 수·섬 수·Template 이미지 수가 유지됨.
- CaptureEnd·LoginEnd·ConnectEnd·ActivityEnd 네 기한이 Guide 교체만으로 바뀌지 않음.
- Init의 Template 반납 뒤에도 이전 월드를 유지하다가 Native State 정리에서 실제 Session/대여를 반납함.
- 같은 작업본 재적재 뒤 요청 이미지 반납, 새 월드/Template 복원·삭제 깊이0, 세대별 Runtime 반납/Renderer 초기화/시작 안내/팔레트 요청1회, 안내 닫기2회, UI모드2/3 각1회.

실제 Runtime·파일·소유권을 검사하지만 Retry/Next/Abort 버튼을 실제 GUI에서 누른 결과는 아니다. 모드 복원 callback과 HUD/표시/음향 경계는 기존 검사 도우미의 명시 장치다. 단일 작업본의 세대 교체를 모든 미션 조합/전략/장기 AI·다리 재고의 동일성으로 확대하지 않는다. 새 패킷 형식이나 실제 통신/MP를 검사하지 않았고 기존 싱글플레이 설치 도우미만 사용했다.

새 검사를 더한 첫 컴파일은 `CampaignSessionTests.cpp`가 MSVC COFF 섹션 한도를 넘어 **C1128**로 실패했다. 이 번역 단위에만 MSVC `/bigobj`를 적용했고 다른 컴파일러나 제품 빌드 설정은 바꾸지 않았다. 최종 빌드는 경고/오류0이다. 새 검사 첫 실행과 기존 World 검사를 합친 재실행도 통과했다. **고유7개(새1·기존6)·실패0**이며 큰 guide_release 대상 전체 실행은 아니다.

보호2787개/fixture967개와 제품939 EXE는 전후 불변이며 새 fixture0이다. 게임 창·사용자 물리 입력·포획/희생·특정 적 공격·시나리오 승리·공중3종 전용·MP/실제 통신 테스트는 없다. [완료 JSON](../../cpppj/mission-three-transitions-20261106.json), Git 제외 `extracted/cpp-resume-944/`에 초기 실패·최종 빌드/검사·보호 해시를 보관했다. 다음945도 **3절3번 첫 항목**의 실제 Abort 메뉴/나머지 연속 전환 경로다. 데모 두 번째360초·Native3/State6 잔여는 그 뒤 순서대로 진행한다.
