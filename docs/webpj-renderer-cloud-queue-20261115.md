# 렌더러 구름 큐의 고정 저장소 (WB04-E496)

기준은 `1f00a25a`의 `client/ClientRendererFrameGlobals.h`,
`client/RawRendererClouds.cpp`와 `tests/RendererCloudTests.cpp`·
`tests/RendererFrameTests.cpp`이다. `225553df`까지 해당 전역 헤더와
구름 처리기의 기준선 뒤 변경은 없다.

구름 사각형 네 칸은 프레임보다 오래 사는 고정 저장소다. 이전 웹 구현은
큐에 기록할 때 배열 원소를 새 객체로 바꾸어 미리 빌린 칸 참조가 갱신을
보지 못했다. 이제 기존 칸에 signed 32비트 좌표를 쓰고 사각형 입력은
여전히 값으로 복사한다. 기존 프레임 전역 초안의 공유 큐를 실제 `Renderer`와
`prepareRendererFrame`에 연결한 검사에서 대여 참조, dirty 표 초기화와
전체 화면 영역의 재등록을 확인한다.

원본 10.78 `renderer-clouds` fixture 695행과 집중 검사 7건이 통과했다.
새 참조 동일성 검사는 기존 객체 교체 코드에서 실패한다. 게임이나 브라우저
실행은 이 단위의 검증에 포함하지 않는다.

전체 회귀는 Node v22.23.3·v24.22.3·v26.13.0에서 각각 4,965건 통과,
실패 0건이다. `port-map.mjs`는 984행·오류 0건이다.

`ClientRendererFrameGlobals` 전체는 실제 `ClientRendererFrame` 소유·프레임
연결이 남아 있으므로 대응표에서 partial을 유지한다. 미니맵 초안의 실제
메뉴·월드 연결과 cpppj 오라클 재기준선도 별도 작업이다.
