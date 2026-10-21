using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 종속 조회·삭제의 보호 조건을 검사한다.
/// 원본 fixture 가 담지 않는 거부 경로(삭제 경계 누락·범위 밖/순환 링크·삭제 뒤 손상된 next)를 직접 구성해 확인한다.
/// 기준 판본은 10.78 이다 (W03 삭제·수명 열째 조각, 2026-10-21).
/// </summary>
public sealed class RawContainedLookupTests
{
    /// <summary>부모 슬롯 번호.</summary>
    private const int Parent = 50;

    /// <summary>종속 슬롯 번호.</summary>
    private const int Child = 5;

    /// <summary>자식 슬롯의 타입 번호.</summary>
    private const byte ChildType = 47;

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

    /// <summary>보호 검사용 장면. 풀과 종속 체인을 함께 만든다.</summary>
    private sealed class Scene
    {
        /// <summary>종속 체인을 가진 풀(32,768칸, 서버).</summary>
        public readonly RawSidPool Pool = new(32768, true);

        /// <summary>자식 슬롯을 부모의 종속 머리에 건다. 타입은 조회 대상과 다르게 둔다.</summary>
        public void Link()
        {
            Put(Pool.RawSlot(Parent), 6, Child, 2);
            Pool.RawSlot(Child)[RawSidPool.TypeOffset] = 46;
            Pool.RawSlot(Child)[RawSidPool.StateOffset] = 8;
        }
    }

    /// <summary>삭제 경계가 없으면 만들 수 없다.</summary>
    [Fact]
    public void MissingDeleteHookIsRejected()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => new RawContainedLookup(scene.Pool, null!));
    }

    /// <summary>범위 밖·순환 링크와 삭제 뒤 손상된 next 는 조회 전에 거부한다.</summary>
    [Fact]
    public void MalformedLinksAreRejected()
    {
        var scene = new Scene();
        int calls = 0;
        // 삭제 뒤 next 를 풀 밖 번호로 바꿔 손상을 만든다
        var lookup = new RawContainedLookup(scene.Pool, (sid, flags) =>
        {
            calls++;
            Put(scene.Pool.RawSlot(Child), 4, (uint)scene.Pool.Capacity, 2);
        });
        scene.Link();
        // 자기가 다음을 가리키는 순환은 거부한다
        Put(scene.Pool.RawSlot(Child), 4, Child, 2);
        Assert.Throws<InvalidOperationException>(() => lookup.Find(Parent, ChildType));
        // 삭제 뒤 손상된 next 도 거부한다
        Put(scene.Pool.RawSlot(Child), 4, 0, 2);
        scene.Pool.RawSlot(Child)[RawSidPool.TypeOffset] = ChildType;
        Assert.Throws<InvalidOperationException>(() => lookup.Find(Parent, ChildType, 0, true));
        Assert.Equal(1, calls);
        // next 를 정상으로 되돌리면 타입이 다른 조회도 0 을 돌려준다
        Put(scene.Pool.RawSlot(Child), 4, 0, 2);
        Assert.Equal(0, lookup.Find(Parent, 46));
        // 풀 밖 부모 번호는 거부한다
        Assert.Throws<ArgumentOutOfRangeException>(() => lookup.Find(scene.Pool.Capacity, ChildType));
    }
}
