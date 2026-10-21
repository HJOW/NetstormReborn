using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 종속 조회·삭제(RawContainedLookup)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/ContainedLookupTests.cpp 의 Scene 과 같다: 실제 풀에 부모와 일곱 종속 슬롯을 두고
/// 타입·순번·삭제 여부·권한에 따라 조회하거나 가상 삭제를 요청한다.
/// 호출마다 반환 번호·삭제 요청 순서·지정 슬롯 8개 전체 바이트를 비교한다.
/// 기준 판본은 10.78 이며 originalCD·original1037 행은 읽지 않는다 (W03 삭제·수명 열째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawContainedLookupTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 10.72·원본 10.37 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수.</summary>
    private const int RowsPerEdition = 805;

    /// <summary>부모 슬롯 번호.</summary>
    private const int Parent = 50;

    /// <summary>슬롯의 가상 표 표식.</summary>
    private const uint Table = 0x12010000;

    /// <summary>입력 열의 순서(서버·패턴·타입·순번·제거·변형).</summary>
    private enum InputField
    {
        /// <summary>서버 풀인지.</summary>
        Server,

        /// <summary>종속 슬롯의 타입·상태 입력 패턴.</summary>
        Pattern,

        /// <summary>찾을 타입 번호.</summary>
        Type,

        /// <summary>앞에서 건너뛸 일치 개수.</summary>
        Ordinal,

        /// <summary>찾은 항목을 삭제할지.</summary>
        Remove,

        /// <summary>삭제 경계에서 바꿀 raw 변형.</summary>
        Mutate,

        /// <summary>입력 칸 수.</summary>
        Count,
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>한 종속 조회 행의 풀과 장면을 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>종속 체인을 가진 풀(32,768칸).</summary>
        public readonly RawSidPool Pool;

        /// <summary>예약·client·서버·예측 경계의 입력 번호 7개.</summary>
        public readonly int[] Children;

        /// <summary>가상 삭제 요청의 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>종속 조회 몸체.</summary>
        public readonly RawContainedLookup Lookup;

        /// <summary>fixture 입력 6칸.</summary>
        public uint[] Input { get; private set; } = new uint[6];

        /// <summary>장면을 만든다. 서버 여부별로 실제 권한을 다르게 준다.</summary>
        /// <param name="server">서버 풀인지.</param>
        public Scene(bool server)
        {
            Pool = new RawSidPool(32768, server);
            Children =
            [
                1,
                4,
                5,
                6,
                RawSidPool.ServerFirst - 1,
                RawSidPool.ServerFirst,
                RawSidPool.PredictableFirst + 1,
            ];
            Lookup = new RawContainedLookup(Pool, (sid, flags) => Delete(sid, flags));
        }

        /// <summary>슬롯 바이트를 직접 쓴다(검사 입력용).</summary>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <returns>쓰기 가능한 슬롯 바이트.</returns>
        private Span<byte> Slot(int sid) => Pool.RawSlot(sid);

        /// <summary>입력 표식과 상태·타입만 설정하며 결과를 재계산하지 않는다.</summary>
        /// <param name="values">fixture 입력 6칸.</param>
        public void Prepare(uint[] values)
        {
            Assert.Equal((int)InputField.Count, values.Length);
            Input = values;
            Events.Clear();
            Assert.Equal(Pool.IsServer, values[(int)InputField.Server] != 0);
            // 부모와 모든 종속 슬롯의 주변 바이트를 실행기와 같은 값으로 채운다
            for (int index = 0; index <= Children.Length; index++)
            {
                int sid = index == 0 ? Parent : Children[index - 1];
                Span<byte> raw = Slot(sid);
                // 표식 바이트는 whole-slot 보존 대조에 사용한다
                for (int i = 0; i < raw.Length; i++)
                {
                    raw[i] = (byte)(i * 29 + index * 17 + 7);
                }
                Put(raw, 0, Table);
                Put(raw, 4, 0, 2);
                Put(raw, 6, index == 0 && values[(int)InputField.Pattern] != 4 ? (uint)Children[0] : 0u, 2);
                raw[RawSidPool.TypeOffset] = (byte)(index == 0 ? 100 : 47);
                raw[RawSidPool.StateOffset] = (byte)(index == 0 ? 0 : 8);
                if (index == 0)
                {
                    continue;
                }
                Put(raw, 4, index < Children.Length ? (uint)Children[index] : 0u, 2);
                // 세 패턴의 타입·상태는 PE 실행에 공급한 입력과 동일하다
                byte[] types1 = [46, 47, 47, 46, 47, 47, 47];
                byte[] states2 = [2, 10, 8, 1, 4, 0, 8];
                byte[] types3 = [0, 255, 46, 47, 47, 255, 0];
                byte[] states3 = [0, 8, 2, 10, 1, 4, 0];
                if (values[(int)InputField.Pattern] == 1)
                {
                    raw[RawSidPool.TypeOffset] = types1[index - 1];
                }
                if (values[(int)InputField.Pattern] == 2)
                {
                    raw[RawSidPool.StateOffset] = states2[index - 1];
                }
                if (values[(int)InputField.Pattern] == 3)
                {
                    raw[RawSidPool.TypeOffset] = types3[index - 1];
                    raw[RawSidPool.StateOffset] = states3[index - 1];
                }
            }
        }

        /// <summary>명시 삭제 경계에서 호출만 기록하고 입력에 지정된 raw 변화를 공급한다.</summary>
        /// <param name="sid">삭제 대상 번호.</param>
        /// <param name="flags">삭제 플래그.</param>
        private void Delete(int sid, uint flags)
        {
            Events.Add($"D:{sid}:{flags}");
            Span<byte> raw = Slot(sid);
            if (Input[(int)InputField.Mutate] == 1)
            {
                Put(raw, 4, (uint)Children[3], 2);
            }
            if (Input[(int)InputField.Mutate] == 2)
            {
                Put(raw, 4, 0, 2);
            }
            if (Input[(int)InputField.Mutate] == 3)
            {
                Slot(Children[1])[RawSidPool.StateOffset] = 2;
            }
            if (Input[(int)InputField.Mutate] == 4)
            {
                byte type = raw[RawSidPool.TypeOffset];
                raw.Clear();
                raw[RawSidPool.TypeOffset] = type;
                raw[RawSidPool.StateOffset] = 3;
            }
            if (Input[(int)InputField.Mutate] == 5)
            {
                Put(Slot(Parent), 6, 0, 2);
            }
        }

        /// <summary>지정 raw 전체를 원본 실행기와 같은 순서로 합친다.</summary>
        /// <returns>슬롯 hex 문자열.</returns>
        public string Dump()
        {
            var text = new List<string> { Convert.ToHexStringLower(Pool.Slot(Parent).ToArray()) };
            foreach (int sid in Children)
            {
                text.Add(Convert.ToHexStringLower(Pool.Slot(sid).ToArray()));
            }
            return string.Join(':', text);
        }

        /// <summary>실제 가상 삭제의 사건 문자열이다.</summary>
        /// <returns>사건 문자열.</returns>
        public string EventText() => Events.Count == 0 ? "-" : string.Join(';', Events);
    }

    /// <summary>
    /// 종속 조회 fixture 의 10.78 몫을 재생한다.
    /// 서버 권한 입력에 따라 서버 풀·client 풀 두 장면을 나눠 쓴다.
    /// </summary>
    [Fact]
    public void ContainedLookupMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("contained");
        int count = 0;
        var client = new Scene(false);
        var server = new Scene(true);
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            uint[] values = row[1].Split(',').Select(X86Fixture.UInt).ToArray();
            Scene scene = values[(int)InputField.Server] != 0 ? server : client;
            scene.Prepare(values);
            int result = scene.Lookup.Find(Parent, values[(int)InputField.Type], values[(int)InputField.Ordinal],
                values[(int)InputField.Remove] != 0);
            // 원본이 저장한 기대 관찰과 비교한다
            Assert.Equal(X86Fixture.UInt(row[2]), (uint)result);
            Assert.Equal(row[3], scene.EventText());
            Assert.Equal(row[4], scene.Dump());
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }
}
