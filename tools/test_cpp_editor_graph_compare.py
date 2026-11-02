#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""독립 원본 표면 관측을 사용해 Graph 대조기가 합계에 숨은 잘못된 소속을 거부하는지 확인한다."""
import copy
import csv
import json
import unittest

from cpp_editor_graph_compare import FIXTURES, NAME, membership


class MembershipTests(unittest.TestCase):
    """게임을 실행하지 않고 원본 고정 관측의 번호만 바꾸거나 의도적인 소속 결함을 넣는다."""

    def setUp(self):
        """최초 원본 자료의 674개 표면과 활성 장부를 읽어 비교 입력을 준비한다."""
        with (FIXTURES / (NAME + '-surfaces.tsv')).open(encoding='utf-8', newline='') as stream:
            self.native = [{key: int(value) for key, value in row.items()} for row in csv.DictReader(stream, delimiter='\t')]
        self.expected = json.loads((FIXTURES / (NAME + '.json')).read_text('utf-8'))['reloaded']
        self.current = copy.deepcopy(self.native)
        self.numbers = {row['sid']: 7 if row['graph'] == 2 else row['graph'] for row in self.current}
        self.records = {7: (670, 1)}

    def compare(self):
        """번호7을 원본2와 직접 같다고 요구하지 않고 실제 성분을 비교한다."""
        return membership(self.native, self.current, self.numbers, self.records, self.expected)

    def test_graph_renumbering_is_allowed(self):
        """별도 Pool에서 번호가 바뀌어도 표면의 연결 성분과 장부가 같으면 통과한다."""
        self.assertTrue(self.compare()['passed'])

    def test_equal_counts_do_not_hide_swapped_no_graph(self):
        """정상 표면과 noGraph를 맞바꿔 합계670/4를 유지해도 잘못된 소속을 찾는다."""
        normal = next(sid for sid, graph in self.numbers.items() if graph == 7)
        excluded = next(sid for sid, graph in self.numbers.items() if graph == 254)
        self.numbers[normal], self.numbers[excluded] = 254, 7
        result = self.compare()
        self.assertFalse(result['passed'])
        self.assertFalse(result['noGraphMembershipMatches'])
        self.assertEqual(result['cppGraphHistogram'], {'7': 670, '254': 4})

    def test_missing_surface_is_rejected(self):
        """개별 소속 행이 없으면 합계가 맞아도 관측 성공으로 바꾸지 않는다."""
        self.numbers.pop(next(iter(self.numbers)))
        with self.assertRaisesRegex(ValueError, '누락/추가'):
            self.compare()

    def test_ledger_difference_is_rejected(self):
        """실제 소속 수가 같아도 원본 장부678 같은 별개 값을 합계670으로 정규화하지 않는다."""
        self.records[7] = (678, 1)
        self.assertFalse(self.compare()['passed'])

    def test_unobserved_active_component_is_rejected(self):
        """소속 표면 없이 활성화된 장부를 추가한 보고는 불완전한 관측으로 거부한다."""
        self.records[8] = (1, 1)
        with self.assertRaisesRegex(ValueError, '표면 없는 활성'):
            self.compare()

    def test_placement_overcount_is_preserved(self):
        """실제 원본 배치의678/실제670 차이를 그대로 허용하고 장부를670으로 바꾸면 거부한다."""
        with (FIXTURES / 'editor-graph-placement-723-surfaces.tsv').open(encoding='utf-8', newline='') as stream:
            self.native = [{key: int(value) for key, value in row.items()} for row in csv.DictReader(stream, delimiter='\t')]
        self.expected = json.loads((FIXTURES / (NAME + '.json')).read_text('utf-8'))['placement']
        self.current = copy.deepcopy(self.native)
        self.numbers = {row['sid']: 7 if row['graph'] == 1 else row['graph'] for row in self.current}
        self.records = {7: (678, 1)}
        self.assertTrue(self.compare()['passed'])
        self.records[7] = (670, 1)
        self.assertFalse(self.compare()['passed'])


if __name__ == '__main__':
    unittest.main()
