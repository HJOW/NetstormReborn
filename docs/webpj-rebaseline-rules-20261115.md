# 재기준선 신규 단위 분류 규칙과 Outpost 대상 전환 (WB04-E512)

기존 기준선 `1f00a25a`와 후보 `225553df`의 `o/`·`client/`·`zacket/` 단위
집합 차이 176개를 경로별 규칙으로 고정했다. 실제 호출 범위의 근거는
[표면](webpj-rebaseline-surface-20261115.md), [플랫폼·편집](webpj-rebaseline-boundaries-20261115.md),
[게임 규칙](webpj-rebaseline-gameplay-20261115.md), [전송](webpj-rebaseline-transport-20261115.md),
[메타·전투](webpj-rebaseline-meta-20261115.md)에 있다. E505의 일반 Sprite 갱신 경로 두 개
재판정을 포함해 port 100·adapt 7·exclude-edit 7·exclude-mp 57·exclude-host 5다.
각 단위의 경로는 `webpj/tools/rebaseline-225553df.json`에 기록하며,
이름이 같은 다른 폴더의 파일까지 새 판정을 적용하지 않는다.

사용자가 결정한 12차 스커미시의 Outpost를 위해 기존 `RawOutpostLifecycle`은
`exclude-mp`에서 `port`/`todo`로 바꿨다. 대응표 재생성 전후 984행을 비교해
그 행만 분류·상태·규칙 접두가 달라짐을 확인했다. 기존 `RawRegionOwnership`의
Outpost 분기는 스커미시에서 이식할 후속 범위로 메모를 바로잡았다.
Outpost의 실제 수명·지역 소유 연결은 아직 구현하지 않았다.

경로별 분류 검사는 후보와 옛 기준선의 단위 집합 차이, 중복 없는 176개,
분류별 개수와 공유 Sprite·Fort·스커미시·시간 경계를 확인한다. 기존 기준선
대응표 검사와 Node 22·26 전체 검사를 수행했다. 새 176개는 아직 대응표에
등록되지 않았으며 후보 오라클 전환과 기준선 변경이 다음 작업이다.
