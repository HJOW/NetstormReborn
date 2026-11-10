# ClientMain 혼합 줄바꿈 복원 — 작업1256

1255에서 Python 텍스트 재작성으로 ClientMain.cpp의 기존 CRLF1,426개가 LF로 바뀐 불필요한 차이를 발견했다. 이 파일은 기존 `.gitattributes`의 `-text`/`cr-at-eol` 경계다.1255 직전 원시가 Git 부모 blob과 동일함을 확인한 뒤 동일 본문 행의 줄바꿈을 그대로 복원하고 새3행은 인접 행 형식을 따른다.

복원 후 CRLF1429개/전체 LF1536개다. **CRLF→LF 정규화 본문은 수정 전후 바이트가 동일**하고 줄 수/코드/주석은 바뀌지 않았다. 확정 실행 제품1255/11.01과 Netstorm/R SHA도 불변이다. 추가 빌드/검사/GUI0이며1255의 Release·비MP300·Tutorial4 근거를 새 실행으로 합산하지 않는다. 보호 원본/CD/fixture·AGENTS2,808파일 불변이다.

앞으로 `-text` 소스는 bytes 기반 국소 수정/apply_patch로 기존 혼합 줄바꿈을 보존한다. 속성/저장소 설정은 바꾸지 않았다. 다음은1255 인계의 Native Session 고정 F 재등록이며 M6-l/전체 목표는 미완료다.

[정리 보고서](../../cpppj/client-main-line-endings-20261110.json), 원시 `extracted/cpp-resume-1256/`.
