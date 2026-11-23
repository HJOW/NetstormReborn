// 메타 NSInfo의 실제 참가자 등록 명부 정렬·행 작성을 검사한다.
// cpppj/src/client/HudNsInfoMeta.h·cpp 의 RefreshNsInfoRoster(0047e630: 전체 39명 초기 순서·원문 쌍교환 정렬·이름/문맥/행 metadata) 를 이식한다.
// 0.906에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 CS/정렬 표를 빌리는 등록 명부의 계약이다.</summary>
public sealed class HudNsInfoMetaRosterTests
{
    /// <summary>같은 실제 CS/정렬 표와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS 참가자40·전투10 기록이다.</summary>
        public ClientChallengeGlobals Challenge { get; } = new();

        /// <summary>같은 005930b0 정렬 표/현재 NSInfo다.</summary>
        public ClientNsInfoGlobals Info { get; } = new();

        /// <summary>같은 594fc8의 현재 메타맵 모드다.</summary>
        public uint Metamap { get; set; } = 1;

        /// <summary>같은 595344의 현재 플레이어 인식 값이다.</summary>
        public uint Known { get; set; } = 1;

        /// <summary>같은 540c70의 현재 로컬 소유자다.</summary>
        public uint Owner { get; set; } = 1;

        /// <summary>실제 0xd8 NSInfo 노드다.</summary>
        public HudNsInfoNode Root { get; } = new();

        /// <summary>대체된 문자열이다.</summary>
        public List<(uint Token, string Text)> Replaced { get; } = [];

        /// <summary>추가된 정렬행이다.</summary>
        public List<(uint Token, string Text, uint[] Context, uint Row)> Appended { get; } = [];

        /// <summary>현재 팔레트색 읽기 값이다.</summary>
        public Dictionary<uint, uint> Reads { get; } = new() { [0x5c78d0u] = 0x11u, [0x59afd8u] = 0x22u, [0x5c85c8u] = 0u };

        /// <summary>SID별 실제 소유자다.</summary>
        public Dictionary<uint, uint> Owners { get; } = [];

        /// <summary>같은 실제 전역 묶음이다.</summary>
        public HudNsInfoMetaGlobals Globals() => new() { Challenge = Challenge, Info = Info, Metamap = () => Metamap, Known = () => Known, Owner = () => Owner };

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public HudNsInfoMetaHooks Hooks() => new()
        {
            Root = _ => Root,
            Replace = (token, text) => Replaced.Add((token, text)),
            Wrap = (_, _) => { },
            Read = address => Reads.TryGetValue(address, out uint value) ? value : throw new InvalidOperationException("읽기 누락:" + address.ToString("x")),
            ObjectOwner = sid => Owners.TryGetValue(sid, out uint value) ? value : 0u,
            Weight = span => BinaryPrimitives.ReadUInt32LittleEndian(span.Span[..4]),
            Append = (token, text, context, row) => Appended.Add((token, text, context.ToArray(), row)),
        };

        /// <summary>같은 실제 CS 기록을 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
            Root.Roster = 0xabu;
        }

        /// <summary>참가자 기록을 실제 필드에 쓴다.</summary>
        public void Write(uint index, byte level, bool valid, string name, ushort minor = 0, uint sid = 0, uint weight = 0)
        {
            var record = Challenge.Participant(index).Span;
            record.Clear();
            record[0x58] = level;
            record[0x64] = valid ? (byte)1 : (byte)0;
            Encoding.Latin1.GetBytes(name, record[0x26..]);
            BinaryPrimitives.WriteUInt16LittleEndian(record[0xb2..], minor);
            BinaryPrimitives.WriteUInt32LittleEndian(record[0xb8..], sid);
            BinaryPrimitives.WriteUInt32LittleEndian(record[0x54..], weight);
        }
    }

    /// <summary>관문 미달이면 같은 실제 장치를 건드리지 않는다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void GuardsReturnWithoutTouchingDevices()
    {
        var scene = new Scene();
        scene.Metamap = 0;
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), new HudNsInfoMetaHooks());
        Assert.Equal(0u, scene.Info.RosterOrder[1]);
        Assert.Empty(scene.Appended);
        scene.Metamap = 1;
        scene.Known = 0;
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), new HudNsInfoMetaHooks());
        Assert.Equal(0u, scene.Info.RosterOrder[1]);
    }

    /// <summary>0번을 보존하며 39DWORD만 초기 순서로 쓴다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void InitialOrderWrittenWithZeroPreserved()
    {
        var scene = new Scene();
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        Assert.Equal(0u, scene.Info.RosterOrder[0]);
        for (uint index = 1; index < 40; index++)
        {
            Assert.Equal(index, scene.Info.RosterOrder[index]);
        }

        Assert.Empty(scene.Appended);
    }

    /// <summary>원문 쌍교환은 레벨 오름차순으로 재배치한다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void PairwiseExchangeSortsLevelsAscending()
    {
        var scene = new Scene();
        scene.Write(1, 5, true, "a", weight: 100);
        scene.Write(2, 9, true, "b", weight: 200);
        scene.Write(3, 3, true, "c", weight: 300);
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        Assert.Equal([3u, 1u, 2u], [scene.Info.RosterOrder[1], scene.Info.RosterOrder[2], scene.Info.RosterOrder[3]]);
        Assert.Equal(3, scene.Appended.Count);
        Assert.Contains("c\n", scene.Appended[0].Text);
        Assert.Contains("a\n", scene.Appended[1].Text);
        Assert.Contains("b\n", scene.Appended[2].Text);
    }

    /// <summary>동률이면 상대→현재 순서로 기술 합계를 읽어 오름차순으로 둔다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void TieBrokenByWeightAscending()
    {
        var scene = new Scene();
        scene.Write(1, 5, true, "a", weight: 200);
        scene.Write(2, 5, true, "b", weight: 100);
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        Assert.Equal([2u, 1u], [scene.Info.RosterOrder[1], scene.Info.RosterOrder[2]]);
    }

    /// <summary>유효행만 추가하되 metadata는 정렬 배열의 행 번호다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void MetadataIsRowNumberNotParticipantIndex()
    {
        var scene = new Scene();
        scene.Write(2, 1, true, "a", weight: 10);
        scene.Write(1, 9, true, "b", weight: 20);
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        Assert.Equal([2u, 1u], [scene.Info.RosterOrder[1], scene.Info.RosterOrder[2]]);
        Assert.Equal([1u, 2u], [scene.Appended[0].Row, scene.Appended[1].Row]);
        Assert.Contains("a\n", scene.Appended[0].Text);
        Assert.Contains("b\n", scene.Appended[1].Text);
    }

    /// <summary>숫자행은 레벨+1·기술합계와 이름·개행이다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void RowTextFormat()
    {
        var scene = new Scene();
        scene.Write(4, 8, true, "ab", weight: 7);
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        var only = Assert.Single(scene.Appended);
        Assert.Equal(" 9.7 ab\n", only.Text);
        Assert.Equal(0xabu, only.Token);
    }

    /// <summary>자신이면 실제 소유자 판정과 메타 모드 조회로 문맥을 고른다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void OwnContextSelectedForOwnedParticipantInMetaMode()
    {
        var scene = new Scene();
        scene.Write(1, 8, true, "a", sid: 100, weight: 7);
        scene.Write(2, 8, true, "b", sid: 200, weight: 7);
        scene.Owners[100] = 1;
        scene.Owners[200] = 2;
        scene.Reads[0x5c85c8u] = 5;
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        Assert.Equal(2, scene.Appended.Count);
        Assert.Equal([2u, 0u, 0x22u, 0u, 0u, 0x11u, 1u, 1u], scene.Appended[0].Context);
        Assert.Equal([2u, 0u, 0xdfu, 0u, 0u, 0x11u, 1u, 1u], scene.Appended[1].Context);
        scene.Reads[0x5c85c8u] = 0;
        scene.Appended.Clear();
        scene.Replaced.Clear();
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        Assert.All(scene.Appended, row => Assert.Equal([2u, 0u, 0xdfu, 0u, 0u, 0x11u, 1u, 1u], row.Context));
    }

    /// <summary>부전공 78은 0xc4·초과는 0x9d·미만은 0xdf다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void MinorColorMapping()
    {
        var scene = new Scene();
        scene.Write(1, 8, true, "a", minor: 77, weight: 3);
        scene.Write(2, 8, true, "b", minor: 78, weight: 2);
        scene.Write(3, 8, true, "c", minor: 79, weight: 1);
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        Assert.Equal(0x9du, scene.Appended[0].Context[2]);
        Assert.Equal(0xc4u, scene.Appended[1].Context[2]);
        Assert.Equal(0xdfu, scene.Appended[2].Context[2]);
    }

    /// <summary>120BYTE 이상 행과 NUL 없는 이름은 명시 거부한다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void LongRowAndMissingNulFailExplicitly()
    {
        // 이름 영역(0x26..)과 기술합계 10BYTE(0x54..)가 겹치므로 긴 이름은 필드를 이름 뒤에 직접 둔다.
        var scene = new Scene();
        var longRecord = scene.Challenge.Participant(1).Span;
        longRecord.Clear();
        Encoding.Latin1.GetBytes(new string('a', 115), longRecord[0x26..]);
        longRecord[0x26 + 115] = 0;
        longRecord[0x58] = 8;
        longRecord[0x64] = 1;
        BinaryPrimitives.WriteUInt32LittleEndian(longRecord[0x54..], 0x41414141);
        Assert.Throws<ArgumentOutOfRangeException>(() => HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks()));
        var broken = new Scene();
        var record = broken.Challenge.Participant(1).Span;
        record.Clear();
        record[0x58] = 8;
        record[0x64] = 1;
        record.Slice(0x26).Fill(0x41);
        Assert.Throws<ArgumentOutOfRangeException>(() => HudNsInfoMetaRoster.Refresh(9, broken.Globals(), broken.Hooks()));
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        scene.Write(1, 8, true, "a", weight: 1);
        Assert.Throws<InvalidOperationException>(() => HudNsInfoMetaRoster.Refresh(9, scene.Globals(), new HudNsInfoMetaHooks()));
        var hooks = scene.Hooks();
        hooks.Root = null;
        Assert.Throws<InvalidOperationException>(() => HudNsInfoMetaRoster.Refresh(9, scene.Globals(), hooks));
        Assert.Throws<ArgumentNullException>(() => HudNsInfoMetaRoster.Refresh(9, null!, scene.Hooks()));
    }

    /// <summary>정렬 표 별칭은 같은 실제 배열을 공유한다. 0.906에서 추가됐다.</summary>
    [Fact]
    public void RosterOrderSharesActualArray()
    {
        var scene = new Scene();
        HudNsInfoMetaRoster.Refresh(9, scene.Globals(), scene.Hooks());
        Assert.Same(scene.Info.RosterOrder, scene.Info.RosterOrder);
        Assert.Equal(39u, scene.Info.RosterOrder[39]);
        Assert.Equal([(0xabu, "")], scene.Replaced);
    }
}
