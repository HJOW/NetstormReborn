// 네 다리 전략의 실제 연결 판정·무작위 시작점·빈 사각형·재시도 기한이다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 다리 계획 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.

// 실제 후보 모듈이 소유한 같은 월드와 현재 난수/시각만 참조한다.
// 근거: cpppj o/RawAiBridgePlanning.h/.cpp의 RawAiBridgePlanning다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.
public sealed class RawAiBridgePlanning
{
    /// <summary>같은 실제 본체/풀/TYPE/코드/지도 getter의 유일한 소유 모듈이다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    private readonly RawAiBridgeCandidates _candidates;

    /// <summary>다른 게임 계산과 같은 00532710 난수 상태다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    private readonly GameRandom _random;

    /// <summary>현재 0055b4d0 QWORD를 기한을 쓸 때 읽는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    private readonly Func<double>? _now;

    /// <summary>현재 실제 object 그래프이며 필요한 연결 판정에서만 호출한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    private readonly Func<Sid, uint>? _graphFor;

    /// <summary>같은 실제 후보 모듈과 현재 난수/시각만 참조한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="candidates">같은 실제 본체/풀/TYPE/코드/지도 getter의 유일한 소유 모듈이다.</param>
    /// <param name="random">다른 게임 계산과 같은 난수 상태다.</param>
    /// <param name="now">현재 게임 시각을 읽는다.</param>
    /// <param name="graphFor">현재 실제 object 그래프를 읽는다.</param>
    public RawAiBridgePlanning(RawAiBridgeCandidates candidates, GameRandom random, Func<double>? now, Func<Sid, uint>? graphFor)
    {
        ArgumentNullException.ThrowIfNull(candidates);
        ArgumentNullException.ThrowIfNull(random);
        _candidates = candidates;
        _random = random;
        _now = now;
        _graphFor = graphFor;
    }

    /// <summary>원본 DWORD의 signed 해석을 구현 의존 변환 없이 적용한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>실제 __ftol의 signed64 하위 DWORD이며 indefinite 하위는 0이다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 float다.</param>
    /// <returns>절삭한 낮은 DWORD다.</returns>
    private static uint Truncate(float value)
    {
        if (!float.IsFinite(value) || (double)value < -9223372036854775808.0 || (double)value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)(long)value);
    }

    /// <summary>실제 슬롯의 비정렬 DWORD만 읽는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트 범위다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int at)
    {
        if (at < 0 || at > raw.Length || raw.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 다리 계획 DWORD 범위");
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(at, 4));
    }

    /// <summary>선택된 실제 하위 누락을 가짜 결과로 처리하지 않는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 다리 계획 실제 하위 미연결");

    /// <summary>같은 현재 시각 값을 읽는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <returns>현재 게임 시각이다.</returns>
    private double Now() => Need(_now)();

    /// <summary>원본은 free/dead 필터나 graph 재비교를 가장자리 판정에 추가하지 않는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="x">검사할 가로 칸이다.</param>
    /// <param name="y">검사할 세로 칸이다.</param>
    /// <param name="side">맞아야 할 방향 글자다.</param>
    /// <returns>가장자리 다리면 참이다.</returns>
    private bool Edge(uint token, uint x, uint y, byte side)
    {
        if (x >= 256 || y >= 256)
        {
            return false;
        }

        ushort surface = _candidates.Surfaces[(int)y * 256 + (int)x];
        if (surface == 0)
        {
            return false;
        }

        ReadOnlySpan<byte> raw = _candidates.Pool.Slot(surface);
        byte type = raw[RawSidPool.TypeOffset];
        if ((uint)type != Need(_candidates.Globals.Bridge)() || raw[34] != _candidates.Field(token, 0x260))
        {
            return false;
        }

        uint frame = Read(raw, 36);
        IReadOnlyList<RiftTypeFrames> frames = _candidates.Frames;
        if (type < 70 || type >= frames.Count || frame >= (uint)frames[type].Codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "AI 다리 연결 현재 코드 범위");
        }

        return frames[type].Codes[(int)frame].Side == side;
    }

    /// <summary>graph 하위 뒤 현재 본체와 실제 목표 좌표를 다시 읽으며 J12/K12로 범위를 넓히지 않는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="geyser">연결을 잴 간헐천 SID다.</param>
    /// <returns>이미 닿아 있으면 참이다.</returns>
    public bool Connected(uint token, Sid geyser)
    {
        _ = _candidates.Body(token);
        if (_graphFor is null)
        {
            throw new InvalidOperationException("AI 다리 연결 현재 graph 미연결");
        }

        uint graph = _graphFor(geyser);
        if (graph != _candidates.Field(token, 0x298))
        {
            return false;
        }

        ReadOnlySpan<byte> raw = _candidates.Pool.Slot(geyser.Value);
        uint x = Truncate(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(14, 4))));
        uint y = Truncate(BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(18, 4))));
        // x-2..x에서 북/남의 J를 이 순서로 검사하는 반복문이다.
        for (uint offset = 0; offset < 3; offset++)
        {
            if (Edge(token, unchecked(x - 2u + offset), unchecked(y - 3u), (byte)'J') || Edge(token, unchecked(x - 2u + offset), unchecked(y + 1u), (byte)'J'))
            {
                return true;
            }
        }

        // y-2..y에서 서/동의 K를 이 순서로 검사하는 반복문이다.
        for (uint offset = 0; offset < 3; offset++)
        {
            if (Edge(token, unchecked(x - 3u), unchecked(y - 2u + offset), (byte)'K') || Edge(token, unchecked(x + 1u), unchecked(y - 2u + offset), (byte)'K'))
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>실제 count 전체 DWORD가 0인지만 검사하고 다른 본문 바이트를 보존한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void DelayRetry(uint token)
    {
        double delay = _candidates.Field(token, 0x350) == 0 ? 40.0 : 120.0;
        ulong bits = BitConverter.DoubleToUInt64Bits(Now() + delay);
        _candidates.Field(token, 0x5d0, (uint)bits);
        _candidates.Field(token, 0x5d4, (uint)(bits >> 32));
    }

    /// <summary>고정 graph/owner와 절삭한 사각형을 보관하지만 현재 지도/코드는 각 후보마다 읽는다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="bounds">고른 사각형이다.</param>
    /// <returns>같은 graph/owner의 첫 열린 지면이며 없으면 {-10000,-10000}이다.</returns>
    public RawPathPoint RandomStart(uint token, AiBuildSearchBounds bounds)
    {
        uint graph = _candidates.Field(token, 0x298);
        uint owner = _candidates.Field(token, 0x260);
        // 원본의 bottom→top→right→left CRT 순서다.
        uint bottom = Truncate(bounds.Bottom);
        uint top = Truncate(bounds.Top);
        uint right = Truncate(bounds.Right);
        uint left = Truncate(bounds.Left);
        // 상한 제외 y→x 두 난수를 항상 소비하고 최대 20번 시도하는 반복문이다.
        for (int remaining = 20; remaining > 0; remaining--)
        {
            uint y = unchecked(_random.Next(unchecked(bottom - top)) + top);
            uint x = unchecked(_random.Next(unchecked(right - left)) + left);
            if (x >= 256 || y >= 256)
            {
                continue;
            }

            ushort surface = _candidates.Surfaces[(int)y * 256 + (int)x];
            if (surface != 0 && _candidates.Graph(new Sid(surface)) == graph && _candidates.Pool.Slot(surface)[34] == owner && (_candidates.Spot(Signed(x) - 1, Signed(y)) == 0 || _candidates.Spot(Signed(x) + 1, Signed(y)) == 0 || _candidates.Spot(Signed(x), Signed(y) - 1) == 0 || _candidates.Spot(Signed(x), Signed(y) + 1) == 0) && _candidates.Open(new Sid(surface), Signed(x), Signed(y)).Value != 0)
            {
                return new RawPathPoint(Signed(x), Signed(y));
            }
        }

        return new RawPathPoint(-10000, -10000);
    }

    /// <summary>행 경계의 실제 flat 주소도 보존하며 물리 지도 바깥은 기존 spot getter가 거부한다. 2026-11-25 R4-5b 다리 계획 이식에서 추가됐다.</summary>
    /// <param name="left">자를 왼쪽 칸이다.</param>
    /// <param name="top">자를 위 칸이다.</param>
    /// <param name="right">자를 오른쪽 칸이다.</param>
    /// <param name="bottom">자를 아래 칸이다.</param>
    /// <returns>사각형이 모두 비어 있으면 참이다.</returns>
    public bool RectangleFree(int left, int top, int right, int bottom)
    {
        // 원본의 inclusive X 순회이며 반전 폭은 빈 사각형이다.
        for (uint x = unchecked((uint)left); Signed(x) <= right; x++)
        {
            // 각 열의 inclusive Y 순서에서 첫 점유 즉시 실패하는 반복문이다.
            for (uint y = unchecked((uint)top); Signed(y) <= bottom; y++)
            {
                if (_candidates.Spot(Signed(x), Signed(y)) != 0)
                {
                    return false;
                }
            }
        }

        return true;
    }
}
