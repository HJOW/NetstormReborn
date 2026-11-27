# dotnetpj 1.080a/b — 미니맵 연결 뒤 진입 검증 복구

2026-11-27, HJOW-Athlon. 1.080 중간 커밋의 생산 루트 Idle 차단 후
Game 재빌드와 관련 검사, 대표 세 미션 진입 덤프를 다시 확인했다. 실제
미니맵 픽셀과 진입·정리 수명은 R5-4c3c에 남는다.

- Game Release 빌드: 오류 0, 기존 Core nullable 경고 6건과 Client 중복
  using 경고 2건. 제품 소스는 이번 검증에서 수정하지 않았다.
- 미니맵·생산 틀·생산 창·Root UI 관련 집중 검사 12/12 통과.
  `cpp_sync.py` 종료 0. 전체 회귀 7,385건은 차단 전 중단 시점 결과로
  유지하며 차단 후 재실행한 결과로 표시하지 않는다.
- `thewarbegins`, `tutorial1`, `surrounded`: 각 CPP/DN 실행 종료 0,
  DN의 90프레임 덤프가 `extracted/dotnet-l2-0998`과 각각 바이트 일치.
  대표 경로 재수집은 34경로 전수 실행이 아니며 M1 기존 0/34 기준은
  바뀌지 않는다. CPP와 DN 간 기존 차이 때문에 비교 도구는 종료 1이다.
  결과는 `extracted/dotnet-l2-1080-retry-war` 및
  `extracted/dotnet-l2-1080-retry-rest`에 보존했다.
- 두 수집 모두 `originals` 1,392파일의 실행 전후 해시 불변을 확인했다.
  이전 세 DN 종료 1 수집 `extracted/dotnet-l2-1080`은 보존했다.

다음은 실제 미니맵 영역을 CPP 화면과 비교하고 미션 진입·정리의 소유
수명을 확인하는 R5-4c3c다. 미니맵 입력과 생산 내용물은 해당 후속 단계에
남는다. 전체 목표는 미완료다.
