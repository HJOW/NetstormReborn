using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 사건/입력의 전체 상태와 명시 가상 응답을 공유하는 검사 영역이다.

/// <summary>원본 GumpEvents/GumpInput의 노드·전역·표·콜백 관측만 읽고 쓴다. 예상 선택/전달 결과는 계산하지 않는다.</summary>
internal sealed class GumpEventFixture
{
    /// <summary>실제 루트 등록/현재 수집 전역.</summary>
    public GumpCreateState Create { get; } = new();
    /// <summary>같은 캡처/dialog/overlay 전역.</summary>
    public GumpLifetimeState Lifetime { get; } = new();
    /// <summary>같은 커서 선택 전역.</summary>
    public GumpEventState Event { get; } = new();
    /// <summary>별도 overlay 사건 전역.</summary>
    public GumpInputState Input { get; } = new();
    /// <summary>현재 모든 노드의 안정된 주소 순서 저장소.</summary>
    public SortedDictionary<uint, GumpCreateNode> Nodes { get; }
    /// <summary>실제 가상 호출 당시 전체 상태의 순서 기록.</summary>
    public List<string> Calls { get; } = [];
    /// <summary>입력 전용4개 스칼라와2표를 포함할지 선택한다.</summary>
    private readonly bool _input;
    /// <summary>가상 종류/토큰별 raw 반환과 지정 DWORD 쓰기.</summary>
    private readonly Dictionary<string, (uint Value, uint[][] Patches)> _responses = [];
    /// <summary>명시 월드 투영 정수 응답.</summary>
    private readonly Dictionary<uint, ScreenPoint> _anchors = [];

    /// <summary>signed/unsigned 십진수를 같은 DWORD 비트로 읽는다.</summary>
    public static uint Word(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));

    /// <summary>판본과 한 행의 전체 명시 입력을 준비하며 초기 직렬화도 원본과 대조한다.</summary>
    public GumpEventFixture(string edition, string[] row, bool input)
    {
        _input = input; Create.Collection.Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        uint[] globals = row[2].Split(',').Select(Word).ToArray(); Assert.Equal(input ? 11 : 7, globals.Length);
        // raw 전역은 같은 공유 필드에 초기화한다.
        for (int i = 0; i < globals.Length; i++) SetGlobal(i, globals[i]);
        Nodes = GumpRawSupport.Nodes(row[3]);
        if (input)
        {
            string[] tables = row[4].Split(';'); uint[] overlay = tables[0].Split(',').Select(Word).ToArray(); uint[] dialogs = tables[1].Split(',').Select(Word).ToArray();
            Assert.Equal(100, overlay.Length); Assert.Equal(16, dialogs.Length); overlay.CopyTo(Lifetime.Overlay, 0); dialogs.CopyTo(Lifetime.Dialogs, 0); Assert.Equal(row[4], Tables());
        }
        Assert.Equal(row[2], State()); Assert.Equal(row[3], GumpRawSupport.EncodeNodes(Nodes));
        string responseText = row[input ? 7 : 6];
        if (responseText != "-")
        {
            // 중복 키는 마지막 입력으로 덮어쓰며 예상 호출은 만들지 않는다.
            foreach (string item in responseText.Split(';'))
            {
                string[] p = item.Split(':'); string[] key = p[0].Split(',');
                _responses[key[0] + "," + key[1]] = (Word(key[2]), p[1] == "-" ? [] : p[1].Split('/').Select(s => s.Split(',').Select(Word).ToArray()).ToArray());
            }
        }
        string anchorText = row[input ? 8 : 7];
        if (anchorText == "-") return;
        // 위치 계산/경계는 실제 수집기에 맡기고 투영 정수점만 공급한다.
        foreach (string item in anchorText.Split(';'))
        {
            uint[] values = item.Split(',').Select(Word).ToArray(); _anchors[values[0]] = new(unchecked((int)values[1]), unchecked((int)values[2]));
        }
    }

    /// <summary>명시 raw 전역을 원본 순서의 같은 필드에 쓴다.</summary>
    private void SetGlobal(int index, uint value)
    {
        switch (index)
        {
            case 0: Create.Entries[0] = Create.Collection.Root = value; break; case 1: Lifetime.CurrentDialog = value; break;
            case 2: Event.Point = Event.Point with { X = unchecked((int)value) }; break; case 3: Event.Point = Event.Point with { Y = unchecked((int)value) }; break;
            case 4: Event.Mask = value; break; case 5: Event.BestDepth = value; break; case 6: Event.Selected = value; break;
            case 7: Lifetime.Focus = value; break; case 8: Create.Collection.OverrideHead = value; break; case 9: Lifetime.DialogCount = value; break;
            case 10: Input.OverlayEvent = value; break; default: throw new ArgumentOutOfRangeException(nameof(index));
        }
    }

    /// <summary>현재 선택/입력 전역의 전체 비트를 기록한다.</summary>
    public string State()
    {
        uint[] values = [Create.Entries[0], Lifetime.CurrentDialog, unchecked((uint)Event.Point.X), unchecked((uint)Event.Point.Y), Event.Mask, Event.BestDepth,
            Event.Selected, Lifetime.Focus, Create.Collection.OverrideHead, Lifetime.DialogCount, Input.OverlayEvent];
        return string.Join(',', values.Take(_input ? 11 : 7));
    }

    /// <summary>overlay/dialog 표의 비활성 뒤 칸까지 현재 순서대로 기록한다.</summary>
    public string Tables() => string.Join(',', Lifetime.Overlay) + ";" + string.Join(',', Lifetime.Dialogs);

    /// <summary>가상 호출 직전 전체 상태를 기록하고 명시한 raw/전역/표 쓰기만 적용한 뒤 원본 반환을 공급한다.</summary>
    private uint Respond(string kind, uint token, params uint[] args)
    {
        string key = $"{kind},{token}";
        Calls.Add(key + (args.Length == 0 ? "" : "," + string.Join(',', args)) + "@" + State() + "@" + GumpRawSupport.EncodeNodes(Nodes) + (_input ? "@" + Tables() : ""));
        if (!_responses.TryGetValue(key, out var response)) return 0;
        // 응답 위치만 바꾸고 선택/전달/등록의 예상 결과를 재계산하지 않는다.
        foreach (uint[] patch in response.Patches)
        {
            if (patch[0] == 0) SetGlobal((int)patch[1], patch[2]);
            else if (_input && patch[0] == 1) Lifetime.Overlay[patch[1]] = patch[2];
            else if (_input && patch[0] == 2) Lifetime.Dialogs[patch[1]] = patch[2];
            else GumpRawSupport.ApplyDword(Nodes[patch[0]], (int)patch[1], patch[2]);
        }
        return response.Value;
    }

    /// <summary>같은 노드와 월드 투영만 공급하는 실제 위치 수집 경계.</summary>
    public GumpCollectionHooks CollectionHooks() => new()
    {
        Node = token => Nodes.GetValueOrDefault(token), Project = sid => { Respond("Project", sid); return _anchors[sid]; },
    };

    /// <summary>기본 필터 또는 명시 가상 필터 응답과 현재 전체 처리기 응답을 연결한다.</summary>
    public GumpEventHooks EventHooks(bool binding) => new()
    {
        Node = token => Nodes.GetValueOrDefault(token), Filter = binding ? null : (token, e, x, y) => Respond("Filter", token, e, x, y) != 0,
        Handle = (token, e, x, y) => Respond("Handle", token, e, x, y),
    };

    /// <summary>직접 캡처/overlay도 사건 전달과 같은 가상 응답을 사용한다.</summary>
    public GumpInputHooks InputHooks() => new()
    {
        Node = token => Nodes.GetValueOrDefault(token), Handle = (token, e, x, y) => Respond("Handle", token, e, x, y), Overflow = code => Respond("Overflow", code),
    };

    /// <summary>현재 노드/전역/표/반환/콜백 상태를 원본 기대값 열과 모두 대조한다.</summary>
    public void Check(string[] row, IEnumerable<uint> results)
    {
        Assert.False(_input); Assert.Equal(row[8], GumpRawSupport.EncodeNodes(Nodes)); Assert.Equal(row[9], State()); Assert.Equal(row[10], string.Join(',', results));
        Assert.Equal(row[11], Calls.Count == 0 ? "-" : string.Join('|', Calls));
    }

    /// <summary>입력 전용 두 표까지 포함한 전체 관측을 대조한다.</summary>
    public void CheckInput(string[] row, IEnumerable<string> results)
    {
        Assert.True(_input); Assert.Equal(row[9], GumpRawSupport.EncodeNodes(Nodes)); Assert.Equal(row[10], State()); Assert.Equal(row[11], Tables());
        Assert.Equal(row[12], string.Join(',', results)); Assert.Equal(row[13], Calls.Count == 0 ? "-" : string.Join('|', Calls));
    }
}

/// <summary>실제 생성/트리/수명에 직계 형제와 손자를 부착해 선택/입력 통합에 사용한다.</summary>
internal sealed class GumpEventRig
{
    /// <summary>실제 생성/트리/수명/해제의 공통 장면.</summary>
    public GumpLifetimeRig Rig { get; } = new();
    /// <summary>같은 커서 전역.</summary>
    public GumpEventState State { get; } = new();
    /// <summary>같은 overlay 사건 전역.</summary>
    public GumpInputState InputState { get; } = new();
    /// <summary>같은 노드의 실제 위치/경계 처리기.</summary>
    public GumpCollection Collection { get; }

    /// <summary>20×16 직계 형제2/3과8×6 손자4를 원본 검사와 같은 위치에 부착한다.</summary>
    public GumpEventRig()
    {
        Rig.Nodes[2].Command = 11; Rig.Nodes[3].Command = 12; Rig.Nodes[4].Command = 13;
        Rig.Nodes[2].BaseWidth = Rig.Nodes[2].Width = Rig.Nodes[3].BaseWidth = Rig.Nodes[3].Width = 20;
        Rig.Nodes[2].BaseHeight = Rig.Nodes[2].Height = Rig.Nodes[3].BaseHeight = Rig.Nodes[3].Height = 16;
        Rig.Nodes[4].BaseWidth = Rig.Nodes[4].Width = 8; Rig.Nodes[4].BaseHeight = Rig.Nodes[4].Height = 6;
        Rig.Tree.Attach(2, 10, 10, 1, 0); Rig.Tree.Attach(3, 10, 10, 1, 0); Rig.Tree.Attach(4, 1, 1, 2, 0);
        Collection = new(Rig.State.Collection, Rig.Create.CollectionHooks());
    }

    /// <summary>같은 저장소 조회를 기본으로 연결하고 필터/처리기는 검사가 선택한다.</summary>
    public GumpEvents Events(GumpEventHooks? hooks = null)
    {
        hooks ??= new(); hooks.Node ??= token => Rig.Nodes.GetValueOrDefault(token);
        return new(Rig.State, Rig.LifeState, State, Collection, hooks);
    }

    /// <summary>같은 저장소/캡처/사건 처리기로 입력 계층을 구성한다.</summary>
    public GumpInput Input(GumpEvents events, GumpInputHooks? hooks = null)
    {
        hooks ??= new(); hooks.Node ??= token => Rig.Nodes.GetValueOrDefault(token);
        return new(Rig.State, Rig.LifeState, InputState, Collection, events, hooks);
    }
}
