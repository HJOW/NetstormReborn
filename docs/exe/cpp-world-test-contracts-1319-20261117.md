# cpppj 1319 — 그림자 기대값과 공유 커널 검사 수명 교정

2026-11-17, HJOW-Athlon. 제품1315의 코드는 그대로 두고 `CampaignSessionTests.cpp`의 오래된 비MP 검사 실패와 검사 종료 접근 위반을 해결했다. 원문·기존 fixture 변경은 없다.

제네레이터 표시 검사는 그림자 팔레트를 소스 잉크에 적용해 예상 화면을 만들었지만, 실제 그림자는 불투명 마스크 안의 **대상 화면색**을 변환한다. `DrawIndexedImage`의 마지막 `shadow` 인자를 빠뜨린 것이 원인이다. 실제 출력은 배경199의 그림자61, 옛 기대는 그림자 도형 소스의 변환70이었다. 기대값 작성기에 `shadow=true`를 전달했고, 세 제네레이터·두 카메라에서 본체/그림자의 전체 화면 바이트 비교를 그대로 유지한다. 향후 차이가 나면 TYPE·프레임·카메라·첫 다른 픽셀을 기록하도록 검사 진단도 더했다.

공유 커널 검사는 Session 소멸이 외부 Client 프로세스까지 모두 지운다는 옛 계약을 기대했다. 현행 `RawSurfaceWorld::DrainCurrentProcessesForShutdown`은 해당 Host가 소유한 프로세스만 반납한다. 이는 다른 Host/Client 객체 보존을 위한 의도된 계약이다. 검사가 만든 외부 관측자를 검사에서 명시 정리하고, Session 소멸 뒤에는 해당 외부 PID가 여전히 존재해야 함을 검사한다.

최종 정리 검사에서 Native 단일 Clear가 앞 슬롯에 남긴 외부 관측자 역시 Session 소멸로 지워지지 않는다. 옛 검사는 Client 최종 소멸 때까지 살아남은 관측자의 callback이 이미 반납한 Session/서비스를 읽어 접근 위반을 일으켰다. PlayerLinks 정리 → Session이 살아 있을 때 명시 전역 Drain → 서비스 반납의 순서로 검사 장치를 정리했다. 실제 단일 Clear의 잔류1개, 세 Player DWORD의0, Player 대여 유지, 서비스 미반납 단언은 모두 보존한다.

- Release `netstorm_world_generation_tests` 빌드 종료0.
- 제네레이터 검사 **1/1 통과**(이전6단언 실패).
- 공유 커널 검사 **4/4 통과**(이전3건 실패 중1건0xC0000005).
- 제품 코드·일반 게임 실행·사제 포획/희생·공중3종·MP 실행은 변경/수행하지 않았다. 전체 world_generation 통과로 확대하지 않는다.
- 원시: 수정 전 개별 관측은 `extracted/cpp-baselines-1318/`, 진단·최종 결과는 `extracted/cpp-world-tests-1319/`의 `generator-diagnostic.log`·`generator-final.log`·`kernel-after.log`·`kernel-final.log`·두 빌드 로그다.

마지막 전체 디컴파일 호스트는 HJOW-Athlon으로 유지한다. 남은 옛 AI 운반체 포로 내용물 검사는 이번 실행에서 제외했으며, Guide/Edit·다른 검사 기준선은 별도다.
