using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 사제 가상 사전 파괴(RawPriestPreDestroy)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/PriestPreDestroyTests.cpp 의 PriestScene 과 같다: 실제 풀의 사제 50 하나에
/// 가상 표·타입·상태·추가 비트를 넣고, 사제 목록을 입력으로 두고, 보호막 조회 이후의 효과를 순서대로 기록한다.
/// 호출마다 목록 count·목록 저장소 전체·효과 문자열·슬롯 전체 50바이트·Carrier 사전 검사 횟수를 비교한다.
/// 기준 판본은 10.78(Patch) 이며, CD 10.72·10.37 몫은 옮기지 않았다 (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class X86RawPriestPreDestroyTests
{
    /// <summary>원본 관찰이 쓰는 사제 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>10.78 실제 사제 클래스의 가상 표 값.</summary>
    private const uint PatchPriestVtable = 0x0050F210;

    /// <summary>실제 사제 타입 번호.</summary>
    private const byte PriestType = 158;

    /// <summary>원본 관찰의 풀 크기(클라이언트 풀).</summary>
    private const int Capacity = 24000;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>사제 fixture 에서 10.78 몫으로 기록된 행 수.</summary>
    private const int PatchRows = 2184;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>슬롯 전체를 소문자 hex 로 만든다.</summary>
    /// <param name="span">슬롯 바이트.</param>
    private static string Hex(ReadOnlySpan<byte> span) => Convert.ToHexStringLower(span.ToArray());

    /// <summary>한 행의 풀·상태·기록을 소유하는 장면. cpppj 가 장면 하나를 행마다 재사용한다.</summary>
    private sealed class Scene
    {
        /// <summary>사제 50 이 실제 할당된 클라이언트 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>사제 목록 상태. 행마다 입력으로 다시 채운다.</summary>
        public readonly PriestPostPopState State = new();

        /// <summary>효과 문자열 조각. 세미콜론으로 잇는다.</summary>
        public readonly List<string> Parts = [];

        /// <summary>Carrier 사전 검사 횟수.</summary>
        public int Validations;

        /// <summary>보호막 조회가 돌려줄 번호.</summary>
        public uint Found;

        /// <summary>보호막 조회 뒤 추가 비트를 바꾸는 값. 빈 문자열이면 바꾸지 않는다.</summary>
        public string After = "-";

        /// <summary>검사 대상 사제 사전 파괴 몸체.</summary>
        public readonly RawPriestPreDestroy Body;

        /// <summary>클라이언트 풀을 만들고 사제 50 까지 실제 할당한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(Capacity, false);
            // 각 행에서 같은 번호를 다시 쓰도록 50 까지만 확보한다
            while (Pool.Allocate(SidAllocation.Client) != Source)
            {
            }
            Body = new RawPriestPreDestroy(Pool, State, Hooks());
        }

        /// <summary>호출 시점의 인자·목록 개수·현재 추가 비트를 기록한다.</summary>
        /// <param name="marker">효과 종류 문자.</param>
        /// <param name="sid">호출 대상 번호.</param>
        /// <param name="flags">플래그 접미사. 보호막 조회는 빈 문자열이다.</param>
        private void Record(char marker, int sid, string flags)
        {
            // 사건 사이에 세미콜론을 둔다
            if (Parts.Count != 0)
            {
                Parts.Add(";");
            }
            Parts.Add($"{marker}:{sid}{flags}:{State.Priests.Count}:{Pool.Slot(Source)[ExtraOffset]}");
        }

        /// <summary>보호막·Carrier 몸체만 명시적 대체로 공급한다.</summary>
        private PriestPreDestroyHooks Hooks() => new()
        {
            FindForcefield = sid =>
            {
                Record('F', sid, string.Empty);
                // 조회가 추가 비트를 바꾸면 다음 분기가 그 값을 읽는다
                if (After != "-")
                {
                    Pool.RawSlot(Source)[ExtraOffset] = (byte)uint.Parse(After, System.Globalization.CultureInfo.InvariantCulture);
                }
                return Found;
            },
            DestroyForcefield = (sid, flags) => Record('D', sid, ":" + flags),
            ValidateCarrier = (_, _) => Validations++,
            CarrierPre = (sid, flags) => Record('C', sid, ":" + flags),
        };

        /// <summary>한 행의 입력만 raw·목록에 넣는다. 기대 압축·분기는 계산하지 않는다.</summary>
        /// <param name="row">fixture 한 행(12칸).</param>
        public void Prepare(string[] row)
        {
            Parts.Clear();
            Validations = 0;
            Span<byte> raw = Pool.RawSlot(Source);
            raw.Fill(0xab);
            Put(raw, 0, PatchPriestVtable);
            raw[10] = PriestType;
            raw[11] = (byte)X86Fixture.UInt(row[2]);
            raw[ExtraOffset] = (byte)X86Fixture.UInt(row[1]);
            State.Priests.Entries = row[5].Split(',').Select(X86Fixture.UInt).ToArray();
            State.Priests.Count = X86Fixture.UInt(row[4]);
            Found = X86Fixture.UInt(row[6]);
            After = row[7];
        }

        /// <summary>목록 저장소 전체를 개수 밖 stale 값까지 원문 순서로 직렬화한다.</summary>
        public string Entries() => string.Join(',', State.Priests.Entries);

        /// <summary>효과 문자열.</summary>
        public string Trace() => string.Concat(Parts);
    }

    /// <summary>사제 사전 파괴 fixture 의 10.78 몫 2,184회를 재생한다.</summary>
    [Fact]
    public void PriestPreDestroyMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("priestdestroy");
        Assert.Equal(6552, rows.Length);
        string[][] patch = rows.Where(row => row[0] == "originals").ToArray();
        Assert.Equal(PatchRows, patch.Length);
        var scene = new Scene();
        int count = 0;
        X86Fixture.CheckRows(patch, row =>
        {
            Assert.Equal(12, row.Length);
            scene.Prepare(row);
            scene.Body.PreDestroy(Source, X86Fixture.UInt(row[3]));
            string entries = scene.Entries();
            bool same = scene.State.Priests.Count == X86Fixture.UInt(row[8]) && entries == row[9]
                && scene.Trace() == row[10] && Hex(scene.Pool.Slot(Source)) == row[11];
            count++;
            return same && scene.Validations == 1;
        });
        Assert.Equal(PatchRows, count);
    }
}
