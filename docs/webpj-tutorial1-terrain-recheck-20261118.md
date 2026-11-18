# WB04-E753 실제 Tutorial1 지형·섬 재검증

E752에서 E304의 `Tutorial 1` 표기가 잘못된 것으로 확인되어, 실제 `tutorial1.english`가 선택하는 `BridgeTheGap.fort`의 지형·섬을 다시 대조했다. E304의 기존 TheWarBegins 검사는 보존하고 이름을 바로잡았다.

실제 원본 Fort(`\d\bridgethegap.fort`, 1,818바이트, SHA-256 `ae21dd37790875183fe0b5eadd7e5e9a87174d655ccb84a5bf3ec182666a064f`)의 Territory로 4-B 원본 월드·섬·지형 Builder를 구성했다. 독립 Python 계산으로 작성된 `webpj/tests/fixtures/terrain-expected.tsv`의 BridgeTheGap 행(447행, 파일 SHA-256 `f389b31dfeae3bc48b3455c5e769a5e1ba98ffa7dbb9c23cd3f53c7f931cf92e`)과 대조한 지면은 영역 0의 763칸이며 65,536바이트 마스크 SHA-256은 `e16dd045520434a91d8ba43d078c7a11f56f3b8a87303ab8a6b2fa2496882584`다. 섬 1개와 256개 청크의 섬·방향·영역 레코드·순번도 타입 모델과 일치한다. 기존 E304의 child는 호출 순서 기록 장치이며 저장 객체 전체의 실제 이식 검사가 아니다.

추가로 같은 Fort를 실제 typed Session에 적재해 별도 763칸 마스크와 같은 해시, 섬 1개 및 실제 256개 청크를 대조했다. 위치 음향은 호출 기록 장치로 받았다. 게임·브라우저 창이나 사제 포획·희생·승리, 공중 3종 전용 행동은 실행하지 않았다. Session Start·프레임·화면과 원본 실행 오라클 비교는 이 시험 범위가 아니다.

관련 집중 검사 55건이 통과했다. Node v26.13.0·v22.23.3 전체 회귀를 순차 실행해 각각 6,248건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이다. 새 fixture는 만들지 않았으며 기존 `terrain-expected.tsv`를 그대로 사용했다.

E304의 잘못된 미션명 때문에 재개했던 4-B의 Tutorial1 지형·섬 대조는 이 검사로 보완됐다. 다음은 4D-3의 남은 개발자 명령과 로그인 기준선 뒤 변경 판정, 이후 4D-3f의 미션 Start/사제 이동과 앱 연결이다.
