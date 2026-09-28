# 원본 데이터 포맷 명세

3단계(원본 분석 — 데이터 포맷) 결과물. 도구는 모두 `tools/` 에 있고, 추출 결과는 `extracted/`(git 제외)에 만든다.

| 문서 | 대상 | 상태 | 도구 |
|---|---|---|---|
| [taff.md](taff.md) | `netstorm.tarc` 아카이브 | 완료 | `tools/taff.py` |
| [shp.md](shp.md) | `d/_shapes.shp` 스프라이트 | 완료 | `tools/shp.py` |
| [type.md](type.md) | `.type` 오브젝트 정의 | 문법 완료, 의미 일부 추정 | `tools/typefile.py` |
| [mission-script.md](mission-script.md) | 미션·메뉴 스크립트 | 문법 개요, 머리·섹션 조회 규칙 완료 | C# `MissionScript` |
| [xlat.md](xlat.md) | 원본 다국어 체계 | 포맷·해석 규칙 완료 | C# `XlatTable`, `GameLanguage` |
| [config.md](config.md) | `options.cfg`, `setup.cfg`, `!color.dat`, 팔레트, 설정 조회·치환 규칙 | 포맷·조회·치환 규칙 완료 | `tools/nscfg.py`, C# `ConfigText`, `ConfigStore` |
| [vfs.md](vfs.md) | 느슨한 파일 + 아카이브 조회 순서 | 정적 분석 완료 | C# `GameFileSystem` |
| [chfnt.md](chfnt.md) | 비트맵 글꼴 캐시 | 용도 확인 | — |
| [fort.md](fort.md) | `.fort` 요새/맵 | 컨테이너·오브젝트 레코드 완료 (일부 섹션 내부 미해석) | `tools/fort.py` |
| [hlp.md](hlp.md) | `help/*.HLP` WinHelp | 본문 토픽·그림 추출 완료, 탐색 정보 미검증 | `tools/hlp.py` + helpdeco |

기타:
* 실행 파일 리소스 추출: `tools/peres.py` (비트맵·문자열·다이얼로그·커서)
* 실행 파일 디컴파일: `tools/ghidra/run_decomp.ps1` → `extracted/decomp/Netstorm.c`, 모듈 맵 `tools/ghidra/module_map.py` → [../exe/modules.md](../exe/modules.md)
* 오디오(`sound/*.wav`, `music/*.mus`): 전부 표준 PCM WAV (별도 문서 없음)
* WinHelp 게임 규칙 요약: [../gameplay/help-manual.md](../gameplay/help-manual.md)

## 추출 순서 (처음 받은 사람용)

```powershell
python tools/taff.py extract originals/netstorm.tarc extracted/tarc
python tools/shp.py export
python tools/typefile.py json
python tools/peres.py originals/Netstorm.exe extracted/res/Netstorm
powershell -ExecutionPolicy Bypass -File tools/ghidra/run_decomp.ps1   # 약 10~20분
```

WinHelp의 `helpdeco` 빌드와 추출 명령은 [hlp.md](hlp.md)의 재현 방법을 따른다.
