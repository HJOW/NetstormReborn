"""L2 수집기의 잘못된 필드 위치·메뉴 덤프·누락 경로를 검출한다. 게임은 실행하지 않는다."""

import tempfile
from pathlib import Path
import unittest

import l2_compare


class L2CompareTests(unittest.TestCase):
    """서로 다른 필드 바이트를 주어 타입·상태·소유자·프레임 혼동을 막는다."""

    def test_native_field_offsets(self):
        """타입은+10BYTE, 소유자는+34BYTE, 프레임은+36DWORD 전체를 비교한다."""
        with tempfile.TemporaryDirectory() as directory:
            cpp, dn = Path(directory) / "cpp.tsv", Path(directory) / "dn.tsv"
            left, right = bytearray(50), bytearray(50)
            left[10] = right[10] = 97
            left[11], right[11] = 0, 4
            left[8], right[8] = 1, 2
            left[34] = right[34] = 7
            left[36], right[36] = 1, 2
            # 양쪽 프레임 상위WORD는 같고 하위BYTE만 달라 잘못된+38비교를 검출한다.
            for path, raw in ((cpp, left), (dn, right)):
                path.write_text("netstorm_session_dump\t1\nrandom\t1\t2\nworld\t1\nslot\t6\t" + raw.hex() + "\nend\n", encoding="ascii")
            result = l2_compare.compare(cpp, dn)
            self.assertEqual(1, result["type_equal"])
            self.assertEqual(1, result["owner_equal"])
            self.assertEqual(0, result["frame_equal"])
            self.assertFalse(result["l2_equal"])
            self.assertTrue(l2_compare.compare(cpp, cpp)["l2_equal"])

    def test_menu_dump_is_rejected(self):
        """실행이0종료해도 world0이면 미션 기준선으로 세지 않는다."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "menu.tsv"
            path.write_text("netstorm_session_dump\t1\nworld\t0\nend\n", encoding="ascii")
            with self.assertRaises(ValueError):
                l2_compare.read_dump(path)

    def test_duplicate_and_short_slots_are_rejected(self):
        """동일 SID 덮어쓰기와 손상된 슬롯을 성공으로 합산하지 않는다."""
        row = ["slot", "5", "00" * 50]
        with self.assertRaises(ValueError):
            l2_compare.slots([row, row])
        with self.assertRaises(ValueError):
            l2_compare.slots([["slot", "5", "00"]])

    def test_routes_include_distinct_demo_starts(self):
        """31개 일반 미션과 시작 방식이 다른3데모를 별도로 센다."""
        self.assertEqual(31, len(l2_compare.MISSIONS))
        self.assertEqual(31, len(set(l2_compare.MISSIONS)))
        self.assertEqual(3, len(l2_compare.DEMOS))


if __name__ == "__main__":
    unittest.main()
