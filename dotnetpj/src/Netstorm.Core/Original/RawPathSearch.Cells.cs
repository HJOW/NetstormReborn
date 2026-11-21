// 같은 원문 거리/큐에서 BFS 방문과 역추적 방향 변경점을 계산한다.
namespace Netstorm.Core.Original;

/// <summary>RawPathSearch.cpp의 SearchCells이며 작업 배열의 미사용 흔적도 보존한다.</summary>
public sealed partial class RawPathSearch
{
    /// <summary>지정 출발 칸에서 BFS하고 끝 칸에서 역추적해 원문 배열 뒤에 경로를 추가한다.</summary>
    public bool SearchCells(RawPathRuntime output, int sx, int sy, int dx, int dy, bool cardinalOnly)
    {
        if (!Inside(sx, sy) || !Inside(dx, dy)) throw new ArgumentOutOfRangeException(nameof(sx), "BFS 시작/끝 칸");
        if (_sharedWork is not null) _sharedWork.Initialized = true; Array.Clear(Work.Distance);
        if (Pool.Edition == OriginalEdition.Patch1078 && (_state.SharedPolicySelector is { } shared ? shared.Value == 5 : _state.PolicySelector == 5)) _state.DirectionPrerequisite[3] = 5;
        Work.Queue[0] = (byte)sx; Work.Queue[1] = (byte)sy; Work.Distance[sy * 256 + sx] = 1; int tail = 1, head = 0, layerEnd = 1; short distance = 2;
        // 원문은 방문 순서 앞 네 직각 방향으로만 지면을 확장한다.
        for (;;)
        {
            if (head >= RawPathSearchWork.QueueEntries) throw new ArgumentOutOfRangeException(nameof(head), "BFS 원본 큐 읽기 넘침");
            if (head == layerEnd) { distance = unchecked((short)(distance + 1)); layerEnd = tail; }
            int x = Work.Queue[head * 2], y = Work.Queue[head * 2 + 1], current = _hash.Cell(0, x, y); if (current == 0) return false; if (x == dx && y == dy) break; uint admitted = 0;
            // 양옆 방향 전제를 확인한 후보만 원문 큐에 등록한다.
            for (int index = 0; index < 4; index++)
            {
                int direction = Direction(index); uint required = _state.DirectionPrerequisite[direction]; if ((admitted & required) != required) continue;
                int nx = x + DirectionX[direction], ny = y + DirectionY[direction]; if (!Inside(nx, ny)) continue; int slot = ny * 256 + nx; byte spot = _spots[slot];
                if (Work.Distance[slot] != 0 || (spot & 6) == 0 || (spot & 8) != 0) continue; int next = _hash.Cell(0, nx, ny); if (next == 0) throw new InvalidOperationException("BFS 허용 지면의 표면0"); if (!Connects(current, next, direction)) continue;
                Work.Queue[tail * 2] = (byte)nx; Work.Queue[tail * 2 + 1] = (byte)ny; Work.Distance[slot] = distance; tail = (tail + 1) & QueueMask; admitted |= 1u << direction;
            }
            head++; if (head == tail) return false;
        }
        var points = new List<RawPathPoint>(); var directions = new List<int>(); var times = new List<double>();
        // 원문은 기존 count/prefix와 각 QWORD 시각을 보존한다.
        for (int index = 0; index < output.Count; index++) { points.Add(output.Point(index)); directions.Add(output.Direction(index)); times.Add(output.Time(index)); }
        int px = dx, py = dy, remaining = Work.Distance[py * 256 + px] - 1, previousDirection = -1; if (remaining == 0) Append(points, directions, new(px, py), 0, output);
        // 각 거리 단계의 방향 비용과 동일 좌표 두 변경점을 원문 순서대로 고른다.
        while (remaining > 0)
        {
            int current = _hash.Cell(0, px, py), alternate = remaining; if (!cardinalOnly && remaining > 1) alternate--; int bestCost = 9999, bestDirection = 0; uint admitted = 0;
            // 일반 경로는 대각선까지 여덟 후보를 비교하고 직각 경로는 앞 네 후보만 쓴다.
            for (int index = 0; index < (cardinalOnly ? 4 : 8); index++)
            {
                int direction = Direction(index), nx = px + DirectionX[direction], ny = py + DirectionY[direction]; if (!Inside(nx, ny)) continue; short value = Work.Distance[ny * 256 + nx]; if (value != remaining && value != alternate) continue;
                int next = _hash.Cell(0, nx, ny); if (next == 0) throw new InvalidOperationException("BFS 역추적 표면0"); uint required = _state.DirectionPrerequisite[direction]; if ((admitted & required) != required || !Connects(current, next, direction)) continue;
                int cost = direction == previousDirection ? 10 : 15; if ((direction & 1) != 0) cost -= 10; if (_state.AvoidFlagTwo && (Pool.Slot(next)[12] & 6) == 2) cost += 30;
                if (cost < bestCost) { bestCost = cost; bestDirection = direction; } admitted |= 1u << direction;
            }
            if (bestCost == 9999) throw new InvalidOperationException("BFS 역추적 후보 없음");
            if (bestDirection != previousDirection) { if (previousDirection != -1) Append(points, directions, new(px, py), previousDirection, output); Append(points, directions, new(px, py), bestDirection, output); previousDirection = bestDirection; }
            px += DirectionX[bestDirection]; py += DirectionY[bestDirection]; remaining = Work.Distance[py * 256 + px] - 1;
        }
        if (previousDirection != -1) Append(points, directions, new(px, py), previousDirection, output); if (px != sx || py != sy) throw new InvalidOperationException("BFS 역추적 시작 불일치");
        // 새 꼭짓점의 시각만0으로 확장하고 기존 시각을 초기화하지 않는다.
        while (times.Count < points.Count) times.Add(0); output.SetPath(points.ToArray(), directions.ToArray(), times.ToArray()); return true;
    }
}
