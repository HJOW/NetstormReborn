#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cpppj 명칭 교정이 지정 치환과 당시 유효한 SHA 연결 갱신만 포함하는지 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

# 저장소와 명칭 교정의 독립 후속 기록 위치다. 보호 원본이나 역사 사본에는 쓰지 않는다.
ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'cpppj/korean-name-migration.json'
# AGENTS.md와 공유 명칭 표에서 정한 한국어 표기다. 영어 심벌과 원본 영어 문자열은 치환하지 않는다.
TERMS = (
    ('썬 아처', '썬 디스크 발사기'), ('윈드 아처', '크로스보우'), ('썬더 아처', '밴더 타워'),
    ('썬 아비어리', '훨리베이스'), ('윈드 아비어리', '데빌 메이커'), ('레인 아비어리', '맨 오 워 풀'),
    ('썬 플라이어', '훨리기그'), ('윈드 플라이어', '더스트 데빌'), ('레인 플라이어', '맨 오 워'),
    ('썬 팩토리', '썬 워크샾'), ('윈드 팩토리', '윈드 워크샾'),
    ('레인 팩토리', '레인 워크샾'), ('썬더 팩토리', '썬더 워크샾'),
    ('썬 블로커', '스톤 타워'), ('윈드 블로커', '윈드 타워'),
    ('레인 블로커', '아이스 타워'), ('썬더 블로커', '불워크'),
    ('썬 펜스', '썬 바리케이드'), ('레인 펜스', '애시드 바리케이드'), ('썬더 펜스', '아크 스파이어'),
    ('썬 워커', '골렘'), ('윈드 워커', '세일 스케이터'), ('레인 워커', '크리스탈 크랩'),
    ('썬 벌룬', '벌룬'), ('윈드 벌룬', '에어 쉽'), ('레인 벌룬', '클라우드 플로터'),
    ('윈드 배터리', '윈드 제네레이터'), ('레인 배터리', '레인 제네레이터'), ('썬더 배터리', '썬더 제네레이터'),
    ('윈드 보텍스', '윈드 신전'), ('레인 보텍스', '레인 신전'), ('썬더 보텍스', '썬더 신전'),
    ('레인 캐논', '아이스 캐논'),
    ('휘리베이스', '훨리베이스'), ('휘리기그', '훨리기그'),
    ('레지던스', '거주 구역'), ('가이저', '간헐천'),
    ('알타', '제단'), ('알터', '제단'), ('거주지', '거주 구역'), ('거처', '거주 구역'),
    ('워크샵', '워크샾'), ('제너레이터', '제네레이터'), ('발전기', '제네레이터'),
    ('프리스트', '사제'), ('엠프티', '고갈된'), ('배리케이드', '바리케이드'),
)
# 받침 유무에 따라 달라지는 조사를 함께 교정한다. 방향 조사의 ㄹ 받침 예외는 따로 처리한다.
PARTICLES = (('가', '이'), ('는', '은'), ('를', '을'), ('와', '과'))
# JSON의 현재 설명 필드만 교정한다. 과거 diff·원문·이전 보고서 문자열은 증거로 보존한다.
DESCRIPTION_KEYS = frozenset(('description', 'scope', 'limits', 'limitations'))
# 현재 파일 경로를 SHA에 연결하는 두 보고서 형식이다. 다른 역사·중간 상태 필드는 건드리지 않는다.
HASH_FIELDS = ('files', 'sha256')


def digest(body):
    """줄 끝을 포함한 파일 전체 바이트의 SHA-256을 반환한다."""
    return hashlib.sha256(body).hexdigest()


def final_consonant(text):
    """마지막 한글 음절의 종성 번호를 반환한다. 0은 받침 없음, 8은 ㄹ이다."""
    return (ord(text[-1]) - 0xac00) % 28


def translate(text):
    """지정한 옛 명칭과 조사만 바꾸고 템플릿 같은 일반 단어는 보존한다."""
    # 긴 명칭부터 처리하여 합성 이름을 다른 이름으로 중복 치환하지 않는다.
    for old, new in TERMS:
        before_final, after_final = final_consonant(old), final_consonant(new)
        if bool(before_final) != bool(after_final):
            # 이름 바로 뒤의 조사를 새 이름의 받침에 맞춘다.
            for vowel, consonant in PARTICLES:
                before, after = (vowel, consonant) if after_final else (consonant, vowel)
                text = text.replace(old + before, new + after)
        before_ro = '로' if before_final in (0, 8) else '으로'
        after_ro = '로' if after_final in (0, 8) else '으로'
        if before_ro != after_ro:
            text = text.replace(old + before_ro, new + after_ro)
        text = text.replace(old, new)
    return re.sub(r'템플(?!릿)', '신전', text)


def descriptions(value):
    """선택한 현재 설명 필드 안의 문자열·목록을 재귀적으로 교정한다."""
    if isinstance(value, str):
        return translate(value)
    if isinstance(value, list):
        return [descriptions(item) for item in value]
    if isinstance(value, dict):
        return {key: descriptions(item) for key, item in value.items()}
    return value


def json_translation(body):
    """보고서 최상위 설명만 고치고 수치·관찰값·역사 원문은 그대로 둔다."""
    value = json.loads(body)
    if not isinstance(value, dict):
        return value
    # 정의된 설명 키 이외의 모든 필드는 원래 JSON 값을 유지한다.
    for key in DESCRIPTION_KEYS & value.keys():
        value[key] = descriptions(value[key])
    return value


def baseline_bytes(commit, name, ending):
    """기준 Git 객체를 읽고 기록된 작업 사본 줄 끝으로만 복원한다."""
    body = subprocess.run(['git', 'show', commit + ':' + name], cwd=ROOT, check=True, capture_output=True).stdout
    if ending == 'crlf':
        return body.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
    if ending != 'git':
        raise RuntimeError('알 수 없는 기준 줄 끝: ' + ending)
    return body


def fixture_data(body):
    """fixture의 주석을 제외한 입력·출력 행을 원래 순서와 바이트로 반환한다."""
    return b''.join(line for line in body.splitlines(keepends=True) if not line.startswith(b'#'))


def verify():
    """이전·현재 바이트, 명칭 치환, 유효 SHA 연결과 fixture 관찰값 불변을 전수 검사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    if report['tool_sha256'] != digest(Path(__file__).read_bytes()):
        raise RuntimeError('명칭 감사 도구 SHA 불일치')
    if report['new_native_inputs'] or report['game_runs']:
        raise RuntimeError('명칭 교정을 새 관찰이나 게임 실행으로 계수할 수 없음')
    old_files = {}
    # 모든 대상의 기준 SHA와 현재 SHA를 먼저 고정하여 연결 갱신의 양 끝을 확인한다.
    for name, entry in report['changes'].items():
        if not name.startswith(('cpppj/src/', 'cpppj/tests/', 'tools/', 'docs/exe/')) and not (name.startswith('cpppj/') and name.count('/') == 1 and name.endswith('.json')):
            raise RuntimeError('명칭 감사 범위 밖 파일: ' + name)
        old = baseline_bytes(report['baseline_commit'], name, entry['baseline_line_endings'])
        if digest(old) != entry['previous_sha256'] or digest((ROOT / name).read_bytes()) != entry['current_sha256']:
            raise RuntimeError('명칭 교정 전후 SHA 불일치: ' + name)
        old_files[name] = old
    # JSON 연결은 교정 직전에도 유효했던 파일 SHA만 새 값으로 바꿀 수 있다.
    for name, entry in report['changes'].items():
        old = old_files[name]
        current = (ROOT / name).read_bytes()
        if entry['kind'] == 'evidence':
            expected = json_translation(old)
            # 과거에 이미 어긋났던 SHA를 현재 값으로 덮어 검증 성공으로 위장하지 않는다.
            for link in entry['hash_links']:
                target = report['changes'][link['path']]
                if link['field'] not in HASH_FIELDS or expected[link['field']][link['path']] != target['previous_sha256']:
                    raise RuntimeError('교정 전부터 유효하지 않았던 SHA 연결: ' + name)
                expected[link['field']][link['path']] = target['current_sha256']
            if json.loads(current) != expected:
                raise RuntimeError('설명·유효 SHA 연결 이외 JSON 변경: ' + name)
        else:
            if current != translate(old.decode('utf-8')).encode('utf-8'):
                raise RuntimeError('명칭·조사 이외 본문 변경: ' + name)
            if entry['kind'] == 'fixture' and fixture_data(old) != fixture_data(current):
                raise RuntimeError('fixture 관찰값 변경: ' + name)
    print('cpppj 명칭 교정 전후 SHA/지정 치환/유효 연결/원본 관찰값 불변 감사 통과:', len(report['changes']))


# 명시적인 검증 요청만 허용하며 보고서와 관찰 자료를 생성·수정하지 않는다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true', required=True)
    parser.parse_args()
    verify()
