using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 자산 후처리(<see cref="RawAssetPostPop"/>)를 원본 x86 기대값에 대조한다.
/// 기대값은 assetpostpop-x86.tsv(전용 몸체, 판본별 818회) 중 10.78 몫이다.
/// 각 호출 뒤 효과 순서·raw 슬롯·목록·Player 신전·주변 받침을 비교한다.
/// 열 구성은 cpppj/tests/AssetPostPopTests.cpp 와 같다.
/// CD 10.72 행은 기준 판본이 아니므로 읽지 않는다 (W02 Asset 조각, 2026-10-20).
/// </summary>
public sealed class X86RawAssetPostPopTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 전용 몸체 가상 표 12개. fixture 1열이 가리키는 번호 순서다.</summary>
    private static readonly uint[] AssetTables =
    [
        0x00501ea0, 0x0050b6e8, 0x0050b7c0, 0x0050b9b8, 0x0050b888, 0x0050b570,
        0x0050f540, 0x0050e940, 0x0050da88, 0x005196f0, 0x00519860, 0x00519918,
    ];

    /// <summary>검사 대상 번호. cpppj RawSceneSupport 의 kSource 와 같다.</summary>
    private const int Source = 50;

    /// <summary>값을 리틀 엔디언으로 쓴다. 폭 밖의 이웃 필드는 보존한다.</summary>
    /// <param name="bytes">대상 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>정렬되지 않은 리틀 엔디언 값을 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수.</param>
    private static uint Get(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }

    /// <summary>한 행의 입력으로 풀·타입·상태·효과 기록기를 준비한다.</summary>
    /// <param name="row">fixture 행(21칸).</param>
    /// <param name="pool">슬롯 풀(5~127번 확보).</param>
    /// <param name="types">타입 표(70번만 채운다).</param>
    /// <param name="book">공통 장부(억제 켜짐).</param>
    /// <param name="carrierState">운반체 상태(보스 켜짐).</param>
    /// <param name="state">자산 상태(행 값으로 채운다).</param>
    /// <param name="events">효과 순서 기록.</param>
    /// <param name="validations">공통 검사 호출 수(길이 1).</param>
    /// <param name="share">예약 할당량 기록(길이 1, allocated 역할).</param>
    /// <param name="reserveMask">예약 마스크(길이 1).</param>
    /// <param name="contentExists">내용물 존재(길이 1).</param>
    private sealed class Scene
    {
        /// <summary>슬롯 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>타입 표.</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>공통 장부.</summary>
        public readonly RawPostPopState Book = new() { Suppressed = true };

        /// <summary>운반체 상태.</summary>
        public readonly CarrierPostPopState CarrierState = new() { Boss = true };

        /// <summary>자산 상태.</summary>
        public readonly AssetPostPopState State = new();

        /// <summary>효과 순서 기록.</summary>
        public readonly List<string> Events = [];

        /// <summary>공통 검사 호출 수.</summary>
        public int Validations;

        /// <summary>예약 할당 횟수.</summary>
        public uint Allocated;

        /// <summary>예약 마스크.</summary>
        public uint ReserveMask;

        /// <summary>내용물 존재 여부.</summary>
        public uint ContentExists;

        /// <summary>풀·이웃·선택 폼을 확보하고 타입 70번을 채운다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, false);
            // 원본 관찰의 번호 범위를 그대로 확보한다
            for (int id = 5; id <= 127; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 모든 타입 번호에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
        }

        /// <summary>한 행의 입력으로 상태를 채운다.</summary>
        /// <param name="row">fixture 행.</param>
        public void Prepare(string[] row)
        {
            Events.Clear();
            Validations = 0;
            Allocated = 0;
            State.InBattle = row[4] != "0";
            State.LoadingDepth = X86Fixture.UInt(row[5]);
            State.FortMode = State.LoadingDepth != 0;
            State.Boss = row[6] != "0";
            ReserveMask = X86Fixture.UInt(row[7]);
            State.ResidenceAnimation = row[8] != "0";
            State.TempleCount = 4;
            State.Selected = row[12] != "0" ? 62u : 0u;
            ContentExists = X86Fixture.UInt(row[13]);
            // 목록은 자리 4칸·과거 값 7~10으로 두고 개수만 행 값으로 둔다
            foreach (RawPostPopList list in new[] { State.Residences, State.Temples, State.Geysers })
            {
                list.Entries = new uint[] { 7, 8, 9, 10 };
                list.Count = X86Fixture.UInt(row[14]);
            }
            Types[70].Flags1 = X86Fixture.UInt(row[10]);
            Types[70].Flags2 = 0x200;
            Types[70].Cost = float.Parse(row[11], System.Globalization.CultureInfo.InvariantCulture);
            Span<byte> raw = Pool.AllocatedBytes(Source);
            raw.Clear();
            Put(raw, 0, AssetTables[X86Fixture.UInt(row[1])]);
            raw[RawSidPool.TypeOffset] = 70;
            raw[RawSidPool.StateOffset] = 0;
            raw[OwnerOffset] = 1;
            raw[ExtraOffset] = (byte)X86Fixture.UInt(row[3]);
            Put(raw, 12, 0xabcd, 2);
            Put(raw, 14, 0x42480000);
            Put(raw, 18, 0x42500000);
            Put(raw, WordOffset, X86Fixture.UInt(row[9]));
            // 기계어의 지면·종유석 이웃 폼 번호를 확보한다
            foreach (int sid in new[] { 60, 61 })
            {
                Span<byte> neighbor = Pool.AllocatedBytes(sid);
                neighbor.Clear();
                neighbor[RawSidPool.TypeOffset] = (byte)(sid == 60 ? 94 : 95);
                neighbor[OwnerOffset] = 2;
            }
            // 보스이면 61번을 할당 자산으로 덮어쓴다
            if (State.Boss)
            {
                Span<byte> other = Pool.AllocatedBytes(61);
                other[RawSidPool.TypeOffset] = 70;
                other[ExtraOffset] = (byte)(ContentExists * 8);
            }
            // 선택 폼(62번)은 타입 70번을 18번 위치에 둔다
            Span<byte> selected = Pool.AllocatedBytes(62);
            selected.Clear();
            Put(selected, 18, 70, 2);
        }

        /// <summary>소유자 바이트 위치(10.78).</summary>
        private const int OwnerOffset = 34;

        /// <summary>추가 비트 바이트 위치(10.78).</summary>
        private const int ExtraOffset = 40;

        /// <summary>프레임 DWORD 위치(10.78).</summary>
        private const int WordOffset = 36;

        /// <summary>직접 Damageable 몸체와 같은 외부 효과로 관찰하는 연결 지점.</summary>
        /// <param name="base">공통 몸체.</param>
        public CarrierPostPopHooks CarrierHooks(RawPostPop @base) => new()
        {
            CarrierCheck = sid => throw new InvalidOperationException("이번 자산 관찰은 Carrier 확인을 호출하지 않습니다"),
            ClaimGround = (x, y, owner) => throw new InvalidOperationException("이번 관찰의 Damageable 은 지면 경계가 아닙니다"),
            ValidateBase = (sid, flags) => { Assert.Equal(Source, sid); Validations++; },
            Base = (sid, flags) =>
            {
                Assert.Equal(1, Validations);
                Events.Add($"D:{flags}");
                @base.PostPopBase(sid, flags);
            },
        };

        /// <summary>원본과 같은 효과·호출 규약으로 입력·raw 변경을 공급하는 연결 지점.</summary>
        public AssetPostPopHooks Hooks() => new()
        {
            ReserveRegular = () =>
            {
                Events.Add($"A:40");
                return (ReserveMask & (1u << (int)(Allocated++ % 3))) != 0;
            },
            AddRegular = (sid, @event, delay) => Events.Add($"R:{sid}:{@event}:{BitConverter.SingleToUInt32Bits(delay)}"),
            CreateGunProcess = sid =>
            {
                // 확보(56)가 성공할 때만 생성 효과를 낸다
                Events.Add($"A:56");
                if ((ReserveMask & (1u << (int)(Allocated++ % 3))) != 0)
                {
                    Events.Add($"P:{sid}");
                }
            },
            RemoveAnimation = sid => Events.Add($"M:{sid}"),
            CheckForms = sid => { },
            GunFrame = angle => { Events.Add($"F:{angle}"); return 2; },
            SetGraph = (sid, graph) => { Events.Add($"Q:{graph}"); Pool.AllocatedBytes(sid)[30] = (byte)graph; },
            Surface = (x, y) => 60,
            SetOwner = (sid, owner) => { Events.Add($"O:{sid}:{owner}"); Pool.AllocatedBytes(sid)[34] = (byte)owner; },
            NotifySurface = sid => Events.Add($"N:{sid}"),
            StartBlocker = sid => Events.Add($"K:{sid}"),
            GeyserRegistration = (sid, value) => Events.Add($"G:{sid}:{value}"),
            AtPosition = (x, y) => new[] { 60, 61 },
            Unpop = (sid, flags) => Events.Add($"U:{sid}:{flags}"),
            Repop = (sid, flags) => Events.Add($"V:{sid}:{flags}"),
            TypeDword = (sid, offset) => offset == 0x74 ? 17u : offset == 0x130 ? 27u : 39u,
            PlayImpactSound = value => Events.Add($"E:{value}"),
            ClearPlacement = () => Events.Add("C"),
            IslandNumber = (x, y) => 3,
            IslandExists = island => true,
            IslandTemple = island => 0,
            RegisterTemple = (island, sid) => Events.Add($"I:{island}:{sid}"),
            RegionNumber = (x, y) => 3,
            Destroy = (sid, flags) => { Events.Add($"X:{sid}:{flags}"); Pool.AllocatedBytes(sid)[RawSidPool.StateOffset] |= 2; },
            HasContent = (sid, slot) => ContentExists != 0,
            CreateSlot = (sid, slot) => Events.Add($"S:{sid}"),
            RefreshProduction = () => Events.Add("T"),
        };

        /// <summary>현재 관찰 결과를 기계어 fixture 형식으로 직렬화한다.</summary>
        public string[] Output()
        {
            string actions = string.Join(';', Events);
            string lists = string.Join('|', new[] { State.Residences, State.Temples, State.Geysers }
                .Select(list => $"{list.Count}:{string.Join(',', list.Entries)}"));
            AssetPostPopPlayer player = State.Players[1];
            string players = $"{player.InitialTemple},{player.InitialTempleCopy},{player.NewTemple},{player.NewTempleCopy}";
            string neighbors = $"{Get(Pool.Slot(60), WordOffset)}|{Get(Pool.Slot(61), WordOffset)}";
            return [actions, Convert.ToHexStringLower(Pool.Slot(Source)), lists, players, State.TempleCount.ToString(), neighbors];
        }
    }

    /// <summary>
    /// 자산 전용 몸체 fixture 의 10.78 몫 818회를 재생한다.
    /// Activate(공통 깊이 증가·공통 몸체 감소) 순서와 같은 깊이 보존을 확인한다.
    /// </summary>
    [Fact]
    public void AssetBodiesMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("assetpostpop");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            Assert.Equal(21, row.Length);
            var scene = new Scene();
            scene.Prepare(row);
            var @base = new RawPostPop(scene.Pool, scene.Types, scene.Book);
            var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, scene.CarrierState, scene.CarrierHooks(@base));
            var assets = new RawAssetPostPop(scene.Pool, scene.Types, @base, damageable, scene.State, scene.Hooks());
            // 파생 Activate 의 깊이 증가·공통 몸체 감소를 그대로 보존한다
            scene.Book.Depth++;
            assets.PostPop(Source, X86Fixture.UInt(row[2]));
            Assert.Equal(0u, scene.Book.Depth);
            string[] output = scene.Output();
            string[] expected = row[15..];
            Assert.Equal(expected, output);
            count++;
            return true;
        });
        Assert.Equal(818, count);
    }

    /// <summary>
    /// 묶은 실제 9 Player 기록으로 같은 입력 818회를 다시 재생한다.
    /// 신전 네 DWORD 쓰기가 묶은 바이트에만 가고 state 사본은 그대로 둔다.
    /// </summary>
    [Fact]
    public void AssetBodiesMatchOriginalX86WithSharedPlayerRecords()
    {
        const int Stride = 0xb4;
        const int Temple = 0x7c;
        string[][] rows = X86Fixture.Read("assetpostpop");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            var scene = new Scene();
            scene.Prepare(row);
            scene.State.Players[1].InitialTemple = 901;
            scene.State.Players[1].InitialTempleCopy = 902;
            scene.State.Players[1].NewTemple = 903;
            scene.State.Players[1].NewTempleCopy = 904;
            // 사용하지 않는 필드 구별을 위해 stale 바이트로 채우고 신전 16바이트만 지운다
            var records = new byte[9 * Stride];
            Array.Fill(records, (byte)0xa5);
            // 9개 소유자 각각의 신전 레코드 16바이트를 지운다
            for (int owner = 0; owner < 9; owner++)
            {
                Array.Clear(records, owner * Stride + Temple, 16);
            }
            var expectedRecords = (byte[])records.Clone();
            uint local = 1;
            var @base = new RawPostPop(scene.Pool, scene.Types, scene.Book);
            var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, scene.CarrierState, scene.CarrierHooks(@base));
            var assets = new RawAssetPostPop(scene.Pool, scene.Types, @base, damageable, scene.State, scene.Hooks());
            assets.BindPlayerRecords(records, () => local);
            scene.Book.Depth++;
            assets.PostPop(Source, X86Fixture.UInt(row[2]));
            Assert.Equal(0u, scene.Book.Depth);
            string[] output = scene.Output();
            string[] fields = row[18].Split(',');
            var rebuilt = new List<string>();
            // 기대 신전 DWORD 로 묶은 바이트 기대값을 만들고 실제 기록 바이트로 출력을 다시 읽는다
            for (int field = 0; field < 4; field++)
            {
                Put(expectedRecords, Stride + Temple + field * 4, X86Fixture.UInt(fields[field]));
                rebuilt.Add(Get(records, Stride + Temple + field * 4).ToString());
            }
            output[3] = string.Join(',', rebuilt);
            Assert.Equal(row[15..], output);
            Assert.Equal(expectedRecords, records);
            // 묶은 뒤에는 state 사본을 쓰지 않는다
            Assert.Equal(901u, scene.State.Players[1].InitialTemple);
            Assert.Equal(902u, scene.State.Players[1].InitialTempleCopy);
            Assert.Equal(903u, scene.State.Players[1].NewTemple);
            Assert.Equal(904u, scene.State.Players[1].NewTempleCopy);
            count++;
            return true;
        });
        Assert.Equal(818, count);
    }

    /// <summary>
    /// 외부 효과가 없을 때 공간 쓰기 전에 거부하고 raw·목록·기록을 바꾸지 않는다.
    /// 간헐천 분기의 필수 효과 누락을 확인한다.
    /// </summary>
    [Fact]
    public void MissingEffectsBeforeMutation()
    {
        string[][] rows = X86Fixture.Read("assetpostpop");
        string[] template = rows.First(row => row[0] == PatchEdition);
        var scene = new Scene();
        scene.Prepare(template);
        Put(scene.Pool.AllocatedBytes(Source), 0, AssetTables[9]);
        var @base = new RawPostPop(scene.Pool, scene.Types, scene.Book);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, scene.CarrierState, scene.CarrierHooks(@base));
        var assets = new RawAssetPostPop(scene.Pool, scene.Types, @base, damageable, scene.State, new AssetPostPopHooks());
        string[] before = scene.Output();
        scene.Book.Depth++;
        // 필수 간헐천 효과가 없어서 쓰기 전에 실패한다
        Assert.ThrowsAny<Exception>(() => assets.PostPop(Source, 1));
        Assert.Equal(before, scene.Output());
        scene.Book.Depth--;
        Assert.Equal(0u, scene.Book.Depth);
    }

    /// <summary>
    /// 효과 중에 바뀌는 battleOwner DWORD 로 Player 신전을 다시 읽는다.
    /// 기대값은 asset-player-x86.tsv(판본별 288회) 중 10.78 몫이다.
    /// 열 구성은 cpppj/tests/AssetPostPopTests.cpp 의 ReplayMutablePlayers 와 같다.
    /// </summary>
    [Fact]
    public void AssetBodiesMatchOriginalX86WithMutablePlayers()
    {
        const int Stride = 0xb4;
        const int Temple = 0x7c;
        string[][] fixture = X86Fixture.Read("asset-player");
        string[][] rows = X86Fixture.Read("assetpostpop");
        string[] template = rows.First(row => row[0] == PatchEdition);
        int count = 0;
        X86Fixture.CheckRows(fixture, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            Assert.Equal(17, row.Length);
            var scene = new Scene();
            scene.Prepare(template);
            scene.State.InBattle = true;
            scene.State.LoadingDepth = 1;
            scene.State.FortMode = true;
            scene.State.Selected = 62;
            scene.ReserveMask = 0;
            Span<byte> raw = scene.Pool.AllocatedBytes(Source);
            Put(raw, 0, AssetTables[X86Fixture.UInt(row[1])]);
            raw[34] = (byte)X86Fixture.UInt(row[4]);
            raw[40] = (byte)X86Fixture.UInt(row[3]);
            // 사용하지 않는 필드 구별을 위해 stale 바이트로 채우고 신전 16바이트만 입력으로 둔다
            var records = new byte[9 * Stride];
            Array.Fill(records, (byte)0xa5);
            // 9개 소유자 각각의 신전 레코드에 입력 값을 채운다
            for (int owner = 0; owner < 9; owner++)
            {
                Put(records, owner * Stride + Temple, X86Fixture.UInt(row[8]));
                Put(records, owner * Stride + Temple + 4, 0x11112222);
                Put(records, owner * Stride + Temple + 8, 0x33334444);
                Put(records, owner * Stride + Temple + 12, 0x55556666);
            }
            uint local = X86Fixture.UInt(row[6]);
            var @base = new RawPostPop(scene.Pool, scene.Types, scene.Book);
            CarrierPostPopHooks carrierHooks = scene.CarrierHooks(@base);
            Action<int, uint> common = carrierHooks.Base!;
            // 실제 Damageable 몸체는 공통 몸체 전에 현재 소유자를 쓴다
            carrierHooks.Base = (sid, flags) =>
            {
                scene.Pool.AllocatedBytes(sid)[34] = (byte)X86Fixture.UInt(row[5]);
                common(sid, flags);
            };
            var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, scene.CarrierState, carrierHooks);
            AssetPostPopHooks hooks = scene.Hooks();
            Action<uint> sound = hooks.PlayImpactSound!;
            // 실제 외부 효과는 충격음 뒤에 battleOwner 를 바꾸고 기록을 고친다
            hooks.PlayImpactSound = token =>
            {
                local = X86Fixture.UInt(row[7]);
                int value = X86Fixture.Int(row[9]);
                if (value >= 0)
                {
                    Put(records, (int)X86Fixture.UInt(row[5]) * Stride + Temple, (uint)value);
                }
                sound(token);
            };
            var assets = new RawAssetPostPop(scene.Pool, scene.Types, @base, damageable, scene.State, hooks);
            assets.BindPlayerRecords(records, () => local);
            scene.Book.Depth++;
            assets.PostPop(Source, X86Fixture.UInt(row[2]));
            Assert.Equal(0u, scene.Book.Depth);
            string[] output = scene.Output();
            output[3] = Convert.ToHexStringLower(records);
            Assert.Equal(row[10..16], output);
            Assert.Equal(X86Fixture.UInt(row[16]), local);
            count++;
            return true;
        });
        Assert.Equal(288, count);
    }

    /// <summary>
    /// 잘못된 9기록 묶음은 거부하고 묶은 바이트를 바꾸지 않는다.
    /// 크기가 맞을 때만 묶는다.
    /// </summary>
    [Fact]
    public void PlayerRecordBindingRejectsWrongSize()
    {
        string[][] rows = X86Fixture.Read("assetpostpop");
        string[] template = rows.First(row => row[0] == PatchEdition);
        var scene = new Scene();
        scene.Prepare(template);
        var @base = new RawPostPop(scene.Pool, scene.Types, scene.Book);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, scene.CarrierState, scene.CarrierHooks(@base));
        var assets = new RawAssetPostPop(scene.Pool, scene.Types, @base, damageable, scene.State, scene.Hooks());
        var records = new byte[9 * 0xb4];
        Array.Fill(records, (byte)0xa5);
        assets.BindPlayerRecords(records);
        byte[] before = (byte[])records.Clone();
        // 한 바이트 짧은 기록과 빈 기록은 묶지 않는다
        Assert.ThrowsAny<Exception>(() => assets.BindPlayerRecords(records[..^1]));
        Assert.Equal(before, records);
        Assert.ThrowsAny<Exception>(() => assets.BindPlayerRecords([]));
    }
}
