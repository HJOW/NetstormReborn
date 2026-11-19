# WB04-E938: Client TextEntry와 Slider·버퍼

cpppj `RawTextEntryControls` 기준선 `225553df`의 생성·수명과 입력 조립 일부를 `ClientTextEntryControls`에 연결했다. 공통 Client 등록표·입력·idle·트리와 기존 TextEntry/Console, 실제 TextEntryStorage 및 두 Slider를 공유한다. 원본처럼 기본 배경을 생성하고, 파생 부착과 자식 우선 삭제를 같은 공통 수명에 맡긴다. 생성 전 할당과 부분 생성 실패에서 버퍼와 미부착 Slider도 정리한다.

공통 입력 분배를 통해 영문 및 확정 한글 태그가 같은 실제 편집 버퍼에 들어가고 점멸과 자식 반납이 일어나는지 확인했다. Console과 자산 장치 누락 오류도 검사했다. 원본 10.78 편집 관측 2,292행을 포함한 연관 Node 검사 63건이 통과했다.

측정·출력·클립보드 등 외부 장치는 호출자가 공급하며 실제 앱 글꼴과 출력, 브라우저 입력 소비 및 미션 UI에는 아직 연결하지 않았다. 실제 OS IME나 미션 플레이를 검증한 것은 아니다.
