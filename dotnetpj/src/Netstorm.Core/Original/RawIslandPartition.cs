// 중앙섬 군도의 실제 지면 연결을 섬 목록의 독립 지역으로 나눈다.
using System;
using System.Collections.Generic;

// 원문 0046f7d0/0046ed50/0046ec00의 현재 표면/프레임/지역을 사용하는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Original;

// 좌표/프레임을 스냅샷으로 대체하지 않고 원문 순회와 지역 쓰기 효과를 유지하는 군도 분리 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class RawIslandPartition
{
    // 같은 실제 표면 SID를 읽는 현재 Pool 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawSidPool _pool;

    // 0046f7d0이 직접 읽는 같은 65536 WORD hash0 머리 배열 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly ushort[] _surfaces;

    // 컴포넌트 생성 때 현재 지면 TYPE가 선택하는 실제 코드 표 배열 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    // 005411dc의 현재 전체 지면 TYPE DWORD를 읽는 콜백 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Func<uint> _getTerrainType;

    // 실제 100 기록의 count/Add/Bounds를 사용하는 장기 목록 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawIslandRuntime _islands;

    // flood 래퍼가 매번 증가시키는 같은 0059a8b0 갱신 콜백 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Action _incrementWorldRedraw;

    // 004acda0의 현재 TYPE 진단/다리/noRegion 관문을 사용하는 지역 조회 콜백 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Func<int, uint> _getRegion;

    // 004acd20의 현재 TYPE 진단/같은 raw WORD 쓰기를 수행하는 지역 설정 콜백 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Action<int, uint> _setRegion;

    // 모든 현재 자료/장치가 Split이 끝날 때까지 유효하도록 검증하고 연결하는 생성자다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public RawIslandPartition(
        RawSidPool pool,
        ushort[] surfaces,
        IReadOnlyList<RiftTypeFrames> frames,
        Func<uint> getTerrainType,
        RawIslandRuntime islands,
        Action incrementWorldRedraw,
        Func<int, uint> getRegion,
        Action<int, uint> setRegion)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(surfaces);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(getTerrainType);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(incrementWorldRedraw);
        ArgumentNullException.ThrowIfNull(getRegion);
        ArgumentNullException.ThrowIfNull(setRegion);
        if (pool.Edition != OriginalEdition.Patch1078 ||
            !ReferenceEquals(islands.Pool(), pool) ||
            surfaces.Length != 65536)
        {
            throw new ArgumentException("군도 분리의 실제10.78 Pool/표면/지역 장치 불일치");
        }

        _pool = pool;
        _surfaces = surfaces;
        _frames = frames;
        _getTerrainType = getTerrainType;
        _islands = islands;
        _incrementWorldRedraw = incrementWorldRedraw;
        _getRegion = getRegion;
        _setRegion = setRegion;
    }

    // y행/x열 순서로 지역 0의 지면을 찾고 실제 raw 좌표에서 flood하여 새 섬/경계를 기록하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint Split()
    {
        uint count = 0u;
        // 원문 바깥 루프는 hash0의 각 256칸 행을 순회하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int y = 0; y < 256; ++y)
        {
            // 같은 행을 왼쪽부터 읽으며 TYPE 일치/현재 지역 0만 새 컴포넌트 후보로 삼는 내부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int x = 0; x < 256; ++x)
            {
                int sid = Surface(x, y);
                if (sid == 0)
                {
                    continue;
                }

                ReadOnlySpan<byte> raw = _pool.Slot(sid);
                uint terrainType = _getTerrainType();
                if (raw[10] != terrainType || _getRegion(sid) != 0u)
                {
                    continue;
                }

                if (terrainType >= (uint)_frames.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(terrainType), "군도 분리의 현재 지면 TYPE 범위");
                }

                IReadOnlyList<RiftFrameCode> codes = _frames[(int)terrainType].Codes;
                uint region = _islands.Add(0u, 0xff);
                int startX = PartitionCoordinate(BitConverter.UInt32BitsToSingle(PartitionRead(raw, 14)));
                int startY = PartitionCoordinate(BitConverter.UInt32BitsToSingle(PartitionRead(raw, 18)));
                FortRandomRectangle bounds = Flood(startX, startY, region, codes);
                _islands.Bounds(unchecked((int)region), bounds);
                ++count;
            }
        }

        return count;
    }

    // 각 필드의 낮은 바이트부터 지정 폭만 결합해 읽는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static uint PartitionRead(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        uint value = 0u;
        // 지정된 바이트 폭만큼 리틀엔디안 순서로 결합하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int index = 0; index < width; ++index)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }

        return value;
    }

    // 원문은 스캔 칸이 아닌 현재 지면의 raw X/Y를 절삭해 flood 시작점으로 사용하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static int PartitionCoordinate(float value)
    {
        const double bound = 9223372036854775808.0;
        if (!float.IsFinite(value) || value < -bound || value >= bound)
        {
            return 0;
        }

        return unchecked((int)(uint)(ulong)(long)value);
    }

    // 원문 정수 좌표 관문 뒤 같은 hash0 WORD를 읽는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private int Surface(int x, int y)
    {
        return (uint)x >= 256u || (uint)y >= 256u
            ? 0
            : _surfaces[(y * 256) + x];
    }

    // 북/동/남/서 순서로 현재 지역을 지정하고 좌표 경계를 누적하는 깊이 우선 탐색 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private FortRandomRectangle Flood(int x, int y, uint region, IReadOnlyList<RiftFrameCode> codes)
    {
        FortRandomRectangle bounds = new(257f, 257f, -1f, -1f);
        List<(int X, int Y)> work = new() { (x, y) };
        // 같은 컴포넌트의 현재 연결/지역만 읽으며 방문 기록은 실제 raw 지역 WORD에 남기는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        while (work.Count > 0)
        {
            (int px, int py) = work[^1];
            work.RemoveAt(work.Count - 1);
            int sid = Surface(px, py);
            if (sid == 0)
            {
                continue;
            }

            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            if (raw[10] != _getTerrainType() || _getRegion(sid) == region)
            {
                continue;
            }

            _setRegion(sid, region);
            bounds = new FortRandomRectangle(
                Math.Min(bounds.Left, (float)px),
                Math.Min(bounds.Top, (float)py),
                Math.Max(bounds.Right, (float)px),
                Math.Max(bounds.Bottom, (float)py));
            raw = _pool.Slot(sid);
            int frame = unchecked((int)PartitionRead(raw, 36));
            if (frame < 0 || frame >= codes.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(frame), "군도 flood의 실제 프레임 코드 범위");
            }

            int mask = TerrainBuilder.Connection((char)codes[frame].Side);
            if ((mask & 8) != 0)
            {
                work.Add((px - 1, py));
            }

            if ((mask & 4) != 0)
            {
                work.Add((px, py + 1));
            }

            if ((mask & 2) != 0)
            {
                work.Add((px + 1, py));
            }

            if ((mask & 1) != 0)
            {
                work.Add((px, py - 1));
            }
        }

        _incrementWorldRedraw();
        return bounds;
    }
}
