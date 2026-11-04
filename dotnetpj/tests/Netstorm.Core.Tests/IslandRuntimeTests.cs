using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 실제 두 판본의 mutable 100기록·owner·Player 첫 지역과 장치 현 목록을 같은 raw 풀·해시에서 비교한다
/// (cpppj <c>tests/IslandRuntimeTests.cpp</c>를 직역했다).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 원본 x86 fixture로 진입점 전체를 관통하므로 PORT_MAP 상태는 L1이다.</remarks>
public sealed class IslandRuntimeTests
{
    /// <summary>원본 독립 실행기의 풀 크기다.</summary>
    private const int Capacity = 24000;

    /// <summary>실제 원본 실행기에 준 float 비트의 거주 구역 현재 좌표 입력이다.</summary>
    private static readonly uint[][] BoundsInputs =
    [
        [0x41200000, 0x41a00000, 0x42700000, 0x428c0000], [0, 0, 0x42700000, 0x428c0000],
        [0xc0000000, 0x41a00000, 0x42700000, 0x428c0000], [0x42480000, 0x42540000, 0x42480000, 0x42540000],
        [0x7fc00001, 0x41a00000, 0x42700000, 0x428c0000], [0x41200000, 0x7fc00001, 0x42700000, 0x428c0000],
        [0x41200000, 0x41a00000, 0x42700000, 0x7fc00001], [0x41200000, 0x41a00000, 0x7fc00001, 0x428c0000],
        [0x41200000, 0x41a00000, 0x43800000, 0x428c0000],
    ];

    /// <summary>실제 원본 실행기에 준 거주 구역 좌표다.</summary>
    private static readonly float[][] PointInputs = [[10, 20], [50.75f, 52.5f], [60, 70], [61, 71]];

    /// <summary>원본 little endian 바이트 필드를 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 버퍼다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    /// <returns>결합한 값이다.</returns>
    private static uint Get(byte[] raw, int offset, int width = 4)
    {
        uint value = 0;
        // 낮은 바이트부터 결합한다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>원본 little endian raw 필드를 쓰고 주변 바이트는 보존한다.</summary>
    /// <param name="raw">쓸 버퍼다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(byte[] raw, int offset, uint value, int width = 4)
    {
        // 원본 필드 폭의 낮은 바이트만 쓴다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>원본 little endian raw 필드를 슬롯 범위에 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 원본 필드 폭의 낮은 바이트만 쓴다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>슬롯 전체를 원본 관측의 소문자 16진수로 표현한다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <returns>16진수 문자열이다.</returns>
    private static string Hex(byte[] bytes)
    {
        char[] digits = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'];
        char[] text = new char[bytes.Length * 2];
        // 바이트마다 두 글자를 둔다
        for (int index = 0; index < bytes.Length; index++)
        {
            text[index * 2] = digits[bytes[index] >> 4];
            text[index * 2 + 1] = digits[bytes[index] & 15];
        }
        return new string(text);
    }

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    /// <returns>원본 DWORD 값이다.</returns>
    private static uint Number(string value)
    {
        return uint.Parse(value, System.Globalization.CultureInfo.InvariantCulture);
    }

    /// <summary>같은 raw pool·해시·record에 기존 모드·Player·등록 목록을 두는 입력 화면이다.</summary>
    private sealed class IslandRuntimeScene
    {
        /// <summary>현재 거주 구역과 표면이 읽는 같은 raw 풀이다.</summary>
        public readonly RawSidPool Pool;

        /// <summary>기존 표면 조회가 사용하는 현재 hash0다.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>CoreData와 같은 3204바이트 mutable 목록이다.</summary>
        public readonly byte[] Records = new byte[RawIslandStorage.Bytes];

        /// <summary>기존 fort·special·chaff·Player 첫 지역을 공유한다.</summary>
        public FortFinishOwnerState Modes = new();

        /// <summary>실제 AssetPostPop의 등록 목록이다.</summary>
        public readonly RawPostPopList Residences = new();

        /// <summary>원본 위치의 독립 전역이다.</summary>
        public uint Revision;

        /// <summary>원본 위치의 독립 변경 값이다.</summary>
        public uint Changed;

        /// <summary>실행기에 전달한 명시 입력이다.</summary>
        public readonly List<uint> Input = [];

        /// <summary>원본 assert·거주 구역 가상 owner의 효과 순서다.</summary>
        public readonly List<string> Events = [];

        /// <summary>원본 진단 장치 호출 횟수다.</summary>
        public uint Assertions;

        /// <summary>현재 raw owner 바이트의 원본 배치다.</summary>
        public readonly bool Patch;

        /// <summary>같은 단일 자료에서 실행하는 복원 몸체다.</summary>
        public RawIslandRuntime? Runtime;

        /// <summary>공유 모드에서 원본이 지역 DWORD를 직접 공유하는 실제 9개 Player 바이트다.</summary>
        public byte[] Players = [];

        /// <summary>원본 입력 SID를 미리 독립 장치로 확보한다.</summary>
        /// <param name="edition">원본 판본이다.</param>
        public IslandRuntimeScene(OriginalEdition edition)
        {
            Patch = edition == OriginalEdition.Patch1078;
            Pool = new RawSidPool(Capacity, false, null, edition);
            // 거주 구역·표면을 명시 공간 번호로 모두 확보한다
            for (int sid = 5; sid <= 127; sid++)
            {
                Assert.Equal(sid, Pool.Allocate((SidAllocation)2));
            }
        }

        /// <summary>가상 owner의 실제 raw 효과와 장치 직후 현재 목록·모드·Player 변화를 관찰한다.</summary>
        /// <returns>연결할 훅 묶음이다.</returns>
        public IslandRuntimeHooks Hooks()
        {
            IslandRuntimeHooks hooks = new();
            hooks.Assertion = reason =>
            {
                Assertions++;
                Events.Add(reason == IslandRuntimeAssertion.InvalidBounds ? "A:B" : "A:E");
            };
            hooks.SetResidenceOwner = (sid, owner) =>
            {
                Pool.AllocatedBytes(sid)[Patch ? 34 : 32] = (byte)owner;
                Events.Add("O:" + sid + ':' + owner);
                if (sid != 60)
                {
                    return;
                }
                if (Input[14] == 1)
                {
                    Residences.Entries[1] = 63;
                }
                if (Input[14] == 2)
                {
                    Residences.Count = 1;
                }
                if (Input[14] == 3)
                {
                    Hash.Cell(0, 51, 53) = 72;
                }
                if (Input[14] == 4)
                {
                    Modes.Fort = true;
                    Modes.Special = false;
                }
                if (Input[14] == 6)
                {
                    if (Players.Length == 0)
                    {
                        Modes.PlayerRegions[(int)Input[6]] = 5;
                    }
                    else
                    {
                        Put(Players, (int)(Input[6] * (Patch ? 0xb4u : 0xacu) + (Patch ? 0x78u : 0x70u)), 5);
                    }
                }
            };
            return hooks;
        }

        /// <summary>기대값을 계산하지 않고 입력 record와 실제 필드 종류 owner와 기존 목록을 구성한다.</summary>
        /// <param name="row">원본 행이다.</param>
        public void Prepare(string[] row)
        {
            Input.Clear();
            Events.Clear();
            Players = [];
            Assertions = 0;
            Hash.Reset();
            Modes = new FortFinishOwnerState();
            // 원본 DWORD 비트를 그대로 읽는다
            foreach (string number in row[2].Split(','))
            {
                Input.Add(Number(number));
            }
            Assert.Equal(15, Input.Count);
            Array.Fill(Records, (byte)Input[6]);
            Put(Records, 0, Input[0]);
            // 100개 모두에 원본 record·타입·data 입력을 배치한다
            for (int index = 0; index < 100; index++)
            {
                int record = 4 + index * 32;
                Put(Records, record, unchecked(0x11110000u + (uint)index));
                Put(Records, record + 4, index < Input[0] ? 1u : 0u);
                Put(Records, record + 8, Input[3]);
                Put(Records, record + 12, Input[5], 1);
                Put(Records, record + 13, Input[4], 1);
                // 네 float는 실행기에 전달한 비트를 그대로 유지한다
                for (int field = 0; field < 4; field++)
                {
                    Put(Records, record + 16 + field * 4, BoundsInputs[(int)Input[13]][field]);
                }
            }
            if (Input[1] < 100)
            {
                Put(Records, 4 + (int)Input[1] * 32 + 4, Input[2]);
            }
            Modes.Fort = Input[10] != 0;
            Modes.Special = Input[8] != 0;
            Modes.ChaffMode = Input[9] != 0;
            Revision = Input[11];
            Changed = Input[12];
            Residences.Entries = [60, 61, 62, 63];
            Residences.Count = 4;
            // 풀을 복사하지 않고 raw에 이전 입력을 지우고 명시 공간을 초기화한다
            for (int sid = 5; sid <= 127; sid++)
            {
                Span<byte> raw = Pool.AllocatedBytes(sid);
                raw.Fill(0);
            }
            // 실제 pool·해시에 기존 표면 도우미와 같은 타입·float·표면 owner 입력을 전달한다
            for (int index = 0; index < 4; index++)
            {
                int sid = 60 + index;
                Span<byte> raw = Pool.AllocatedBytes(sid);
                raw[10] = 89;
                raw[Patch ? 34 : 32] = 7;
                Put(raw, 14, BitConverter.SingleToUInt32Bits(PointInputs[index][0]));
                Put(raw, 18, BitConverter.SingleToUInt32Bits(PointInputs[index][1]));
                int support = 70 + Math.Min(index, 2);
                Pool.AllocatedBytes(support)[Patch ? 34 : 32] = new byte[] { 2, 3, 8 }[Math.Min(index, 2)];
                int x = (int)((double)PointInputs[index][0] + (double)0.9999f);
                int y = (int)((double)PointInputs[index][1] + (double)0.9999f);
                Hash.Cell(0, x, y) = (ushort)support;
            }
            Runtime = new RawIslandRuntime(Pool, Hash, Records, Modes, Residences,
                () => Revision, value => Revision = value,
                () => Changed, value => Changed = value, Hooks());
        }
    }

    /// <summary>원본 scalar·전체 3204 raw·출력 범위·공유 지역·등록 목록·가상 owner를 한 행씩 비교한다.</summary>
    /// <param name="edition">원본 판본 이름이다.</param>
    /// <param name="sharedPlayers">참이면 실제 Player 바이트를 공유 모드로 재생한다.</param>
    private static void ReplayIslandRuntime(string edition, bool sharedPlayers = false)
    {
        IslandRuntimeScene scene = new(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        int count = 0;
        // 각 독립 실행기 입력을 복원 몸체에 전달한다
        foreach (string[] row in Netstorm.Tests.X86Fixture.Read("islandruntime"))
        {
            if (row[0] != edition)
            {
                continue;
            }
            Assert.Equal(15, row.Length);
            scene.Prepare(row);
            int island = unchecked((int)scene.Input[1]);
            uint result = 0;
            string bounds = "-";
            if (sharedPlayers)
            {
                // 원본 Player 첫 지역 DWORD의 위치다
                uint stride = scene.Patch ? 0xb4u : 0xacu;
                uint field = scene.Patch ? 0x78u : 0x70u;
                scene.Players = new byte[9 * stride];
                Array.Fill(scene.Players, (byte)0xa5);
                // 기대값을 기존 입력 상태와 흡착 지역 필드로 준비한다
                for (int owner = 0; owner < 9; owner++)
                {
                    Put(scene.Players, (int)(owner * stride + field), unchecked((uint)scene.Modes.PlayerRegions[owner]));
                }
                scene.Runtime!.BindPlayerRecords(scene.Players);
                Array.Fill(scene.Modes.PlayerRegions, 999);
            }
            string kind = row[1];
            RawIslandRuntime runtime = scene.Runtime!;
            if (kind == "Reset")
            {
                runtime.Reset();
            }
            else if (kind == "Count")
            {
                result = runtime.Count();
            }
            else if (kind == "Exists")
            {
                result = runtime.Exists(island);
            }
            else if (kind == "Add")
            {
                result = runtime.Add(scene.Input[6], (byte)scene.Input[5]);
            }
            else if (kind == "Delete")
            {
                runtime.Delete(island);
            }
            else if (kind == "OwnerGet")
            {
                result = runtime.Owner(island);
            }
            else if (kind == "OwnerSet")
            {
                runtime.Owner(island, scene.Input[6]);
            }
            else if (kind == "TempleGet")
            {
                result = runtime.Temple(island);
            }
            else if (kind == "TempleSet")
            {
                runtime.Temple(island, scene.Input[6]);
            }
            else if (kind == "Match")
            {
                result = runtime.Matches(island, unchecked((int)scene.Input[6]), unchecked((int)scene.Input[7])) ? 1u : 0u;
            }
            else if (kind == "GetBounds")
            {
                FortRandomRectangle value = runtime.Bounds(island);
                byte[] raw = new byte[16];
                float[] fields = [value.Left, value.Top, value.Right, value.Bottom];
                // 실제가 반환한 float 비트를 읽어 비교한다
                for (int field = 0; field < 4; field++)
                {
                    Put(raw, field * 4, BitConverter.SingleToUInt32Bits(fields[field]));
                }
                bounds = Hex(raw);
            }
            else if (kind == "SetBounds")
            {
                uint[] value = BoundsInputs[(int)scene.Input[13]];
                runtime.Bounds(island, new FortRandomRectangle(
                    BitConverter.UInt32BitsToSingle(value[0]),
                    BitConverter.UInt32BitsToSingle(value[1]),
                    BitConverter.UInt32BitsToSingle(value[2]),
                    BitConverter.UInt32BitsToSingle(value[3])));
            }
            else
            {
                Assert.Fail("알 수 없는 섬 목록 종류: " + kind);
            }
            string players = "";
            // 현재 Player 첫 지역 DWORD를 읽는다
            for (int owner = 0; owner < 9; owner++)
            {
                if (owner != 0)
                {
                    players += ',';
                }
                players += sharedPlayers
                    ? Get(scene.Players, (int)(owner * (scene.Patch ? 0xb4u : 0xacu) + (scene.Patch ? 0x78u : 0x70u)))
                    : unchecked((uint)scene.Modes.PlayerRegions[owner]);
            }
            if (sharedPlayers)
            {
                // 첫 지역 네 바이트 밖의 Player 바이트는 그대로 유지되어야 한다
                for (int index = 0; index < scene.Players.Length; index++)
                {
                    int offset = index % (int)(scene.Patch ? 0xb4u : 0xacu);
                    int field = scene.Patch ? 0x78 : 0x70;
                    if (offset < field || offset >= field + 4)
                    {
                        Assert.Equal(0xa5, scene.Players[index]);
                    }
                }
                Assert.All(scene.Modes.PlayerRegions, value => Assert.Equal(999, value));
            }
            // 장치에 포함된 실제 효과 순서를 모은다
            string effects = "";
            foreach (string effect in scene.Events)
            {
                if (effects.Length != 0)
                {
                    effects += ';';
                }
                effects += effect;
            }
            if (effects.Length == 0)
            {
                effects = "-";
            }
            // 네 목록의 현재 등록 SID와 실제 owner 바이트를 비교한다
            string entries = "";
            string owners = "";
            for (int index = 0; index < 4; index++)
            {
                if (index != 0)
                {
                    entries += ',';
                    owners += ',';
                }
                entries += scene.Residences.Entries[index];
                owners += scene.Pool.Slot((int)(60 + index))[scene.Patch ? 34 : 32];
            }
            string modes = (scene.Modes.Fort ? 1 : 0) + "," + (scene.Modes.Special ? 1 : 0) + "," + (scene.Modes.ChaffMode ? 1 : 0);
            bool same = result == Number(row[3]) && Hex(scene.Records) == row[4] && bounds == row[5]
                && scene.Revision == Number(row[6]) && scene.Changed == Number(row[7]) && players == row[8] && effects == row[9]
                && scene.Residences.Count == Number(row[10]) && entries == row[11] && owners == row[12] && modes == row[13]
                && scene.Assertions == Number(row[14]);
            Assert.True(same, $"{edition} 섬 목록 {kind} {row[2]}");
            count++;
        }
        Assert.Equal(1009, count);
    }

    /// <summary>패치의 원본 owner·현재 owner와 100기록의 필드별 초기화·진단을 실제 출력과 비교한다.</summary>
    [Fact]
    public void ReplaysPatch()
    {
        ReplayIslandRuntime("originals");
    }

    /// <summary>CD가 조회한 getter·setter와 범위 순서 없음·owner 파괴 차이를 실제 출력과 비교한다.</summary>
    [Fact]
    public void ReplaysCd()
    {
        ReplayIslandRuntime("originalCD");
    }

    /// <summary>기존 위치 원본 1009입력을 실제 Player 바이트 공유 뒤 현재값으로 재생한다.</summary>
    [Fact]
    public void PatchSharedPlayerRecords()
    {
        ReplayIslandRuntime("originals", true);
    }

    /// <summary>CD 첫 지역 위치 주변 바이트를 같은 기존 원본 1009입력으로 검증한다.</summary>
    [Fact]
    public void CdSharedPlayerRecords()
    {
        ReplayIslandRuntime("originalCD", true);
    }

    /// <summary>기존 두 판본 실행기 Reset 6개의 전체 3204바이트를 세션 없는 같은 공통 도우미와 비교한다.</summary>
    [Fact]
    public void DataOnlyResetMatchesExistingNativeBothEditions()
    {
        int count = 0;
        // 각 기대값이나 실행기 상태를 바꾸지 않는다
        foreach (string[] row in Netstorm.Tests.X86Fixture.Read("islandruntime"))
        {
            if (row[1] != "Reset")
            {
                continue;
            }
            IslandRuntimeScene scene = new(row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            scene.Prepare(row);
            uint revision = scene.Revision;
            uint changed = scene.Changed;
            uint[] entries = (uint[])scene.Residences.Entries.Clone();
            RawIslandRuntime.ResetRawIslandRecords(scene.Records);
            Assert.Equal(row[4], Hex(scene.Records));
            Assert.Equal(revision, scene.Revision);
            Assert.Equal(changed, scene.Changed);
            Assert.Equal(entries, scene.Residences.Entries);
            Assert.Empty(scene.Events);
            count++;
        }
        Assert.Equal(6, count);
    }

    /// <summary>별도 3204바이트 쓰기 배열을 실제 실행기가 계속 빌고 쓰며 잘못된 도우미 크기는 쓰기 전에 거부한다.</summary>
    [Fact]
    public void DataOnlyStorageAliasesRuntimeAndRejectsWrongWidth()
    {
        RawIslandStorage storage = new();
        Array.Fill(storage.Records, (byte)0xa5);
        IslandRuntimeScene scene = new(OriginalEdition.Patch1078);
        RawIslandRuntime runtime = new(scene.Pool, scene.Hash, storage.Records, scene.Modes, scene.Residences,
            () => scene.Revision, value => scene.Revision = value,
            () => scene.Changed, value => scene.Changed = value, scene.Hooks());
        RawIslandRuntime.ResetRawIslandRecords(storage.Records);
        Assert.Equal(0u, runtime.Count());
        Assert.Equal(0xa5, storage.Records[18]);
        Assert.Equal(0xa5, storage.Records[19]);
        Assert.Equal(0u, runtime.Add(3, 7));
        Assert.Equal(1u, Get(storage.Records, 0));
        Assert.Equal(3u, Get(storage.Records, 12));
        Assert.Equal(7, storage.Records[16]);
        Assert.Equal(3u, storage.Records[17]);
        Put(storage.Records, 4, 0x12345678u);
        Assert.Equal(0x12345678u, runtime.Temple(0));
        runtime.Reset();
        Assert.Equal(0u, Get(storage.Records, 0));
        Assert.Equal(0u, Get(storage.Records, 4));
        Assert.Equal(0xa5, storage.Records[18]);
        Assert.Equal(0xa5, storage.Records[19]);
        byte[] before = (byte[])storage.Records.Clone();
        bool rejected = false;
        // 범위가 크거나 모자란 입력에서 부분 초기화를 막는다
        try
        {
            byte[] narrow = new byte[RawIslandStorage.Bytes - 1];
            Array.Copy(storage.Records, narrow, narrow.Length);
            RawIslandRuntime.ResetRawIslandRecords(narrow);
        }
        catch (ArgumentException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(before, storage.Records);
    }

    /// <summary>물리 100기록 경계·필수 효과를 원본의 정상 반환으로 꾸미지 않고 명시적으로 거부한다.</summary>
    [Fact]
    public void RejectsMissingEffectsAndPhysicalOverflow()
    {
        string[] first = Netstorm.Tests.X86Fixture.Read("islandruntime")[0];
        IslandRuntimeScene scene = new(first[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        scene.Prepare(first);
        Put(scene.Records, 0, 100);
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Runtime!.Add(1, 2));
        Assert.Equal(100u, scene.Runtime!.Count());
        Assert.Throws<ArgumentException>(() => new RawIslandRuntime(scene.Pool, scene.Hash, scene.Records, scene.Modes,
            scene.Residences, () => scene.Revision, value => scene.Revision = value,
            () => scene.Changed, value => scene.Changed = value, new IslandRuntimeHooks()));
    }

    /// <summary>실제 0046ef10의 059a8b0 증가와 005949d4 쓰기를 그대로 둔 실제 소유자에서 관측한다.</summary>
    [Fact]
    public void ActualClientOwnersKeepWorldIncrementAndMapStoreSeparate()
    {
        // 실제 게임 화면의 지도 다시 그리기와 지도 개정 공유 값이다
        uint worldRedraw = 0;
        uint mapRevision = 0;
        IslandRuntimeScene scene = new(OriginalEdition.Patch1078);
        Span<byte> residence = scene.Pool.AllocatedBytes(60);
        residence.Fill(0);
        residence[10] = 89;
        residence[34] = 7;
        Put(residence, 14, BitConverter.SingleToUInt32Bits(50.75f));
        Put(residence, 18, BitConverter.SingleToUInt32Bits(52.5f));
        scene.Pool.AllocatedBytes(70)[34] = 2;
        scene.Hash.Cell(0, 51, 53) = 70;
        scene.Residences.Entries = [60];
        scene.Residences.Count = 1;
        // 실제 거주 구역의 관측용 관측 입력이다
        uint notices = 0;
        uint assertions = 0;
        uint expectedWorld = 18;
        uint expectedOwner = 3;
        bool mutateDuringNotice = false;
        IslandRuntimeHooks hooks = new();
        hooks.Assertion = reason =>
        {
            Assert.Equal(IslandRuntimeAssertion.MissingIsland, reason);
            assertions++;
            // 복구 진단은 같은 존재·지역을 바꾸는 원문 뒤 현재값에서 계속한다
            worldRedraw = 40;
            mapRevision = 0x76543210u;
            Put(scene.Records, 8, 1);
        };
        hooks.SetResidenceOwner = (sid, owner) =>
        {
            Assert.Equal(60, sid);
            Assert.Equal(2u, owner);
            Assert.Equal(expectedWorld, worldRedraw);
            Assert.Equal(1u, mapRevision);
            Assert.Equal(expectedOwner, Get(scene.Records, 12));
            notices++;
            scene.Pool.AllocatedBytes(sid)[34] = (byte)owner;
            // 원문 지역 읽기 뒤의 실제 가상 효과는 반환 뒤 원본대로 다시 읽는다
            if (mutateDuringNotice)
            {
                worldRedraw = 700;
                mapRevision = 99;
            }
        };
        RawIslandRuntime runtime = new(scene.Pool, scene.Hash, scene.Records, scene.Modes, scene.Residences,
            () => worldRedraw, value => worldRedraw = value,
            () => mapRevision, value => mapRevision = value, hooks);
        runtime.Reset();
        Assert.Equal(0u, runtime.Add(1, 0));
        runtime.Bounds(0, new FortRandomRectangle(10, 20, 60, 70));
        worldRedraw = 17;
        mapRevision = 0xffffffffu;
        runtime.Owner(0, 3);
        Assert.Equal(18u, worldRedraw);
        Assert.Equal(1u, mapRevision);
        Assert.Equal(3u, runtime.Owner(0));
        Assert.Equal(2, scene.Pool.Slot(60)[34]);
        Assert.Equal(1u, notices);
        Assert.Equal(0u, assertions);
        // 진단 뒤 현재 존재·지역 변경을 참조한 뒤 같은 1 쓰기를 먼저 실행하고 가상 효과는 계속 변경을 잇는다
        Put(scene.Records, 8, 0);
        worldRedraw = 17;
        mapRevision = 0xffffffffu;
        expectedWorld = 41;
        expectedOwner = 4;
        mutateDuringNotice = true;
        runtime.Owner(0, 4);
        Assert.Equal(700u, worldRedraw);
        Assert.Equal(99u, mapRevision);
        Assert.Equal(4u, runtime.Owner(0));
        Assert.Equal(2u, notices);
        Assert.Equal(1u, assertions);
        // 부호 없는 감소와 증가는 통계 DWORD에만 사용하며 미니맵 DWORD는 이전 값을 직접 유지한다
        worldRedraw = 0xffffffffu;
        mapRevision = 0xffffffffu;
        expectedWorld = 0;
        expectedOwner = 5;
        mutateDuringNotice = false;
        runtime.Owner(0, 5);
        Assert.Equal(0u, worldRedraw);
        Assert.Equal(1u, mapRevision);
        Assert.Equal(5u, runtime.Owner(0));
        Assert.Equal(3u, notices);
        Assert.Equal(1u, assertions);
    }
}
