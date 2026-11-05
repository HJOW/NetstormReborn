# Chat Helper의 현재 모드·설정과 Console 재개방 수명 (단위922)

같은 실제 Chat/Console에서 현재 battle/challenge와 Config의 여덟 조합을 바꾸며 helper 메뉴를 재개방했다. 새1/기존9의 고유10사례가 첫 실행에서 통과했다. 제품 변경은 없어921/11.01을 유지한다.

원문 근거는 같은 HJOW-Athlon의2026-11-03 정밀 디컴파일 Handle `0042fab0`과 기존 [Chat 부모 복원](cpp-chat-window-20261023.md)이다. 현재 라이브러리에 기존 원문180행을 재대조했고 정밀 산출물/fixture는 수정하지 않았다.

새 검사는 실제 helper serial을 같은 Handle에 전달한다. 현재 모드는 명시 입력이며 MP/송신 효과는 실행하지 않는다. 설정 reader는 같은 Client Config다. 두 모드가 모두0이면 설정/Format 행이 없고 challenge만1이면 효과4설정/Format2, battle이1이면 효과4와채널/Player/표시9설정·Format2다. 설정0/1 두 경우의 반전 문자열과 체크 비트도 확인했다. 총60설정 행/12Format 행이다.

원문 고정1000등록표의 실제 메뉴 행을 읽었으며, 매번 RequestClosePopups→CleanupFrame 뒤 메뉴 노드가 반납되고 같은 Console·입력 버퍼·`EVERYONE: kept`·표시 상태와 전체 등록 수가 보존됐다. 마지막 Chat 반납은 current0·편집 버퍼0이다. 설정을 실제로 실행하거나 일반 메시지/패킷을 보내지 않았다.

최종10명령/고유10사례·실패0이다. 기존 원문180행·helper 장기 문자열·동시 루트 반납·Enter/Tab 입력·부분 생성 실패·빈 제출의 challenge·실제 Console 서식 설정·스타일/클립 표면을 재확인했다. Release1명령 종료0·경고/오류0이다.921의 줄바꿈 정리로 재컴파일된 파일도 동작 변경은 없으며 빌드 중 소스를 편집하지 않았다.

보호2787개/fixture967개 불변·새 Native/fixture0·checking8·제품921 EXE SHA `fca9945c0f858e6d9088680ad359fd948ab4106d6dba0ce31ad2d47b5620dcd0` 불변이다. 실제 게임/GUI·포획/희생/공격/승리·공중3종 전용·신규MP/패킷/통신·물리입력/PCM/CD·전체 검사/34미션/ZIP은 실행하지 않았다. 자연 도달·전체 Chat 픽셀/물리 클립보드 완료로 합산하지 않는다.

기계 판독 결과는 [JSON](../../cpppj/chat-helper-modes-ui-20261105.json), 원문/검사·빌드·보호 해시는 Git 제외 `extracted/cpp-resume-922/`이다. 다음923도 UI1의 남은 명령 효과와 상위 수명을 계속한다.
