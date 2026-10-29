using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 기본/루트 생성의 전체 바이트를 검증하는 영역이다.

/// <summary>원본 PE 관측으로 생성160바이트·등록 표·일련번호·할당 순서를 대조한다.
/// 근거: cpppj/tests/GumpCreateTests.cpp.</summary>
public sealed class GumpCreateTests
{
    /// <summary>입력에 적힌 길이:값 run만 풀어1000칸 표를 만든다.</summary>
    private static uint[] Table(string text)
    {
        var result = new uint[GumpCreateState.RegistryCapacity]; int position = 0;
        // 원본 완성 입력 run을 그대로 복사한다.
        foreach (string part in text.Split(';'))
        {
            string[] values = part.Split(':'); int count = X86Fixture.Int(values[0]);
            Assert.InRange(count, 0, result.Length - position); Array.Fill(result, X86Fixture.UInt(values[1]), position, count); position += count;
        }
        Assert.Equal(result.Length, position); return result;
    }

    /// <summary>실제 현재 등록 표의 연속 같은 DWORD만 길이:값으로 직렬화한다.</summary>
    private static string EncodeTable(uint[] entries)
    {
        var runs = new List<string>();
        // 현재 run 끝을 찾으며 등록 위치/조건은 계산하지 않는다.
        for (int first = 0; first < entries.Length;)
        {
            int end = first + 1;
            // 같은 토큰이 이어진 길이만 센다.
            while (end < entries.Length && entries[end] == entries[first]) end++;
            runs.Add($"{end - first}:{entries[first]}"); first = end;
        }
        return string.Join(';', runs);
    }

    /// <summary>현재 카운터와 화면 크기의 원본 관측 순서.</summary>
    private static string Globals(GumpCreateState s) => $"{s.Count},{s.Serial},{s.ScreenWidth},{s.ScreenHeight}";

    /// <summary>각 판본315행의 연속 생성/루트/할당 응답을 전체 바이트로 검증한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeCreation(string edition)
    {
        string[][] rows = X86Fixture.Read("gumpcreate", edition); Assert.Equal(315, rows.Length);
        // 한 행의 모든 연산이 같은 현재 상태를 공유하며 행 사이에는 완전히 분리한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(13, row.Length); string[] globals = row[2].Split(',');
            var state = new GumpCreateState { Count = X86Fixture.UInt(globals[0]), Serial = X86Fixture.UInt(globals[1]), ScreenWidth = X86Fixture.Int(globals[2]), ScreenHeight = X86Fixture.Int(globals[3]) };
            Table(row[3]).CopyTo(state.Entries, 0); state.Collection.Root = state.Entries[0];
            state.Collection.Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            SortedDictionary<uint, GumpCreateNode> nodes = GumpRawSupport.Nodes(row[4]); Assert.Equal(row[4], GumpRawSupport.EncodeNodes(nodes));
            string[] action = []; var allocations = new List<string>(); var results = new List<uint>();
            var hooks = new GumpCreateHooks
            {
                Node = token => nodes.GetValueOrDefault(token),
                Allocate = size =>
                {
                    bool cleared = state.Entries.All(token => token == 0);
                    allocations.Add($"{size},{Globals(state)},{(cleared ? 1 : 0)}");
                    if (action.Length == 6)
                    {
                        state.Count = X86Fixture.UInt(action[2]); state.Serial = X86Fixture.UInt(action[3]);
                        state.ScreenWidth = X86Fixture.Int(action[4]); state.ScreenHeight = X86Fixture.Int(action[5]);
                    }
                    return X86Fixture.UInt(action[1]);
                },
            };
            var create = new GumpCreate(state, hooks);
            // 기대 결과를 재계산하지 않고 완성 입력에 적힌 연산만 호출한다.
            foreach (string item in row[5].Split(';'))
            {
                action = item.Split(',');
                results.Add(action[0] == "Root" ? create.InitializeRoot() : create.Construct(X86Fixture.UInt(action[1]), X86Fixture.UInt(action[2]),
                    X86Fixture.FloatBits(action[3]), X86Fixture.FloatBits(action[4]), X86Fixture.UInt(action[5])));
            }
            Assert.Equal(row[6], GumpRawSupport.EncodeNodes(nodes)); Assert.Equal(row[7], EncodeTable(state.Entries));
            Assert.Equal(row[8], Globals(state)); Assert.Equal(row[9], string.Join(',', results));
            Assert.Equal(row[10], allocations.Count == 0 ? "-" : string.Join(';', allocations)); Assert.Equal(state.Entries[0], state.Collection.Root);
        }
    }

    /// <summary>할당 실패는 비운 표를 유지하고 카운터/override/이전 객체를 보존한다.</summary>
    [Fact]
    public void RootFailurePreservesOtherState()
    {
        var node = new GumpCreateNode { Untouched = 0x81234567 };
        var state = new GumpCreateState { Count = 12, Serial = 23 }; Array.Fill(state.Entries, 1U); state.Collection.Root = 1; state.Collection.OverrideHead = 9;
        string before = GumpRawSupport.Encode(node);
        var create = new GumpCreate(state, new()
        {
            Node = token => token == 1 ? node : null,
            Allocate = size => { Assert.Equal(160, size); Assert.Equal(0U, state.Collection.Root); return 0; },
        });
        Assert.Equal(0U, create.InitializeRoot()); Assert.Equal(12U, state.Count); Assert.Equal(23U, state.Serial); Assert.Equal(9U, state.Collection.OverrideHead);
        Assert.Equal(before, GumpRawSupport.Encode(node)); Assert.All(state.Entries, token => Assert.Equal(0U, token));
    }

    /// <summary>할당 뒤 현재 화면 크기/카운터와 serial DWORD 감김을 읽는다.</summary>
    [Fact]
    public void CurrentGlobalsAfterAllocation()
    {
        var node = new GumpCreateNode { Untouched = 0x81234567 }; var state = new GumpCreateState { ScreenWidth = 640, ScreenHeight = 480 }; state.Collection.OverrideHead = 9;
        var create = new GumpCreate(state, new()
        {
            Node = token => token == 1 ? node : null,
            Allocate = size => { Assert.Equal(160, size); state.ScreenWidth = 16777217; state.ScreenHeight = -16777217; state.Count = 997; state.Serial = uint.MaxValue; return 1; },
        });
        Assert.Equal(1U, create.InitializeRoot()); Assert.Equal(16777216F, node.Width); Assert.Equal(-16777216F, node.Height);
        Assert.Equal(0U, node.Id); Assert.Equal(9U, node.Flags); Assert.Equal(0U, node.Serial); Assert.Equal(0U, node.Command);
        Assert.Equal(0x81234567U, node.Untouched); Assert.Equal(998U, state.Count); Assert.Equal(0U, state.Serial); Assert.Equal(9U, state.Collection.OverrideHead);
    }

    /// <summary>첫 빈 칸/미등록 생성과 꽉 찬 표의 원본 정상 반환을 구별한다.</summary>
    [Fact]
    public void FirstHoleUnregisteredAndFullTable()
    {
        var nodes = new[] { new GumpCreateNode(), new GumpCreateNode() }; var state = new GumpCreateState { Count = 12, Serial = 23 };
        Array.Fill(state.Entries, 17U); state.Entries[7] = state.Entries[999] = 0;
        var create = new GumpCreate(state, new() { Node = token => token >= 1 && token <= 2 ? nodes[token - 1] : null });
        Assert.Equal(1U, create.Construct(1, 27, 4, 5, 1)); Assert.Equal(7U, nodes[0].Id); Assert.Equal(1U, state.Entries[7]); Assert.Equal(0U, state.Entries[999]);
        Assert.Equal(13U, state.Count); Assert.Equal(24U, state.Serial);
        create.Construct(2, 28, 6, 7, 0x100001); Assert.Equal(0U, nodes[1].Id); Assert.Equal(25U, nodes[1].Serial); Assert.Equal(13U, state.Count);
        Array.Fill(state.Entries, 17U); create.Construct(2, 29, 8, 9, 1); Assert.Equal(0U, nodes[1].Id); Assert.Equal(14U, state.Count);
        Assert.Equal(17U, state.Entries[0]); Assert.Equal(17U, state.Entries[999]);
    }

    /// <summary>개수 assert/없는 저장소/할당 경계를 변경 전에 거부하고 미등록 생성은 한도와 분리한다.</summary>
    [Fact]
    public void ContractsAndCountLimit()
    {
        var node = new GumpCreateNode { Untouched = 17 }; var state = new GumpCreateState { Count = 999 };
        var hooks = new GumpCreateHooks { Node = token => token == 1 ? node : null }; var create = new GumpCreate(state, hooks);
        string before = GumpRawSupport.Encode(node);
        Assert.Throws<ArgumentException>(() => new GumpCreate(state, new()));
        Assert.Throws<InvalidOperationException>(() => create.Construct(0, 0, 1, 1, 1)); Assert.Throws<InvalidOperationException>(() => create.Construct(99, 0, 1, 1, 1));
        Assert.Throws<ArgumentException>(() => create.Construct(1, 0, 1, 1, 1)); Assert.Equal(before, GumpRawSupport.Encode(node));
        Assert.Equal(0U, state.Serial); Assert.Equal(999U, state.Count); Assert.Throws<ArgumentException>(() => create.InitializeRoot());
        create.Construct(1, 0, 1, 1, 0x100001); Assert.Equal(999U, state.Count); Assert.Equal(1U, state.Serial);
        hooks.Allocate = _ => 99; Assert.Throws<InvalidOperationException>(() => create.InitializeRoot()); Assert.Equal(0U, state.Collection.Root);
    }
}
