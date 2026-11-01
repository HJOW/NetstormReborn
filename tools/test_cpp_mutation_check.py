#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""검사 프로세스 실패/빈 필터/불일치가 변이 기준선 통과로 기록되지 않게 확인한다."""
import unittest
from unittest.mock import patch
import cpp_mutation_check as mutation


class MutationProcessTests(unittest.TestCase):
    """실제 외부 프로세스의 출력/종료코드 계약을 명시 고장 입력으로 검사한다."""
    def test_expected_failure_and_target(self):
        """의도한 실패 검사만 검출하고 지정 EXE/필터를 실제 명령 인자로 사용한다."""
        output = '[FAIL] ViewSpot_NativeArguments\n1 test(s), 2 failed check(s)\n'
        with patch.object(mutation, 'run', return_value=(1, output)) as run:
            self.assertEqual(mutation.test('ViewSpot_', 'netstorm_viewspot_tests'), (['ViewSpot_NativeArguments'], 2))
        self.assertEqual(run.call_args.args[0], [str(mutation.BUILD/'bin/Release/netstorm_viewspot_tests.exe'), '--filter', 'ViewSpot_'])

    def test_valid_baseline(self):
        """등록된 검사가 실행되고 요약/종료코드가 일치하는 경우만 정상 기준선이다."""
        with patch.object(mutation, 'run', return_value=(0, '[ OK ] ViewSpot_NativeArguments\n2 test(s), 0 failed check(s)\n')):
            self.assertEqual(mutation.test(target='netstorm_viewspot_tests'), ([], 0))

    def test_crash_empty_or_inconsistent_process_is_not_pass(self):
        """크래시·미실행·실패 목록 누락·0종료 오보를 통과로 집계하지 않는다."""
        cases = ((1, ''), (0, ''), (1, '0 test(s), 1 failed check(s)\n'),
                 (1, '2 test(s), 0 failed check(s)\n'), (0, '[FAIL] ViewSpot_Bad\n2 test(s), 1 failed check(s)\n'),
                 (1, '2 test(s), 1 failed check(s)\n'))
        for code, output in cases: # 기대값은 각 외부 고장 신호를 거부하는 계약이다.
            with self.subTest(code=code, output=output), patch.object(mutation, 'run', return_value=(code, output)):
                with self.assertRaises(RuntimeError):
                    mutation.test(target='netstorm_viewspot_tests')


if __name__ == '__main__':
    unittest.main()
