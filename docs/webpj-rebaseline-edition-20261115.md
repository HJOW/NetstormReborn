# CD 분기와 카메라 저장소 재기준선 판정

WB04-E506은 기존 이식분 8단위를 각 행의 기존 base와 후보 `225553df` 사이에서 대조했다. 웹은 10.78을 대상으로 하며 CD 10.72 실행 경로는 포함하지 않는다.

| 단위 | 판정 | 근거 |
| --- | --- | --- |
| `ClientPlayerRecords` | ① | 판본별 생성자·stride·할당 크기는 CD 172BYTE 행을 지원하기 위한 것이다. 기본 PATCH는 기존처럼 180BYTE씩 9행이다. |
| `ClientPlayerRuntime` | ① | 새 동맹 초기화는 CD 분기이며 PATCH의 기존 공통 시작과 동맹 본문은 유지된다. |
| `ConfigInterface` | ① | 추가 `resolveCdDirectory`는 설정을 읽은 뒤 CD 보조 자료를 선택한다. `ClientMain`은 CD 10.72에서만 설치하며 웹은 파일 어댑터를 사용한다. |
| `RawMissionAiSettings` | ① | 별도 초기 자금·NUL 절단·진단·지역 버퍼는 CD 분기다. PATCH의 600BYTE 설정 경로는 동일하다. |
| `RawMissionBattleOptions` | ① | CD 8BYTE 초기화가 추가됐고 PATCH 87BYTE 초기화는 바뀌지 않았다. |
| `RawMissionStart` | ① | CD의 시작 순서·단일 SP·문자열과 조회기 계약이 추가됐고 PATCH 시작 순서와 처리 결과는 유지된다. |
| `RawPlayerInitialization` | ① | CD 172BYTE Player 행 초기화가 별도로 추가됐고 PATCH 180BYTE 부모는 바뀌지 않았다. |
| `ClientCameraGlobals` | ② 기이식 | 추가된 건설 이력·숫자 키 저장 위치는 웹 E333에서 이미 Client 수명의 별도 객체로 만들었고 접근 시 같은 객체와 초기값을 확인한다. |

기존 웹의 Player·설정·AI·미션 시작·전투 옵션·카메라 관련 검사 50건을 다시 실행해 모두 통과했다. 이 판정은 CD 구현이나 미션 세션 연결을 완료했다는 뜻이 아니다. 4-R의 다른 변경 판정, 신규 단위의 대응표 반영, 기준선 전환과 세션 조립은 계속 남는다.
