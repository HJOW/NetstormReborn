using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Bridges;

/// <summary>
/// 다리를 시작(이어 붙이기)할 수 없는 섬 칸 판정 (docs/exe/bridge-pieces.md 8절).
/// 규칙(사용자 확인 2026-09-30): 섬 가장자리 중 **초목이 있는 부분에서는 다리 건설을 시작할 수 없다**.
/// 원본 구현: 가장자리 초목은 edgeFarm 오브젝트이고 typeflags 에 dropBlocking(플래그2 0x10)이 있다.
/// 원본은 오브젝트 발자국 칸의 스폿 지도에 이 비트를 남기고(Squid.cpp FUN_004b02d0), 다리 배치 판정(Rifttype.cpp FUN_0049b510)은
/// 이 비트가 있는 칸을 이어 붙일 곳으로 쓰지 않는다. 같은 비트를 가진 건물·나무·신전·가이저 칸도 마찬가지다.
/// </summary>
public static class BridgeAnchors
{
    /// <summary>타입이 놓기 막음(dropBlocking) 비트를 가졌는지</summary>
    /// <param name="type">타입</param>
    public static bool IsDropBlocking(TypeInfo type) => (type.Flags2 & TypeFlagBits.DropBlocking) != 0;

    /// <summary>
    /// 놓기 막음 칸을 모은다: 저장 오브젝트 중 dropBlocking 타입의 발자국 칸 + 가장자리 초목(edgeFarm) 칸.
    /// </summary>
    /// <param name="objects">맵 오브젝트</param>
    /// <param name="edgeFarmCells">가장자리 초목 칸 (원본은 지면 생성 뒤 edgeFarm 오브젝트로 만든다)</param>
    public static HashSet<(int X, int Y)> DropBlockingCells(IEnumerable<FortMapObject> objects, IEnumerable<(int X, int Y)> edgeFarmCells)
    {
        var cells = new HashSet<(int X, int Y)>(edgeFarmCells);
        // dropBlocking 타입 오브젝트의 발자국 칸을 더한다
        foreach (FortMapObject item in objects.Where(o => IsDropBlocking(o.Object.Type)))
        {
            cells.UnionWith(Footprint.ForType(item.Object.Type.Definition, item.X, item.Y).Cells());
        }
        return cells;
    }

    /// <summary>BridgeGrid 의 섬 가장자리 부착 판정: 놓기 막음 칸이 아니면 붙일 수 있다</summary>
    /// <param name="blocked">놓기 막음 칸</param>
    public static Func<int, int, BridgeLinks, int, bool> CanAttach(IReadOnlySet<(int X, int Y)> blocked) =>
        (x, y, _, _) => !blocked.Contains((x, y));
}
