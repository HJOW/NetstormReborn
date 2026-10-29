using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 프레임의 현재 등록/idle/링크와 실제 수명 통합을 검증하는 영역이다.

/// <summary>GumpFrame 전체 상태/콜백 순서를 실제 세 PE 기록과 대조한다.
/// 근거: cpppj/tests/GumpFrameTests.cpp.</summary>
public sealed class GumpFrameTests
{
    /// <summary>현재 표의 비활성 칸까지 길이:DWORD 연속 구간으로 기록한다.</summary>
    private static string Table(ReadOnlySpan<uint> values)
    {
        var runs = new List<string>();
        // 현재 값의 연속 구간만 압축하며 예상 호출 대상은 만들지 않는다.
        for (int i = 0; i < values.Length;)
        {
            int first = i; uint value = values[i];
            // 같은 현재 값의 끝 위치만 찾는다.
            while (i < values.Length && values[i] == value) i++;
            runs.Add($"{i - first}:{value}");
        }
        return string.Join(';', runs);
    }

    /// <summary>정확한 크기의 표를 완성 입력 RLE로만 채운다.</summary>
    private static void ReadTable(string text, Span<uint> output)
    {
        int cursor = 0;
        // 입력 구간을 그대로 펼치며 프레임 처리 결과는 계산하지 않는다.
        foreach (string item in text.Split(';'))
        {
            string[] p = item.Split(':'); int size = X86Fixture.Int(p[0]); Assert.InRange(size, 0, output.Length - cursor);
            output.Slice(cursor, size).Fill(X86Fixture.UInt(p[1])); cursor += size;
        }
        Assert.Equal(output.Length, cursor);
    }

    /// <summary>판본마다270행의 연속 공개 함수와 콜백 당시 전체 표/노드 쓰기를 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeFrame(string edition)
    {
        string[][] rows = X86Fixture.Read("gumpframe", edition); Assert.Equal(270, rows.Length);
        // 한 행의 연속 연산만 같은 현재 노드/표를 공유한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(11, row.Length); var create = new GumpCreateState(); var lifetime = new GumpLifetimeState();
            create.Collection.Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; int idleSize = edition == "originals" ? 500 : 80;
            SortedDictionary<uint, GumpCreateNode> nodes = GumpRawSupport.Nodes(row[3]); string[] parts = row[4].Split('#'); create.Count = X86Fixture.UInt(parts[0]);
            ReadTable(parts[1], create.Entries); ReadTable(parts[2], lifetime.Idle.AsSpan(0, idleSize)); create.Collection.Root = create.Entries[0];
            // 현재 개수/표 전체만 원본 형식으로 직렬화한다.
            string State() => create.Count + "#" + Table(create.Entries) + "#" + Table(lifetime.Idle.AsSpan(0, idleSize));
            Assert.Equal(row[3], GumpRawSupport.EncodeNodes(nodes)); Assert.Equal(row[4], State());
            var responses = new Dictionary<string, uint[][]>(); var calls = new List<string>();
            if (row[5] != "-")
            {
                // 중복 응답은 마지막 완성 입력으로 덮어쓴다.
                foreach (string item in row[5].Split(';'))
                {
                    string[] p = item.Split(':'); responses[p[0]] = p[1].Length == 0 ? [] : p[1].Split('/').Select(s => s.Split(',').Select(X86Fixture.UInt).ToArray()).ToArray();
                }
            }
            // 콜백 직전의 현재 상태를 기록한 뒤 명시 raw/표/개수 쓰기만 적용한다.
            void Respond(string kind, uint token, params uint[] args)
            {
                string key = $"{kind},{token}";
                calls.Add(key + (args.Length == 0 ? "" : "," + string.Join(',', args)) + "@" + State() + "@" + GumpRawSupport.EncodeNodes(nodes));
                if (!responses.TryGetValue(key, out uint[][]? patches)) return;
                // 기대 통지/재그리기 대상은 계산하지 않고 지정 DWORD만 바꾼다.
                foreach (uint[] p in patches)
                {
                    if (p[0] == 0) { Assert.Equal(0U, p[1]); create.Count = p[2]; }
                    else if (p[0] == 1) { create.Entries[p[1]] = p[2]; create.Collection.Root = create.Entries[0]; }
                    else if (p[0] == 2) lifetime.Idle[p[1]] = p[2];
                    else GumpRawSupport.ApplyDword(nodes[p[0]], (int)p[1], p[2]);
                }
            }
            var frame = new GumpFrame(create, lifetime, new()
            {
                Node = token => nodes.GetValueOrDefault(token), Idle = token => Respond("Idle", token), Redraw = (token, value) => Respond("Redraw", token, value ? 1U : 0),
                Broadcast = (token, e, x, y) => Respond("Broadcast", token, e, x, y),
            });
            // 실제 공개 API와 입력 인자만 호출한다. void의 불확정 반환은 비교하지 않는다.
            foreach (string item in row[2].Split(';'))
            {
                string[] p = item.Split(','); uint token = X86Fixture.UInt(p[1]);
                switch (p[0])
                {
                    case "Idle": frame.Idle(); break; case "RedrawNode": frame.RedrawNode(token); break; case "RedrawRoots": frame.RedrawRoots(); break;
                    case "Broadcast": frame.Broadcast(X86Fixture.UInt(p[2]), X86Fixture.UInt(p[3]), X86Fixture.UInt(p[4])); break;
                    default: throw new InvalidDataException("Gump 프레임 연산 오류");
                }
            }
            Assert.Equal(row[6], GumpRawSupport.EncodeNodes(nodes)); Assert.Equal(row[7], State()); Assert.Equal(row[8], calls.Count == 0 ? "-" : string.Join('|', calls));
        }
    }

    /// <summary>실제 생성/수명에 프레임 통지/재그리기 검사 장면을 부착한다.</summary>
    private static GumpLifetimeRig Scene()
    {
        var rig = new GumpLifetimeRig(); rig.Nodes[2].Flags |= 0x100; rig.Nodes[3].Flags |= 0x100; rig.Nodes[4].Flags |= 0x100;
        rig.Tree.Attach(3, 0, 0, 1, 1); rig.Tree.Attach(2, 0, 0, 1, 1); rig.Tree.Attach(4, 0, 0, 2, 1); rig.Nodes[2].Flags |= 0x20;
        return rig;
    }

    /// <summary>현재 idle/통지 표와 실제 지연 삭제/소멸자/해제가 같은 프레임에 반영된다.</summary>
    [Fact]
    public void CreateLifetimeIntegration()
    {
        GumpLifetimeRig rig = Scene(); var visited = new List<uint>(); rig.Lifetime.SetIdle(2, true); rig.Lifetime.SetIdle(3, true);
        var frame = new GumpFrame(rig.State, rig.LifeState, new()
        {
            Node = token => rig.Nodes.GetValueOrDefault(token), Idle = token => { visited.Add(token); if (token == 2 && rig.Nodes.ContainsKey(3)) rig.Lifetime.Kill(3); },
        });
        frame.Idle(); Assert.Equal(new uint[] { 2 }, visited); Assert.Equal(1U, rig.LifeState.KillCount); Assert.NotEqual(0U, rig.Nodes[3].Flags & 0x10000);
        visited.Clear(); new GumpFrame(rig.State, rig.LifeState, new() { Node = token => rig.Nodes.GetValueOrDefault(token), Broadcast = (token, _, _, _) => visited.Add(token) }).Broadcast(7, 11, 13);
        Assert.Equal(new uint[] { 2, 4 }, visited); rig.Lifetime.Cleanup(); Assert.False(rig.Nodes.ContainsKey(3)); Assert.Equal(3U, rig.State.Count); Assert.Equal(0U, rig.LifeState.KillCount); Assert.Equal(0U, rig.Nodes[2].Next);
        visited.Clear(); frame.Idle(); Assert.Equal(new uint[] { 2 }, visited);
    }

    /// <summary>CD는 비활성420칸을 건드리지 않고 패치판은 마지막500번째 칸까지 플래그 검사 없이 호출한다.</summary>
    [Fact]
    public void IdleEditionLimitAndUnfilteredNodes()
    {
        GumpLifetimeRig rig = Scene(); rig.Nodes[2].Flags = 0x10008; rig.LifeState.Idle[79] = 2; rig.LifeState.Idle[80] = 3; rig.LifeState.Idle[499] = 4;
        var visited = new List<uint>(); var frame = new GumpFrame(rig.State, rig.LifeState, new() { Node = token => rig.Nodes.GetValueOrDefault(token), Idle = visited.Add });
        rig.State.Collection.Edition = OriginalEdition.Cd1072; frame.Idle(); Assert.Equal(new uint[] { 2 }, visited); Assert.Equal(3U, rig.LifeState.Idle[80]); Assert.Equal(4U, rig.LifeState.Idle[499]);
        visited.Clear(); rig.State.Collection.Edition = OriginalEdition.Patch1078; frame.Idle(); Assert.Equal(new uint[] { 2, 3, 4 }, visited);
    }

    /// <summary>루트는 제외하고 비영 칸을 세며 콜백 뒤 현재 signed count에 따라 종료한다.</summary>
    [Fact]
    public void BroadcastCurrentCountAndRootExclusion()
    {
        GumpLifetimeRig rig = Scene(); rig.Nodes[1].Flags = 0x101; rig.Nodes[2].Flags = 1; rig.State.Count = 1; var visited = new List<uint>();
        var frame = new GumpFrame(rig.State, rig.LifeState, new()
        {
            Node = token => rig.Nodes.GetValueOrDefault(token), Broadcast = (token, e, x, y) => { Assert.Equal(0x80000001U, e); Assert.Equal(11U, x); Assert.Equal(13U, y); visited.Add(token); rig.State.Count = 1; },
        });
        frame.Broadcast(0x80000001, 11, 13); Assert.Empty(visited); rig.State.Count = 4; frame.Broadcast(0x80000001, 11, 13); Assert.Equal(new uint[] { 3 }, visited);
        rig.State.Count = uint.MaxValue; visited.Clear(); frame.Broadcast(0x80000001, 11, 13); Assert.Empty(visited);
    }

    /// <summary>최상위 next는 저장하고 하위 next는 재귀 뒤 재조회하며 하위 dead/void는 visible이면 통과한다.</summary>
    [Fact]
    public void RedrawCurrentChildAndSavedRootLinks()
    {
        GumpLifetimeRig rig = Scene(); rig.Nodes[4].Flags = 0x10009; var visited = new List<uint>();
        var frame = new GumpFrame(rig.State, rig.LifeState, new() { Node = token => rig.Nodes.GetValueOrDefault(token), Redraw = (token, value) => { Assert.False(value); visited.Add(token); if (token == 2) rig.Nodes[2].Next = 0; } });
        rig.State.Collection.OverrideHead = 4; rig.Nodes[1].Flags = 0; frame.RedrawRoots(); Assert.Equal(new uint[] { 2, 4, 3 }, visited);
        visited.Clear(); rig.Nodes[2].Child = 3; rig.Nodes[3].Next = 4;
        var current = new GumpFrame(rig.State, rig.LifeState, new() { Node = token => rig.Nodes.GetValueOrDefault(token), Redraw = (token, _) => { visited.Add(token); if (token == 3) rig.Nodes[3].Next = 0; } });
        current.RedrawNode(2); Assert.Equal(new uint[] { 2, 3 }, visited);
    }

    /// <summary>없는 토큰/가상 경계/순환은 거부하고 실제 도달하지 않는 조기 종료에는 조회를 요구하지 않는다.</summary>
    [Fact]
    public void ContractsAndCycles()
    {
        GumpLifetimeRig rig = Scene(); Assert.Throws<ArgumentException>(() => new GumpFrame(rig.State, rig.LifeState, new()));
        var missing = new GumpFrame(rig.State, rig.LifeState, new() { Node = token => rig.Nodes.GetValueOrDefault(token) });
        missing.Idle(); rig.State.Count = 0; missing.Broadcast(1, 2, 3); rig.Nodes[2].Flags = rig.Nodes[3].Flags = 0x42001; missing.RedrawRoots();
        rig.LifeState.Idle[0] = 999; Assert.Throws<InvalidOperationException>(() => missing.Idle()); rig.LifeState.Idle[0] = 2; Assert.Throws<ArgumentException>(() => missing.Idle());
        rig.Nodes[2].Flags = 1; Assert.Throws<ArgumentException>(() => missing.RedrawNode(2)); rig.Nodes[2].Flags = 0x101; rig.State.Count = 4; Assert.Throws<ArgumentException>(() => missing.Broadcast(1, 2, 3));
        var frame = new GumpFrame(rig.State, rig.LifeState, new() { Node = token => rig.Nodes.GetValueOrDefault(token), Redraw = (_, _) => { } });
        rig.Nodes[2].Flags = 0x21; rig.Nodes[2].Child = 2; Assert.Throws<InvalidOperationException>(() => frame.RedrawNode(2)); rig.Nodes[2].Child = 0; rig.Nodes[2].Next = 2;
        Assert.Throws<InvalidOperationException>(() => frame.RedrawRoots());
    }

    /// <summary>idle 콜백이 기록한 노드/표 쓰기는 예외 뒤에도 유지한다.</summary>
    [Fact]
    public void CallbackExceptionKeepsState()
    {
        GumpLifetimeRig rig = Scene(); rig.LifeState.Idle[0] = 2;
        var frame = new GumpFrame(rig.State, rig.LifeState, new() { Node = token => rig.Nodes.GetValueOrDefault(token), Idle = token => { rig.Nodes[token].Command = 999; rig.LifeState.Idle[0] = 3; throw new InvalidOperationException("idle 경계 실패"); } });
        Assert.Throws<InvalidOperationException>(() => frame.Idle()); Assert.Equal(999U, rig.Nodes[2].Command); Assert.Equal(3U, rig.LifeState.Idle[0]);
    }
}
