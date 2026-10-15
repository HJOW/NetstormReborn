using System.Globalization;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 10.78 Early Mission 1 단계 판정(004c3a20)의 실제 기계어 관찰 266개를 <see cref="TutorialOne"/>의
/// 반환값과 외부 경계 호출 순서로 대조한다(단락 평가·중복 Advance·다중 Graph·float32 SP 경계·NaN 포함).
/// </summary>
/// <remarks>
/// 기대값은 cpppj/tests/fixtures/tutorial-one-x86.tsv 의 <c>Stage</c> 행(원본 PE 독립 실행, 링크 공유)이며 C# 에서 계산하지 않는다.
/// 입력 구성은 cpppj/tests/TutorialOneTests.cpp 의 장면을 따른다. 2026-10-15 감사 D05 에서 추가.
/// </remarks>
public sealed class X86TutorialOneTests
{
    /// <summary>fixture 의 고유 단계 입력 개수.</summary>
    private const int ExpectedRows = 266;

    /// <summary>fixture 열 수: 종류·단계·사건·누적 제작 수·SP 조회 허용·SP 바이트·Graph 표·반환·호출 순서.</summary>
    private const int ColumnCount = 9;

    /// <summary>입력의 4바이트 little endian float 를 반올림하지 않고 읽는다.</summary>
    private static float FloatBytes(string text)
    {
        uint bits = 0;
        // 네 바이트를 원래 순서로 조합한다.
        for (int i = 0; i < 4; i++) bits |= (uint)Convert.ToByte(text.Substring(i * 2, 2), 16) << (8 * i);
        return BitConverter.UInt32BitsToSingle(bits);
    }

    /// <summary>부호 없는 십진수 칸을 읽는다.</summary>
    private static uint Number(string text) => uint.Parse(text, CultureInfo.InvariantCulture);

    /// <summary>원본 266개 입력의 반환값과 호출 순서(집계·Graph·SP·Advance·생성)를 모두 대조한다.</summary>
    [Fact]
    public void NativeStageJudge_MatchesReturnsAndCallOrder()
    {
        string[][] rows = X86Fixture.Read("tutorial-one").Where(row => row[0] == "Stage").ToArray();
        Assert.Equal(ExpectedRows, rows.Length);
        var stages = new HashSet<int>();
        // 각 행은 독립 native 와 같은 새 상태에서 시작한다.
        bool Matches(string[] row)
        {
            Assert.Equal(ColumnCount, row.Length);
            int stage = unchecked((int)Number(row[1]));
            stages.Add(stage);
            var graphs = new Dictionary<uint, uint>();
            var geysers = new RecentGeyserList();
            // 같은 현재 Graph 표와 실제 목록 순서를 입력한다. 신전(55)은 최근 간헐천 목록에 넣지 않는다.
            foreach (string item in row[6].Split(','))
            {
                string[] pair = item.Split(':');
                uint sid = Number(pair[0]);
                graphs[sid] = Number(pair[1]);
                if (sid != 55) geysers.Entries.Add(sid);
            }
            geysers.Count = (uint)geysers.Entries.Count;
            var trace = new List<string>();
            var state = new TutorialOneState { Stage = () => stage, LocalPlayer = 1, BattleShown = Number(row[4]), HomeCommand = 0x730000, ReclaimCommand = 0x123456 };
            var hooks = new TutorialOneHooks
            {
                BuiltForGenus = genus => { Assert.Equal(4u, genus); trace.Add("B"); return int.Parse(row[3], CultureInfo.InvariantCulture); },
                GraphFor = sid => { trace.Add("G" + sid.ToString(CultureInfo.InvariantCulture)); return graphs[sid]; },
                Money = owner => { trace.Add("M" + owner.ToString(CultureInfo.InvariantCulture)); return FloatBytes(row[5]); },
                Advance = () => trace.Add("A"),
                SpawnGeysers = () => trace.Add("S"),
                TempleOf = owner => owner == 1 ? 55u : 0u,
            };
            var judge = new TutorialOne(state, geysers, hooks);
            uint result = judge.Handle(Number(row[2]));
            return result == Number(row[7]) && (trace.Count == 0 ? "-" : string.Join(',', trace)) == row[8];
        }
        X86Fixture.CheckRows(rows, Matches);
        // 단계 A~F 의 분기가 모두 재생됐는지 확인한다.
        Assert.True(stages.IsSupersetOf(['A', 'B', 'C', 'D', 'E', 'F']));
    }

    /// <summary>판정은 상태·목록을 복사하지 않고 훅 뒤의 현재 값을 읽는다: 목록이 줄거나 단계가 바뀌어도 같은 호출에서 반영된다.</summary>
    [Fact]
    public void Handle_RereadsSharedStateAndShrinkingListAfterHooks()
    {
        int stage = 'D';
        var geysers = new RecentGeyserList { Count = 3 };
        geysers.Entries.AddRange([80u, 81u, 82u]);
        int advances = 0, lookups = 0;
        var state = new TutorialOneState { Stage = () => stage, LocalPlayer = 1, BattleShown = 1 };
        var hooks = new TutorialOneHooks
        {
            BuiltForGenus = _ => 0,
            GraphFor = _ => { lookups++; return 2; },
            Money = _ => 600.0F,
            Advance = () => { advances++; stage = 'E'; geysers.Count = 1; },
            SpawnGeysers = () => Assert.Fail("단계 D 에서 간헐천을 생성하면 안 됩니다."),
            TempleOf = _ => 55,
        };
        var judge = new TutorialOne(state, geysers, hooks);
        Assert.Equal(0u, judge.Handle(0));
        // 신전 Graph 1회 + 첫 항목 1회 비교 뒤 Advance 가 목록을 1개로 줄이므로 반복은 거기서 끝난다.
        Assert.Equal('E', stage);
        Assert.Equal(1, advances);
        Assert.Equal(2, lookups);
        // 단계 E 로 바뀐 뒤의 다음 호출은 SP 조건으로 판정한다.
        Assert.Equal(0u, judge.Handle(0));
        Assert.Equal(2, advances);
    }

    /// <summary>경계가 하나라도 비어 있으면 생성하지 못하고, 간헐천 목록이 물리 범위를 넘거나 로컬 번호가 범위 밖이면 거부한다.</summary>
    [Fact]
    public void Construction_RejectsMissingBoundaries_AndHandleRejectsBadRanges()
    {
        var geysers = new RecentGeyserList();
        var state = new TutorialOneState { Stage = () => 'D', LocalPlayer = 1 };
        var complete = new TutorialOneHooks
        {
            BuiltForGenus = _ => 0, GraphFor = _ => 0, Money = _ => 0, Advance = () => { }, SpawnGeysers = () => { }, TempleOf = _ => 0,
        };
        Assert.Throws<ArgumentException>(() => new TutorialOne(state, geysers, new TutorialOneHooks()));
        Assert.Throws<ArgumentException>(() => new TutorialOne(new TutorialOneState(), geysers, complete));
        var judge = new TutorialOne(state, geysers, complete);
        geysers.Count = 2;
        geysers.Entries.Add(7);
        Assert.Throws<InvalidDataException>(() => judge.Handle(0));
        state.LocalPlayer = 9;
        Assert.Throws<InvalidDataException>(() => judge.Handle(0));
    }
}
