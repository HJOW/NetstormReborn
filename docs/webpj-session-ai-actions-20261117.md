# Session AI 다리·생산·부족 좌표 공개 행동 (WB04-E674)

기준선 `225553df`의 `RawCampaignSession.cpp` 4880..4904를 같은 Session의 현재 Player·AI 본체·기술·다리 경로·생산·경제·공용 난수·부족 좌표 장치에 전달한다. 실제 설치와 owner 1..8 관문, 본체가 없을 때의 false/0, 호출자 scratch와 float 입력 폭을 보존한다. 기준선 뒤 해당 cpppj 파일 변경은 없다.

실제 미설치 외곽과 독립 owner 계약으로 현재 토큰·기술·scratch·난수·반환값 및 부재 관문을 확인했다. 기존 원본 PE 다리 초기화 144행(SHA-256 `0450a3947aa6a4c6d07c5f5d368c0abd7656ea817c84756e8c5c83a219314901`)과 단계 600행(SHA-256 `2cd0d570668e97f0c45ffb720692b417bed0c8d4ba117e0a1a0be30822c6f19e`), 부족 좌표·경제 검사를 포함해 집중 47건 통과했다. 실제 미션 Load와 AI 자연 행동은 실행하지 않았으며 `loaded=false`다. 나머지 이동·배치 공개 전달은 후속이다.

Node v26.13.0 전체 6,100건 통과(실패·취소·건너뜀 0), 로그는 Git 제외 `extracted/webpj-e674-node26.log`다. Node 22는 E669 후 여덟 번째 전체 회귀에서 다시 실행한다.
