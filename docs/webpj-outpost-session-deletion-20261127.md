# 실제 Session의 Outpost 삭제 연결

WB04-E1434, 2026-11-27, HJOW-Athlon. Outpost의 실제 가상 표0x509210 삭제를 기존 RawOutpostLifecycle.preDestroyNative와 같은 Session의 현재 Outpost/거주 구역 두 목록, 지역 소유 갱신, Damageable 부모에 연결했다. 실제 가상 삭제 래퍼도 Outpost를 선택하고 후처리는 기존 공통 Damageable 경로를 사용한다. 표시 TYPE를 가상 자격 대신 쓰지 않는다.

수정 전에는 실제 Session을 조립하는 새 통합 검사3개 모두 가상 전처리 미연결로 실패했다. 수정 뒤 두 목록의 중복 전체 압축·비활성 꼬리 보존 → 지역 통지 → 피해 부모 순서, abstract/묻힌 상태의 생략, 표시 TYPE가 달라도 현재 VT로 선택, 지역 콜백 예외 때 앞선 두 목록 제거 보존/뒤 부모 미호출을 확인했다. 현재 부모와 풀을 사용하며 선택되지 않은 효과는 가짜 성공으로 대신하지 않았다.

후보와 본 소스 원시 바이트/SHA-256이 같다. 원본10.78 Outpost768행을 읽는 기존6개와 새3개 후보9/9, 기존 가상 삭제/자산 삭제 표까지 포함한 본 소스23/23, ESLint 선행 운영 빌드, 재번들 일치1/1, 기존 엄격 Chromium L2 화면/덤프1/1을 통과했다. 번들은3,514,344바이트다. 실제 플레이에서 Outpost를 강제 생성/파괴한 시험은 아니다. 실제 부모의 조립/전처리·후처리 분배 검증이며 일반 건설·금지·AI/전투 등은 별도 점검한다.

첫 본 소스 실행은 존재하지 않는 session-world-deletion.test.mjs 이름도 전달했는데 Node26은 그 입력을 선택하지 않아 실제9개만 실행했다. 이름을 확인해 raw-campaign-session-deletion.test.mjs와 asset-deletion-vtables.test.mjs를 넣고23개를 다시 통과했다. 기존 단언을 제거하거나 첫9개를23개로 보고하지 않는다. E1436에서도 이름이 다른 Session AI 두 경로를 확인해 올바른 파일의11개를 추가 통과했다. [추가 원문](webpj-e1436-evidence/webpj-e1436-main-session-focus.log)을 함께 보관한다.

v26=4/4·v22=8/8로 다음 작업 전에 작성 입력을 고정하고 Node26/Node22 전체를 순차 실행한다. 전체 예정6,919개이며 결과는 실제 실행 뒤 별도 기록한다.

[수정 전 실패](webpj-e1434-evidence/before.log) · [후보](webpj-e1434-evidence/candidate-first.log) · [본 소스23개](webpj-e1434-evidence/webpj-e1434-main-full-focus.log) · [빌드](webpj-e1434-evidence/webpj-e1434-build.log) · [번들](webpj-e1434-evidence/webpj-e1434-bundle.log) · [L2](webpj-e1434-evidence/webpj-e1434-l2.log) · [출처/해시](webpj-e1434-evidence/provenance.json).
