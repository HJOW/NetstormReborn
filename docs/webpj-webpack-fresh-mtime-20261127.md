# 동일 번들의 자동 갱신 반복 방지

WB04-E1406, 2026-11-27, HJOW-Athlon. ESLint 설정 변경 뒤 게임 번들이 같은 바이트이면 webpack이 출력을 건너뛰어 파일 시각이 남았다. 실제로 두 정상 빌드 뒤에도 번들은12:17 시각이고 ESLint 설정이 더 최신이라 `bundleIsFresh()`가 false였다. 이 상태에서는 서버 시작 때 같은 빌드를 반복한다.

webpack의 `output.compareBeforeEmit`을 false로 설정해 **검사를 통과한 정상 빌드는 같은 바이트라도 기록**하도록 했다. [공식 출력 옵션](https://webpack.js.org/configuration/output/#outputcomparebeforeemit)을 사용하며 새 메타 파일이나 배포 파일을 추가하지 않는다. ESLint 선행 관문이 컴파일 전에 실패하므로 오류/경고 때는 출력과 시각을 보존한다.

정상 운영 빌드는 성공했고3,500,786바이트·SHA-256 e24f9de3…0b로 E1400과 바이트가 같다. 실제 운영 오류·개발 경고·자동 재생성 오류는 모두 종료2였으며 기존 SHA/mtime이 같았다. 임시 파일을 제거한 뒤 자동 최신 검사를 두 번 연속 실행해 모두 재생성 없이 성공했고 수정 시각도 같았다.

[변경 전 입력/출력 시각](webpj-e1406-evidence/before-mtimes.json), [정상 빌드](webpj-e1406-evidence/build.log), [세 실패 경로](webpj-e1406-evidence/gate-results.json), [첫 최신 확인](webpj-e1406-evidence/ensure-first.log), [둘째 최신 확인](webpj-e1406-evidence/ensure-second.log), [출처·해시](webpj-e1406-evidence/provenance.json)를 보존했다. 게임 소스는 바꾸지 않았다.

동시에 준비한 E1404 메모리 도구도 전체 lint에 포함됐지만, 이번 단위로 메모리 관문을 완료했다고 세지 않는다. E1397 정기 뒤 이미 진행하던 병행 단위까지5/4·5/8이며, 임시 오류 파일을 모두 제거했으므로 E1405 Node26 전체를 바로 시작한다.
