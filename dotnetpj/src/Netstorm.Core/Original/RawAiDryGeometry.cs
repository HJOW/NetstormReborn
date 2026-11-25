// 가짜 부족 좌표의 원본 선분/수직 양방향 탐색과 실제 디버그 표식 수명을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 가짜 좌표 탐색 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.

// 0041cfc0: 두 float 좌표 차이의 제곱합/제곱근을 중간 float 저장 없이 반환한다.
public static class AiPointDistanceHelper
{
    /// <summary>두 float 좌표의 차이 제곱합/제곱근을 중간 float 저장 없이 반환한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <param name="first">첫 좌표다.</param>
    /// <param name="second">둘째 좌표다.</param>
    /// <returns>유클리드 거리다.</returns>
    public static double Distance(RawPathPoint first, RawPathPoint second)
    {
        double dy = (double)first.Y - second.Y;
        double dx = (double)first.X - second.X;
        return Math.Sqrt(dy * dy + dx * dx);
    }
}

// 원본 현재 경계/디버그 종류/표식 TYPE의 같은 저장소를 빌린다.
public sealed class AiDryGeometryGlobals
{
    /// <summary>00544d14..20의 현재 네 float이며 양끝을 포함한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public Func<AiBuildSearchBounds>? Bounds { get; set; }

    /// <summary>0052f3dc가 29일 때만 실제 표식을 만든다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public Func<uint>? Debug { get; set; }

    /// <summary>005411f8의 현재 TYPE이며 초기값 104다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public Func<uint>? MarkerType { get; set; }
}

// 이미 복원된 실제 지도/가상 생성·kind WORD/Pop/삭제만 선택 위치에서 호출한다.
public sealed class AiDryGeometryHooks
{
    /// <summary>현재 00542514의 256폭 WORD 지도다. 범위 밖은 호출하지 않는다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public Func<int, int, ushort>? Surface { get; set; }

    /// <summary>004af530의 현재 TYPE/flags2다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public Func<uint, uint, Sid>? Create { get; set; }

    /// <summary>004ac220의 kind 93 쓰기와 현재 다리 통지다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public Action<Sid, ushort>? Word { get; set; }

    /// <summary>실제 가상+90의 좌표/flags0이다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public Action<Sid, RawPathPoint, uint>? Pop { get; set; }

    /// <summary>실제 가상+10의 flags0 삭제다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public Action<Sid, uint>? Destroy { get; set; }
}

// 별도 지도나 후보 사본 없이 같은 실제 풀/경계/현재 효과를 사용한다.
// 근거: cpppj o/RawAiDryGeometry.h/.cpp의 RawAiDryGeometry다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.
public sealed class RawAiDryGeometry
{
    /// <summary>원본 활성 디버그 종류다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private const uint DebugKind = 29;

    /// <summary>원본 표식 구분 WORD다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private const ushort MarkerWord = 93;

    /// <summary>물리 지도 한 변이다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private const int Side = 256;

    /// <summary>방향별 필요한 연속 빈칸 수다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private const int Run = 4;

    /// <summary>00500edc의 float 비트 3f7ff972이며 사제의 0.99999와 구분한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private const float CellBias = 0.9999f;

    /// <summary>00500ee4의 중점 상수다. 합 뒤 float 저장 전에 곱한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private const double Half = 0.5;

    /// <summary>표식 순회와 실제 가상 효과가 공유하는 PATCH 풀이다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>경계/디버그/TYPE의 현재 참조다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private readonly AiDryGeometryGlobals _globals;

    /// <summary>현재 지도와 선택된 실제 표식 수명 효과다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    private readonly AiDryGeometryHooks _hooks;

    /// <summary>PATCH 풀과 현재 전역/실제 효과를 빌린다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <param name="pool">표식 순회와 실제 가상 효과가 공유하는 PATCH 풀이다.</param>
    /// <param name="globals">경계/디버그/TYPE의 현재 참조다.</param>
    /// <param name="hooks">현재 지도와 선택된 실제 표식 수명 효과다.</param>
    public RawAiDryGeometry(RawSidPool pool, AiDryGeometryGlobals globals, AiDryGeometryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || pool.Capacity > 65536)
        {
            throw new ArgumentException("AI 가짜좌표 탐색의 PATCH WORD 풀 누락");
        }

        _pool = pool;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>선택된 실제 하위가 없으면 앞서 수행한 효과를 유지하고 그 위치에서 거부한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 가짜좌표 탐색의 실제 하위 미연결");

    /// <summary>현재 경계의 less/equal만 허용한다. NaN은 제외된다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <param name="point">검사할 좌표다.</param>
    /// <returns>경계 안이면 참이다.</returns>
    private bool Inside(RawPathPoint point)
    {
        AiBuildSearchBounds bounds = Need(_globals.Bounds)();
        return bounds.Left <= point.X && point.X <= bounds.Right && bounds.Top <= point.Y && point.Y <= bounds.Bottom;
    }

    /// <summary>원본 CRT signed64 절삭의 하위 DWORD이며 잘못된 FISTP의 하위값은 0이다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭 하위 DWORD의 signed 해석이다.</returns>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((int)(long)Math.Truncate(value));
    }

    /// <summary>각 축의 편향 합은 ftol 전에 float로 저장하지 않는다. 지도 밖은 빈칸으로 계산한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <param name="point">검사할 좌표다.</param>
    /// <returns>빈칸이면 참이다.</returns>
    private bool Empty(RawPathPoint point)
    {
        int x = Ftol((double)point.X + CellBias);
        int y = Ftol((double)point.Y + CellBias);
        return x < 0 || y < 0 || x >= Side || y >= Side || Need(_hooks.Surface)(x, y) == 0;
    }

    /// <summary>원본은 SID 1에서 Next를 먼저 호출하며, 삭제 전에 다음 SID를 미리 잡는다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    public void ClearMarkers()
    {
        int end = _pool.Capacity;
        uint cursor = 1;
        // 원본 iterator는 free bit만 건너뛰고 dead 객체는 방문한다.
        void Next()
        {
            ++cursor;
            while (cursor < end && (_pool.Slot((int)cursor)[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0)
            {
                ++cursor;
            }
        }

        Next();
        // 가상 삭제 효과 뒤 현재 TYPE 전역/슬롯을 읽으며 논리 순서를 바꾸지 않는 반복문이다.
        while (cursor >= 1 && cursor < (uint)_pool.Capacity)
        {
            var sid = new Sid(unchecked((ushort)cursor));
            Next();
            ReadOnlySpan<byte> raw = _pool.Slot(sid.Value);
            ushort word = (ushort)(raw[12] | ((ushort)raw[13] << 8));
            if (raw[10] == Need(_globals.MarkerType)() && word == MarkerWord)
            {
                Need(_hooks.Destroy)(sid, 0);
            }
        }
    }

    /// <summary>생성/WORD/Pop 경계가 실패해도 원본처럼 앞선 생성이나 쓰기를 취소하지 않는다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <param name="point">표식할 좌표다.</param>
    public void Mark(RawPathPoint point)
    {
        Sid sid = Need(_hooks.Create)(Need(_globals.MarkerType)(), 2);
        Need(_hooks.Word)(sid, MarkerWord);
        Need(_hooks.Pop)(sid, point, 0);
    }

    /// <summary>진행이 경계에서 정지하는 원본 경로에 임의 성공/예산/대체 좌표를 넣지 않는다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <param name="source">시작 좌표다.</param>
    /// <param name="target">목표 좌표다.</param>
    /// <returns>찾은 빈칸 후보이며 없으면 마지막에 이른 점이다.</returns>
    public RawPathPoint Candidate(RawPathPoint source, RawPathPoint target)
    {
        if (Need(_globals.Debug)() == DebugKind)
        {
            ClearMarkers();
            Mark(source);
            Mark(target);
        }

        float distance = (float)AiPointDistanceHelper.Distance(target, source);
        double inverse = 1.0 / distance;
        float dx = (float)(((double)target.X - source.X) * inverse);
        float dy = (float)(((double)target.Y - source.Y) * inverse);
        var middle = new RawPathPoint((float)(((double)target.X + source.X) * Half), (float)(((double)target.Y + source.Y) * Half));
        RawPathPoint along = source;
        float progress = 0;
        // 디버그가 꺼져도 선분의 float 누적/횟수 진행은 원래 순서대로 수행하는 반복문이다.
        while ((double)progress < distance)
        {
            along.X = (float)((double)along.X + dx);
            along.Y = (float)((double)along.Y + dy);
            if (Need(_globals.Debug)() == DebugKind)
            {
                Mark(along);
            }

            double advanced = (double)progress + 1.0;
            progress = (float)advanced;
            if (!(advanced < distance))
            {
                break;
            }
        }

        RawPathPoint left = middle;
        RawPathPoint right = middle;
        int leftRun = 0;
        int rightRun = 0;
        // 왼쪽을 먼저 움직이고 오른쪽도 같은 반복에서 검사한 뒤 왼쪽 성공을 우선 반환하는 반복문이다.
        for (;;)
        {
            if (rightRun >= Run)
            {
                return leftRun >= Run ? left : right;
            }

            if (Inside(left))
            {
                left.X = (float)((double)left.X - dy);
                left.Y = (float)((double)left.Y + dx);
                leftRun = Empty(left) ? leftRun + 1 : 0;
            }

            if (Inside(right))
            {
                right.X = (float)((double)right.X + dy);
                right.Y = (float)((double)right.Y - dx);
                rightRun = Empty(right) ? rightRun + 1 : 0;
            }

            if (Need(_globals.Debug)() == DebugKind)
            {
                Mark(left);
                Mark(right);
            }

            if (leftRun >= Run)
            {
                return left;
            }
        }
    }

    /// <summary>같은 실제 월드 연결을 확인한다. 2026-11-25 R4-5b 가짜 좌표 탐색 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
