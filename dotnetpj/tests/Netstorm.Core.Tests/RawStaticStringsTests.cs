using Netstorm.Core.Original;

namespace Netstorm.Tests; // 같은 네 DWORD의 원문 임시 고리 외곽을 대조한다.

// cpppj o/RawStaticStrings의 초기화와 예약과 서식 외곽 계약이다. 0.985에서 추가됐다.
public sealed class RawStaticStringsTests
{
    /// <summary>같은 네 DWORD 저장소를 만든다.</summary>
    private static StaticStringGlobals Globals() => new();

    /// <summary>같은 실제 물리를 빌리는 검사 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 네 DWORD다.</summary>
        public readonly StaticStringGlobals Globals = new();
        /// <summary>장치에 빌려준 실제 바이트 소유자다.</summary>
        public readonly Dictionary<uint, byte[]> Stores = new();
        /// <summary>다음 할당 토큰이다.</summary>
        public uint Next = 0x1000;
        /// <summary>기록한 진단 줄이다.</summary>
        public readonly List<uint> Assertions = [];
        /// <summary>기록한 서식 호출이다.</summary>
        public readonly List<(uint Destination, uint Pattern)> Formats = [];
        /// <summary>같은 장치 묶음이다.</summary>
        public readonly StaticStringHooks Hooks;

        /// <summary>같은 물리로 검사 장치를 만든다.</summary>
        public Scene()
        {
            Hooks = new StaticStringHooks
            {
                Allocate = size => { uint result = Next; Next += size + 16; Stores[result] = new byte[size + 16]; return result; },
                Memory = (address, size) =>
                {
                    foreach (KeyValuePair<uint, byte[]> entry in Stores)
                    {
                        if (address >= entry.Key && unchecked(address - entry.Key) + size <= (uint)entry.Value.Length)
                            return entry.Value.AsMemory(checked((int)unchecked(address - entry.Key)), checked((int)size));
                    }

                    throw new ArgumentOutOfRangeException(nameof(address));
                },
                Assertion = line => Assertions.Add(line),
                Format = (destination, pattern) => Formats.Add((destination, pattern)),
            };
        }
    }

    /// <summary>초기화는 새 할당 뒤 전체 0과 첫 D5와 current+1을 남긴다.</summary>
    [Fact]
    public void InitializeWritesZerosAndFirstSentinel()
    {
        var scene = new Scene();
        RawStaticStrings.InitializeStaticStrings(10, scene.Globals, scene.Hooks);
        Assert.Equal(10u, scene.Globals.Size);
        Assert.NotEqual(0u, scene.Globals.Base);
        Assert.Equal(scene.Globals.Base, scene.Globals.Current - 1u);
        byte[] store = scene.Stores[scene.Globals.Base];
        Assert.Equal(0xd5, store[0]);
        // 전체 0 뒤 첫 바이트만 D5다.
        for (int index = 1; index < 10; index++) Assert.Equal(0, store[index]);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>이미 base가 있으면 17 진단을 남기고 다시 할당한다.</summary>
    [Fact]
    public void InitializeWithExistingBaseReports17()
    {
        var scene = new Scene();
        RawStaticStrings.InitializeStaticStrings(8, scene.Globals, scene.Hooks);
        RawStaticStrings.InitializeStaticStrings(8, scene.Globals, scene.Hooks);
        Assert.Contains(17u, scene.Assertions);
    }

    /// <summary>첫 예약은 8192 초기화를 거쳐 last와 current 끝 D5를 남긴다.</summary>
    [Fact]
    public void FirstReserveInitializes8192()
    {
        var scene = new Scene();
        uint result = RawStaticStrings.ReserveStaticString(4, scene.Globals, scene.Hooks);
        Assert.Equal(8192u, scene.Globals.Size);
        Assert.Equal(result, scene.Globals.Last);
        Assert.Equal(unchecked(result + 4u + 1u), scene.Globals.Current);
    }

    /// <summary>끝에 닿으면 base 다음으로 감아 같은 주소를 재사용한다.</summary>
    [Fact]
    public void ReserveWrapsAtEnd()
    {
        var scene = new Scene();
        RawStaticStrings.InitializeStaticStrings(16, scene.Globals, scene.Hooks);
        uint first = RawStaticStrings.ReserveStaticString(8, scene.Globals, scene.Hooks);
        Assert.Equal(unchecked(scene.Globals.Base + 1u), first);
        uint second = RawStaticStrings.ReserveStaticString(8, scene.Globals, scene.Hooks);
        Assert.Equal(first, second);
        Assert.Equal(unchecked(scene.Globals.Base + 1u), scene.Globals.Current - 9u);
    }

    /// <summary>예약 결과와 서식 주소가 같으면 CRT를 생략한다.</summary>
    [Fact]
    public void FormatSkipsWhenPatternIsResult()
    {
        var scene = new Scene();
        uint first = RawStaticStrings.ReserveStaticString(4, scene.Globals, scene.Hooks);
        int before = scene.Formats.Count;
        uint result = RawStaticStrings.FormatStaticString(4, first, scene.Globals, scene.Hooks);
        Assert.NotEqual(first, result);
        Assert.Equal(before + 1, scene.Formats.Count);
        // 다음 예약 결과를 미리 읽어 서식으로 주면 별칭으로 생략한다.
        scene.Formats.Clear();
        uint expected = scene.Globals.Current;
        uint same = RawStaticStrings.FormatStaticString(4, expected, scene.Globals, scene.Hooks);
        Assert.Equal(expected, same);
        Assert.Empty(scene.Formats);
    }

    /// <summary>도달하지 않은 장치는 요구할 때만 실패한다.</summary>
    [Fact]
    public void MissingDevicesFailOnlyOnUse()
    {
        var globals = Globals();
        Assert.Throws<InvalidOperationException>(() => RawStaticStrings.InitializeStaticStrings(8, globals, new StaticStringHooks()));
        var fresh = new Scene();
        Assert.Throws<InvalidOperationException>(() => RawStaticStrings.ReserveStaticString(4, fresh.Globals, new StaticStringHooks()));
        var scene = new Scene();
        RawStaticStrings.InitializeStaticStrings(8, scene.Globals, scene.Hooks);
        Assert.Throws<InvalidOperationException>(() => RawStaticStrings.ReserveStaticString(4, scene.Globals, new StaticStringHooks { Allocate = scene.Hooks.Allocate }));
    }
}
