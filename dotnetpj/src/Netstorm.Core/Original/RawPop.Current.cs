using System.Buffers.Binary;

// 현재 TYPE·SHP·진단·가상 호출을 원본 Pop/Activate 부모의 도달 시점에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>일반 Pop의 PATCH 실제 조회와 독립 명시 입력을 같은 원문 부모로 실행한다.</summary>
public sealed partial class RawPop
{
    /// <summary>World 수명의 실제 직렬화·방송 writer이며 선택된 송신에서만 요구한다.</summary>
    private Action<int, uint>? _transmission;
    /// <summary>현재 Client의 조회·진단을 빌리는 함수 객체 묶음이다.</summary>
    private SquidPopDiagnostics _diagnostics = new();

    /// <summary>선택한 장치만 값으로 가져와 실행 중 자기 재연결에도 현재 호출을 유지한다.</summary>
    private static T NeedCurrent<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("Pop의 선택된 현재 실제 장치 미연결");

    /// <summary>현재 원문 writer를 빌린다. null은 송신을 선택했을 때 명시 실패한다.</summary>
    public void BindTransmission(Action<int, uint>? transmission) => _transmission = transmission;

    /// <summary>판본별 같은 물리 길이의 현재 TYPE와 진단·지연 SHP 효과를 값 복사로 연결한다.</summary>
    public void BindDiagnostics(SquidPopDiagnostics diagnostics)
    {
        ArgumentNullException.ThrowIfNull(diagnostics);
        if (diagnostics.Types.Count != 0 && diagnostics.Types.Count != (_pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
            throw new ArgumentException("Pop 현재 TYPE의 판본·길이 오류", nameof(diagnostics));
        _diagnostics = diagnostics.Copy();
    }

    /// <summary>실제 PATCH World가 사전 TYPE/SHP 조회 없이 같은 현재 부모에 진입한다.</summary>
    public RawPopResult PopCurrent(int sid, float x, float y, uint flags = 0)
    {
        if (_pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("지연 현재 Pop은10.78 전용");
        return PopPatch(sid, x, y, flags, null, []);
    }

    /// <summary>프레임 재등록에 필요한 동일 풀·표시기·Unpop 해시/지도 대여를 구성 시점에 확인한다.</summary>
    public void ValidateFrameBinding(RawSidPool pool, SquidDisplay display, RawUnpop unpop)
    {
        ArgumentNullException.ThrowIfNull(unpop);
        if (!ReferenceEquals(pool, _pool) || !ReferenceEquals(display, _display))
            throw new ArgumentException("프레임 Pop 풀·표시 연결 불일치");
        unpop.ValidateFrameBinding(pool, display, _hash, _spots);
    }

    /// <summary>현재 대여자만 미니맵 연결을 해제한다. 후속으로 설치한 다른 관측자는 유지한다.</summary>
    public bool ReleaseMetadisplay(IRawPopMetadisplay? expected)
    {
        if (expected is null || !ReferenceEquals(expected, _metadisplay)) return false;
        _metadisplay = null; return true;
    }

    /// <summary>실제 현재 원문 부모의 조회·공간·가상 효과를 연결하며 전체 선행 검사를 넣지 않는다.</summary>
    private RawPopResult PopPatch(int sid, float x, float y, uint flags, OriginalTypeRecord? fallback, float[] size)
    {
        var effects = new SquidPopParentHooks
        {
            Type = number =>
            {
                IReadOnlyList<OriginalTypeRecord> types = _diagnostics.Types;
                if (types.Count == 0)
                {
                    if (fallback is null) throw new InvalidOperationException("Pop 현재 전체 TYPE 미연결");
                    return new SquidPopTypeReference(() => fallback);
                }
                if (number >= types.Count) throw new ArgumentOutOfRangeException(nameof(number), "Pop 현재 TYPE 번호 범위");
                return new SquidPopTypeReference(() => types[number]);
            },
            FrameSize = target =>
            {
                Func<int, float[]>? current = _diagnostics.FrameSize;
                if (current is not null) return current(target);
                if (_display is SquidDisplay display)
                {
                    ReadOnlySpan<byte> raw = _pool.Slot(target);
                    return display.FrameSize(_pool.Edition, raw, BinaryPrimitives.ReadInt32LittleEndian(raw[36..]));
                }
                if (fallback is null) throw new InvalidOperationException("Pop 현재 SHP 조회 미연결");
                return size;
            },
            WorldCells = () => { Func<int>? current = _diagnostics.WorldCells; return current is not null ? current() : 256; },
            Metamap = () => { Func<bool>? current = _diagnostics.Metamap; return current is not null ? current() : true; },
            Trace = (level, text) => NeedCurrent(_diagnostics.Trace)(level, text),
            Assertion = (expression, file, line) => NeedCurrent(_diagnostics.Assertion)(expression, file, line),
            MiniMap = (target, _) => _metadisplay?.Enqueue(target),
            Side = (cx, cy, bx, by) =>
            {
                IRawPopBuildingSide current = _building ?? throw new InvalidOperationException("Pop 실제 건물 옆면 미연결");
                current.PopSide(cx, cy, bx, by);
            },
            Update = (target, requested) =>
            {
                ReadOnlySpan<byte> raw = _pool.Slot(target);
                if (!OriginalUnpopDisplay.Supports(_pool.Edition, BinaryPrimitives.ReadUInt32LittleEndian(raw), requested))
                    throw new InvalidOperationException("Pop 현재 파생 표시 갱신 미복원");
                _display?.Update(raw, requested);
            },
            FirstPop = target =>
            {
                ReadOnlySpan<byte> raw = _pool.Slot(target);
                if (!RawSquidFactory.SupportsConstructedVtable(_pool.Edition, BinaryPrimitives.ReadUInt32LittleEndian(raw)))
                    throw new InvalidOperationException("Pop 현재 가상 firstPop 미복원");
                return FirstPopBits(raw[40]);
            },
        };
        effects.Activate = (target, requested) =>
        {
            uint isolatedDepth = 0;
            var current = new SquidActivateHooks
            {
                Server = () => _pool.IsServer, Assertion = effects.Assertion,
                Transmit = (objectSid, input) => NeedCurrent(_transmission)(objectSid, input),
                PostPop = (objectSid, input) =>
                {
                    if (_postPop is not null) _postPop.PostPopVirtual(objectSid, input);
                    else
                    {
                        if (!OriginalPopDisplay.Supports(_pool.Edition, BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(objectSid)), input))
                            throw new InvalidOperationException("Pop 현재 가상 후처리 미복원");
                        isolatedDepth = unchecked(isolatedDepth - 1);
                    }
                },
            };
            if (_postPop is not null) RawSquidActivate.Run(_pool, target, requested, ref _postPop.ActivationDepth, current);
            else RawSquidActivate.Run(_pool, target, requested, ref isolatedDepth, current);
        };
        return RawSquidPopParent.Run(_pool, _hash, _spots, sid, x, y, flags, _diagnostics.Checking, effects);
    }
}
