// Player CoreData의 실제 포인터·캐시·부분 쓰기와 가변 alias를 검증한다.
using System.Globalization;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawPlayerCoreData의 조회·선택을 검증하는 테스트 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreDataTests
{
    /// <summary>명시 중단을 물리 fault와 분리한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class CoreStopException : Exception
    {
    }

    /// <summary>실제 원시 입력과 동일 배열·슬롯만 제공하는 장치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class CoreDevice
    {
        /// <summary>실제 9×180바이트다.</summary>
        public byte[] PlayerBytes = new byte[1620];

        /// <summary>실제 10바이트 캐시다.</summary>
        public byte[] Cached = new byte[10];

        /// <summary>짧은 실제 외부 입력이다.</summary>
        public byte[] External = new byte[16];

        /// <summary>현재 전역이다.</summary>
        public PlayerCoreDataGlobals GlobalsValue = new();

        /// <summary>소유자 인자다.</summary>
        public uint OwnerArg;

        /// <summary>Template 인자다.</summary>
        public uint TemplateArg;

        /// <summary>진입점이다.</summary>
        public int Mode;

        /// <summary>유일한 명시 하위 변화다.</summary>
        public string Mutation = "none";

        /// <summary>진단 호출 기록이다.</summary>
        public List<string> Trace = [];

        /// <summary>현재 포인터는 실제 Player·캐시·외부 입력의 같은 주소를 빌린다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="values">원문 입력 10칸이다.</param>
        public CoreDevice(string[] values)
        {
            if (values.Length != 10)
            {
                throw new InvalidOperationException("CoreData 원문 입력 수");
            }

            Mode = int.Parse(values[0], CultureInfo.InvariantCulture);
            int seed = int.Parse(values[1], CultureInfo.InvariantCulture);
            int kind = int.Parse(values[2], CultureInfo.InvariantCulture);
            int length = (int)uint.Parse(values[3], CultureInfo.InvariantCulture);
            GlobalsValue.TemplateOwner = 66u;
            GlobalsValue.BattleOwner = 55u;
            GlobalsValue.Metamap = uint.Parse(values[4], CultureInfo.InvariantCulture);
            GlobalsValue.PlayerKnown = uint.Parse(values[5], CultureInfo.InvariantCulture);
            GlobalsValue.Active = uint.Parse(values[6], CultureInfo.InvariantCulture);
            OwnerArg = uint.Parse(values[7], CultureInfo.InvariantCulture);
            TemplateArg = uint.Parse(values[8], CultureInfo.InvariantCulture);
            Mutation = values[9];
            // 전체 Player 미작성 바이트도 비교하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < PlayerBytes.Length; ++index)
            {
                PlayerBytes[index] = unchecked((byte)(seed + index * 17));
            }

            // 부분 캐시 쓰기를 구별하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < Cached.Length; ++index)
            {
                Cached[index] = unchecked((byte)(seed + index * 11 + 23));
            }

            // 외부 입력의 실제 값을 채우는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < External.Length; ++index)
            {
                External[index] = unchecked((byte)(seed + index * 7 + 49));
            }

            GlobalsValue.Cached = Cached;
            GlobalsValue.Players = PlayerBytes;
            if (kind == 1)
            {
                GlobalsValue.Current = PlayerBytes.AsMemory(0xb4 + 0x6c, 10);
            }
            else if (kind == 2)
            {
                GlobalsValue.Current = PlayerBytes.AsMemory(8 * 0xb4 + 0x6c, 10);
            }
            else if (kind == 3)
            {
                GlobalsValue.Current = Cached;
            }
            else if (kind == 4)
            {
                GlobalsValue.Current = External.AsMemory(0, length);
            }
            else if (kind != 0)
            {
                throw new InvalidOperationException("CoreData 원문 포인터 종류");
            }
        }

        /// <summary>같은 참조와 실제 물리뷰만 신규 두 몸체에 공급한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>현재 전역이다.</returns>
        public PlayerCoreDataGlobals Globals() => GlobalsValue;

        /// <summary>가리키는 같은 배열 주소로만 원문 가상주소와 대응한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="value">이름을 물을 메모리다.</param>
        /// <returns>주소 이름이다.</returns>
        public string Pointer(Memory<byte> value)
        {
            MemoryMarshal.TryGetArray(value, out ArraySegment<byte> segment);
            if (segment.Array is null || segment.Array.Length == 0)
            {
                return "0";
            }

            if (ReferenceEquals(segment.Array, Cached))
            {
                return "CACHED";
            }

            if (ReferenceEquals(segment.Array, External))
            {
                return "EXTERNAL";
            }

            // 실제 주소를 비교하며 내용으로 소유자를 추정하지 않는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int owner = 0; owner < 9; ++owner)
            {
                if (ReferenceEquals(segment.Array, PlayerBytes) && segment.Offset == owner * 0xb4 + 0x6c)
                {
                    return "PLAYER" + owner;
                }
            }

            throw new InvalidOperationException("CoreData 미등록 포인터:" + (segment.Array is null ? "null" : segment.Array.Length + "@" + segment.Offset + "+" + segment.Count));
        }

        /// <summary>두 소유자·알림·활성·메타맵의 현재 값을 원문과 같은 순서로 출력한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>상태 문자열이다.</returns>
        public string State() => $"{GlobalsValue.TemplateOwner},{GlobalsValue.BattleOwner},{GlobalsValue.PlayerKnown},{GlobalsValue.Active},{GlobalsValue.Metamap}";

        /// <summary>유일한 실제 하위 진단에서 현재값 변화·중단만 수행한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>훅 묶음이다.</returns>
        public PlayerCoreDataHooks Hooks()
        {
            return new PlayerCoreDataHooks
            {
                Assertion = line =>
                {
                    Trace.Add("A:" + line + ":_myCoreData == &lastKnownCoreData");
                    if (Mutation == "current0")
                    {
                        GlobalsValue.Current = default;
                    }
                    else if (Mutation == "currentPlayer8")
                    {
                        GlobalsValue.Current = PlayerBytes.AsMemory(8 * 0xb4 + 0x6c, 10);
                    }
                    else if (Mutation == "currentCache")
                    {
                        GlobalsValue.Current = Cached;
                    }
                    else if (Mutation == "metamap0")
                    {
                        GlobalsValue.Metamap = 0u;
                    }
                    else if (Mutation == "active0")
                    {
                        GlobalsValue.Active = 0u;
                    }
                    else if (Mutation == "stop")
                    {
                        throw new CoreStopException();
                    }
                    else if (Mutation != "none")
                    {
                        throw new InvalidOperationException("CoreData 원문 변경 종류");
                    }
                },
            };
        }

        /// <summary>정상 반환·실제 물리 읽기 실패·명시 중단을 서로 다른 원시 결과로 유지한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>상태와 결과다.</returns>
        public (string Status, string Result) Run()
        {
            try
            {
                if (Mode == 0)
                {
                    Memory<byte> result = RawPlayerCoreData.ReadPlayerCoreData(Globals(), Hooks());
                    return ("normal", Pointer(result));
                }

                if (Mode != 1)
                {
                    throw new InvalidOperationException("CoreData 원문 진입점");
                }

                RawPlayerCoreData.SelectPlayerCoreData(Globals(), OwnerArg, TemplateArg);
                return ("normal", "-");
            }
            catch (CoreStopException)
            {
                return ("stop", "-");
            }
            catch (ArgumentOutOfRangeException)
            {
                return ("fault", "-");
            }
        }
    }

    /// <summary>바이트를 소문자 16진수로 만든다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    /// <returns>소문자 16진수다.</returns>
    private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>메모리 뷰가 같은 주소를 가리키는지 본다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="first">첫 뷰다.</param>
    /// <param name="second">둘째 뷰다.</param>
    /// <returns>같은 주소면 참이다.</returns>
    private static bool SameMemory(Memory<byte> first, Memory<byte> second)
    {
        if (!MemoryMarshal.TryGetArray(first, out ArraySegment<byte> a) || !MemoryMarshal.TryGetArray(second, out ArraySegment<byte> b))
        {
            return false;
        }

        return ReferenceEquals(a.Array, b.Array) && a.Offset == b.Offset && a.Count == b.Count;
    }

    /// <summary>유효 비영 Player 선택과 짧은 현재·진단 변경·중단의 관측 804행을 전부 대조한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ReplaysIndependentPe804()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "player-core-data-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int rows = 0;
        Dictionary<string, int> statuses = new();
        // 양 정밀도의 804행을 생략하지 않는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string[] values = line.Split('\t');
            Assert.Equal(11, values.Length);
            Assert.True(values[0] == "639" || values[0] == "895");
            Assert.True(uint.Parse(values[10], CultureInfo.InvariantCulture) > 0u);
            CoreDevice device = new(values[1].Split(','));
            string status;
            string result;
            try
            {
                (status, result) = device.Run();
            }
            catch (Exception error)
            {
                throw new InvalidOperationException($"CoreData {rows + 1}행 실행 실패 [{values[1]}]", error);
            }
            ++rows;
            statuses[status] = statuses.TryGetValue(status, out int had) ? had + 1 : 1;
            Assert.Equal(values[2], status);
            Assert.Equal(values[3], result);
            Assert.Equal(values[4], device.State());
            Assert.Equal(values[5], device.Pointer(device.Globals().Current));
            Assert.Equal(values[6], Hex(device.Cached));
            Assert.Equal(values[7], Hex(device.PlayerBytes));
            Assert.Equal(values[8], Hex(device.External));
            Assert.Equal(values[9], device.Trace.Count == 0 ? "-" : string.Join(';', device.Trace));
        }

        Assert.Equal(804, rows);
        Assert.Equal(730, statuses.GetValueOrDefault("normal", 0));
        Assert.Equal(72, statuses.GetValueOrDefault("fault", 0));
        Assert.Equal(2, statuses.GetValueOrDefault("stop", 0));
    }

    /// <summary>반환뷰의 쓰기가 실제 Player에 반영되고 정리가 같은 10바이트를 캐시에 보존하는지 확인한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ReturnedViewAliasesPlayerAndCachedStorage()
    {
        CoreDevice device = new("1,165,0,10,0,7,9,8,3,none".Split(','));
        RawPlayerCoreData.SelectPlayerCoreData(device.Globals(), 8u, 3u);
        Memory<byte> current = RawPlayerCoreData.ReadPlayerCoreData(device.Globals(), new PlayerCoreDataHooks());
        Assert.True(SameMemory(current, device.PlayerBytes.AsMemory(8 * 0xb4 + 0x6c, 10)));
        Assert.Equal(10, current.Length);
        Assert.Equal(3u, device.Globals().TemplateOwner);
        Assert.Equal(9u, device.Globals().Active);
        current.Span[4] = 201;
        Assert.Equal(201, device.PlayerBytes[8 * 0xb4 + 0x70]);
        uint known = device.Globals().PlayerKnown;
        RawPlayerCoreData.SelectPlayerCoreData(device.Globals(), 0u, 1u);
        Assert.Equal(0, device.Globals().Current.Length);
        Assert.Equal(0u, device.Globals().Active);
        Assert.Equal(known, device.Globals().PlayerKnown);
        Assert.Equal(201, device.Cached[4]);
        device.Globals().Metamap = 1u;
        RawPlayerCoreData.SelectPlayerCoreData(device.Globals(), 1u, 7u);
        current = RawPlayerCoreData.ReadPlayerCoreData(device.Globals(), new PlayerCoreDataHooks());
        Assert.True(SameMemory(current, device.Cached.AsMemory()));
        current.Span[8] = 177;
        Assert.Equal(177, device.Cached[8]);
        Assert.NotEqual(177, device.PlayerBytes[8 * 0xb4 + 0x74]);
    }

    /// <summary>범위 밖 가상 포인터를 host 주소로 꾸미지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsUnrepresentablePlayerViewAfterEarlierStores()
    {
        CoreDevice device = new("1,165,3,10,0,7,9,8,3,none".Split(','));
        device.Globals().Players = device.PlayerBytes.AsMemory(0, 100);
        bool rejected = false;
        try
        {
            RawPlayerCoreData.SelectPlayerCoreData(device.Globals(), 8u, 3u);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }

        Assert.True(rejected);
        Assert.Equal(3u, device.Globals().TemplateOwner);
        Assert.Equal(8u, device.Globals().BattleOwner);
        Assert.Equal(1u, device.Globals().PlayerKnown);
        Assert.True(SameMemory(device.Globals().Current, device.Cached.AsMemory()));
        Assert.Equal(9u, device.Globals().Active);
    }
}
