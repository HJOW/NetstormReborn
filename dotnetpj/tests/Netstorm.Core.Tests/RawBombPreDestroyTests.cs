using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// Point Blast Bomb 가상 사전 파괴(<see cref="RawBombPreDestroy"/>)의 경계 계약을 cpppj
/// RawBombPreDestroy 검사와 원본 관찰 변형으로 확인한다.
/// 가상 표가 다르면 효과 전에 거부하는지, 추가 비트 9 와 폭발 플래그가 폭발·소리를 막는지,
/// 폭발이 바꾼 좌표를 소리가 다시 읽는지, 공통 전처리가 항상 마지막에 한 번 불리는지 본다.
/// 기준 판본은 10.78 이다 (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class RawBombPreDestroyTests
{
    /// <summary>검사 대상 Bomb 번호.</summary>
    private const int Source = 50;

    /// <summary>10.78 Point Blast Bomb 클래스의 가상 표 값.</summary>
    private const uint PatchBombVtable = 0x00502370;

    /// <summary>폭발을 요구하는 삭제 플래그.</summary>
    private const uint ExplosionFlag = 0x100000;

    /// <summary>검사 장면의 풀·슬롯·기록.</summary>
    private sealed class Scene
    {
        /// <summary>Bomb 50 이 실제 할당된 서버 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>효과 문자열 조각.</summary>
        public readonly List<string> Parts = [];

        /// <summary>검사 대상 몸체.</summary>
        public readonly RawBombPreDestroy Body;

        /// <summary>클라이언트 풀을 만들고 Bomb 50 까지 실제 할당한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, true);
            // 검사 대상 50 까지 실제 번호를 확보한다(서버 풀도 클라이언트 영역을 할당할 수 있다)
            while (Pool.Allocate(SidAllocation.Client) != Source)
            {
            }
            Body = new RawBombPreDestroy(Pool, Hooks());
        }

        /// <summary>슬롯을 원본 관찰과 같은 초기값으로 되돌린다.</summary>
        /// <param name="extra">추가 비트.</param>
        /// <param name="x">x 좌표.</param>
        /// <param name="y">y 좌표.</param>
        public void Prepare(byte extra, float x, float y)
        {
            Parts.Clear();
            Span<byte> raw = Pool.RawSlot(Source);
            raw.Fill(0xa5);
            Put(raw, 0, PatchBombVtable);
            raw[10] = 113;
            raw[11] = 2;
            raw[40] = extra;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
        }

        /// <summary>효과 문자열에 조각을 더한다.</summary>
        /// <param name="text">추가할 조각.</param>
        private void Event(string text)
        {
            // 사건 사이에 세미콜론을 둔다
            if (Parts.Count != 0)
            {
                Parts.Add(";");
            }
            Parts.Add(text);
        }

        /// <summary>폭발·소리·공통 전처리를 기록하는 훅 묶음.</summary>
        public BombPreDestroyHooks Hooks() => new()
        {
            Explosion = sid =>
            {
                Event("E:" + sid);
                // 폭발이 좌표를 바꾸면 소리는 그 새 좌표를 읽어야 한다
                Put(Pool.RawSlot(Source), 14, BitConverter.SingleToUInt32Bits(7.5f));
                Put(Pool.RawSlot(Source), 18, BitConverter.SingleToUInt32Bits(8.25f));
            },
            Sound = (x, y) => Event($"S:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}"),
            Base = (sid, flags) => Event($"B:{sid}:{flags}"),
        };

        /// <summary>효과 문자열.</summary>
        public string Trace() => string.Concat(Parts);
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>폭발·소리는 플래그와 추가 비트가 모두 맞을 때만 나고 공통 전처리는 항상 마지막이다.</summary>
    [Fact]
    public void ExplodesOnlyWithFlagAndVisibleExtra()
    {
        var scene = new Scene();
        scene.Prepare(0, 98.0f, 122.0f);
        scene.Body.PreDestroy(Source, ExplosionFlag);
        // 폭발이 바꾼 현재 좌표로 소리를 낸다
        Assert.Equal($"E:50;S:{BitConverter.SingleToUInt32Bits(7.5f)}:{BitConverter.SingleToUInt32Bits(8.25f)};B:50:1048576",
            scene.Trace());
        // 폭발 플래그가 없으면 공통 전처리만 부른다
        scene.Prepare(0, 98.0f, 122.0f);
        scene.Body.PreDestroy(Source, 1);
        Assert.Equal("B:50:1", scene.Trace());
        // 추가 비트 9 가 켜져 있으면 플래그가 있어도 폭발하지 않는다
        scene.Prepare(9, 98.0f, 122.0f);
        scene.Body.PreDestroy(Source, ExplosionFlag);
        Assert.Equal("B:50:1048576", scene.Trace());
    }

    /// <summary>가상 표가 다르면 슬롯을 바꾸기 전에 거부한다.</summary>
    [Fact]
    public void RejectsForeignVtableBeforeEffects()
    {
        var scene = new Scene();
        scene.Prepare(0, 98.0f, 122.0f);
        // 타입 이름이 아니라 가상 표로만 판별한다
        Put(scene.Pool.RawSlot(Source), 0, 0x005034C8);
        Assert.False(scene.Body.Handles(Source));
        Assert.Throws<InvalidOperationException>(() => scene.Body.PreDestroy(Source, ExplosionFlag));
        Assert.Empty(scene.Parts);
        // 거부는 슬롯을 바꾸지 않는다
        Assert.Equal(0x005034C8u, BitConverter.ToUInt32(scene.Pool.Slot(Source)[..4]));
    }

    /// <summary>선택된 경계가 없으면 그 효과 전에 명시 거부한다.</summary>
    [Fact]
    public void RejectsMissingSelectedHook()
    {
        var scene = new Scene();
        scene.Prepare(0, 98.0f, 122.0f);
        var noExplosion = new RawBombPreDestroy(scene.Pool, new BombPreDestroyHooks
        {
            Sound = (_, _) => throw new InvalidOperationException("폭발 없는 장면에서 소리를 요구했다"),
            Base = (_, _) => throw new InvalidOperationException("경계 누락 장면에서 공통 전처리를 요구했다"),
        });
        Assert.Throws<InvalidOperationException>(() => noExplosion.PreDestroy(Source, ExplosionFlag));
        var noBase = new RawBombPreDestroy(scene.Pool, new BombPreDestroyHooks { Explosion = _ => { }, Sound = (_, _) => { } });
        Assert.Throws<InvalidOperationException>(() => noBase.PreDestroy(Source, 0));
        Assert.Empty(scene.Parts);
    }
}
