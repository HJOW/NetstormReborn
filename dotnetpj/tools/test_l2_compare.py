"""L2 수집기의 잘못된 필드 위치·메뉴 덤프·누락 경로를 검출한다. 게임은 실행하지 않는다."""

import struct
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
                path.write_text("netstorm_session_dump\t1\nframe\t40\t4607182418800017408\nrandom\t1\t2\nworld\t1\nslot\t6\t" + raw.hex() + "\nend\n", encoding="ascii")
            result = l2_compare.compare(cpp, dn)
            self.assertTrue(result["game_time_bits_equal"])
            self.assertEqual({"frame": 40, "game_bits": 4607182418800017408, "game_seconds": 1.0}, result["cpp_clock"])
            self.assertEqual(1, result["type_equal"])
            self.assertEqual(1, result["owner_equal"])
            self.assertEqual(0, result["frame_equal"])
            self.assertFalse(result["l2_equal"])
            self.assertTrue(l2_compare.compare(cpp, cpp)["l2_equal"])

    def test_clock_detects_different_game_time_and_rejects_invalid_rows(self):
        """같은 프레임의 다른 실제 시각을 분리하고 손상된 행을 성공으로 처리하지 않는다."""
        def clock(value):
            """시험용 배정밀도 값을 실제 덤프의 정수 비트로 바꾼다."""
            return str(struct.unpack("<Q", struct.pack("<d", value))[0])
        left = [["frame", "40", clock(9.35)]]
        right = [["frame", "40", clock(1.0)]]
        self.assertNotEqual(l2_compare.frame_clock(left)["game_bits"], l2_compare.frame_clock(right)["game_bits"])
        self.assertEqual(9.35, l2_compare.frame_clock(left)["game_seconds"])
        # 누락, 중복, 범위 초과와 비유한 시각은 실제 대조에서 오류다.
        for rows in ([], left + left, [["frame", "40", "18446744073709551616"]], [["frame", "40", clock(float("nan"))]]):
            with self.assertRaises(ValueError):
                l2_compare.frame_clock(rows)

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

    def test_demo_entry_uses_matching_paths(self):
        """cpp 데모는 StartDemo 명령으로, DN 데모는 --mission/--demo로 진입한다."""
        self.assertEqual(["--mission", "guardmyback", "--handle", "Youpson", "--demo"], l2_compare.entry_args("dn", "guardmyback", True))
        self.assertEqual(["--mission", "thewarbegins", "--handle", "Youpson"], l2_compare.entry_args("dn", "thewarbegins", False))
        self.assertEqual([], l2_compare.entry_args("cpp", "demostormrages", True))
        self.assertEqual(["--mission", "thewarbegins"], l2_compare.entry_args("cpp", "thewarbegins", False))
        self.assertEqual("5\tscript-command\tStartDemo,demostormrages\n", l2_compare.entry_script("cpp", "demostormrages", True))
        self.assertEqual("", l2_compare.entry_script("dn", "demostormrages", True))
        self.assertIn("Play Mission", l2_compare.entry_script("cpp", "thewarbegins", False))
        self.assertEqual("", l2_compare.entry_script("dn", "thewarbegins", False))


if __name__ == "__main__":
    unittest.main()
