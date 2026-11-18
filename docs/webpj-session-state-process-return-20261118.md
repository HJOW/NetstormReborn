# WB04-E785 Session State Process Host 반납

cpppj `RawCampaignSession.cpp`의 `ClearProcessesForState`, `ReturnProcessBindingsForState`를 같은 Session World의 실제 공유 Process Host 대여에 연결했다. Host의 Clear가 정상 복귀해 발급한 증빙만 같은 대여가 소비한다. 반납 성공 뒤에만 Native 건설 장치 대여 칸을 비우며, 현재 웹에서는 Native 건설 장치 자체는 아직 설치하지 않는다.

서로 독립된 실제 Session에서 일반 Regular 프로세스를 Host에 부착하고 Clear 뒤 Kernel이 비워짐을 확인했다. 증빙 없는 반납, 다른 Session에 건네기, 중복 반납 및 별도 Kernel의 공유 대여 없는 경로를 거부했다. 해당 프로세스는 실행하지 않고 정리만 했다. 실제 Native 건설 수명과 다른 State 자식·전체 종료 순서 및 앱 연결은 남는다.

Node v26.13.0 전체 회귀 6,288건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E779의 Node v22 전체 회귀 뒤 여섯 번째 v26 회귀다.
