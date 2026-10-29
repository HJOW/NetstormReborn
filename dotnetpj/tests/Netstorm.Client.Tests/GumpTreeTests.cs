using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // Gump 트리의 현재 노드/가상 경계 순서를 대조하는 영역이다.

/// <summary>원본의 전체 트리 바이트와 콜백 당시 상태를 대조하고 실제 생성→부착→수집 연결을 검증한다.
/// 근거: cpppj/tests/GumpTreeTests.cpp. 공통 정렬/Draw 픽셀 통합은 R5에서 잇는다.</summary>
public sealed class GumpTreeTests
{
    /// <summary>signed/unsigned 십진수 입력을 같은 DWORD 비트로 읽는다.</summary>
    private static uint Word(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));

    /// <summary>판본마다574행의 연속 연산·전체 노드·전역·정렬 반환·콜백 시점 상태를 실제 PE 관측과 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeTree(string edition)
    {
        string[][] rows = X86Fixture.Read("gumptree", edition); Assert.Equal(574, rows.Length);
        // 한 행의 연속 연산은 같은 저장소/전역을 공유하고 행 사이에는 분리한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(13, row.Length); var state = new GumpCreateState(); var treeState = new GumpTreeState();
            state.Collection.Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            // 명시 응답의 전역 DWORD를 같은 필드에 기록한다.
            void Global(uint index, uint value)
            {
                int signed = unchecked((int)value);
                switch (index)
                {
                    case 0: state.Collection.Root = value; break;
                    case 1: state.ScreenWidth = signed; break;
                    case 2: state.ScreenHeight = signed; break;
                    case 3: treeState.MapView = treeState.MapView with { Left = signed }; break;
                    case 4: treeState.MapView = treeState.MapView with { Top = signed }; break;
                    case 5: treeState.MapView = treeState.MapView with { Right = signed }; break;
                    case 6: treeState.MapView = treeState.MapView with { Bottom = signed }; break;
                    case 7: treeState.PushDebugMode = value; break;
                    default: throw new ArgumentOutOfRangeException(nameof(index));
                }
            }
            // 원본 관측과 같은 전역 출력 순서다.
            string Globals() => $"{state.Collection.Root},{state.ScreenWidth},{state.ScreenHeight},{treeState.MapView.Left},{treeState.MapView.Top},{treeState.MapView.Right},{treeState.MapView.Bottom},{treeState.PushDebugMode}";
            uint[] globals = row[2].Split(',').Select(Word).ToArray();
            // 초기 전역은 fixture의 명시 입력만 사용한다.
            for (uint i = 0; i < globals.Length; i++) Global(i, globals[i]);
            SortedDictionary<uint, GumpCreateNode> nodes = GumpRawSupport.Nodes(row[3]); Assert.Equal(row[3], GumpRawSupport.EncodeNodes(nodes));
            var responses = new Dictionary<string, List<uint[]>>(); var anchors = new Dictionary<uint, ScreenPoint>(); var calls = new List<string>();
            if (row[5] != "-")
            {
                // 가상 종류/토큰별 완성 응답의 위치/값만 읽는다.
                foreach (string item in row[5].Split(';'))
                {
                    string[] part = item.Split(':'); responses.Add(part[0], part[1].Split('/').Select(p => p.Split(',').Select(Word).ToArray()).ToList());
                }
            }
            if (row[6] != "-")
            {
                // 월드 투영은 fixture에 기록된 정수 응답 그대로다.
                foreach (string item in row[6].Split(';'))
                {
                    string[] p = item.Split(','); anchors.Add(Word(p[0]), new(X86Fixture.Int(p[1]), X86Fixture.Int(p[2])));
                }
            }
            // 경계 직전의 전체 상태를 기록한 뒤 지정 DWORD 응답만 적용한다.
            void Respond(string kind, uint token, params uint[] args)
            {
                string key = $"{kind},{token}";
                calls.Add(key + (args.Length == 0 ? "" : "," + string.Join(',', args)) + "@" + Globals() + "@" + GumpRawSupport.EncodeNodes(nodes));
                if (!responses.TryGetValue(key, out List<uint[]>? patches)) return;
                // 부착/정렬 결과는 계산하지 않고 명시한 raw/전역 입력만 바꾼다.
                foreach (uint[] patch in patches)
                {
                    if (patch[0] == 0) Global(patch[1], patch[2]);
                    else GumpRawSupport.ApplyDword(nodes[patch[0]], (int)patch[1], patch[2]);
                }
            }
            var tree = new GumpTree(state, treeState, new()
            {
                Node = token => nodes.GetValueOrDefault(token), Redraw = (token, pushed) => Respond("Redraw", token, pushed ? 1U : 0U),
                Push = token => Respond("Push", token), Attach = (token, x, y, parent, mode) => Respond("Attach", token, BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y), parent, mode),
                Kill = token => Respond("Kill", token), Project = sid => { Respond("Project", sid); return anchors[sid]; },
            });
            var results = new List<string>();
            // 실제 함수를 입력 순서로 호출하고 결과는 원본 관측에서 읽는다.
            foreach (string item in row[4].Split(';'))
            {
                string[] p = item.Split(','); uint token = Word(p[1]); string result = "-";
                switch (p[0])
                {
                    case "Attach": tree.Attach(token, X86Fixture.FloatBits(p[2]), X86Fixture.FloatBits(p[3]), Word(p[4]), Word(p[5])); break;
                    case "Push": tree.Push(token); break;
                    case "Align":
                        (float X, float Y) position = tree.Align(token); byte[] raw = new byte[8];
                        BinaryPrimitives.WriteSingleLittleEndian(raw, position.X); BinaryPrimitives.WriteSingleLittleEndian(raw.AsSpan(4), position.Y); result = Convert.ToHexStringLower(raw); break;
                    case "AddDepth": tree.AddDepth(token, Word(p[2])); break;
                    case "SetDepth": tree.SetDepth(token, Word(p[2])); break;
                    case "Move": tree.Move(token, X86Fixture.FloatBits(p[2]), X86Fixture.FloatBits(p[3]), Word(p[4])); break;
                    case "Relative": tree.MoveRelative(token, Word(p[2]), X86Fixture.FloatBits(p[3]), X86Fixture.FloatBits(p[4]), unchecked((byte)Word(p[5])), Word(p[6])); break;
                    default: throw new InvalidDataException("알 수 없는 트리 연산");
                }
                results.Add(result);
            }
            Assert.Equal(row[7], GumpRawSupport.EncodeNodes(nodes)); Assert.Equal(row[8], Globals()); Assert.Equal(row[9], string.Join('/', results));
            Assert.Equal(row[10], calls.Count == 0 ? "-" : string.Join('|', calls));
        }
    }

    /// <summary>기본 생성기와 같은 저장소로 트리/수집 통합을 구성하는 최소 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>안정된 실제 생성/트리 노드.</summary>
        public Dictionary<uint, GumpCreateNode> Nodes { get; } = new() { [1] = new(), [2] = new(), [3] = new(), [4] = new() };
        /// <summary>같은 등록/화면 전역.</summary>
        public GumpCreateState State { get; } = new() { ScreenWidth = 32, ScreenHeight = 24 };
        /// <summary>같은 지도/진단 전역.</summary>
        public GumpTreeState Tree { get; } = new();
        /// <summary>같은 노드 저장소의 생성기.</summary>
        public GumpCreate Create { get; }
        /// <summary>기본 루트와 크기가 다른 일반 노드 세 개를 생성한다.</summary>
        public Scene()
        {
            Create = new(State, new() { Node = token => Nodes.GetValueOrDefault(token), Allocate = _ => 1 }); Create.InitializeRoot();
            Create.Construct(2, 27, 8, 6, 1); Create.Construct(3, 27, 3, 2, 1); Create.Construct(4, 27, 2, 2, 1);
        }
        /// <summary>명시 가상 경계는 기본적으로 관측만 하며 실제 기본 호출 연결은 검사가 선택한다.</summary>
        public GumpTreeHooks Hooks() => new() { Node = token => Nodes.GetValueOrDefault(token), Redraw = (_, _) => { }, Push = _ => { }, Attach = (_, _, _, _, _) => { }, Kill = _ => { } };
    }

    /// <summary>실제 생성→부착→분리/재부착→표시 수집이 같은 링크/캐시를 사용한다.</summary>
    [Fact]
    public void CreatedAttachPushCollection()
    {
        var scene = new Scene(); var tree = new GumpTree(scene.State, scene.Tree, scene.Hooks()); tree.Attach(2, 2, 3, 1, 0); tree.Attach(3, 1, 1, 2, 0);
        Assert.Equal(2U, scene.Nodes[1].Child); Assert.Equal(3U, scene.Nodes[2].Child); Assert.Equal(unchecked(scene.Nodes[2].Depth - 3), scene.Nodes[3].Depth);
        var collection = new GumpCollection(scene.State.Collection, scene.Create.CollectionHooks()); var work = new RenderCollectionEntry[8];
        Assert.Equal(2, collection.Collect(new(0, 0, 32, 24), work, 8)); Assert.Equal(2U, work[0].Token); Assert.Equal(3U, work[1].Token); Assert.Equal(new ScreenPoint(3, 4), scene.Nodes[3].Cached);
        tree.Push(2); Assert.Equal(0U, scene.Nodes[1].Child); Assert.Equal(1U, scene.Nodes[2].Parent); Assert.Equal(0, collection.Collect(new(0, 0, 32, 24), work, 8));
        tree.Attach(2, 7, 8, 0, 1); Assert.Equal(2, collection.Collect(new(0, 0, 32, 24), work, 8)); Assert.Equal(new ScreenPoint(8, 9), scene.Nodes[3].Cached);
    }

    /// <summary>가상 Push/Attach를 기본 구현에 연결한 자식 재배치가 같은 형제 링크를 갱신한다.</summary>
    [Fact]
    public void RealVirtualChildReflow()
    {
        var scene = new Scene(); GumpTree? current = null; GumpTreeHooks hooks = scene.Hooks(); hooks.Push = token => current!.Push(token); hooks.Attach = (token, x, y, parent, mode) => current!.Attach(token, x, y, parent, mode);
        current = new(scene.State, scene.Tree, hooks); current.Attach(2, 2, 3, 1, 0); current.Attach(3, 1, 1, 2, 0); current.Attach(4, 2, 2, 2, 0);
        scene.Nodes[3].AlignFlags = 0x10; scene.Nodes[4].AlignFlags = 0x20; scene.Nodes[4].SecondaryDepth = 7; current.Push(2); current.Attach(2, 4, 5, 0, 0);
        Assert.Equal(3U, scene.Nodes[2].Child); Assert.Equal(4U, scene.Nodes[3].Next); Assert.Equal(3U, scene.Nodes[4].Previous);
        Assert.Equal(5F, scene.Nodes[3].X); Assert.Equal(4F, scene.Nodes[4].Y); Assert.Equal(unchecked(scene.Nodes[2].Depth + 7), scene.Nodes[4].Depth);
        Assert.Equal(0U, scene.Nodes[3].Flags & 8); Assert.Equal(0U, scene.Nodes[4].Flags & 8);
    }

    /// <summary>자식 우선 깊이 DWORD 변경이 실제 수집의 signed WORD 키에 반영된다.</summary>
    [Fact]
    public void DepthChangesReachCollection()
    {
        var scene = new Scene(); var tree = new GumpTree(scene.State, scene.Tree, scene.Hooks()); tree.Attach(2, 2, 3, 1, 0); tree.Attach(3, 1, 1, 2, 0);
        tree.SetDepth(2, 0xfffffff0); tree.AddDepth(2, 0x80000000); Assert.Equal(0x7ffffff0U, scene.Nodes[2].Depth); Assert.Equal(0x7ffffff0U, scene.Nodes[3].Depth);
        var collection = new GumpCollection(scene.State.Collection, scene.Create.CollectionHooks()); var work = new RenderCollectionEntry[4];
        Assert.Equal(2, collection.Collect(new(0, 0, 32, 24), work, 4)); Assert.Equal(-16, work[0].Depth); Assert.Equal(-16, work[1].Depth);
    }

    /// <summary>없는 노드/처리기·순환·CD의void assert·지도 정렬 부모 계약을 명시 오류로 거부한다.</summary>
    [Fact]
    public void ContractsAndCycles()
    {
        var scene = new Scene(); Assert.Throws<ArgumentException>(() => new GumpTree(scene.State, scene.Tree, new())); var tree = new GumpTree(scene.State, scene.Tree, scene.Hooks());
        Assert.Throws<InvalidOperationException>(() => tree.Attach(99, 0, 0, 1, 0)); string before = GumpRawSupport.Encode(scene.Nodes[2]);
        Assert.Throws<InvalidOperationException>(() => tree.Attach(2, 0, 0, 99, 0)); Assert.Equal(before, GumpRawSupport.Encode(scene.Nodes[2]));
        Assert.Throws<InvalidOperationException>(() => tree.Push(2)); scene.State.Collection.Edition = OriginalEdition.Cd1072; scene.Nodes[2].Flags = 1;
        Assert.Throws<InvalidOperationException>(() => tree.Attach(2, 0, 0, 1, 0)); scene.Nodes[2].Parent = 1; scene.Nodes[2].Child = 3; scene.Nodes[3].Child = 2;
        Assert.Throws<InvalidOperationException>(() => tree.AddDepth(2, 1)); scene.Nodes[2].Parent = 3; scene.Nodes[2].AlignFlags = 0x40;
        Assert.Throws<InvalidOperationException>(() => tree.Align(2));
    }

    /// <summary>가상 redraw가 예외를 내도 원본 순서상 이미 쓴 링크/위치는 원복하지 않는다.</summary>
    [Fact]
    public void CallbackExceptionKeepsWrittenState()
    {
        var scene = new Scene(); GumpTreeHooks hooks = scene.Hooks(); hooks.Redraw = (_, _) => throw new InvalidOperationException("입력 처리기 실패");
        var tree = new GumpTree(scene.State, scene.Tree, hooks); Assert.Throws<InvalidOperationException>(() => tree.Attach(2, 2, 3, 1, 0));
        Assert.Equal(2U, scene.Nodes[1].Child); Assert.Equal(1U, scene.Nodes[2].Parent); Assert.Equal(2F, scene.Nodes[2].X); Assert.Equal(0U, scene.Nodes[2].Flags & 8);
        Assert.Throws<InvalidOperationException>(() => tree.Push(2)); Assert.Equal(0U, scene.Nodes[1].Child); Assert.NotEqual(0U, scene.Nodes[2].Flags & 8);
    }
}
