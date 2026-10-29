using System.Globalization;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 수명/루트 교체의 원본 표·전역·콜백 관측을 공유하는 검사 영역이다.

/// <summary>기존 PE fixture의 완성 입력과 현재 전체 상태만 직렬화한다. 기대 삭제/필터/정리 결과는 계산하지 않는다.</summary>
internal sealed class GumpLifetimeFixture
{
    /// <summary>현재 생성 전역과 실제1000칸 등록 표.</summary>
    public GumpCreateState Create { get; } = new();
    /// <summary>현재 수명 표와 스칼라.</summary>
    public GumpLifetimeState Life { get; } = new();
    /// <summary>같은 객체 참조를 유지하는 원본 주소 순서 저장소.</summary>
    public SortedDictionary<uint, GumpCreateNode> Nodes { get; }
    /// <summary>루트 교체 관측에 포함되는 공유 제외 signed BYTE. ref로 실제 함수에 전달한다.</summary>
    public sbyte Excluded = 11;
    /// <summary>호출 전 전체 상태/노드 관측의 순서 목록.</summary>
    public List<string> Calls { get; } = [];
    /// <summary>가상 종류/토큰별 명시 DWORD 응답.</summary>
    private readonly Dictionary<string, List<uint[]>> _responses = [];
    /// <summary>기본 수명은7개 스칼라, reset은 serial/제외 BYTE까지9개를 쓴다.</summary>
    private readonly bool _reset;

    /// <summary>현재 판본과 한 행의 전체 입력을 적재하며 초기 직렬화 자체도 원본과 대조한다.</summary>
    public GumpLifetimeFixture(string edition, string[] row, bool reset)
    {
        _reset = reset; Create.Collection.Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string[] parts = row[2].Split('#'); string[] globals = parts[0].Split(','); Assert.Equal(reset ? 9 : 7, globals.Length);
        // 명시된 전역 값을 각 실제 공유 필드에 적재한다.
        for (int i = 0; i < globals.Length; i++) SetGlobal(i, X86Fixture.UInt(globals[i]));
        // 다섯 고정 표의 활성 구간 전체를 입력 RLE 그대로 푼다.
        for (int i = 1; i <= 5; i++) ReadTable(parts[i], Table(i));
        Create.Collection.Root = Create.Entries[0]; Nodes = GumpRawSupport.Nodes(row[3]);
        Assert.Equal(row[2], State()); Assert.Equal(row[3], GumpRawSupport.EncodeNodes(Nodes));
        if (row[5] == "-") return;
        // 기대 호출을 계산하지 않고 경계마다 명시한 대상/offset/값만 준비한다.
        foreach (string item in row[5].Split(';'))
        {
            string[] p = item.Split(':');
            if (!_responses.TryGetValue(p[0], out List<uint[]>? patches)) { patches = []; _responses.Add(p[0], patches); }
            patches.AddRange(p[1].Split('/').Select(s => s.Split(',').Select(X86Fixture.UInt).ToArray()));
        }
    }

    /// <summary>판본별 실제 표 길이만 선택한다. 비활성 CD idle 뒤쪽은 직렬화 범위 밖이다.</summary>
    private Span<uint> Table(int index) => index switch
    {
        1 => Create.Entries, 2 => Life.Idle.AsSpan(0, Create.Collection.Edition == OriginalEdition.Patch1078 ? 500 : 80),
        3 => Life.Overlay, 4 => Life.Kill, 5 => Life.Dialogs, _ => throw new ArgumentOutOfRangeException(nameof(index)),
    };

    /// <summary>현재 스칼라를 fixture의 원래 순서와 폭으로 읽는다.</summary>
    private uint Global(int index) => index switch
    {
        0 => Create.Count, 1 => Life.KillCount, 2 => Life.DialogCount, 3 => Life.Focus, 4 => Create.Collection.OverrideHead,
        5 => Life.CurrentDialog, 6 => Life.Diagnostics, 7 => Create.Serial, 8 => unchecked((byte)Excluded), _ => throw new ArgumentOutOfRangeException(nameof(index)),
    };

    /// <summary>명시 응답의 raw 스칼라를 같은 공유 필드에 기록한다.</summary>
    private void SetGlobal(int index, uint value)
    {
        switch (index)
        {
            case 0: Create.Count = value; break; case 1: Life.KillCount = value; break; case 2: Life.DialogCount = value; break;
            case 3: Life.Focus = value; break; case 4: Create.Collection.OverrideHead = value; break; case 5: Life.CurrentDialog = value; break;
            case 6: Life.Diagnostics = value; break; case 7: Create.Serial = value; break; case 8: Excluded = unchecked((sbyte)value); break;
            default: throw new ArgumentOutOfRangeException(nameof(index));
        }
    }

    /// <summary>완성 입력의 길이:값 구간만 정확한 길이의 표에 푼다.</summary>
    private static void ReadTable(string input, Span<uint> values)
    {
        int offset = 0;
        // 각 구간은 원본 입력이고 삭제 결과를 유추하지 않는다.
        foreach (string item in input.Split(';'))
        {
            string[] p = item.Split(':'); int length = X86Fixture.Int(p[0]); Assert.InRange(length, 0, values.Length - offset);
            values.Slice(offset, length).Fill(X86Fixture.UInt(p[1])); offset += length;
        }
        Assert.Equal(values.Length, offset);
    }

    /// <summary>현재 연속 같은 DWORD만 길이:값으로 압축한다.</summary>
    private static string Rle(ReadOnlySpan<uint> values)
    {
        var runs = new List<string>();
        // 모든 활성 칸을 읽으며 등록/삭제될 위치는 계산하지 않는다.
        for (int i = 0; i < values.Length;)
        {
            int first = i; uint value = values[i];
            // 같은 값의 구간 끝만 찾는다.
            while (i < values.Length && values[i] == value) i++;
            runs.Add($"{i - first}:{value}");
        }
        return string.Join(';', runs);
    }

    /// <summary>현재 스칼라와 다섯 표의 전체 활성 구간을 원본 형식으로 기록한다.</summary>
    public string State()
    {
        string result = string.Join(',', Enumerable.Range(0, _reset ? 9 : 7).Select(i => Global(i).ToString(CultureInfo.InvariantCulture)));
        // 현재 표의 모든 활성 칸을 원래 순서로 직렬화한다.
        for (int i = 1; i <= 5; i++) result += "#" + Rle(Table(i));
        return result;
    }

    /// <summary>경계 직전 상태를 기록한 뒤 현재 노드/표/전역에 명시 입력 DWORD만 쓴다.</summary>
    private void Respond(string kind, uint token, params uint[] args)
    {
        string key = $"{kind},{token}";
        Calls.Add(key + (args.Length == 0 ? "" : "," + string.Join(',', args)) + "@" + State() + "@" + GumpRawSupport.EncodeNodes(Nodes));
        if (!_responses.TryGetValue(key, out List<uint[]>? patches)) return;
        // 같은 노드 참조를 유지하며 응답 offset/값만 적용한다.
        foreach (uint[] p in patches)
        {
            if (p[0] == 0) SetGlobal((int)p[1], p[2]);
            else if (p[0] <= 5) { Table((int)p[0])[(int)p[1]] = p[2]; Create.Collection.Root = Create.Entries[0]; }
            else GumpRawSupport.ApplyDword(Nodes[p[0]], (int)p[1], p[2]);
        }
    }

    /// <summary>실제 수명 처리의 외부 경계를 현재 전체 상태 관측기로 연결한다.</summary>
    public GumpLifetimeHooks Hooks() => new()
    {
        Node = token => Nodes.GetValueOrDefault(token), Push = token => Respond("Push", token),
        DeleteForm = (form, flags) => Respond("Form", form, flags), DeletingDestructor = (token, flags) => Respond("Destructor", token, flags),
        Release = token => Respond("Free", token),
    };

    /// <summary>관측에 포함된 전체 노드/표/전역/반환/가상 호출 당시 상태를 모두 대조한다.</summary>
    public void Check(string[] row, IEnumerable<string> results)
    {
        Assert.Equal(row[7], GumpRawSupport.EncodeNodes(Nodes)); Assert.Equal(row[8], State()); Assert.Equal(row[9], string.Join('/', results));
        Assert.Equal(row[10], Calls.Count == 0 ? "-" : string.Join('|', Calls)); Assert.Equal(Create.Entries[0], Create.Collection.Root);
    }
}

/// <summary>실제 기본 생성·트리·지연 삭제·기본 소멸자·저장소 해제를 같은 노드에 연결하는 검사 장면이다.</summary>
internal sealed class GumpLifetimeRig
{
    /// <summary>원본 포인터 대신 작은 안정 토큰을 사용하는 현재 노드 저장소.</summary>
    public Dictionary<uint, GumpCreateNode> Nodes { get; } = [];
    /// <summary>같은 등록/루트/화면 전역.</summary>
    public GumpCreateState State { get; } = new() { ScreenWidth = 64, ScreenHeight = 48 };
    /// <summary>같은 수명 표/전역.</summary>
    public GumpLifetimeState LifeState { get; } = new();
    /// <summary>실제 해제 순서.</summary>
    public List<uint> Freed { get; } = [];
    /// <summary>실제 기본 생성기.</summary>
    public GumpCreate Create { get; }
    /// <summary>실제 트리 처리기.</summary>
    public GumpTree Tree { get; }
    /// <summary>실제 수명 처리기.</summary>
    public GumpLifetime Lifetime { get; }

    /// <summary>루트/명령27 일반 노드 세 개와 실제 수명/트리 가상 경계를 연결한다.</summary>
    public GumpLifetimeRig(OriginalEdition edition = OriginalEdition.Patch1078)
    {
        State.Collection.Edition = edition;
        // 아직 생성하지 않은5/6도 안정된 저장소로 먼저 확보한다.
        for (uint token = 1; token <= 6; token++) Nodes.Add(token, new());
        Create = new(State, new() { Node = token => Nodes.GetValueOrDefault(token), Allocate = _ => 1 }); Create.InitializeRoot();
        Create.Construct(2, 27, 8, 6, 1); Create.Construct(3, 27, 3, 2, 1); Create.Construct(4, 27, 2, 2, 1);
        Lifetime = new(State, LifeState, new()
        {
            Node = token => Nodes.GetValueOrDefault(token), Push = token => Tree!.Push(token),
            DeletingDestructor = (token, flags) => Lifetime!.DeleteScalar(token, flags), Release = token => { Freed.Add(token); Nodes.Remove(token); },
        });
        Tree = new(State, new(), new()
        {
            Node = token => Nodes.GetValueOrDefault(token), Redraw = (_, _) => { }, Push = token => Tree!.Push(token),
            Attach = (token, x, y, parent, mode) => Tree!.Attach(token, x, y, parent, mode), Kill = token => Lifetime.Kill(token),
        });
    }
}
