using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공통 삭제 본체(<see cref="RawSquidDestroy"/>)와 다리 수명주기(<see cref="RawBridgeLifecycle"/>)를
/// 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/RawDestroyTests.cpp 의 Scene 과 같다: 실제 슬롯 풀·해시·점유 지도 위에서
/// 죽음 표시→종속 끊기·죽음 알림→전송·선택 알림→Unpop→반납을 실행하고,
/// 다리 행은 뿌리 삭제를 다리 사전·사후(연결 끊기·제거 알림·추락 소리·보행자 추락·공통 사전·사후)에 잇는다.
/// 호출마다 효과 문자열·슬롯 바이트·풀 카운터·머리·꼬리·체크섬·고른 번호·깊이를 비교한다.
/// 기준 판본은 10.78 이며 원본1037·CD 행은 읽지 않는다 (W03 삭제·수명 둘째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawDestroyTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 원본1037·CD 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 Destroy 행 수. 다리 64행은 별도로 센다.</summary>
    private const int DestroyRowsPerEdition = 600;

    /// <summary>10.78 Bridge 행 수(서버 풀·역할 0~1·갈래 0~3).</summary>
    private const int BridgeRowsPerEdition = 64;

    /// <summary>삭제 뿌리·종속의 타입 번호(자산). 종속 일부는 20·75 로 바뀐다.</summary>
    private const int RootType = 74;

    /// <summary>장면이 확보하는 클라이언트 슬롯 7개.</summary>
    private static readonly int[] KidOffsets = [5, 6, 7, 8, 9, 10, 11];

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
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

    /// <summary>네 단계 해시 머리를 0단계부터 16비트 리틀 엔디언으로 늘어놓는다.</summary>
    /// <param name="hash">공간 해시.</param>
    private static byte[] HeadBytes(RawSquidHash hash)
    {
        var bytes = new List<byte>();
        // 단계 0~3 순서로 모든 머리를 쓴다
        for (int level = 0; level < RawSquidHash.Levels; level++)
        {
            // 머리마다 두 바이트
            foreach (ushort value in hash.Entries(level))
            {
                bytes.Add((byte)value);
                bytes.Add((byte)(value >> 8));
            }
        }
        return bytes.ToArray();
    }

    /// <summary>삭제 기록(클라이언트→서버)을 DWORD 리틀 엔디언으로 늘어놓는다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    private static byte[] LogBytes(RawSidPool pool)
    {
        var bytes = new List<byte>();
        // 클라이언트 기록부터 서버 기록 순서로 쓴다
        foreach (bool client in new[] { true, false })
        {
            // 기록마다 번호·타입을 DWORD 로 쓴다
            foreach (SidDeletion record in pool.Deletions(client))
            {
                // 번호·타입을 네 바이트씩 쓴다
                foreach (uint value in new[] { record.Sid, record.Type })
                {
                    bytes.Add((byte)value);
                    bytes.Add((byte)(value >> 8));
                    bytes.Add((byte)(value >> 16));
                    bytes.Add((byte)(value >> 24));
                }
            }
        }
        return bytes.ToArray();
    }

    /// <summary>한 삭제 행의 풀·해시·점유 지도·장면을 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(32,768칸, 서버 여부는 행 입력).</summary>
        public readonly RawSidPool Pool;

        /// <summary>공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>점유 지도(65,536칸, 항상 0).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개, 전부 기본값).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>자산 뿌리의 공간 해제기.</summary>
        public readonly RawUnpop Unpop;

        /// <summary>삭제 본체.</summary>
        public readonly RawSquidDestroy Destroy;

        /// <summary>일반 찾기. 다리 장면의 탐색기가 쓴다.</summary>
        public readonly RawSquidFinder Finder;

        /// <summary>다리 사전·사후 파괴. 다리 장면에서 뿌리 삭제가 부른다.</summary>
        public readonly RawBridgeLifecycle Bridge;

        /// <summary>다리 탐색기 콜백 묶음.</summary>
        public readonly BridgeLifecycleHooks BridgeHooks;

        /// <summary>다리 장면인지. 참이면 뿌리 삭제가 다리 수명주기를 탄다.</summary>
        public readonly bool BridgeMode;

        /// <summary>다리 장면의 연결 타입 번호. 사전 탐색이 찾는 타입이다.</summary>
        private const uint LinkType = 75;

        /// <summary>다리 뿌리의 가상 표 값(10.78 다리 클래스 식별 값).</summary>
        private const uint BridgeVtable = 0x5034C8;

        /// <summary>뿌리 번호(역할 0 이면 5, 1 이면 15009, 2 이면 23002).</summary>
        public readonly int Root;

        /// <summary>확보한 클라이언트 슬롯 7개.</summary>
        public readonly int[] Ids = new int[7];

        /// <summary>장면의 입력 갈래(0~7). 종속 체인·선택·사전 훼손을 정한다.</summary>
        public readonly int Scenario;

        /// <summary>현재 고른 번호. 홀수 갈래는 뿌리, 짝수는 없음이다.</summary>
        public int Selected;

        /// <summary>나간 효과 문자열.</summary>
        public readonly List<string> Events = [];

        /// <summary>바깥 효과 콜백 묶음.</summary>
        public readonly SquidDestroyHooks Hooks;

        /// <summary>행 입력(서버 여부·역할·갈래)으로 장면을 준비한다.</summary>
        /// <param name="server">서버 풀인지.</param>
        /// <param name="role">뿌리 역할(0 클라이언트, 1 서버, 2 예측).</param>
        /// <param name="scenario">입력 갈래(0~7, 다리 장면은 0~3).</param>
        /// <param name="bridgeMode">다리 장면이면 참. 뿌리 삭제를 다리 수명주기에 잇는다.</param>
        public Scene(bool server, int role, int scenario, bool bridgeMode = false)
        {
            Scenario = scenario;
            BridgeMode = bridgeMode;
            Pool = new RawSidPool(32768, server);
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            Unpop = new RawUnpop(Pool, Hash, Spots);
            var factory = new RawSquidFactory(Pool, Types);
            Destroy = new RawSquidDestroy(Pool, Unpop, Types);
            // 원본처럼 클라이언트 슬롯 7개를 순서대로 확보한다
            for (int i = 0; i < Ids.Length; i++)
            {
                Ids[i] = Pool.Allocate(SidAllocation.Client);
            }
            Root = role == 0 ? Ids[0] : role == 1 ? RawSidPool.ServerFirst + 9 : RawSidPool.PredictableFirst + 1;
            if (role != 0)
            {
                factory.Take(RootType, Root);
            }
            Selected = scenario % 2 == 1 ? Root : 0;
            var active = new List<int> { Root };
            // 뿌리와 다른 클라이언트 슬롯을 뒤에 붙인다
            foreach (int id in Ids)
            {
                if (id != Root)
                {
                    active.Add(id);
                }
            }
            // 장면의 슬롯마다 타입·상태·좌표·해시 머리를 원본 순서로 쓴다
            foreach (int sid in active)
            {
                int index = Array.IndexOf(Ids, sid);
                int type = sid == Root ? RootType : RootType + index;
                int state = sid == Root ? (scenario == 1 ? 4 : scenario == 2 ? 2 : 0) : 0;
                if (scenario >= 3 && scenario <= 6 && (sid == Ids[1] || (scenario != 3 && sid == Ids[2])))
                {
                    type = scenario == 3 ? 20 : 75;
                    state = scenario == 3 ? 0 : 8;
                }
                if (BridgeMode)
                {
                    if (sid == Ids[1] || sid == Ids[4])
                    {
                        type = (int)LinkType;
                    }
                    if (sid == Ids[2])
                    {
                        state = 2;
                    }
                }
                Span<byte> raw = Pool.AllocatedBytes(sid);
                raw.Clear();
                Put(raw, 0, BridgeMode && sid == Root ? BridgeVtable : OriginalConstructors.BaseVtable);
                Put(raw, 4, 0, 2);
                raw[10] = (byte)type;
                raw[11] = (byte)state;
                Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f));
                Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
                Types[type].Flags1 = 0;
                Types[type].Flags2 = BridgeMode && sid == Root ? 4U : 0U;
                Types[type].FootX = 1;
                Types[type].FootY = 1;
                if (BridgeMode && sid == Ids[5])
                {
                    Types[type].Flags2 = 0x10000;
                }
                if ((state & 12) == 0 && type >= OriginalConstructors.FirstAssetType)
                {
                    ref ushort head = ref Hash.Bucket(sid == Root ? 0 : 1, 20.75f, 21.9f);
                    Put(raw, 4, head, 2);
                    head = (ushort)sid;
                }
            }
            if (!BridgeMode && scenario >= 3 && scenario <= 6)
            {
                Put(Pool.AllocatedBytes(Root), 6, (uint)Ids[1], 2);
                Put(Pool.AllocatedBytes(Ids[1]), 4, scenario == 3 ? 0U : (uint)Ids[2], 2);
                if (scenario != 3)
                {
                    Put(Pool.AllocatedBytes(Ids[2]), 4, 0, 2);
                }
            }
            if (BridgeMode)
            {
                Types[RootType].Flags2 = 4;
                Put(Pool.AllocatedBytes(Ids[1]), 12, (uint)Ids[2], 2);
                Put(Pool.AllocatedBytes(Ids[1]), 8, 8, 2);
                Put(Pool.AllocatedBytes(Ids[4]), 12, (uint)Ids[1], 2);
                Put(Pool.AllocatedBytes(Ids[4]), 8, 8, 2);
            }
            Finder = new RawSquidFinder(Pool, Hash, Types);
            Bridge = new RawBridgeLifecycle(Pool, Types);
            BridgeHooks = RawBridgeLifecycle.MakeHooks(Finder, EmitBridge);
            Hooks = new SquidDestroyHooks { Emit = Emit, Selected = () => Selected };
        }

        /// <summary>효과 한 건을 문자열로 기록한다. 상태는 그 시점의 슬롯 바이트다.</summary>
        /// <param name="tag">효과 표시 문자.</param>
        /// <param name="sid">효과 대상 번호.</param>
        /// <param name="flags">효과 플래그.</param>
        private void Hook(char tag, int sid, uint flags)
        {
            Events.Add($"{tag}:{sid}:{flags}:{Pool.Slot(sid)[11]}");
        }

        /// <summary>삭제 본체의 바깥 효과를 받아 원본 시험처럼 풀을 직접 고친다.</summary>
        /// <param name="effect">나간 삭제 효과.</param>
        private void Emit(SquidDestroyEvent effect)
        {
            switch (effect.Effect)
            {
                case SquidDestroyEffect.PreDestroy:
                    Hook('P', effect.Sid, effect.Flags);
                    if (BridgeMode && effect.Sid == Root)
                    {
                        Bridge.PreDestroy(Root, effect.Flags, LinkType, BridgeHooks);
                    }
                    else
                    {
                        Destroy.CompletePreDestroy();
                        if (effect.Sid == Root && Scenario == 6)
                        {
                            Put(Pool.AllocatedBytes(Root), 6, 7, 2);
                        }
                        if (effect.Sid == Root && Scenario == 7)
                        {
                            Pool.AllocatedBytes(Root)[11] |= 4;
                        }
                    }
                    break;
                case SquidDestroyEffect.PostDestroy:
                    Hook('O', effect.Sid, effect.Flags);
                    if (BridgeMode && effect.Sid == Root)
                    {
                        Bridge.PostDestroy(Root, effect.Flags, BridgeHooks);
                    }
                    else
                    {
                        Destroy.CompletePostDestroy();
                    }
                    break;
                case SquidDestroyEffect.ReleaseDependent:
                    Events.Add($"R:{effect.Sid}:{effect.Parent}");
                    Put(Pool.AllocatedBytes(effect.Sid), 4, 0, 2);
                    if (Scenario == 5 && effect.Sid == Ids[1])
                    {
                        Pool.AllocatedBytes(effect.Sid)[11] = 1;
                    }
                    break;
                case SquidDestroyEffect.DestroyDependent:
                    Events.Add($"C:{effect.Sid}:{effect.Flags}");
                    Pool.AllocatedBytes(effect.Sid)[11] |= 2;
                    break;
                case SquidDestroyEffect.Transmit:
                    Events.Add($"T:{effect.Sid}:{effect.Flags}");
                    break;
                case SquidDestroyEffect.ClearSelection:
                    Events.Add($"S:{Selected}");
                    Selected = 0;
                    break;
                default:
                    throw new InvalidOperationException("알 수 없는 삭제 효과");
            }
        }

        /// <summary>다리 수명주기의 바깥 효과를 받아 원본 시험처럼 풀을 직접 고친다.</summary>
        /// <param name="effect">나간 다리 삭제 효과.</param>
        private void EmitBridge(BridgeLifecycleEvent effect)
        {
            switch (effect.Effect)
            {
                case BridgeLifecycleEffect.DestroyLink:
                    Destroy.Destroy(effect.Sid, effect.Flags, Hooks);
                    break;
                case BridgeLifecycleEffect.BasePreDestroy:
                    Hook('A', effect.Sid, effect.Flags);
                    Destroy.CompletePreDestroy();
                    break;
                case BridgeLifecycleEffect.BasePostDestroy:
                    Hook('B', effect.Sid, effect.Flags);
                    Destroy.CompletePostDestroy();
                    break;
                case BridgeLifecycleEffect.NotifyRemoval:
                    Events.Add($"N:{effect.Sid}");
                    break;
                case BridgeLifecycleEffect.FallSound:
                    Events.Add($"W:{BitConverter.SingleToUInt32Bits(effect.X)}:{BitConverter.SingleToUInt32Bits(effect.Y)}");
                    break;
                case BridgeLifecycleEffect.FallWalker:
                    Events.Add($"F:{effect.Sid}");
                    Pool.AllocatedBytes(effect.Sid)[11] |= 2;
                    break;
                default:
                    throw new InvalidOperationException("알 수 없는 다리 삭제 효과");
            }
        }

        /// <summary>나간 효과 문자열. 없으면 "-" 다.</summary>
        public string EventText() => Events.Count == 0 ? "-" : string.Join(';', Events);
    }

    /// <summary>한 행을 재생하고 효과·슬롯·요약을 대조한다. 다리 행은 다리 장면으로 만든다.</summary>
    /// <param name="row">fixture 행(10칸).</param>
    /// <param name="bridgeMode">다리 장면인지.</param>
    private static void PlayRow(string[] row, bool bridgeMode)
    {
        Assert.Equal(10, row.Length);
        var scene = new Scene(row[3] == "1", X86Fixture.Int(row[4]), X86Fixture.Int(row[6]), bridgeMode);
        scene.Destroy.Destroy(scene.Root, X86Fixture.UInt(row[5]), scene.Hooks);
        Assert.Equal(row[7], scene.EventText());
        string[] slots = row[8].Split(';');
        // 슬롯마다 전체 50바이트를 원본과 비교한다
        foreach (string entry in slots)
        {
            string[] fields = entry.Split(':');
            int sid = int.Parse(fields[0]);
            Assert.Equal(fields[1], Convert.ToHexStringLower(scene.Pool.Slot(sid).ToArray()));
        }
        uint[] expected = row[9].Split(',').Select(uint.Parse).ToArray();
        uint[] actual =
        [
            scene.Pool.FreeCount,
            scene.Pool.PredictableCursor,
            (uint)scene.Pool.FirstFree(true),
            (uint)scene.Pool.FirstFree(false),
            (uint)scene.Pool.Tail(true),
            (uint)scene.Pool.Tail(false),
            X86Fixture.Adler32(scene.Pool.Bytes),
            X86Fixture.Adler32(HeadBytes(scene.Hash)),
            X86Fixture.Adler32(scene.Spots),
            X86Fixture.Adler32(LogBytes(scene.Pool)),
            (uint)scene.Selected,
            scene.Destroy.PreDepth,
            scene.Destroy.PostDepth,
        ];
        Assert.Equal(expected, actual);
    }

    /// <summary>
    /// 삭제 fixture 의 10.78 Destroy 몫 600회를 재생한다.
    /// 호출마다 효과 순서·슬롯 바이트·풀 카운터·머리·꼬리·체크섬·고른 번호·깊이를 대조한다.
    /// </summary>
    [Fact]
    public void DestroyMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("destroy");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != "Destroy" || row[1] != PatchEdition)
            {
                return true;
            }
            PlayRow(row, false);
            count++;
            return true;
        });
        Assert.Equal(DestroyRowsPerEdition, count);
    }

    /// <summary>
    /// 삭제 fixture 의 10.78 Bridge 몫 64회를 다리 수명주기와 이어 재생한다.
    /// 연결 끊기·제거 알림·추락 소리·보행자 추락·공통 사전·사후의 순서와 슬롯·요약을 대조한다.
    /// </summary>
    [Fact]
    public void BridgeMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("destroy");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != "Bridge" || row[1] != PatchEdition)
            {
                return true;
            }
            PlayRow(row, true);
            count++;
            return true;
        });
        Assert.Equal(BridgeRowsPerEdition, count);
    }
}
