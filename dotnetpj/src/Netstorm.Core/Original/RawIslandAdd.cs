// 편집기의 실제 한 섬 배치를 지형·population·Graph·Territory 저장까지 조립한다.
using System;
using System.Collections.Generic;

// UI 드래그 수명과 실제 월드 변경 부모를 구별하는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Original;

// 편집기 섬 추가 시 필요한 상위 콜백을 묶는 훅 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class IslandAddHooks
{
    // 0046e0d0의 Fort 모드 진단 뒤 본문을 계속하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action? FortAssertion { get; set; }

    // 같은 현재 Builder/지역으로 실제 0046da70/정상 지형 본문을 실행하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<RawIslandBuilderState, int>? Generate { get; set; }

    // 00463110의 실제 전체 Graph 재구성이며 false를 그대로 전달하는 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<bool>? Graph { get; set; }

    // Builder의 현재 owner를 사용한 004bf6d0 실제 전체 저장 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<uint>? Territory { get; set; }
}

// 실제 Place 반환 지역을 그 뒤 모든 하위에 전달하며 현재 owner는 마지막 저장 직전에 읽는 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class RawIslandAdd
{
    // 실제 현재 월드 청크 지도 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly ChunkMap _world;

    // 실제 현재 섬 런타임 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawIslandRuntime _islands;

    // 패턴 조회에 사용하는 프레임 배열 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    // 현재 퍼즐 조각 TYPE 번호를 읽는 콜백 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Func<uint> _getPuzzlePieceType;

    // 공유 섬 빌더 상태 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawIslandBuilderState _state;

    // 현재 Fort 모드 여부를 읽는 콜백 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Func<bool> _getFort;

    // 섬 인구 생성기 참조 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawIslandPopulation _population;

    // 기존 완료 Builder 진단 콜백 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly Action<RawIslandBuilderAssertion>? _assertion;

    // 실제 큰 하위 장치를 연결하는 훅 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly IslandAddHooks _hooks;

    // 실제 단일 World/Pool/builder 참조만 빌리는 생성자다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public RawIslandAdd(
        ChunkMap world,
        RawIslandRuntime islands,
        IReadOnlyList<RiftTypeFrames> frames,
        Func<uint> getPuzzlePieceType,
        RawIslandBuilderState builderState,
        Func<bool> getFort,
        RawIslandPopulation population,
        Action<RawIslandBuilderAssertion>? assertion,
        IslandAddHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(getPuzzlePieceType);
        ArgumentNullException.ThrowIfNull(builderState);
        ArgumentNullException.ThrowIfNull(getFort);
        ArgumentNullException.ThrowIfNull(population);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(islands.Pool(), population.Pool()))
        {
            throw new ArgumentException("실제섬추가의Pool불일치");
        }

        _world = world;
        _islands = islands;
        _frames = frames;
        _getPuzzlePieceType = getPuzzlePieceType;
        _state = builderState;
        _getFort = getFort;
        _population = population;
        _assertion = assertion;
        _hooks = hooks;
    }

    // 0046e0d0의 원문 인자를 직접 받고 성공/실패 부분 효과를 그대로 보존하는 섬 추가 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint Add(ReadOnlySpan<byte> record, uint territory)
    {
        if (record.Length < 6)
        {
            throw new ArgumentException("섬 레코드는 최소 6바이트여야 합니다.", nameof(record));
        }

        if (!_getFort())
        {
            if (_hooks.FortAssertion is null)
            {
                throw new InvalidOperationException("섬추가Fort진단미연결");
            }

            _hooks.FortAssertion();
        }

        RawIslandBuilder builder = new(_world, _world, _islands, _frames, _getPuzzlePieceType, _state, _assertion);
        uint region = builder.Place(record[..6], territory, false);
        if (_hooks.Generate is null)
        {
            throw new InvalidOperationException("섬추가실제지형생성미연결");
        }

        _hooks.Generate(_state, unchecked((int)region));
        _population.Populate(unchecked((int)region));
        if (_hooks.Graph is null)
        {
            throw new InvalidOperationException("섬추가실제Graph미연결");
        }

        _hooks.Graph(false);
        if (_hooks.Territory is null)
        {
            throw new InvalidOperationException("섬추가실제Territory저장미연결");
        }

        _hooks.Territory(_state.Owner);
        return region;
    }

    // 0047fb90의 CanPlace → flag&fd → 실제 SetMap의 bounds/owner 1 → 현재 count → Add까지 실행하는 드롭 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public bool TryDrop(ChunkMap preview, Span<byte> record)
    {
        ArgumentNullException.ThrowIfNull(preview);
        if (record.Length < 6)
        {
            throw new ArgumentException("섬 레코드는 최소 6바이트여야 합니다.", nameof(record));
        }

        if (!RawIslandPlacement.CanPlaceIsland(
            OriginalEdition.Patch1078,
            preview,
            _frames,
            _getPuzzlePieceType,
            _state,
            record[..6],
            _assertion))
        {
            return false;
        }

        record[1] &= 0xfd;
        _state.Bounds = ChunkMap.DefaultChunkRect();
        _state.Owner = 1u;
        _ = Add(record[..6], _islands.Count());
        return true;
    }
}
