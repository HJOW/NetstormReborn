using System.Buffers.Binary;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 새 조회 본체의 원시 입력/소비 순서를 검사한다. 두 함수 독립 x86 fixture 통과로 세지 않는다.
namespace Netstorm.Client.Tests;

/// <summary>같은 실제 풀·목록·해시·현재 CS와 선택된 가상 경계의 명시 입력을 사용하는 좁은 검사다.</summary>
public sealed class SceneMusicWorldQueriesTests
{
    /// <summary>유효 물리 입력만 준비하며 정답 희생/대기실 bool을 저장하지 않는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>동일 실제 PATCH 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>실제 일반 finder가 읽는 네 단계 해시다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>조회 중 레코드 교체가 그대로 보이는 실제 TYPE 저장소다.</summary>
        public readonly RawTypeTableStorage Types = new();
        /// <summary>실제 원문 등록 목록과 같은 모양/수명의 소유자다.</summary>
        public readonly RawStateSidLists Lists = new();
        /// <summary>실제 CS 할당·현재 토큰의 소유자다.</summary>
        public readonly ClientChallengeGlobals Cs = new();
        /// <summary>현재 모드/PlayerKnown/소유자 원시 입력이다.</summary>
        public bool Challenge;
        /// <summary>현재 전체 준비 DWORD다.</summary>
        public uint Known;
        /// <summary>현재 전체 소유자 DWORD다.</summary>
        public uint Local = 1;
        /// <summary>선택된 외부 조회의 호출 순서다.</summary>
        public readonly List<string> Events = [];
        /// <summary>명시된 가상+80 입력이며 실제 Game 분배가 아니다.</summary>
        public Func<int, uint> Count = _ => 1;
        /// <summary>Local getter의 현재 값 반환 전 명시 재진입 효과다.</summary>
        public Action? OnLocal;
        /// <summary>직접 실행할 동일 조회 몸체다.</summary>
        public readonly SceneMusicWorldQueries Queries;

        /// <summary>실제 풀 할당 후 원시 입력을 만들고 첫 살아있는 사제 목록을 공유한다.</summary>
        public Scene()
        {
            Types.AddRange(Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }));
            // 지정 SID는 명시 입력이며 Factory/세션 성공을 흉내내지 않는다.
            for (int sid = 5; sid <= 8; sid++) { Assert.Equal(sid, Pool.Allocate(SidAllocation.Client)); Pool.AllocatedBytes(sid).Clear(); }
            Slot(5, 158, 40.25F, 41.25F, 1); Lists.Priests.Append(5);
            var lookup = new RawPriestLookup(Pool, new() { SharedPriests = Lists.Priests });
            Queries = new(Pool, Hash, Types, lookup, sid => { Events.Add("count:" + sid); return Count(sid); },
                () => { Events.Add("challenge"); return Challenge; }, () => { Events.Add("known"); return Known; },
                () => { Events.Add("local"); OnLocal?.Invoke(); return Local; }, Cs);
        }

        /// <summary>실제 raw 슬롯의 좌표·TYPE·현재 owner만 입력한다.</summary>
        public void Slot(int sid, byte type, float x, float y, byte owner)
        {
            Span<byte> raw = Pool.AllocatedBytes(sid); raw[10] = type; raw[34] = owner;
            BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
        }

        /// <summary>일반 해시 체인에 현재 후보를 입력하고 원문 altar genus를 현재 TYPE에 준다.</summary>
        public void Altar(int sid = 6, float x = 43.5F, float y = 41.5F, int frame = 14, int level = 1)
        {
            Slot(sid, 17, x, y, 8); Types[17].Flags2 = 0x400000;
            Span<byte> raw = Pool.AllocatedBytes(sid); BinaryPrimitives.WriteInt32LittleEndian(raw[36..], frame);
            ref ushort head = ref Hash.Bucket(level, x, y); BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], head); head = (ushort)sid;
        }
    }

    /// <summary>제공되지 않은 목록/해시 후보는 원문처럼 false이며 가상Count/모드/현재Player는 읽지 않는다.</summary>
    [Fact]
    public void MissingPriestOrAltarStopsBeforeSelectedVirtualBoundary()
    {
        var scene = new Scene(); scene.Count = _ => throw new IOException("미선택Count");
        Assert.False(scene.Queries.Sacrificing(1)); Assert.Empty(scene.Events);
        scene.Altar(); scene.Lists.Priests.Count = 0;
        Assert.False(scene.Queries.Sacrificing(1)); Assert.Empty(scene.Events);
    }

    /// <summary>목록의 첫 살아있는 소유 사제만 사용하고 소유자 DWORD를 BYTE로 줄이지 않는다.</summary>
    [Fact]
    public void CurrentPriestOwnerAndDeadBitAreReadFromSameRegisteredList()
    {
        var scene = new Scene(); scene.Altar();
        scene.Pool.AllocatedBytes(5)[11] = 2;
        scene.Slot(7, 158, 20.25F, 20.25F, 1); scene.Lists.Priests.Append(7);
        Assert.False(scene.Queries.Sacrificing(1)); Assert.Empty(scene.Events);
        scene.Pool.AllocatedBytes(5)[11] = 0;
        Assert.False(scene.Queries.Sacrificing(0x101)); Assert.Empty(scene.Events);
        Assert.True(scene.Queries.Sacrificing(1)); Assert.Equal(new[] { "count:6" }, scene.Events);
    }

    /// <summary>좌표 거의올림과 반경3을 사용하고 현재TYPE교체 및 buried/level0 제외만 적용한다.</summary>
    [Fact]
    public void RadiusThreeAndCurrentGenusUseSameActualFinder()
    {
        var scene = new Scene(); scene.Altar(x: 43.5F);
        Assert.True(scene.Queries.Sacrificing(1)); scene.Events.Clear();
        scene.Types[17] = new() { FootX = 1, FootY = 1, Flags2 = 0 };
        Assert.False(scene.Queries.Sacrificing(1)); Assert.Empty(scene.Events);
        scene.Types[17].Flags2 = 0x400000; scene.Pool.AllocatedBytes(6)[40] = 8;
        Assert.False(scene.Queries.Sacrificing(1)); Assert.Empty(scene.Events);
        scene.Pool.AllocatedBytes(6)[40] = 0; scene.Hash.Reset(); scene.Hash.Bucket(0, 43.5F, 41.5F) = 6;
        Assert.False(scene.Queries.Sacrificing(1)); Assert.Empty(scene.Events);
    }

    /// <summary>첫 altar는 owner/free/dead/고정TYPE를 거르지 않으며 프레임은 Count 부작용보다 먼저 저장한다.</summary>
    [Fact]
    public void FrameIsSavedBeforeCountAndNoExtraLivenessFilterIsAdded()
    {
        var scene = new Scene(); scene.Altar(frame: 14); scene.Pool.AllocatedBytes(6)[11] = 2;
        scene.Count = sid => { Assert.Equal(6, sid); BinaryPrimitives.WriteInt32LittleEndian(scene.Pool.AllocatedBytes(sid)[36..], 0); return 1; };
        Assert.True(scene.Queries.Sacrificing(1));
        Assert.False(scene.Queries.Sacrificing(1)); Assert.Equal(new[] { "count:6", "count:6" }, scene.Events);
    }

    /// <summary>미리 할당한 후보의 free/dead 비트는 조회 거부 조건이 아니다. 공개 쓰기 범위로 입력한 뒤 읽기만 한다.</summary>
    [Theory]
    [InlineData((byte)1)]
    [InlineData((byte)2)]
    [InlineData((byte)3)]
    public void AltarFreeAndDeadBitsDoNotAddLookupFilters(byte flags)
    {
        var scene = new Scene(); scene.Altar(frame: 14); scene.Pool.AllocatedBytes(6)[11] = flags;
        scene.Count = sid => { Assert.Equal(flags, scene.Pool.Slot(sid)[11]); return 1; };
        Assert.True(scene.Queries.Sacrificing(1)); Assert.Equal(new[] { "count:6" }, scene.Events);
        Assert.Equal(flags, scene.Pool.Slot(6)[11]);
    }

    /// <summary>signed단계 첫 경계와 uint뺄셈 감김을 기존 의식 본체에서 그대로 소비한다.</summary>
    [Theory]
    [InlineData(13, 1u, false)]
    [InlineData(14, 1u, true)]
    [InlineData(74, 2u, false)]
    [InlineData(75, 2u, true)]
    [InlineData(int.MinValue, 1u, true)]
    [InlineData(-1, 0u, false)]
    public void SavedFrameAndCountKeepNativeSignedStageBoundary(int frame, uint count, bool expected)
    {
        var scene = new Scene(); scene.Altar(frame: frame); scene.Count = _ => count;
        Assert.Equal(expected, scene.Queries.Sacrificing(1)); Assert.Equal(new[] { "count:6" }, scene.Events);
    }

    /// <summary>가상 장치 실패 뒤 임의bool을 반환하지 않고 이미 실행한 조회 접두만 남긴다.</summary>
    [Fact]
    public void SelectedVirtualFailureIsPreservedAndGlobalGatesAreNotRead()
    {
        var scene = new Scene(); scene.Altar(); var failure = new IOException("실제가상미연결"); scene.Count = _ => throw failure;
        Assert.Same(failure, Assert.Throws<IOException>(() => scene.Queries.Sacrificing(1)));
        Assert.Equal(new[] { "count:6" }, scene.Events);
    }

    /// <summary>일반 모드나 아직 모르는 Player는 CS0/owner를 읽지 않고 자연스럽게 false다.</summary>
    [Fact]
    public void WaitingRoomKeepsChallengeThenWholeKnownShortCircuit()
    {
        var scene = new Scene(); scene.OnLocal = () => throw new IOException("미선택소유자");
        Assert.False(scene.Queries.WaitingRoom()); Assert.Equal(new[] { "challenge" }, scene.Events); scene.Events.Clear();
        scene.Challenge = true;
        Assert.False(scene.Queries.WaitingRoom()); Assert.Equal(new[] { "challenge", "known" }, scene.Events);
    }

    /// <summary>현재CS의+a4 DWORD만 판정하고 valid/배틀/Subscriber/인접DWORD를 조건으로 추가하지 않는다.</summary>
    [Fact]
    public void WaitingRoomReadsExactCurrentParticipantFieldWithoutExtraFilters()
    {
        var scene = new Scene { Challenge = true, Known = 0x80000000, Local = 39 }; Assert.True(scene.Cs.Prepare());
        Span<byte> record = scene.Cs.Participant(39).Span; record.Clear();
        BinaryPrimitives.WriteUInt32LittleEndian(record[0xa0..], 0xffffffff); BinaryPrimitives.WriteUInt32LittleEndian(record[0xa8..], 0xffffffff);
        Assert.False(scene.Queries.WaitingRoom()); BinaryPrimitives.WriteUInt32LittleEndian(record[0xa4..], 0x80000000);
        Assert.True(scene.Queries.WaitingRoom()); Assert.Equal(new[] { "challenge", "known", "local", "challenge", "known", "local" }, scene.Events);
    }

    /// <summary>소유자 getter 뒤 현재CS를 재조회하며 이미 빌린 옛 참가자 사본을 읽지 않는다.</summary>
    [Fact]
    public void OwnerGetterCanReplaceCurrentCsBeforeParticipantRead()
    {
        var scene = new Scene { Challenge = true, Known = 1 }; Assert.True(scene.Cs.Prepare()); uint old = scene.Cs.Current;
        Memory<byte> oldRecord = scene.Cs.Participant(1); BinaryPrimitives.WriteUInt32LittleEndian(oldRecord.Span[0xa4..], 0);
        scene.OnLocal = () => { scene.Cs.Current = 0; Assert.True(scene.Cs.Prepare()); BinaryPrimitives.WriteUInt32LittleEndian(scene.Cs.Participant(1).Span[0xa4..], 9); };
        Assert.True(scene.Queries.WaitingRoom()); Assert.NotEqual(old, scene.Cs.Current);
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(oldRecord.Span[0xa4..]));
    }

    /// <summary>선택된 미등록CS는 성공false로 숨기지 않고 원문 역참조에 해당하는 호스트 경계를 드러낸다.</summary>
    [Fact]
    public void SelectedMissingCsAndOwnerFailureKeepObservedPrefix()
    {
        var scene = new Scene { Challenge = true, Known = 1 };
        Assert.Throws<InvalidOperationException>(() => scene.Queries.WaitingRoom()); Assert.Equal(new[] { "challenge", "known", "local" }, scene.Events);
        scene.Events.Clear(); var failure = new IOException("owner"); scene.OnLocal = () => throw failure;
        Assert.Same(failure, Assert.Throws<IOException>(() => scene.Queries.WaitingRoom())); Assert.Equal(new[] { "challenge", "known", "local" }, scene.Events);
    }

    /// <summary>실제Surface 대여만으로 시각/CS준비/적재/목록정리가 일어나지 않고 같은 Player/CS의 다음 변경을 읽는다.</summary>
    [Fact]
    public void RealSurfaceBindingIsQuietAndReadsSameCurrentPlayerAndCs()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new());
        var surface = new ClientSurfaceGlobals(game, new(local), new(), () => throw new IOException("현재시각소비금지"));
        var cs = new ClientChallengeGlobals(); uint before = game.Player.BattleOwnerBox.Value;
        SceneMusicWorldQueries queries = SceneMusicWorldQueries.ForCurrentSurface(surface, cs, _ => throw new IOException("미선택가상"));
        Assert.Equal(before, game.Player.BattleOwnerBox.Value); Assert.Equal(0u, cs.Current); Assert.Empty(surface.Storage.Types);
        Assert.Equal(0u, surface.Storage.SidLists.Priests.Count); Assert.False(queries.WaitingRoom());
        surface.Storage.OwnerMode.Challenge = true; game.Player.PlayerKnown = 1; game.Player.BattleOwnerBox.Value = 3;
        Assert.True(cs.Prepare()); BinaryPrimitives.WriteUInt32LittleEndian(cs.Participant(3).Span[0xa4..], 1);
        Assert.True(queries.WaitingRoom()); game.Player.PlayerKnown = 0; Assert.False(queries.WaitingRoom());
    }
}
